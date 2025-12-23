#ifndef PERFORMANCE_MONITOR_H
#define PERFORMANCE_MONITOR_H

#include <QThread>
#include <QElapsedTimer>
#include <atomic>

class PerformanceMonitor : public QThread {
    Q_OBJECT
public:
    explicit PerformanceMonitor(QObject *parent = nullptr);
    ~PerformanceMonitor();

    void markFrame(); // 每处理一帧调用一次
    void stop();      // 安全停止线程

signals:
    // 发送给 UI 的统计数据
    void updateStatistics(float fps, double cpuUsage);

protected:
    void run() override;

private:
    std::atomic<int> m_frameCount{0};
    std::atomic<bool> m_running{true};
    QElapsedTimer m_fpsTimer;

    double getCpuUsage(); // 获取系统 CPU 占用
};

#endif