#include "config/config_manager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>

ConfigManager& ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() {
    // 默认路径: 程序运行目录下的 config.json
    m_configPath = "config.json";
    
    // 初始化默认值 (来自 config.h)
    m_recognitionThreshold = Config::Default::RECOGNITION_THRESHOLD;
    m_registrationSampleCount = Config::Default::REGISTRATION_SAMPLE_COUNT;
    m_audioVolume = Config::Default::AUDIO_VOLUME;
    m_audioEnabled = Config::Default::AUDIO_ENABLED;
    
    m_workStartHour = Config::Default::WORK_START_HOUR;
    m_workStartMinute = Config::Default::WORK_START_MINUTE;
    m_workEndHour = Config::Default::WORK_END_HOUR;
    m_workEndMinute = Config::Default::WORK_END_MINUTE;
    
    m_lateThreshold = Config::Default::LATE_THRESHOLD;
    m_earlyLeaveThreshold = Config::Default::EARLY_LEAVE_THRESHOLD;
    
    m_showFps = true; // 默认显示FPS

    load();
}

void ConfigManager::load() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Config file not found, using defaults.";
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    QMutexLocker locker(&m_mutex);
    if (obj.contains("recognition_threshold")) 
        m_recognitionThreshold = obj["recognition_threshold"].toDouble();
    if (obj.contains("registration_sample_count")) 
        m_registrationSampleCount = obj["registration_sample_count"].toInt();
    if (obj.contains("audio_volume")) 
        m_audioVolume = obj["audio_volume"].toInt();
    if (obj.contains("audio_enabled")) 
        m_audioEnabled = obj["audio_enabled"].toBool();
        
    if (obj.contains("work_start_hour")) 
        m_workStartHour = obj["work_start_hour"].toInt();
    if (obj.contains("work_start_minute")) 
        m_workStartMinute = obj["work_start_minute"].toInt();
    if (obj.contains("work_end_hour")) 
        m_workEndHour = obj["work_end_hour"].toInt();
    if (obj.contains("work_end_minute")) 
        m_workEndMinute = obj["work_end_minute"].toInt();
        
    if (obj.contains("late_threshold")) 
        m_lateThreshold = obj["late_threshold"].toInt();
    if (obj.contains("early_leave_threshold")) 
        m_earlyLeaveThreshold = obj["early_leave_threshold"].toInt();
        
    if (obj.contains("show_fps")) 
        m_showFps = obj["show_fps"].toBool();
        
    qDebug() << "Config loaded from" << m_configPath;
}

void ConfigManager::save() {
    QJsonObject obj;
    {
        QMutexLocker locker(&m_mutex);
        obj["recognition_threshold"] = m_recognitionThreshold;
        obj["registration_sample_count"] = m_registrationSampleCount;
        obj["audio_volume"] = m_audioVolume;
        obj["audio_enabled"] = m_audioEnabled;
        
        obj["work_start_hour"] = m_workStartHour;
        obj["work_start_minute"] = m_workStartMinute;
        obj["work_end_hour"] = m_workEndHour;
        obj["work_end_minute"] = m_workEndMinute;
        
        obj["late_threshold"] = m_lateThreshold;
        obj["early_leave_threshold"] = m_earlyLeaveThreshold;
        
        obj["show_fps"] = m_showFps;
    }

    QJsonDocument doc(obj);
    QFile file(m_configPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        qDebug() << "Config saved to" << m_configPath;
    } else {
        qWarning() << "Failed to save config to" << m_configPath;
    }
}

// Getters
float ConfigManager::getRecognitionThreshold() const { QMutexLocker l(&m_mutex); return m_recognitionThreshold; }
int ConfigManager::getRegistrationSampleCount() const { QMutexLocker l(&m_mutex); return m_registrationSampleCount; }
int ConfigManager::getAudioVolume() const { QMutexLocker l(&m_mutex); return m_audioVolume; }
bool ConfigManager::isAudioEnabled() const { QMutexLocker l(&m_mutex); return m_audioEnabled; }
int ConfigManager::getWorkStartHour() const { QMutexLocker l(&m_mutex); return m_workStartHour; }
int ConfigManager::getWorkStartMinute() const { QMutexLocker l(&m_mutex); return m_workStartMinute; }
int ConfigManager::getWorkEndHour() const { QMutexLocker l(&m_mutex); return m_workEndHour; }
int ConfigManager::getWorkEndMinute() const { QMutexLocker l(&m_mutex); return m_workEndMinute; }
int ConfigManager::getLateThreshold() const { QMutexLocker l(&m_mutex); return m_lateThreshold; }
int ConfigManager::getEarlyLeaveThreshold() const { QMutexLocker l(&m_mutex); return m_earlyLeaveThreshold; }
bool ConfigManager::isShowFps() const { QMutexLocker l(&m_mutex); return m_showFps; }

// Setters
void ConfigManager::setRecognitionThreshold(float value) { 
    { QMutexLocker l(&m_mutex); m_recognitionThreshold = value; }
    save(); emit configChanged(); 
}
void ConfigManager::setRegistrationSampleCount(int value) { 
    { QMutexLocker l(&m_mutex); m_registrationSampleCount = value; }
    save(); emit configChanged(); 
}
void ConfigManager::setAudioVolume(int value) { 
    { QMutexLocker l(&m_mutex); m_audioVolume = value; }
    save(); emit configChanged(); 
}
void ConfigManager::setAudioEnabled(bool enabled) { 
    { QMutexLocker l(&m_mutex); m_audioEnabled = enabled; }
    save(); emit configChanged(); 
}
void ConfigManager::setWorkStartHour(int hour) { 
    { QMutexLocker l(&m_mutex); m_workStartHour = hour; }
    save(); emit configChanged(); 
}
void ConfigManager::setWorkStartMinute(int minute) { 
    { QMutexLocker l(&m_mutex); m_workStartMinute = minute; }
    save(); emit configChanged(); 
}
void ConfigManager::setWorkEndHour(int hour) { 
    { QMutexLocker l(&m_mutex); m_workEndHour = hour; }
    save(); emit configChanged(); 
}
void ConfigManager::setWorkEndMinute(int minute) { 
    { QMutexLocker l(&m_mutex); m_workEndMinute = minute; }
    save(); emit configChanged(); 
}
void ConfigManager::setLateThreshold(int value) { 
    { QMutexLocker l(&m_mutex); m_lateThreshold = value; }
    save(); emit configChanged(); 
}
void ConfigManager::setEarlyLeaveThreshold(int value) { 
    { QMutexLocker l(&m_mutex); m_earlyLeaveThreshold = value; }
    save(); emit configChanged(); 
}
void ConfigManager::setShowFps(bool show) { 
    { QMutexLocker l(&m_mutex); m_showFps = show; }
    save(); emit configChanged(); 
}
