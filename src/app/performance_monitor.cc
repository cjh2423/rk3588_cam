/**
 * @file performance_monitor.cc
 * @brief 性能监控器
 * @details
 * 职责：
 * 1. FPS 统计：统计系统整体的处理帧率。
 * 2. 线程安全：使用原子变量 (std::atomic) 记录帧数，支持多线程高频打点。
 * 3. 实时反馈：每秒计算一次 FPS，并通过 Qt 信号槽机制发送给 UI 进行显示。
 * 
 * 使用方式：
 * - 任意工作线程调用 markFrame() 进行打点。
 * - UI 线程连接 updateStatistics 信号接收数据。
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

void PerformanceMonitor::stop() {
    m_running = false;
    wait();
}

void PerformanceMonitor::run() {
    while (m_running) {
        msleep(1000); // 每一秒统计一次

        float elapsed = m_fpsTimer.restart() / 1000.0f;
        
        // 获取当前计数值并清零，原子交换
        int currentCount = m_frameCount.exchange(0);
        
        float fps = (elapsed > 0) ? (static_cast<float>(currentCount) / elapsed) : 0.0f;

        // 发送信号给 UI (CPU 传 0 即可)
        emit updateStatistics(fps, 0.0);
    }
}