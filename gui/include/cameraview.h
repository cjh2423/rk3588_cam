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
    // 更新摄像头画面
    void updateFrame(const QImage &frame);
    
    // 更新性能参数显示 (FPS 和 CPU)
    void updateStats(float fps, double cpu);

private:
    QLabel *m_imageLabel;    // 画面显示控件
    QLabel *m_statsLabel;    // 性能数据显示控件 (新增)
    
    QPushButton *m_btnManage;   // 用户管理
    QPushButton *m_btnRegister; // 人脸注册
    QPushButton *m_closeButton;
    
signals:
    void openUserManager();
    void openRegistration();
};

#endif