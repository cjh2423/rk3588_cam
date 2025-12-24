#ifndef REGISTRATION_DIALOG_H
#define REGISTRATION_DIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include "app/app_controller.h"

class RegistrationDialog : public QDialog {
    Q_OBJECT
public:
    explicit RegistrationDialog(AppController* controller, QWidget *parent = nullptr);
    ~RegistrationDialog();

public slots:
    void updateFrame(const QImage &frame);
    void onRegisterClicked();
    void onRegistrationFinished(bool success, const QString& message);

private:
    void resetState();

    AppController* m_controller;
    
    // 采集状态
    std::vector<std::vector<float>> m_capturedFeatures;
    int m_maxSamples; // 从配置读取

    QLabel *m_imageLabel;
    QLineEdit *m_inputName;
    QLineEdit *m_inputDept;
    
    QProgressBar *m_progressBar; // 新增进度条
    QPushButton *m_btnRegister;
    QPushButton *m_btnClose;
    QLabel *m_statusLabel;
};

#endif // REGISTRATION_DIALOG_H
