#ifndef CAMERA_DEVICE_H
#define CAMERA_DEVICE_H

#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <linux/videodev2.h>

/**
 * @brief V4L2 缓冲区简单的封装结构体
 */
struct Buffer {
    void *start;
    size_t length;
};

/**
 * @brief 异步摄像头管理类 (V4L2 高性能版)
 * 采用 Linux 原生 V4L2 接口 + mmap 零拷贝 + 独立线程解码
 * 实现了与参考代码一致的 30fps 性能
 */
class CameraDevice {
public:
    CameraDevice();
    ~CameraDevice();

    // 开启摄像头，可指定分辨率
    bool open(int index, int width = 640, int height = 480);
    
    // 从共享内存中获取“当前最新”的一帧
    bool read(cv::Mat &frame); 
    
    // 安全关闭硬件并销毁采集线程
    void release();

private:
    // 后台采集线程的函数体
    void capture_thread_work(); 
    
    // 清理 V4L2 缓冲区
    void cleanup_buffers();

    // V4L2 成员变量
    int m_fd = -1;                // 摄像头设备文件描述符
    Buffer* m_buffers = nullptr;  // 用户空间映射的缓冲区数组
    unsigned int m_n_buffers = 0; // 实际申请到的缓冲区数量
    
    std::thread m_thread;         // 管理后台线程的对象
    std::atomic<bool> m_running{false}; // 线程运行标志
    std::mutex m_mutex;           // 互斥锁
    std::shared_ptr<cv::Mat> m_latest_frame; // 当前最新帧

    // 帧追踪
    uint64_t m_frame_count{0};    
    uint64_t m_last_read_id{0};   
};

#endif // CAMERA_DEVICE_H