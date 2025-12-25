/**
 * @file performance_monitor.cc
 * @brief 性能监控器实现
 * @details 职责：
 * 1. FPS 统计：统计系统各环节（采集、推理、后处理）的处理帧率。
 * 2. 耗时统计：记录 NPU 推理和 CPU 后处理的平均延迟（ms）。
 * 3. 实时反馈：每秒计算一次数据并通过信号发送至 UI。
 */

#include "app/performance_monitor.h"

PerformanceMonitor::PerformanceMonitor(QObject *parent) 
    : QThread(parent), m_frameCount(0), m_running(true) {
    m_fpsTimer.start();
}

PerformanceMonitor::~PerformanceMonitor() {
    stop();
}

void PerformanceMonitor::markFrame() {
    // 原子操作，线程安全
    m_frameCount.fetch_add(1, std::memory_order_relaxed);
}

void PerformanceMonitor::markInference(double latencyMs) {
    m_inferCount.fetch_add(1, std::memory_order_relaxed);
    
    // 累加耗时 (简单自旋锁或直接原子加，这里用原子加)
    // std::atomic<double> 不支持 fetch_add，使用 CAS 循环
    double current = m_totalLatency.load();
    while (!m_totalLatency.compare_exchange_weak(current, current + latencyMs));
}

void PerformanceMonitor::markPostProcess(double latencyMs) {
    m_postCount.fetch_add(1, std::memory_order_relaxed);
    double current = m_postLatency.load();
    while (!m_postLatency.compare_exchange_weak(current, current + latencyMs));
}

void PerformanceMonitor::stop() {
    m_running = false;
    wait();
}

void PerformanceMonitor::run() {
    while (m_running) {
        msleep(1000); // 每一秒统计一次

        float elapsed = m_fpsTimer.restart() / 1000.0f;
        
        // 1. 计算 Camera FPS
        int camCount = m_frameCount.exchange(0);
        float fps = (elapsed > 0) ? (static_cast<float>(camCount) / elapsed) : 0.0f;

        // 2. 计算 Inference FPS & Latency
        int inferCount = m_inferCount.exchange(0);
        double totalLat = 0.0;
        
        // 原子读取并清零耗时
        double currentLat = m_totalLatency.load();
        while (!m_totalLatency.compare_exchange_weak(currentLat, 0.0));
        totalLat = currentLat;

        float inferFps = (elapsed > 0) ? (static_cast<float>(inferCount) / elapsed) : 0.0f;
        double avgLatency = (inferCount > 0) ? (totalLat / inferCount) : 0.0;

        // 3. 计算 PostProcess FPS & Latency
        int postCount = m_postCount.exchange(0);
        double totalPostLat = 0.0;
        
        double currentPostLat = m_postLatency.load();
        while (!m_postLatency.compare_exchange_weak(currentPostLat, 0.0));
        totalPostLat = currentPostLat;
        
        float postFps = (elapsed > 0) ? (static_cast<float>(postCount) / elapsed) : 0.0f;
        double avgPostLatency = (postCount > 0) ? (totalPostLat / postCount) : 0.0;

        // 发送信号给 UI
        emit updateStatistics(fps, 0.0, inferFps, avgLatency, postFps, avgPostLatency);
    }
}