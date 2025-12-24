/**
 * @file inference_thread.cc
 * @brief 推理线程 (The Consumer)
 * @details
 * 职责：
 * 1. 任务消费：从 PreprocessingThread 生产的队列中获取任务（含预处理好的图像）。
 * 2. NPU 推理：
 *    - 调用 YOLOv8-face 模型进行人脸检测。
 *    - (TODO) 调用 FaceNet 模型进行人脸特征提取。
 * 3. 结果后处理：
 *    - 解析 NPU 输出的 Tensor。
 *    - 将检测框坐标还原回原始图像尺寸。
 *    - 在原图上绘制人脸框和关键点。
 * 4. 结果更新：将处理完成并带有标注的图像更新到 latest_result，供 UI 线程读取。
 * 
 * 关键特性：
 * - 异步处理：独立于 UI 和 采集线程，确保重型计算不卡界面。
 * - 丢帧策略：如果推理速度跟不上采集速度，会自动丢弃旧帧，保证实时性。
 * - 纯净架构：直接使用 core 层的底层函数，不污染 ModelManager。
 */

#include "app/inference_thread.h"
#include <iostream>
#include <opencv2/imgproc.hpp>
#include "config.h"
#include "core/yolov8_face.h"
#include "core/facenet.h"
#include "core/postprocess.h"

#include "service/feature_library.h"
#include "service/attendance_service.h"
#include "database/user_dao.h"

InferenceThread::InferenceThread(ModelManager* model_manager, PerformanceMonitor* monitor)
    : model_manager_(model_manager)
    , monitor_(monitor)
    , running_(false)
    , has_new_result_(false)
{
}

InferenceThread::~InferenceThread() {
    stop();
}

void InferenceThread::start() {
    if (running_) return;
    running_ = true;
    thread_ = std::thread(&InferenceThread::thread_loop, this);
}

void InferenceThread::stop() {
    running_ = false;
    queue_cv_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void InferenceThread::push_task(const PreprocessTask& task) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    if (task_queue_.size() >= MAX_QUEUE_SIZE) {
        // 丢弃旧帧，保证实时性
        task_queue_.pop();
    }
    task_queue_.push(task);
    lock.unlock();
    queue_cv_.notify_one();
}

bool InferenceThread::get_latest_result(detect_result_group_t& result) {
    std::lock_guard<std::mutex> lock(result_mutex_);
    if (!has_new_result_) return false;
    
    // 拷贝结果
    memcpy(&result, &latest_result_, sizeof(detect_result_group_t));
    
    // 不置 false，允许 UI 持续获取该结果直到有更新的
    // has_new_result_ = false; 
    return true;
}

void InferenceThread::thread_loop() {
    while (running_) {
        PreprocessTask task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] { return !task_queue_.empty() || !running_; });
            
            if (!running_) break;
            
            task = task_queue_.front();
            task_queue_.pop();
        }

        // --- 1. YOLOv8-face 检测 ---
        detect_result_group_t detect_result;
        memset(&detect_result, 0, sizeof(detect_result));
        
        // 从 ModelManager 获取必要的参数
        int model_w, model_h, model_c;
        model_manager_->get_face_detector_size(model_w, model_h, model_c);
        
        // 准备输出 buffer
        std::array<std::vector<uint8_t>, YOLOV8_FACE_OUTPUT_NUM> output_buffers;
        
        // 运行推理
        int ret = yolov8_face_run(
            model_manager_->get_face_detector_ctx(),
            task.processed_img,
            model_w, model_h, model_c,
            task.orig_img.cols, task.orig_img.rows, // 仅用于保持接口一致，实际不影响 run
            model_manager_->get_face_detector_io_num(),
            model_manager_->get_face_detector_inputs(),
            model_manager_->get_face_detector_outputs(),
            model_manager_->get_face_detector_output_attrs(),
            output_buffers
        );

        if (ret == 0) {
            // 后处理
            ret = yolov8_face_postprocess(
                output_buffers,
                model_manager_->get_face_detector_output_attrs(),
                model_manager_->get_face_detector_io_num().n_output,
                model_h, model_w, 
                task.orig_img.cols, task.orig_img.rows, // 传入原图尺寸用于坐标还原
                BOX_THRESH, NMS_THRESH,
                &detect_result
            );
        
            if (ret == 0) {
                // --- 2. 遍历人脸 (特征提取与识别) ---
                int fn_w, fn_h, fn_c;
                model_manager_->get_facenet_size(fn_w, fn_h, fn_c);

                for (int i = 0; i < detect_result.count; i++) {
                    detect_result_t& face = detect_result.results[i];
                    
                    // 简单的裁剪 (TODO: 使用关键点进行人脸对齐)
                    cv::Rect roi(face.box.left, face.box.top, 
                                 face.box.right - face.box.left, 
                                 face.box.bottom - face.box.top);
                                 
                    // 边界检查
                    roi = roi & cv::Rect(0, 0, task.orig_img.cols, task.orig_img.rows);
                    if (roi.area() <= 0) continue;
                    
                    if (fn_w > 0 && fn_h > 0) {
                        cv::Mat face_img = task.orig_img(roi).clone();
                        cv::Mat resized_face;
                        cv::resize(face_img, resized_face, cv::Size(fn_w, fn_h));
                        
                        // FaceNet 推理
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
                            // 搜索
                            std::vector<float> feature(embedding, embedding + 512);
                            float similarity = 0.0f;
                            // 阈值设为 0.5 (根据 postprocess.h 定义)
                            int64_t user_id = service::FeatureLibrary::instance().search(feature, FACENET_THRESH, similarity);
                            
                            if (user_id != -1) {
                                // 找到用户
                                db::UserDao userDao;
                                auto user = userDao.get_user_by_id(user_id);
                                if (user) {
                                    strncpy(face.name, user->user_name.c_str(), OBJ_NAME_MAX_SIZE - 1);
                                    
                                    // 记录考勤
                                    service::AttendanceService attendanceService;
                                    attendanceService.record_attendance(user_id, similarity);
                                }
                            } else {
                                strncpy(face.name, "Unknown", OBJ_NAME_MAX_SIZE - 1);
                            }
                            
                            // 释放 FaceNet 输出
                            facenet_output_release(
                                model_manager_->get_facenet_ctx(),
                                model_manager_->get_facenet_io_num(),
                                model_manager_->get_facenet_outputs()
                            );
                        }
                    }
                }
            }
        }

        // --- 3. 更新结果 (只更新数据) ---
        {
            std::lock_guard<std::mutex> lock(result_mutex_);
            latest_result_ = detect_result; 
            has_new_result_ = true;
        }

        // 性能监控
        if (monitor_) {
            // monitor_->markInference(); 
        }
    }
}
