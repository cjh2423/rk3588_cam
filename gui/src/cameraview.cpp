#include "cameraview.h"

CameraView::CameraView(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);

    m_imageLabel = new QLabel("等待摄像头画面...", this);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setStyleSheet("background-color: black; border: 2px solid #333;");
    m_imageLabel->setMinimumSize(640, 480);

    // 性能标签：只显示 FPS
    m_statsLabel = new QLabel("FPS: 0.0", this);
    m_statsLabel->setStyleSheet(
        "color: #00FF00; background-color: #222222; "
        "font-weight: bold; font-family: 'Monospace'; "
        "padding: 5px; border-radius: 3px;"
    );

    m_closeButton = new QPushButton("退出程序", this);
    m_closeButton->setFixedHeight(40);

    layout->addWidget(m_imageLabel);
    layout->addWidget(m_statsLabel); 
    layout->addWidget(m_closeButton);

    connect(m_closeButton, &QPushButton::clicked, this, &QWidget::close);
    setWindowTitle("RK3588 Camera Test");
}

void CameraView::updateFrame(const QImage &frame) {
    m_imageLabel->setPixmap(QPixmap::fromImage(frame).scaled(
        m_imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void CameraView::updateStats(float fps, double /*unused*/) {
    // 只更新 FPS 文字
    m_statsLabel->setText(QString("Camera FPS: %1").arg(static_cast<double>(fps), 0, 'f', 1));
}