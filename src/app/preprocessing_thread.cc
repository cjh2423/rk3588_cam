/**
 * @file preprocessing_thread.cc
 * @brief 预处理线程 (The Producer)
 * @details
 * 职责：
 * 1. 视频采集：封装并调用 CameraDevice (V4L2) 进行高速视频帧采集。
 * 2. 硬件加速：利用 RK3588 的 RGA (Rockchip Graphics Acceleration) 2D 硬件引擎进行图像处理。
 *    - 翻转 (Flip): 解决摄像头镜像问题，使用硬件替代 CPU 软解。
 *    - 缩放 (Resize): 将高清原图 (1280x720) 缩放到模型输入尺寸 (640x640)，极大减轻 CPU 负担。
 * 3. Letterbox 处理：对缩放后的图像进行 padding（补黑边），保持纵横比，以适配 YOLO 模型要求。
 * 4. 任务生成：打包原始图像和处理后的图像为 PreprocessTask，供推理线程使用。
 * 
 * 关键技术：
 * - RGA API：使用 C 风格的 imflip_t/imresize_t 接口，确保与底层 librga.so 兼容。
 * - 零拷贝/虚拟地址包装：通过 wrapbuffer_virtualaddr 减少内存拷贝。
 */

#include "app/preprocessing_thread.h"
#include "config.h"  // 必须引用配置，确保分辨率统一
#include "RgaUtils.h"
#include "im2d.h"
#include "rga.h"
#include <chrono>

PreprocessingThread::PreprocessingThread(int resize_w, int resize_h, 
                                         int img_width, int img_height,
                                         PerformanceMonitor* perf_monitor)
    : running_(false)
    , resize_w_(resize_w)
    , resize_h_(resize_h)
    , img_width_(img_width)
    , img_height_(img_height)
    , perf_monitor_(perf_monitor)
    , flipped_buffer_(img_height, img_width, CV_8UC3)
    , resized_buffer_(resize_h, resize_w, CV_8UC3)
{
    // 计算 Letterbox Padding 逻辑（适配正方形模型输入）
    target_w_ = std::max(resize_w_, resize_h_);
    target_h_ = target_w_;
    
    // 计算居中填充偏移
    pad_top_ = (target_h_ - resize_h_) / 2;
    pad_left_ = (target_w_ - resize_w_) / 2;
    pad_bottom_ = target_h_ - resize_h_ - pad_top_;
    pad_right_ = target_w_ - resize_w_ - pad_left_;
}

PreprocessingThread::~PreprocessingThread() {
    stop();
}

void PreprocessingThread::start(int camIndex) {
    if (!running_) {
        // 使用 Config 中的宽度和高度开启摄像头，确保 V4L2 分辨率正确
        if (!m_camera.open(camIndex, img_width_, img_height_)) {
            return;
        }
        running_ = true;
        thread_ = std::thread(&PreprocessingThread::thread_func, this);
    }
}

void PreprocessingThread::stop() {
    if (running_) {
        running_ = false;
        if (thread_.joinable()) thread_.join();
        m_camera.release();
    }
}

bool PreprocessingThread::get_result(PreprocessTask& task) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (output_queue_.empty()) return false;
    
    task = output_queue_.front();
    output_queue_.pop();
    return true;
}

void PreprocessingThread::thread_func() {
    cv::Mat frame;
    while (running_) {
        auto t0 = std::chrono::steady_clock::now();

        // 1. 读取摄像头原始帧 (非阻塞)
        if (!m_camera.read(frame)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        PreprocessTask task;
        // 注意：此处先引用 frame 地址，后续 RGA 会处理
        task.orig_img = frame; 
        gettimeofday(&task.timestamp, NULL);

        // 2. 执行 RGA 硬件加速预处理
        process_with_rga(task);

        // 3. 入队逻辑 (丢弃旧帧，保留最新)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            while (output_queue_.size() >= MAX_QUEUE_SIZE) {
                output_queue_.pop();
            }
            output_queue_.push(task);
        }

        // 4. 性能监控打点
        auto t1 = std::chrono::steady_clock::now();
        if (perf_monitor_) {
            double ms = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() / 1000.0;
            // perf_monitor_->record_preprocess_time(ms); 
        }
    }
}

void PreprocessingThread::process_with_rga(PreprocessTask& task) {
    // A. 初始化推理用的输出容器（黑色背景）
    task.processed_img = cv::Mat(target_h_, target_w_, CV_8UC3, cv::Scalar(0, 0, 0));

    // B. RGA 翻转：使用虚拟地址包装原始数据和缓冲区
    rga_buffer_t flip_src = wrapbuffer_virtualaddr(task.orig_img.data, img_width_, img_height_, RK_FORMAT_BGR_888);
    rga_buffer_t flip_dst = wrapbuffer_virtualaddr(flipped_buffer_.data, img_width_, img_height_, RK_FORMAT_BGR_888);
    
    // 执行硬件水平翻转
    imflip_t(flip_src, flip_dst, IM_HAL_TRANSFORM_FLIP_H, IM_SYNC);

    // C. RGA 缩放：将翻转后的图缩放到 resize 目标尺寸
    rga_buffer_t src_buf = wrapbuffer_virtualaddr(flipped_buffer_.data, img_width_, img_height_, RK_FORMAT_BGR_888);
    rga_buffer_t dst_buf = wrapbuffer_virtualaddr(resized_buffer_.data, resize_w_, resize_h_, RK_FORMAT_BGR_888);
    
    // 同步模式执行缩放 (使用 imresize_t 替代 C++ 的 improcess)
    imresize_t(src_buf, dst_buf, 0, 0, INTER_LINEAR, IM_SYNC);

    // D. Padding (Letterbox)：将缩放后的图像嵌入黑色正方形背景
    cv::copyMakeBorder(resized_buffer_, task.processed_img,
                       pad_top_, pad_bottom_, pad_left_, pad_right_,
                       cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    // E. 克隆镜像后的原图：确保 UI 线程读取时，数据不会被下一帧覆盖
    task.orig_img = flipped_buffer_.clone();
}