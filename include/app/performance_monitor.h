/**
 * @file performance_monitor.h
 * @brief 性能监控模块
 * @details 支持多维度 FPS 统计：
 * 1. Camera FPS: 摄像头真实采集帧率。
 * 2. NPU FPS: YOLO 模型纯推理吞吐量。
 * 3. Post FPS: 后处理与人脸识别帧率。
 */

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
    void markPostProcess(double latencyMs); // 记录一次后处理耗时 (Post FPS)
    void stop();

signals:
    // fps: 摄像头采集帧率
    // cpu: 预留
    // inferFps: 模型推理帧率
    // latency: 模型单次耗时(ms)
    // postFps: 后处理帧率
    // postLatency: 后处理耗时
    void updateStatistics(float fps, double cpu, float inferFps, double latency, float postFps, double postLatency);

protected:
    void run() override;

private:
    std::atomic<int> m_frameCount;
    
    // NPU 统计相关
    std::atomic<int> m_inferCount{0};
    std::atomic<double> m_totalLatency{0.0};

    // PostProcess 统计相关
    std::atomic<int> m_postCount{0};
    std::atomic<double> m_postLatency{0.0};

    QElapsedTimer m_fpsTimer;
    bool m_running;
};

#endif