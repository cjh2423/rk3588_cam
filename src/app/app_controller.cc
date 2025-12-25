/**
 * @file app_controller.cc
 * @brief 应用控制器 (The Engine)
 * @details
 * 职责：
 * 1. 核心调度器：作为 Qt 主线程与后台工作线程（采集、推理、后处理）之间的桥梁。
 * 2. 生命周期管理：负责初始化 ModelManager、InferenceThread、PostProcessThread、PreprocessingThread 和 PerformanceMonitor。
 * 3. 数据流转：
 *    - 从 PreprocessingThread 获取 RGA 处理后的图像。
 *    - 将图像推送到 InferenceThread 进行异步 NPU 推理。
 *    - InferenceThread 将推理输出传递给 PostProcessThread。
 *    - 从 PostProcessThread 获取最终识别结果。
 *    - 将最终结果转换为 QImage 并推送到 UI 显示。
 * 4. 模式切换：通过 PROJECT_MODE 宏控制是纯相机预览模式还是 AI 智能模式。
 * 
 * 核心机制：
 * - 生产者-消费者流水线：通过多级非阻塞队列实现全链路并行处理。
 * - 零拷贝：在 UI 渲染时尽量复用 cv::Mat 内存，避免不必要的 memcpy。
 */

#include "app/app_controller.h"

#include "config.h"            // 包含配置
#include <QDebug>
#include <QCoreApplication>
#include <iostream>

#include <opencv2/imgproc.hpp>
#include "database/database_manager.h"
#include "database/user_dao.h"
#include "database/face_feature_dao.h"
#include "service/feature_library.h"
#include "app/postprocess_thread.h" // 新增

AppController::AppController(CameraView *view, QObject *parent) 
    : QObject(parent), m_view(view) {

    // 初始化性能监控线程
    m_monitor = new PerformanceMonitor(this);

    // 初始化模型管理器 (但不加载模型，start时加载)
    m_modelManager = new ModelManager();

    // 初始化后处理线程
    m_postThread = new PostProcessThread(m_modelManager, m_monitor);

    // 初始化推理线程 (传入后处理线程指针)
    m_inferenceThread = new InferenceThread(m_modelManager, m_monitor, m_postThread);

    // 清空结果缓存
    memset(&m_latestResult, 0, sizeof(m_latestResult));

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
    
    // 停止后处理线程
    if (m_postThread) {
        m_postThread->stop();
        delete m_postThread;
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
    
    // 0. 初始化数据库
    if (!db::DatabaseManager::instance().open(Config::Path::DATABASE)) {
        std::cerr << "Failed to open database: " << Config::Path::DATABASE << std::endl;
        // 数据库失败不应该阻塞相机预览，但考勤功能将不可用
    } else {
        // 加载特征库
        service::FeatureLibrary::instance().load_from_database();
    }

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

    // 1.5 启动后处理线程
    m_postThread->start();

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

bool AppController::getLatestFeature(std::vector<float>& feature) {
    if (!m_postThread) return false;
    return m_postThread->get_latest_feature(feature);
}

int64_t AppController::registerUser(const std::string& name, const std::string& dept, const std::vector<float>& feature) {
#if (PROJECT_MODE == 1)
    if (feature.empty()) {
        emit registrationFinished(false, "Feature vector is empty!");
        return -1;
    }
    
    // 1. 写入 User 表
    db::User user;
    user.user_name = name;
    user.department = dept;
    user.status = 1;
    
    db::UserDao userDao;
    // 简单查重：名字是否已存在
    auto exist = userDao.get_user_by_name(name);
    if (exist) {
         emit registrationFinished(false, "User name already exists!");
         return -1;
    }
    
    int64_t uid = userDao.add_user(user);
    if (uid == -1) {
        emit registrationFinished(false, "Database error: add_user failed");
        return -1;
    }
    
    // 2. 写入 Feature 表
    db::FaceFeature ff;
    ff.user_id = uid;
    ff.feature_vector = feature;
    ff.feature_quality = 1.0f; // 暂未做质量评估
    
    db::FaceFeatureDao featureDao;
    if (featureDao.add_feature(ff) == -1) {
        // 回滚用户
        userDao.delete_user(uid);
        emit registrationFinished(false, "Database error: add_feature failed");
        return -1;
    }
    
    // 3. 刷新内存特征库
    service::FeatureLibrary::instance().load_from_database();
    
    emit registrationFinished(true, QString("Success! User ID: %1").arg(uid));
    return uid;
#else
    emit registrationFinished(false, "AI Mode not enabled");
    return -1;
#endif
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
        emit frameReady(qimg); // 广播信号
    }
#elif(PROJECT_MODE == 1)
    // --- 模式 1：流水线处理 (UI 与 推理分离) ---
    
    // 1. 尝试从预处理线程获取新帧 (Fast Path)
    PreprocessTask rawTask;
    bool hasFrame = false;
    if (m_preThread && m_preThread->get_result(rawTask)) {
        hasFrame = true;
        if (m_monitor) {
            m_monitor->markFrame(); // 统计采集 FPS
        }
        
        // 2. 将新帧推送到推理线程 (Slow Path)
        // 只有当有新帧时才推，防止推理线程空转
        if (m_inferenceThread) {
            m_inferenceThread->push_task(rawTask);
        }
    }

    // 3. 检查是否有新的推理结果
    // 无论是否有新帧，都可以去查一下结果，因为推理可能比采集慢
    if (m_postThread) {
        detect_result_group_t newResult;
        if (m_postThread->get_latest_result(newResult)) {
            m_latestResult = newResult; // 原子更新结果
        }
    }

    // 4. 显示逻辑
    if (hasFrame) {
        // 关键点：我们始终显示最新的摄像头画面 rawTask.orig_img
        // 并把"当前能拿到的最新"检测框 m_latestResult 画上去
        
        // 注意：orig_img 是 BGR，drawResult 会在上面直接画线
        // 由于 rawTask.orig_img 是 RGA clone 出来的，是线程私有的，可以安全修改
        // 警告：drawResult 会就地修改 orig_img。如果推理线程需要干净的 orig_img（例如用于 FaceNet），则必须改为在副本上绘制
        drawResult(rawTask.orig_img, m_latestResult);

        // 构造 QImage
        // 重点：rawTask.orig_img 可能是 RGA 内存对齐的，必须传入 step
        QImage qimg(rawTask.orig_img.data, 
                    rawTask.orig_img.cols, 
                    rawTask.orig_img.rows, 
                    rawTask.orig_img.step, 
                    QImage::Format_RGB888);

        // 显示：交换颜色通道 (BGR->RGB)
        if (m_view) {
            m_view->updateFrame(qimg.rgbSwapped()); 
        }
        emit frameReady(qimg.rgbSwapped()); // 广播信号
    }
#endif
}

void AppController::drawResult(cv::Mat& frame, const detect_result_group_t& result) {
    for (int i = 0; i < result.count; i++) {
        const detect_result_t& face = result.results[i];
        
        // 判定是否是已知用户 (假设 name 不为 "Unknown" 且非空即为已知)
        bool isKnown = (strlen(face.name) > 0 && strcmp(face.name, "Unknown") != 0 && strcmp(face.name, "face") != 0);
        cv::Scalar boxColor = isKnown ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255); // 绿: 已知, 红: 未知
        
        // 1. 画框
        cv::rectangle(frame, 
            cv::Point(face.box.left, face.box.top), 
            cv::Point(face.box.right, face.box.bottom), 
            boxColor, 2);

        // 2. 显示名字 (如果是已知用户)
        if (isKnown) {
             std::string label = face.name;
             int baseLine = 0;
             cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.8, 2, &baseLine);
             
             // 名字背景框
             cv::rectangle(frame, 
                 cv::Point(face.box.left, face.box.top - labelSize.height - 10), 
                 cv::Point(face.box.left + labelSize.width, face.box.top), 
                 boxColor, cv::FILLED);
                 
             // 名字文本 (黑色字体)
             cv::putText(frame, label, 
                 cv::Point(face.box.left, face.box.top - 5), 
                 cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);
        }

        // 3. 画关键点
        cv::Scalar pointColor(0, 255, 255); // 黄色关键点
        int radius = 2;
        cv::circle(frame, cv::Point(face.point.point_1_x, face.point.point_1_y), radius, pointColor, -1);
        cv::circle(frame, cv::Point(face.point.point_2_x, face.point.point_2_y), radius, pointColor, -1);
        cv::circle(frame, cv::Point(face.point.point_3_x, face.point.point_3_y), radius, pointColor, -1);
        cv::circle(frame, cv::Point(face.point.point_4_x, face.point.point_4_y), radius, pointColor, -1);
        cv::circle(frame, cv::Point(face.point.point_5_x, face.point.point_5_y), radius, pointColor, -1);
    }
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