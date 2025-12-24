#include "registration_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <cmath>
#include <numeric>
#include "config.h"

RegistrationDialog::RegistrationDialog(AppController* controller, QWidget *parent) 
    : QDialog(parent), m_controller(controller) {
    
    m_maxSamples = Config::Default::REGISTRATION_SAMPLE_COUNT;

    setWindowTitle("人脸注册 (手动采集)");
    resize(800, 600);
    
    auto *mainLayout = new QHBoxLayout(this);
    
    // Left: Video Preview
    m_imageLabel = new QLabel("等待摄像头画面...", this);
    m_imageLabel->setMinimumSize(640, 480);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setStyleSheet("background-color: black; border: 1px solid #555;");
    mainLayout->addWidget(m_imageLabel, 2);
    
    // Right: Controls
    auto *rightPanel = new QVBoxLayout();
    
    auto *formLayout = new QFormLayout();
    m_inputName = new QLineEdit(this);
    m_inputDept = new QLineEdit(this);
    formLayout->addRow("姓名:", m_inputName);
    formLayout->addRow("部门:", m_inputDept);
    rightPanel->addLayout(formLayout);
    
    // Progress Bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, m_maxSamples);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("已采集样本: %v/%m");
    m_progressBar->setTextVisible(true);
    rightPanel->addWidget(m_progressBar);
    
    m_btnRegister = new QPushButton("采集样本", this);
    m_btnRegister->setFixedHeight(50);
    m_btnRegister->setStyleSheet("background-color: #2196F3; color: white; font-weight: bold; font-size: 14px;");
    rightPanel->addWidget(m_btnRegister);
    
    m_statusLabel = new QLabel("", this);
    m_statusLabel->setWordWrap(true);
    rightPanel->addWidget(m_statusLabel);
    
    rightPanel->addStretch();
    
    m_btnClose = new QPushButton("关闭", this);
    rightPanel->addWidget(m_btnClose);
    
    mainLayout->addLayout(rightPanel, 1);
    
    // Connect Signals
    connect(m_controller, &AppController::frameReady, this, &RegistrationDialog::updateFrame);
    connect(m_controller, &AppController::registrationFinished, this, &RegistrationDialog::onRegistrationFinished);
    
    connect(m_btnRegister, &QPushButton::clicked, this, &RegistrationDialog::onRegisterClicked);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);
    
    resetState();
}

RegistrationDialog::~RegistrationDialog() {
    disconnect(m_controller, &AppController::frameReady, this, &RegistrationDialog::updateFrame);
    disconnect(m_controller, &AppController::registrationFinished, this, &RegistrationDialog::onRegistrationFinished);
}

void RegistrationDialog::resetState() {
    m_capturedFeatures.clear();
    m_progressBar->setValue(0);
    m_btnRegister->setText(QString("采集样本 (1/%1)").arg(m_maxSamples));
    m_btnRegister->setEnabled(true);
    m_statusLabel->setText("就绪.");
    m_statusLabel->setStyleSheet("color: black;");
}

void RegistrationDialog::updateFrame(const QImage &frame) {
    if (isVisible()) {
        m_imageLabel->setPixmap(QPixmap::fromImage(frame).scaled(
            m_imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void RegistrationDialog::onRegisterClicked() {
    QString name = m_inputName->text().trimmed();
    
    if (name.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入姓名。");
        return;
    }
    
    // 尝试采集当前帧特征
    std::vector<float> feature;
    if (!m_controller->getLatestFeature(feature)) {
        QMessageBox::warning(this, "采集失败", "未检测到人脸或画面中有多个人脸！请调整位置。");
        return;
    }
    
    // 保存样本
    m_capturedFeatures.push_back(feature);
    int currentSamples = m_capturedFeatures.size();
    
    // 更新 UI
    m_progressBar->setValue(currentSamples);
    
    if (currentSamples < m_maxSamples) {
        m_btnRegister->setText(QString("采集样本 (%1/%2)").arg(currentSamples + 1).arg(m_maxSamples));
        m_statusLabel->setText(QString("第 %1 个样本已采集。请稍微移动角度再次点击。").arg(currentSamples));
        m_statusLabel->setStyleSheet("color: blue;");
    } else {
        // 采集完成，处理注册
        m_btnRegister->setEnabled(false);
        m_btnRegister->setText("正在处理...");
        m_statusLabel->setText("正在合成特征并录入数据库...");
        
        // 计算平均特征
        size_t dim = m_capturedFeatures[0].size();
        std::vector<float> averaged(dim, 0.0f);
        for (const auto& f : m_capturedFeatures) {
            for (size_t i = 0; i < dim; ++i) {
                averaged[i] += f[i];
            }
        }
        
        // 归一化
        float sq_sum = 0.0f;
        for (size_t i = 0; i < dim; ++i) {
            averaged[i] /= m_maxSamples;
            sq_sum += averaged[i] * averaged[i];
        }
        float norm = std::sqrt(sq_sum);
        if (norm > 1e-6) {
            for (size_t i = 0; i < dim; ++i) averaged[i] /= norm;
        }
        
        // 提交注册
        m_controller->registerUser(name.toStdString(), 
                                 m_inputDept->text().toStdString(), 
                                 averaged);
    }
}

void RegistrationDialog::onRegistrationFinished(bool success, const QString& message) {
    if (success) {
        m_statusLabel->setText("注册成功！");
        m_statusLabel->setStyleSheet("color: green;");
        QMessageBox::information(this, "成功", "人脸信息已成功录入数据库。");
        resetState(); // 重置状态，准备下一次注册
        m_inputName->clear();
        m_inputDept->clear();
    } else {
        m_statusLabel->setText("错误: " + message);
        m_statusLabel->setStyleSheet("color: red;");
        QMessageBox::critical(this, "注册失败", message);
        // 失败后是否重置？或者允许重试？这里选择重置以防脏数据
        resetState();
    }
}
