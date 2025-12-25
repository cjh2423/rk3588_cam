/**
 * @file camera_device.cpp
 * @brief 异步摄像头驱动实现
 * @details 采用双缓冲区机制：
 * 1. 后台线程死循环抓取 V4L2 图像 ID。
 * 2. 增加帧追踪 ID 机制，确保 read() 只在有新硬件帧时返回 true，防止重复处理。
 */

#include "hardware/camera_device.h"

CameraDevice::CameraDevice() {}

CameraDevice::~CameraDevice() {
    release(); // 确保对象销毁时，后台线程能被正确关闭
}

bool CameraDevice::open(int index, int width, int height) {
    // 1. 打开设备，CAP_V4L2 是 Linux 下操作摄像头的原生后端，速度最快
    if (!m_cap.open(index, cv::CAP_V4L2)) return false;

    // 2. 硬件参数配置
    // MJPEG 是一种压缩格式，能显著降低 USB 总线的带宽占用，提升 1080P 等高分率下的帧率
    m_cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    m_cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
    m_cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    m_cap.set(cv::CAP_PROP_FPS, 30); // 告诉硬件尽量以 30 帧运行

    if (m_cap.isOpened()) {
        m_running = true;
        // 3. 【核心精华】：启动后台采集线程
        // 使用 std::thread 开启一个新线程运行 capture_thread_work
        m_thread = std::thread(&CameraDevice::capture_thread_work, this);
        
        printf("[Camera] 后台采集线程已启动，配置: %dx%d\n", width, height);
    }
    return m_cap.isOpened();
}

/**
 * 后台线程的工作内容：
 * 这个函数一直在跑，它唯一的任务就是把硬件缓冲区里的数据“掏出来”放到内存里。
 */
void CameraDevice::capture_thread_work() {
    while (m_running) {
        cv::Mat tmp_frame;
        // m_cap.read 会阻塞当前线程直到硬件产生新的一帧
        // 因为这是后台线程，所以它“等得起”，主线程不会卡顿
        if (!m_cap.read(tmp_frame)) {
            continue;
        }

        if (!tmp_frame.empty()) {
            // 【核心精华】：零拷贝封装
            // std::move 把 tmp_frame 的图像数据直接“移交给”智能指针，不产生内存复制
            auto new_ptr = std::make_shared<cv::Mat>(std::move(tmp_frame));
            
            {
                // 加锁：确保在替换 m_latest_frame 时，主线程没有正在读取它
                std::lock_guard<std::mutex> lock(m_mutex);
                m_latest_frame = new_ptr; // 原子替换：旧帧指针会被释放，新帧上位
                m_frame_count++;          // 计数器 +1，标记这是一张新图
            }
            // 此时，如果主线程处理得慢，旧帧会直接在内存里销毁，永远保持“最新一帧”
        }
    }
}

/**
 * 主线程调用的 read：
 * 它的任务不是去等硬件，而是去“瞅一眼”内存里的 m_latest_frame 准备好了没。
 */
bool CameraDevice::read(cv::Mat &frame) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 如果内存中有最新帧，且该帧的 ID 比上一次读取的 ID 大
    if (m_latest_frame && !m_latest_frame->empty() && m_frame_count > m_last_read_id) {
        // 【核心精华】：OpenCV Mat 的赋值是“浅拷贝”
        // 它只是把指针指过去，底层图像数据不进行 memcpy，因此极快（微秒级）
        frame = *m_latest_frame;
        m_last_read_id = m_frame_count; // 更新已读 ID，防止下一轮重复读取
        return true;
    }
    return false; // 如果是旧帧，或者是空帧，都返回 false
}

void CameraDevice::release() {
    m_running = false; // 先通知线程停止循环
    
    // 如果线程还在运行，则等待它运行完最后一步
    if (m_thread.joinable()) {
        m_thread.join(); 
    }
    
    // 释放 OpenCV 硬件资源
    if (m_cap.isOpened()) {
        m_cap.release();
    }
}