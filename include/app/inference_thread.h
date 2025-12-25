/**
 * @file inference_thread.h
 * @brief 推理线程定义
 * @details 负责 YOLOv8 模型的 NPU 推理调度。
 */

#ifndef INFERENCE_THREAD_H
#define INFERENCE_THREAD_H

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <opencv2/core/core.hpp>
#include "core/model_manager.h"
#include "app/performance_monitor.h"
#include "app/preprocessing_thread.h" // for PreprocessTask
#include "app/postprocess_thread.h" // for PostProcessTask

class InferenceThread {
public:
    InferenceThread(ModelManager* model_manager, PerformanceMonitor* monitor, PostProcessThread* post_thread);
    ~InferenceThread();

    void start();
    void stop();

    void push_task(const PreprocessTask& task);

    // 结果获取现在移交给了 PostProcessThread，这里不再提供
    // bool get_latest_result(detect_result_group_t& result);
    // bool get_latest_feature(std::vector<float>& feature);

private:
    void thread_loop();

    ModelManager* model_manager_;
    PerformanceMonitor* monitor_;
    PostProcessThread* post_thread_; // 新增

    std::thread thread_;
    std::atomic<bool> running_;
    
    std::queue<PreprocessTask> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // 移除结果存储
    // detect_result_group_t latest_result_;
    // std::vector<float> latest_feature_;
    // bool has_new_result_;
    // std::mutex result_mutex_;
};

#endif // INFERENCE_THREAD_H