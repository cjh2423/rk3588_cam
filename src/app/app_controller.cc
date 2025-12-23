/**
 * @file app_controller.cc
 * @brief 应用控制器 (The Engine)
 * @details
 * 职责：
 * 1. 核心调度器：作为 Qt 主线程与后台工作线程（采集、推理）之间的桥梁。
 * 2. 生命周期管理：负责初始化 ModelManager、InferenceThread、PreprocessingThread 和 PerformanceMonitor。
 * 3. 数据流转：
 *    - 从 PreprocessingThread 获取 RGA 处理后的图像。
 *    - 将图像推送到 InferenceThread 进行异步 AI 推理。
 *    - 从 InferenceThread 获取推理结果（含画框图）。
 *    - 将最终结果转换为 QImage 并推送到 UI 显示。
 * 4. 模式切换：通过 PROJECT_MODE 宏控制是纯相机预览模式还是 AI 智能模式。
 * 
 * 核心机制：
 * - 生产者-消费者模型：通过非阻塞队列传递图像任务。
 * - 零拷贝：在 UI 渲染时尽量复用 cv::Mat 内存，避免不必要的 memcpy。
 */

#include "app/app_controller.h"

#include "config.h"            // 包含配置
#include <QDebug>
#include <QCoreApplication>
#include <iostream>

AppController::AppController(CameraView *view, QObject *parent) 
    : QObject(parent), m_view(view) {

    // 初始化性能监控线程
    m_monitor = new PerformanceMonitor(this);

    // 初始化模型管理器 (但不加载模型，start时加载)
    m_modelManager = new ModelManager();

    // 初始化推理线程
    m_inferenceThread = new InferenceThread(m_modelManager, m_monitor);

#if (PROJECT_MODE == 0)
    // 创建一个定时器作为 UI 刷新的触发源
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &AppController::onFrameTick);
    
    // 建立通信链路 监控线程的 emit updateStatistics()信号-----UI的 updateStats() 槽函数
    if (m_view) {
        connect(m_monitor, &PerformanceMonitor::updateStatistics, 
                m_view, &CameraView::updateStats);
    }
#elif(PROJECT_MODE == 1)
    // --- 模式 1：人脸识别预处理架构 ---
    // 初始化预处理线程
    m_preThread = new PreprocessingThread(
        Config::Model::YOLO_INPUT_SIZE,  // 640
        Config::Model::YOLO_INPUT_SIZE,  // 640
        Config::Camera::WIDTH,           // 1280 (必须与摄像头输出一致)
        Config::Camera::HEIGHT,          // 720
        m_monitor
    );

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &AppController::onFrameTick);

    if (m_view) {
        connect(m_monitor, &PerformanceMonitor::updateStatistics, 
                m_view, &CameraView::updateStats);
    }
#endif
}

AppController::~AppController() {
    if (m_monitor) {
        m_monitor->stop();
    }
    
    // 停止推理线程
    if (m_inferenceThread) {
        m_inferenceThread->stop();
        delete m_inferenceThread;
    }
    
    // 释放模型
    if (m_modelManager) {
        delete m_modelManager;
    }

#if (PROJECT_MODE == 0)
    m_camera.release();
#elif(PROJECT_MODE == 1)
    // 停止预处理线程
    if (m_preThread) {
        m_preThread->stop();
        delete m_preThread;
    }
#endif
}

bool AppController::start(int camIndex, int w, int h, 
                          const std::string& yolo_path, 
                          const std::string& facenet_path) {
#if (PROJECT_MODE == 0)
    // 1. 启动底层硬件采集线程
    if (!m_camera.open(camIndex, w, h)) {
        qDebug() << "Failed to open camera!";
        return false;
    }

    // 2. 启动监控线程
    m_monitor->start(); 
    // 3. 启动 UI 定时器
    m_timer->start(10);     // 30 Timer tick ~ 33 FPS
#elif(PROJECT_MODE == 1)
    // --- 加载模型 ---
    // 使用传入的参数，不再自己拼装路径
    
    // 加载 YOLOv8
    if (m_modelManager->init_face_detector(yolo_path.c_str()) != 0) {
        std::cerr << "Failed to load YOLOv8 model: " << yolo_path << std::endl;
        return false;
    }
    
    // 加载 FaceNet
    if (m_modelManager->init_facenet(facenet_path.c_str()) != 0) {
        std::cerr << "Failed to load FaceNet model: " << facenet_path << std::endl;
        // FaceNet 失败不应该阻塞程序运行，可能只是识别功能不可用
    }

    // --- 启动线程 ---
    // 1. 启动推理线程
    m_inferenceThread->start();

    // 2. 启动预处理线程 (内部开启摄像头)
    // 注意：内部会调用 m_camera.open()
    m_preThread->start(camIndex); 
    
    // 3. 启动监控
    m_monitor->start(); 
    
    // 4. 启动 UI 定时器
    m_timer->start(10); // 100 FPS 的尝试频率
#endif
    return true;
}

void AppController::onFrameTick() {
#if (PROJECT_MODE == 0)
    cv::Mat frame;
    // read() 是非阻塞浅拷贝
    if (m_camera.read(frame)) {
        // 【高性能处理】：不在 OpenCV 层面做 flip 和 clone
        // 直接打点统计 FPS
        if (m_monitor) {
            m_monitor->markFrame();
        }
        // 转换并镜像
        QImage qimg = cvMatToQImage(frame);
        // 显示
        if (m_view) {
            m_view->updateFrame(qimg);
        }
    }
#elif(PROJECT_MODE == 1)
    // --- 模式 1：流水线处理 ---
    
    // 1. 尝试从预处理线程获取新帧，推送到推理线程
    PreprocessTask rawTask;
    if (m_preThread && m_preThread->get_result(rawTask)) {
        if (m_monitor) {
            m_monitor->markFrame(); // 统计采集 FPS
        }
        if (m_inferenceThread) {
            m_inferenceThread->push_task(rawTask);
        }
    }

    // 2. 尝试从推理线程获取处理结果 (带画框的图)，显示到 UI
    PreprocessTask resultTask;
    if (m_inferenceThread && m_inferenceThread->get_latest_result(resultTask)) {
        // 重点：这里的 resultTask.orig_img 已经在 InferenceThread 中画上了框
        // 且由于 RGA 处理后内存可能对齐，必须传入 step
        QImage qimg(resultTask.orig_img.data, 
                    resultTask.orig_img.cols, 
                    resultTask.orig_img.rows, 
                    resultTask.orig_img.step, 
                    QImage::Format_RGB888);

        // 显示：只需要交换颜色通道 (BGR->RGB)，不再需要 mirrored() (RGA 已做)
        if (m_view) {
            m_view->updateFrame(qimg.rgbSwapped()); // 更新 UI
        }
    }
#endif
}

QImage AppController::cvMatToQImage(const cv::Mat &mat) {
    if (mat.empty()) return QImage();
    
    // 1. 构造 QImage (浅拷贝，指向 Mat 的数据地址)
    // 假设输入是 BGR (CV_8UC3)
    QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
    
    // 2. mirrored(true, false) 实现水平镜像
    // 3. rgbSwapped() 实现 BGR -> RGB
    // Qt 内部会对这两个操作进行高度优化，且不影响原始 cv::Mat 的内存安全
    return image.mirrored(true, false).rgbSwapped(); 
}