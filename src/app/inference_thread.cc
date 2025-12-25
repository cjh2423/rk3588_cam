/**
 * @file inference_thread.cc
 * @brief 推理线程 (The Consumer 1)
 * @details 职责：
 * 1. 任务消费：从预处理队列获取图像。
 * 2. NPU 推理：专注于执行 YOLOv8-face 模型，不进行后处理。
 * 3. 任务中转：将 NPU 输出的原始数据推送到 PostProcessThread。
 */
#include "app/inference_thread.h"
#include <iostream>
#include <chrono>
#include "config.h" // 新增

InferenceThread::InferenceThread(ModelManager* model_manager, PerformanceMonitor* monitor, PostProcessThread* post_thread)
    : model_manager_(model_manager)
    , monitor_(monitor)
    , post_thread_(post_thread)
    , running_(false)
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
    if (task_queue_.size() >= Config::Performance::QUEUE_MAX_SIZE) {
        task_queue_.pop();
    }
    task_queue_.push(task);
    lock.unlock();
    queue_cv_.notify_one();
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

        auto t0 = std::chrono::steady_clock::now();

        // --- YOLOv8-face Inference Only ---
        
        // 从 ModelManager 获取必要的参数
        int model_w, model_h, model_c;
        model_manager_->get_face_detector_size(model_w, model_h, model_c);
        
        // 准备输出 buffer
        std::array<std::vector<uint8_t>, YOLOV8_FACE_OUTPUT_NUM> output_buffers;
        
        // 运行推理 (NPU)
        int ret = yolov8_face_run(
            model_manager_->get_face_detector_ctx(),
            task.processed_img,
            model_w, model_h, model_c,
            task.orig_img.cols, task.orig_img.rows, 
            model_manager_->get_face_detector_io_num(),
            model_manager_->get_face_detector_inputs(),
            model_manager_->get_face_detector_outputs(),
            model_manager_->get_face_detector_output_attrs(),
            output_buffers
        );

        if (ret == 0) {
            // 推送到后处理线程
            if (post_thread_) {
                PostProcessTask pptask;
                pptask.raw_task = task;
                pptask.output_buffers = output_buffers; // Move or copy
                pptask.model_w = model_w;
                pptask.model_h = model_h;
                
                post_thread_->push_task(pptask);
            }
        }

        // 性能监控 (Inference FPS - 仅统计 NPU 耗时)
        if (monitor_) {
             auto t1 = std::chrono::steady_clock::now();
             double ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
             monitor_->markInference(ms);
        }
    }
}