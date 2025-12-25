#ifndef CAMERA_DEVICE_H
#define CAMERA_DEVICE_H

#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

/**
 * @brief 异步摄像头管理类
 * 解决了 OpenCV 原生 read() 带来的阻塞和画面堆积问题
 */
class CameraDevice {
public:
    CameraDevice();
    ~CameraDevice();

    // 开启摄像头，可指定分辨率，默认 640x480
    bool open(int index, int width = 640, int height = 480);
    
    // 从共享内存中获取“当前最新”的一帧，不再从硬件实时等待
    bool read(cv::Mat &frame); 
    
    // 安全关闭硬件并销毁采集线程
    void release();

private:
    // 后台采集线程的函数体：在这个线程里死循环抓取硬件数据
    void capture_thread_work(); 

    cv::VideoCapture m_cap;      // OpenCV 硬件操作句柄
    std::thread m_thread;        // 管理后台线程的对象
    
    // 原子布尔变量：用于线程间的状态控制，比普通的 bool 更安全
    std::atomic<bool> m_running{false}; 
    
    // 互斥锁：防止主线程读取和后台线程写入同一块内存时发生冲突
    std::mutex m_mutex;
    
    // 核心精华：智能指针指向的 Mat 对象
    // 使用 shared_ptr 是为了方便在两个线程间安全地共享同一个 Mat 的所有权
    std::shared_ptr<cv::Mat> m_latest_frame;

    // 帧追踪：用于判断是否为新帧
    uint64_t m_frame_count{0};    // 后台线程累计抓取到的总帧数
    uint64_t m_last_read_id{0};   // 记录上一次读取时的帧 ID
};

#endif // CAMERA_DEVICE_H