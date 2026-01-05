#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "config.h"
#include <QObject>
#include <QString>
#include <QMutex>

class ConfigManager : public QObject {
    Q_OBJECT

public:
    static ConfigManager& instance();

    // 加载/保存配置
    void load();
    void save();

    // Getters
    float getRecognitionThreshold() const;
    int getRegistrationSampleCount() const;
    int getAudioVolume() const;
    bool isAudioEnabled() const;
    
    // 考勤时间
    int getWorkStartHour() const;
    int getWorkStartMinute() const;
    int getWorkEndHour() const;
    int getWorkEndMinute() const;
    
    int getLateThreshold() const;
    int getEarlyLeaveThreshold() const;
    
    // 界面显示
    bool isShowFps() const;

    // Setters
    void setRecognitionThreshold(float value);
    void setRegistrationSampleCount(int value);
    void setAudioVolume(int value);
    void setAudioEnabled(bool enabled);
    
    void setWorkStartHour(int hour);
    void setWorkStartMinute(int minute);
    void setWorkEndHour(int hour);
    void setWorkEndMinute(int minute);
    
    void setLateThreshold(int value);
    void setEarlyLeaveThreshold(int value);
    
    void setShowFps(bool show);

signals:
    void configChanged();

private:
    ConfigManager();
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    QString m_configPath;
    
    // 配置项
    float m_recognitionThreshold;
    int m_registrationSampleCount;
    int m_audioVolume;
    bool m_audioEnabled;
    
    int m_workStartHour;
    int m_workStartMinute;
    int m_workEndHour;
    int m_workEndMinute;
    
    int m_lateThreshold;
    int m_earlyLeaveThreshold;
    
    bool m_showFps;
    
    mutable QMutex m_mutex;
};

#endif // CONFIG_MANAGER_H
