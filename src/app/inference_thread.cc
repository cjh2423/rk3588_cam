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

bool InferenceThread::get_latest_result(PreprocessTask& result) {
    std::lock_guard<std::mutex> lock(result_mutex_);
    if (!has_new_result_) return false;
    
    result = latest_result_;
    // 不置 false，允许 UI 重复读取（虽然 UI 应该也是事件驱动）
    // 或者置 false 避免重复渲染
    has_new_result_ = false; 
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
                // --- 2. 遍历人脸进行识别 ---
                for (int i = 0; i < detect_result.count; i++) {
                    detect_result_t& face = detect_result.results[i];
                    
                    // 简单的画框测试
                    cv::rectangle(task.orig_img, 
                        cv::Point(face.box.left, face.box.top), 
                        cv::Point(face.box.right, face.box.bottom), 
                        cv::Scalar(0, 255, 0), 2);

                    // 简单的关键点画圆
                    cv::circle(task.orig_img, cv::Point(face.point.point_1_x, face.point.point_1_y), 2, cv::Scalar(0, 0, 255), -1);
                    cv::circle(task.orig_img, cv::Point(face.point.point_2_x, face.point.point_2_y), 2, cv::Scalar(0, 0, 255), -1);
                    cv::circle(task.orig_img, cv::Point(face.point.point_3_x, face.point.point_3_y), 2, cv::Scalar(0, 0, 255), -1);
                    cv::circle(task.orig_img, cv::Point(face.point.point_4_x, face.point.point_4_y), 2, cv::Scalar(0, 0, 255), -1);
                    cv::circle(task.orig_img, cv::Point(face.point.point_5_x, face.point.point_5_y), 2, cv::Scalar(0, 0, 255), -1);

                    // TODO: 
                    // 1. 根据 5点 做仿射变换对齐 (Align) -> 112x112
                    // 2. Facenet inference (使用 model_manager->get_facenet_ctx())
                    // 3. 数据库搜索
                }
            }
        }

        // --- 3. 更新结果 ---
        {
            std::lock_guard<std::mutex> lock(result_mutex_);
            latest_result_ = task; // 这里 task.orig_img 已经被画上了框
            has_new_result_ = true;
        }

        // 性能监控
        if (monitor_) {
            // monitor_->markInference(); // 需要在 Monitor 加接口
        }
    }
}
