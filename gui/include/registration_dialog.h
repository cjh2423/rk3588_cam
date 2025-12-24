#ifndef REGISTRATION_DIALOG_H
#define REGISTRATION_DIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
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
    AppController* m_controller;
    
    QLabel *m_imageLabel;
    QLineEdit *m_inputName;
    QLineEdit *m_inputDept;
    QPushButton *m_btnRegister;
    QPushButton *m_btnClose;
    QLabel *m_statusLabel;
};

#endif // REGISTRATION_DIALOG_H
