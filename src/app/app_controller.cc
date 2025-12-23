#include "app/app_controller.h"

#include "config.h"            // 包含配置
#include <QDebug>

AppController::AppController(CameraView *view, QObject *parent) 
    : QObject(parent), m_view(view) {

    // 初始化性能监控线程
    m_monitor = new PerformanceMonitor(this);

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

bool AppController::start(int camIndex, int w, int h) {
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
    // --- 模式 1：启动预处理线程 ---
    // 注意：内部会调用 m_camera.open()
    m_preThread->start(camIndex); 
    m_monitor->start(); 
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
    // --- 模式 1：从预处理线程获取结果 ---
    PreprocessTask task;
    if (m_preThread && m_preThread->get_result(task)) {
        
        if (m_monitor) {
            m_monitor->markFrame();
        }

        // 重点：这里的 task.orig_img 已经是 RGA 镜像翻转后的数据
        // 且由于 RGA 处理后内存可能对齐，必须传入 task.orig_img.step
        QImage qimg(task.orig_img.data, 
                    task.orig_img.cols, 
                    task.orig_img.rows, 
                    task.orig_img.step, 
                    QImage::Format_RGB888);

        // 显示：只需要交换颜色通道 (BGR->RGB)，不再需要 mirrored()
        if (m_view) {
            m_view->updateFrame(qimg.rgbSwapped()); // 更新 UI
        }

        // 预留给 NPU 识别路：
        // m_npuThread->pushTask(task.processed_img);
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