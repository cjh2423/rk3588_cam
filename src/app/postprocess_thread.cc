/**
 * @file postprocess_thread.cc
 * @brief 后处理线程实现
 * @details 实现了基于流水线并行的后处理逻辑，将耗时的 NMS 和人脸识别从推理线程中分离。
 */

#include "app/postprocess_thread.h"
#include <iostream>
#include <opencv2/imgproc.hpp>
#include "config.h"
#include "core/postprocess.h"
#include "core/facenet.h"
#include "service/feature_library.h"
#include "service/attendance_service.h"
#include "database/user_dao.h"
#include <chrono>

PostProcessThread::PostProcessThread(ModelManager* model_manager, PerformanceMonitor* monitor)
    : model_manager_(model_manager)
    , monitor_(monitor)
    , running_(false)
    , has_new_result_(false)
{
}

PostProcessThread::~PostProcessThread() {
    stop();
}

void PostProcessThread::start() {
    if (running_) return;
    running_ = true;
    thread_ = std::thread(&PostProcessThread::thread_loop, this);
}

void PostProcessThread::stop() {
    running_ = false;
    queue_cv_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void PostProcessThread::push_task(const PostProcessTask& task) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    // 同样需要丢帧策略，防止后处理积压
    if (task_queue_.size() >= Config::Performance::QUEUE_MAX_SIZE) {
        task_queue_.pop();
    }
    task_queue_.push(task);
    lock.unlock();
    queue_cv_.notify_one();
}

bool PostProcessThread::get_latest_result(detect_result_group_t& result) {
    std::lock_guard<std::mutex> lock(result_mutex_);
    if (!has_new_result_) return false;
    memcpy(&result, &latest_result_, sizeof(detect_result_group_t));
    return true;
}

bool PostProcessThread::get_latest_feature(std::vector<float>& feature) {
    std::lock_guard<std::mutex> lock(result_mutex_);
    if (latest_feature_.empty()) return false;
    feature = latest_feature_;
    return true;
}

void PostProcessThread::thread_loop() {
    while (running_) {
        PostProcessTask task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] { return !task_queue_.empty() || !running_; });
            
            if (!running_) break;
            
            task = task_queue_.front();
            task_queue_.pop();
        }

        auto t0 = std::chrono::steady_clock::now();

        // 1. YOLOv8 Post-process (NMS, Box decoding)
        detect_result_group_t detect_result;
        memset(&detect_result, 0, sizeof(detect_result));

        int ret = yolov8_face_postprocess(
            task.output_buffers,
            model_manager_->get_face_detector_output_attrs(),
            model_manager_->get_face_detector_io_num().n_output,
            task.model_h, task.model_w, 
            task.raw_task.orig_img.cols, task.raw_task.orig_img.rows,
            BOX_THRESH, NMS_THRESH,
            &detect_result
        );

        if (ret == 0) {
            // 2. FaceNet Recognition & Search
            int fn_w, fn_h, fn_c;
            model_manager_->get_facenet_size(fn_w, fn_h, fn_c);

            for (int i = 0; i < detect_result.count; i++) {
                detect_result_t& face = detect_result.results[i];
                
                cv::Rect roi(face.box.left, face.box.top, 
                             face.box.right - face.box.left, 
                             face.box.bottom - face.box.top);
                             
                roi = roi & cv::Rect(0, 0, task.raw_task.orig_img.cols, task.raw_task.orig_img.rows);
                if (roi.area() <= 0) continue;
                
                if (fn_w > 0 && fn_h > 0) {
                    cv::Mat face_img = task.raw_task.orig_img(roi).clone();
                    cv::Mat resized_face;
                    cv::resize(face_img, resized_face, cv::Size(fn_w, fn_h));
                    
                    float* embedding = nullptr;
                    int ret_fn = facenet_inference(
                        model_manager_->get_facenet_ctx(),
                        resized_face,
                        model_manager_->get_facenet_io_num(),
                        model_manager_->get_facenet_inputs(),
                        model_manager_->get_facenet_outputs(),
                        &embedding
                    );
                    
                    if (ret_fn == 0 && embedding) {
                        std::vector<float> feature(embedding, embedding + 512);
                        
                        // 注册用的特征缓存 (单人脸时)
                        if (detect_result.count == 1) {
                            std::lock_guard<std::mutex> lock(result_mutex_);
                            latest_feature_ = feature;
                        } else {
                            std::lock_guard<std::mutex> lock(result_mutex_);
                            latest_feature_.clear();
                        }

                        // 搜索
                        float similarity = 0.0f;
                        int64_t user_id = service::FeatureLibrary::instance().search(feature, FACENET_THRESH, similarity);
                        
                        if (user_id != -1) {
                            db::UserDao userDao;
                            auto user = userDao.get_user_by_id(user_id);
                            if (user) {
                                strncpy(face.name, user->user_name.c_str(), OBJ_NAME_MAX_SIZE - 1);
                                service::AttendanceService attendanceService;
                                attendanceService.record_attendance(user_id, similarity);
                            }
                        } else {
                            strncpy(face.name, "Unknown", OBJ_NAME_MAX_SIZE - 1);
                        }
                        
                        facenet_output_release(
                            model_manager_->get_facenet_ctx(),
                            model_manager_->get_facenet_io_num(),
                            model_manager_->get_facenet_outputs()
                        );
                    }
                }
            }
        }

        // 3. Update Result
        {
            std::lock_guard<std::mutex> lock(result_mutex_);
            latest_result_ = detect_result; 
            has_new_result_ = true;
        }

        // 4. Performance Monitor (PostProcess FPS)
        if (monitor_) {
             auto t1 = std::chrono::steady_clock::now();
             double ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
             monitor_->markPostProcess(ms); // Need to add this method to monitor
        }
    }
}