#include "settings_dialog.h"
#include "config/config_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout> // Added
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("系统设置");
    resize(400, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // --- 1. 人脸识别设置 ---
    QGroupBox *groupFace = new QGroupBox("人脸识别", this);
    QFormLayout *layoutFace = new QFormLayout(groupFace);
    
    m_spinThreshold = new QDoubleSpinBox(this);
    m_spinThreshold->setRange(0.0, 1.0);
    m_spinThreshold->setSingleStep(0.05);
    layoutFace->addRow("相似度阈值:", m_spinThreshold);
    
    m_spinSampleCount = new QSpinBox(this);
    m_spinSampleCount->setRange(1, 10);
    layoutFace->addRow("注册采集帧数:", m_spinSampleCount);
    
    mainLayout->addWidget(groupFace);

    // --- 2. 考勤设置 ---
    QGroupBox *groupAttend = new QGroupBox("考勤时间", this);
    QFormLayout *layoutAttend = new QFormLayout(groupAttend);
    
    // 上班时间: [Hour] : [Minute]
    QHBoxLayout *layoutStart = new QHBoxLayout();
    m_comboStartHour = new QComboBox(this);
    m_comboStartMin = new QComboBox(this);
    layoutStart->addWidget(m_comboStartHour);
    layoutStart->addWidget(new QLabel(":", this));
    layoutStart->addWidget(m_comboStartMin);
    layoutStart->addStretch();
    layoutAttend->addRow("上班时间:", layoutStart);
    
    // 下班时间: [Hour] : [Minute]
    QHBoxLayout *layoutEnd = new QHBoxLayout();
    m_comboEndHour = new QComboBox(this);
    m_comboEndMin = new QComboBox(this);
    layoutEnd->addWidget(m_comboEndHour);
    layoutEnd->addWidget(new QLabel(":", this));
    layoutEnd->addWidget(m_comboEndMin);
    layoutEnd->addStretch();
    layoutAttend->addRow("下班时间:", layoutEnd);
    
    m_spinLateThreshold = new QSpinBox(this);
    m_spinLateThreshold->setRange(0, 120);
    m_spinLateThreshold->setSingleStep(5);
    m_spinLateThreshold->setSuffix(" 分钟");
    layoutAttend->addRow("迟到阈值:", m_spinLateThreshold);
    
    m_spinEarlyThreshold = new QSpinBox(this);
    m_spinEarlyThreshold->setRange(0, 120);
    m_spinEarlyThreshold->setSingleStep(5);
    m_spinEarlyThreshold->setSuffix(" 分钟");
    layoutAttend->addRow("早退阈值:", m_spinEarlyThreshold);
    
    // 填充数据
    for (int i = 0; i < 24; ++i) {
        QString s = QString("%1").arg(i, 2, 10, QChar('0'));
        m_comboStartHour->addItem(s);
        m_comboEndHour->addItem(s);
    }
    for (int i = 0; i < 60; ++i) {
        QString s = QString("%1").arg(i, 2, 10, QChar('0'));
        m_comboStartMin->addItem(s);
        m_comboEndMin->addItem(s);
    }
    
    mainLayout->addWidget(groupAttend);

    // --- 3. 系统设置 ---
    QGroupBox *groupSys = new QGroupBox("系统", this);
    QFormLayout *layoutSys = new QFormLayout(groupSys);
    
    m_spinVolume = new QSpinBox(this);
    m_spinVolume->setRange(0, 100);
    m_spinVolume->setSingleStep(5);
    m_spinVolume->setSuffix(" %");
    layoutSys->addRow("音量:", m_spinVolume);
    
    m_chkShowFps = new QCheckBox("显示 FPS 和性能参数", this);
    layoutSys->addRow("", m_chkShowFps);
    
    mainLayout->addWidget(groupSys);

    // --- 按钮 ---
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_btnSave = new QPushButton("保存", this);
    m_btnCancel = new QPushButton("取消", this);
    
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnSave);
    btnLayout->addWidget(m_btnCancel);
    
    mainLayout->addLayout(btnLayout);

    // 连接
    connect(m_btnSave, &QPushButton::clicked, this, &SettingsDialog::saveSettings);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    // 加载初始值
    loadCurrentSettings();
}

void SettingsDialog::loadCurrentSettings() {
    auto& config = ConfigManager::instance();
    
    m_spinThreshold->setValue(config.getRecognitionThreshold());
    m_spinSampleCount->setValue(config.getRegistrationSampleCount());
    
    m_comboStartHour->setCurrentIndex(config.getWorkStartHour());
    m_comboStartMin->setCurrentIndex(config.getWorkStartMinute());
    
    m_comboEndHour->setCurrentIndex(config.getWorkEndHour());
    m_comboEndMin->setCurrentIndex(config.getWorkEndMinute());
    
    m_spinLateThreshold->setValue(config.getLateThreshold());
    m_spinEarlyThreshold->setValue(config.getEarlyLeaveThreshold());
    
    m_spinVolume->setValue(config.getAudioVolume());
    m_chkShowFps->setChecked(config.isShowFps());
}

void SettingsDialog::saveSettings() {
    auto& config = ConfigManager::instance();
    
    config.setRecognitionThreshold(m_spinThreshold->value());
    config.setRegistrationSampleCount(m_spinSampleCount->value());
    
    config.setWorkStartHour(m_comboStartHour->currentIndex());
    config.setWorkStartMinute(m_comboStartMin->currentIndex());
    
    config.setWorkEndHour(m_comboEndHour->currentIndex());
    config.setWorkEndMinute(m_comboEndMin->currentIndex());
    
    config.setLateThreshold(m_spinLateThreshold->value());
    config.setEarlyLeaveThreshold(m_spinEarlyThreshold->value());
    
    config.setAudioVolume(m_spinVolume->value());
    config.setShowFps(m_chkShowFps->isChecked());
    
    QMessageBox::information(this, "提示", "设置已保存");
    accept();
}
