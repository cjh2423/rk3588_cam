#ifndef _PREPROCESSING_THREAD_H_
#define _PREPROCESSING_THREAD_H_

#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <sys/time.h>
// hardware
#include "hardware/camera_device.h"
// app
#include "app/performance_monitor.h"

/*-------------------------------------------
    预处理任务结构
-------------------------------------------*/
struct PreprocessTask {
    cv::Mat orig_img;      // 翻转后的原图（给UI显示）
    cv::Mat processed_img; // 缩放+Padding后的图（给NPU推理）
    struct timeval timestamp;
};

class PreprocessingThread {
public:
    /**
     * @brief 构造函数
     * @param resize_w 缩放目标宽度
     * @param resize_h 缩放目标高度
     * @param img_width 摄像头图像宽度
     * @param img_height 摄像头图像高度
     * @param perf_monitor 性能监控对象指针
     */
    PreprocessingThread(int resize_w, int resize_h, 
                         int img_width, int img_height,
                         PerformanceMonitor* perf_monitor);
    ~PreprocessingThread();

    // 修改：启动时需要传入摄像头索引
    void start(int camIndex);
    void stop();

    // 获取结果接口
    bool get_result(PreprocessTask& task);

private:
    void thread_func();
    void process_with_rga(PreprocessTask& task);

private:
    std::thread thread_;
    std::atomic<bool> running_;
    
    // 你的核心组件：摄像头
    CameraDevice m_camera; 

    // 输出队列
    mutable std::mutex mutex_;
    std::queue<PreprocessTask> output_queue_;

    // 配置参数
    int resize_w_, resize_h_;
    int img_width_, img_height_;
    PerformanceMonitor* perf_monitor_;

    // RGA 辅助变量
    int target_w_, target_h_;
    int pad_top_, pad_bottom_, pad_left_, pad_right_;
    
    // 缓存区，避免反复申请内存
    cv::Mat flipped_buffer_;
    cv::Mat resized_buffer_;

    static const int MAX_QUEUE_SIZE = 2; // 队列深度
};

#endif