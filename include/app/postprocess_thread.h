/**
 * @file postprocess_thread.h
 * @brief 后处理线程 (The Consumer 2)
 * @details 职责：
 * 1. 接收 InferenceThread 传来的原始 Tensor 数据。
 * 2. 执行 NMS、坐标还原等后处理算法。
 * 3. 对检测到的人脸进行裁剪并调用 FaceNet 进行特征提取。
 * 4. 检索数据库识别身份并更新最终结果。
 */

#ifndef POSTPROCESS_THREAD_H
#define POSTPROCESS_THREAD_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <atomic>
#include <array>
#include <chrono> // Added for steady_clock

#include "core/model_manager.h"
#include "app/performance_monitor.h"
#include "app/preprocessing_thread.h" // for PreprocessTask
#include "core/yolov8_face.h" // for YOLOV8_FACE_OUTPUT_NUM

// 定义传递给后处理线程的任务包
struct PostProcessTask {
    PreprocessTask raw_task; // 包含原图
    std::array<std::vector<uint8_t>, YOLOV8_FACE_OUTPUT_NUM> output_buffers; // YOLO 输出张量
    int model_w;
    int model_h;
};

// 考勤事件 (用于 UI 通知)
struct AttendanceEvent {
    std::string name;
    std::string time;
    int type;   // 1=In, 2=Out
    int status; // 1=Normal, 2=Late, 3=Early
};

class PostProcessThread {
public:
    PostProcessThread(ModelManager* model_manager, PerformanceMonitor* monitor);
    ~PostProcessThread();

    void start();
    void stop();

    // 由 InferenceThread 调用，推入 YOLO 输出数据
    void push_task(const PostProcessTask& task);

    // 获取最终结果 (供 UI 读取)
    bool get_latest_result(detect_result_group_t& result);
    bool get_latest_feature(std::vector<float>& feature);
    
    // 获取考勤事件 (非阻塞)
    bool get_next_event(AttendanceEvent& event);

private:
    void thread_loop();

    ModelManager* model_manager_;
    PerformanceMonitor* monitor_;

    std::thread thread_;
    std::atomic<bool> running_;
    
    // 任务队列
    std::queue<PostProcessTask> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    // 结果数据
    detect_result_group_t latest_result_;
    std::vector<float> latest_feature_;
    bool has_new_result_;
    std::mutex result_mutex_;
    
    // 事件队列 (UI通知)
    std::queue<AttendanceEvent> event_queue_;
    std::mutex event_mutex_;
    
    // 确认逻辑状态
    int64_t last_user_id_ = -1;
    std::chrono::steady_clock::time_point confirm_start_time_;
    bool is_confirming_ = false;
};

#endif // POSTPROCESS_THREAD_H