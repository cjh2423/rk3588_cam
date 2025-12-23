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