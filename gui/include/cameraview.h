/**
 * @file cameraview.h
 * @brief 摄像头预览窗口
 * @details 负责实时视频流显示以及多维性能参数（FPS/Latency）的展示。
 */

#ifndef CAMERAVIEW_H
#define CAMERAVIEW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QImage>

class CameraView : public QWidget {
    Q_OBJECT

public:
    explicit CameraView(QWidget *parent = nullptr);

public slots:
    // 更新图像显示
    void updateFrame(const QImage& image);
    
    // 更新性能参数显示 (FPS 和 CPU)
    void updateStats(float fps, double cpu, float inferFps, double latency, float postFps, double postLatency);

private:
    QLabel *m_imageLabel;
    QLabel *m_statsLabel;
    
    QPushButton *m_btnManage;   // 用户管理
    QPushButton *m_btnRegister; // 人脸注册
    QPushButton *m_closeButton;
    
signals:
    void openUserManager();
    void openRegistration();
};

#endif