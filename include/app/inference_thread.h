#ifndef INFERENCE_THREAD_H
#define INFERENCE_THREAD_H

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <opencv2/core/core.hpp>
#include "core/model_manager.h"
#include "app/preprocessing_thread.h" // 复用 PreprocessTask
#include "app/performance_monitor.h"

class InferenceThread {
public:
    InferenceThread(ModelManager* model_manager, PerformanceMonitor* monitor = nullptr);
    ~InferenceThread();

    // 启动线程
    void start();
    
    // 停止线程
    void stop();

    // 推送任务 (由 PreprocessingThread 或 AppController 调用)
    void push_task(const PreprocessTask& task);

    // 获取最新检测结果
    bool get_latest_result(detect_result_group_t& result);

    // 获取当前人脸特征 (用于注册)
    // 只有当画面中仅有一个人脸时有效
    bool get_latest_feature(std::vector<float>& feature);

private:
    void thread_loop();

private:
    ModelManager* model_manager_;
    PerformanceMonitor* monitor_;
    
    std::thread thread_;
    std::atomic<bool> running_;
    
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::queue<PreprocessTask> task_queue_;
    
    std::mutex result_mutex_;
    detect_result_group_t latest_result_;
    std::vector<float> latest_feature_; // 缓存的特征向量
    bool has_new_result_;
};

#endif // INFERENCE_THREAD_H