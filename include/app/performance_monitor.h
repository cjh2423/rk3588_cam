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

    void markFrame(); // 每处理一帧调用一次 (Camera FPS)
    void markInference(double latencyMs); // 记录一次推理耗时 (NPU FPS)
    void stop();

signals:
    // fps: 摄像头采集帧率
    // cpu: 预留
    // inferFps: 模型推理帧率
    // latency: 模型单次耗时(ms)
    void updateStatistics(float fps, double cpu, float inferFps, double latency);

protected:
    void run() override;

private:
    std::atomic<int> m_frameCount;
    
    // NPU 统计相关
    std::atomic<int> m_inferCount{0};
    std::atomic<double> m_totalLatency{0.0};

    QElapsedTimer m_fpsTimer;
    bool m_running;
};

#endif