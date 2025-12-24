#include "registration_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>

RegistrationDialog::RegistrationDialog(AppController* controller, QWidget *parent) 
    : QDialog(parent), m_controller(controller) {
    setWindowTitle("Face Registration");
    resize(800, 600);
    
    auto *mainLayout = new QHBoxLayout(this);
    
    // Left: Video Preview
    m_imageLabel = new QLabel("Waiting for camera...", this);
    m_imageLabel->setMinimumSize(640, 480);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setStyleSheet("background-color: black; border: 1px solid #555;");
    mainLayout->addWidget(m_imageLabel, 2);
    
    // Right: Controls
    auto *rightPanel = new QVBoxLayout();
    
    auto *formLayout = new QFormLayout();
    m_inputName = new QLineEdit(this);
    m_inputDept = new QLineEdit(this);
    formLayout->addRow("Name:", m_inputName);
    formLayout->addRow("Department:", m_inputDept);
    rightPanel->addLayout(formLayout);
    
    m_btnRegister = new QPushButton("Register Face", this);
    m_btnRegister->setFixedHeight(50);
    m_btnRegister->setStyleSheet("background-color: #2196F3; color: white; font-weight: bold; font-size: 14px;");
    rightPanel->addWidget(m_btnRegister);
    
    m_statusLabel = new QLabel("", this);
    m_statusLabel->setWordWrap(true);
    rightPanel->addWidget(m_statusLabel);
    
    rightPanel->addStretch();
    
    m_btnClose = new QPushButton("Close", this);
    rightPanel->addWidget(m_btnClose);
    
    mainLayout->addLayout(rightPanel, 1);
    
    // Connect Signals
    connect(m_controller, &AppController::frameReady, this, &RegistrationDialog::updateFrame);
    connect(m_controller, &AppController::registrationFinished, this, &RegistrationDialog::onRegistrationFinished);
    
    connect(m_btnRegister, &QPushButton::clicked, this, &RegistrationDialog::onRegisterClicked);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);
}

RegistrationDialog::~RegistrationDialog() {
    disconnect(m_controller, &AppController::frameReady, this, &RegistrationDialog::updateFrame);
    disconnect(m_controller, &AppController::registrationFinished, this, &RegistrationDialog::onRegistrationFinished);
}

void RegistrationDialog::updateFrame(const QImage &frame) {
    if (isVisible()) {
        m_imageLabel->setPixmap(QPixmap::fromImage(frame).scaled(
            m_imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void RegistrationDialog::onRegisterClicked() {
    QString name = m_inputName->text().trimmed();
    QString dept = m_inputDept->text().trimmed();
    
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter a name.");
        return;
    }
    
    m_btnRegister->setEnabled(false);
    m_statusLabel->setText("Processing...");
    m_statusLabel->setStyleSheet("color: blue;");
    
    // Call controller
    m_controller->registerUser(name.toStdString(), dept.toStdString());
}

void RegistrationDialog::onRegistrationFinished(bool success, const QString& message) {
    m_btnRegister->setEnabled(true);
    if (success) {
        m_statusLabel->setText(message);
        m_statusLabel->setStyleSheet("color: green;");
        QMessageBox::information(this, "Success", message);
        m_inputName->clear();
        m_inputDept->clear();
    } else {
        m_statusLabel->setText("Error: " + message);
        m_statusLabel->setStyleSheet("color: red;");
        QMessageBox::critical(this, "Failed", message);
    }
}
