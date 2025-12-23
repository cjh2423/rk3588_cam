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

    // 获取最新处理结果 (用于 UI 显示)
    // 简单起见，这里我们返回带画图结果的 task，或者仅返回检测数据
    bool get_latest_result(PreprocessTask& result);

private:
    void thread_loop();
    
    // 人脸对齐辅助函数
    cv::Mat align_face(const cv::Mat& src, const float* landmarks);

private:
    ModelManager* model_manager_;
    PerformanceMonitor* monitor_;
    
    std::thread thread_;
    std::atomic<bool> running_;
    
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::queue<PreprocessTask> task_queue_;
    
    std::mutex result_mutex_;
    PreprocessTask latest_result_;
    bool has_new_result_;

    // 限制队列长度，防止堆积
    const size_t MAX_QUEUE_SIZE = 3;
};

#endif // INFERENCE_THREAD_H