#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QImage>
#include "app/performance_monitor.h"        // 性能监控线程
#include "app/preprocessing_thread.h"       // 预处理线程
#include "hardware/camera_device.h"
#include "cameraview.h"


// 前向声明，提高编译速度
class PreprocessingThread;          // 预处理线程
class PerformanceMonitor;           // 性能监控线程
class CameraView;

/**
 * @brief 业务逻辑控制器 (The Engine)
 * 职责：管理硬件生命周期、抓取图像、格式转换、分发数据
 */
class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(CameraView *view, QObject *parent = nullptr);
    ~AppController();

    // 启动整个流水线
    bool start(int camIndex, int w = 640, int h = 480);

private slots:
    // 定时器槽函数：负责从硬件层“抽取”数据并推送到 UI
    void onFrameTick();

private:
    // 图像转换辅助函数：cv::Mat -> QImage
    QImage cvMatToQImage(const cv::Mat &mat);

    // 硬件
    CameraDevice m_camera;   // 硬件生产者 (之前写的异步 Camera 类)
    // GUI
    CameraView *m_view;      // UI 消费者指针
    QTimer *m_timer;         // 控制消费频率的“节拍器”

    // APP
    PreprocessingThread *m_preThread; // 预处理线程
    // 辅助线程
    PerformanceMonitor *m_monitor; // 性能监控线程
};

#endif