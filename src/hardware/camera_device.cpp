/**
 * @file camera_device.cpp
 * @brief 异步摄像头驱动实现 (V4L2 高性能版)
 * @details 
 * 参考 https://github.com/ccl-123/RK3588-NPU 实现
 * 使用 V4L2 直接控制摄像头，启用 MJPEG 格式以获得 30fps 性能。
 */

#include "hardware/camera_device.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <iostream>
#include <cstring> // for memset

#define REQ_COUNT 4

CameraDevice::CameraDevice() {}

CameraDevice::~CameraDevice() {
    release(); 
}

void CameraDevice::cleanup_buffers() {
    if (m_buffers != nullptr) {
        for (unsigned int i = 0; i < m_n_buffers; ++i) {
            if (m_buffers[i].start != nullptr && m_buffers[i].start != MAP_FAILED) {
                munmap(m_buffers[i].start, m_buffers[i].length);
                m_buffers[i].start = nullptr;
            }
        }
        delete[] m_buffers;
        m_buffers = nullptr;
    }
    m_n_buffers = 0;
}

bool CameraDevice::open(int index, int width, int height) {
    if (m_fd >= 0) {
        std::cerr << "[Camera] Device already opened." << std::endl;
        return false;
    }

    // 1. 打开设备文件
    std::string device_path = "/dev/video" + std::to_string(index);
    m_fd = ::open(device_path.c_str(), O_RDWR); // ::open 防止与 open 成员函数混淆
    if (m_fd < 0) {
        perror(("[Camera] Failed to open " + device_path).c_str());
        return false;
    }

    // 2. 查询能力
    v4l2_capability cap;
    if (ioctl(m_fd, VIDIOC_QUERYCAP, &cap) == -1) {
        perror("[Camera] VIDIOC_QUERYCAP");
        ::close(m_fd); m_fd = -1;
        return false;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        std::cerr << "[Camera] Device does not support video capture" << std::endl;
        ::close(m_fd); m_fd = -1;
        return false;
    }

    // 3. 配置格式：强制 MJPEG 以支持高帧率 (30fps)
    v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG; // 关键配置
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(m_fd, VIDIOC_S_FMT, &fmt) == -1) {
        perror("[Camera] VIDIOC_S_FMT");
        ::close(m_fd); m_fd = -1;
        return false;
    }
    
    // 打印实际协商的分辨率
    std::cout << "[Camera] V4L2 initialized: " << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height 
              << " (MJPEG)" << std::endl;

    // 4. 申请内核缓冲区
    v4l2_requestbuffers req = {};
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    req.count = REQ_COUNT;

    if (ioctl(m_fd, VIDIOC_REQBUFS, &req) == -1) {
        perror("[Camera] VIDIOC_REQBUFS");
        ::close(m_fd); m_fd = -1;
        return false;
    }

    // 5. 内存映射 (mmap)
    m_buffers = new Buffer[req.count];
    m_n_buffers = req.count;
    memset(m_buffers, 0, sizeof(Buffer) * req.count);

    for (unsigned int i = 0; i < req.count; ++i) {
        v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(m_fd, VIDIOC_QUERYBUF, &buf) == -1) {
            perror("[Camera] VIDIOC_QUERYBUF");
            cleanup_buffers();
            ::close(m_fd); m_fd = -1;
            return false;
        }

        m_buffers[i].length = buf.length;
        m_buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, buf.m.offset);

        if (m_buffers[i].start == MAP_FAILED) {
            perror("[Camera] mmap failed");
            cleanup_buffers();
            ::close(m_fd); m_fd = -1;
            return false;
        }

        // 入队
        if (ioctl(m_fd, VIDIOC_QBUF, &buf) == -1) {
            perror("[Camera] VIDIOC_QBUF");
            cleanup_buffers();
            ::close(m_fd); m_fd = -1;
            return false;
        }
    }

    // 6. 开启视频流
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_STREAMON, &type) == -1) {
        perror("[Camera] VIDIOC_STREAMON");
        cleanup_buffers();
        ::close(m_fd); m_fd = -1;
        return false;
    }

    // 7. 启动采集线程
    m_running = true;
    m_thread = std::thread(&CameraDevice::capture_thread_work, this);

    return true;
}

void CameraDevice::capture_thread_work() {
    v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    while (m_running) {
        // 出队 (DQBUF)
        if (ioctl(m_fd, VIDIOC_DQBUF, &buf) == -1) {
            if (errno == EAGAIN) {
                usleep(1000);
                continue;
            }
            perror("[Camera] VIDIOC_DQBUF failed");
            // 错误处理：简单休眠重试，或者退出
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (!m_running) break;
            continue;
        }

        // 解码 (MJPEG -> BGR)
        // 这里的 raw_data 只是构建了一个 OpenCV 矩阵头指向 mmap 的内存，没有数据拷贝
        cv::Mat raw_data(1, buf.bytesused, CV_8UC1, m_buffers[buf.index].start);
        cv::Mat decoded_frame = cv::imdecode(raw_data, cv::IMREAD_COLOR);

        if (!decoded_frame.empty()) {
            auto new_ptr = std::make_shared<cv::Mat>(std::move(decoded_frame));
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_latest_frame = new_ptr; 
                m_frame_count++;          
            }
        }

        // 重新入队 (QBUF)
        if (ioctl(m_fd, VIDIOC_QBUF, &buf) == -1) {
            perror("[Camera] VIDIOC_QBUF re-queue failed");
        }
    }
}

bool CameraDevice::read(cv::Mat &frame) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 如果内存中有最新帧，且该帧的 ID 比上一次读取的 ID 大
    if (m_latest_frame && !m_latest_frame->empty() && m_frame_count > m_last_read_id) {
        // 浅拷贝
        frame = *m_latest_frame;
        m_last_read_id = m_frame_count; 
        return true;
    }
    return false; 
}

void CameraDevice::release() {
    m_running = false; 
    
    if (m_thread.joinable()) {
        m_thread.join(); 
    }
    
    if (m_fd >= 0) {
        // 停止视频流
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(m_fd, VIDIOC_STREAMOFF, &type);
        
        cleanup_buffers();
        ::close(m_fd);
        m_fd = -1;
    }
    
    // 清空引用
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_latest_frame.reset();
    }
}
