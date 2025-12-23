#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <string>
#include <iostream>  // 引入 cout 所在的头文件
#include <cstdlib>
#include "config.h"
// app
#include "app/app_controller.h"
// gui
#include "cameraview.h"

#if 1
int main(int argc, char *argv[]) {
    // 创建 Qt 应用
    QApplication app(argc, argv);

    // 获取路径配置 (直接从 Config 获取)
    std::string yolov8_face_model = Config::Path::YOLO_MODEL;
    std::string facenet_model = Config::Path::FACENET_MODEL;
    std::string db_path = Config::Path::DATABASE;
    int camera_id = Config::Default::CAMERA_ID;
    
    // 确保数据库目录存在
    QDir dbDir = QFileInfo(QString::fromStdString(db_path)).dir();
    if (!dbDir.exists()) {
        dbDir.mkpath(".");
        std::cout << "Created database directory: " << dbDir.absolutePath().toStdString() << std::endl;
    }
    
    // 打印配置信息
    std::cout << "========================================" << std::endl;
    std::cout << "Configuration (from config.h):" << std::endl;
    std::cout << "  YOLOv8-face model: " << yolov8_face_model << std::endl;
    std::cout << "  FaceNet model: " << facenet_model << std::endl;
    std::cout << "  Camera ID: " << Config::Default::CAMERA_ID << std::endl;
    std::cout << "  Database: " << db_path << std::endl;
    std::cout << "========================================" << std::endl;

    // 创建主窗口
    CameraView main_window;
    
    // 创建控制器并绑定视图
    AppController controller(&main_window);

    // 启动应用逻辑
    if (controller.start(Config::Default::CAMERA_ID, 
                         Config::Camera::WIDTH, 
                         Config::Camera::HEIGHT,
                         yolov8_face_model,
                         facenet_model)) {
        main_window.show();
        return app.exec();
    }

    return -1;
}
#endif
