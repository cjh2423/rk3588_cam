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

    // 获取可执行文件所在目录
    QString appDir = QCoreApplication::applicationDirPath();
    
    // 使用固定配置的模型路径 (相对于可执行文件目录)
    std::string yolov8_face_model = (appDir + "/" + QString::fromStdString(Config::Path::YOLO_MODEL)).toStdString();
    std::string facenet_model = (appDir + "/" + QString::fromStdString(Config::Path::FACENET_MODEL)).toStdString();
    
    // 从配置文件加载摄像头设置（固定 USB + 异步）
    std::string camera_source = "usb";
    int camera_id = Config::Default::CAMERA_ID;
    
    // 数据库路径 (相对于可执行文件目录)
    std::string db_path = (appDir + "/" + QString::fromStdString(Config::Path::DATABASE)).toStdString();
    
    // 确保数据库目录存在
    QDir dbDir = QFileInfo(QString::fromStdString(db_path)).dir();
    if (!dbDir.exists()) {
        dbDir.mkpath(".");
        std::cout << "Created database directory: " << dbDir.absolutePath().toStdString() << std::endl;
    }
    
    // 命令行参数解析
    if (argc >= 3) {
        yolov8_face_model = argv[1];
        facenet_model = argv[2];
    }
    if (argc >= 5) {
        camera_source = argv[3];
        camera_id = std::atoi(argv[4]);
    }
    if (argc >= 6) {
        db_path = argv[5];
    }
    
    // 使用 cout 打印配置信息，安全且易读
    std::cout << "========================================" << std::endl;
    std::cout << "Configuration:" << std::endl;
    std::cout << "  YOLOv8-face model: " << yolov8_face_model << std::endl;
    std::cout << "  FaceNet model: " << facenet_model << std::endl;
    std::cout << "  Camera source: " << camera_source << std::endl;
    std::cout << "  Camera ID: " << camera_id << std::endl;
    std::cout << "  Database: " << db_path << std::endl;
    std::cout << "========================================" << std::endl;

    // 创建主窗口
    CameraView main_window;
    
    // 创建控制器并绑定视图
    AppController controller(&main_window);

    // 启动应用逻辑 (开启摄像头 index 21)
    if (controller.start(Config::Default::CAMERA_ID, 
                         Config::Camera::WIDTH, 
                         Config::Camera::HEIGHT)) {
        main_window.show();
        return app.exec();
    }

    return -1;
}
#endif
