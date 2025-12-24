#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QImage>
#include <string>                           // 新增
#include "app/performance_monitor.h"        // 性能监控线程
#include "app/preprocessing_thread.h"       // 预处理线程
#include "app/inference_thread.h"           // 推理线程 (新增)
#include "core/model_manager.h"             // 模型管理 (新增)
#include "core/postprocess.h"               // 结果结构体 (新增)
#include "hardware/camera_device.h"
#include "cameraview.h"


// 前向声明，提高编译速度
class PreprocessingThread;          // 预处理线程
class PerformanceMonitor;           // 性能监控线程
class InferenceThread;              // 推理线程 (新增)
class ModelManager;                 // 模型管理 (新增)
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
    // 增加模型路径参数
    bool start(int camIndex, int w, int h, 
               const std::string& yolo_path, 
               const std::string& facenet_path);

    // 获取当前画面中的人脸特征 (用于注册)
    bool getLatestFeature(std::vector<float>& feature);

    // 注册新用户 (传入已采集并处理好的特征)
    int64_t registerUser(const std::string& name, const std::string& dept, const std::vector<float>& feature);

signals:
    // 图像更新信号 (用于多界面分发)
    void frameReady(const QImage& image);
    
    // 注册结果信号
    void registrationFinished(bool success, const QString& message);

private slots:
    // 定时器槽函数：负责从硬件层“抽取”数据并推送到 UI
    void onFrameTick();

private:
    // 图像转换辅助函数：cv::Mat -> QImage
    QImage cvMatToQImage(const cv::Mat &mat);
    
    // 辅助函数：将检测结果画在图上 (解决延迟问题的关键)
    void drawResult(cv::Mat& frame, const detect_result_group_t& result);

    // 硬件
    CameraDevice m_camera;   // 硬件生产者 (之前写的异步 Camera 类)
    // GUI
    CameraView *m_view;      // UI 消费者指针
    QTimer *m_timer;         // 控制消费频率的“节拍器”

    // APP
    PreprocessingThread *m_preThread; // 预处理线程
    InferenceThread *m_inferenceThread; // 推理线程 (新增)
    ModelManager *m_modelManager;       // 模型管理 (新增)
    
    // 状态
    detect_result_group_t m_latestResult; // 缓存最新的检测结果

    // 辅助线程
    PerformanceMonitor *m_monitor; // 性能监控线程
};

#endif