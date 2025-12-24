/**
 * @file config.h
 * @brief 全局配置常量 - 集中管理所有可调参数
 * @author CL
 * @date 2025-12-04
 * 
 * 配置分类：
 * - [固定] 系统常量，不可通过 UI 修改
 * - [默认] UI 可配置项的默认值，运行时从 ConfigManager 读取
 * 
 * 使用方法：
 *   #include "config.h"
 *   int dim = Config::Model::FEATURE_DIM;  // 固定常量
 *   float threshold = Config::Default::RECOGNITION_THRESHOLD;  // 默认值
 */

#pragma once

/*
0:仅摄像头，无人脸识别
1:摄像头+人脸识别(未完成)
*/
# define PROJECT_MODE        1

namespace Config {

// ==================== 路径配置 [固定] ====================
namespace Path {
    // 模型绝对路径 (针对 RK3588 开发板环境)
    constexpr const char* YOLO_MODEL = "/home/firefly/RK_NPU2_SDK/my_model/rknn/yolov8n-face-zjykzj.rknn";
    constexpr const char* FACENET_MODEL = "/home/firefly/RK_NPU2_SDK/my_model/rknn/w600k_resnet50.rknn";
    
    // 数据路径 (绝对路径)
    constexpr const char* FEATURE_LIB = "/home/firefly/cjh/cam_demo/data/face_feature_lib/";
    constexpr const char* DATABASE = "./data.db";
}

// ==================== 模型参数 [固定] ====================
namespace Model {
    constexpr int FEATURE_DIM = 512;               // 特征向量维度 (MobileFaceNet)
    constexpr int YOLO_INPUT_SIZE = 640;           // YOLO 输入尺寸
}
// ==================== 性能参数 [固定] ====================
namespace Performance {
    constexpr int REPORT_INTERVAL = 50;            // 性能报告间隔 (帧数)
    constexpr int QUEUE_MAX_SIZE = 2;              // 线程队列最大大小
    constexpr bool USE_RGA = true;                // 是否启用RGA硬件加速 (禁用可避免Valgrind警告)
}
// ==================== 摄像头参数 [固定] ====================
namespace Camera {
    constexpr int WIDTH = 1280;                    // 摄像头宽度1280
    constexpr int HEIGHT = 720;                    // 摄像头高度720
    constexpr bool USE_ASYNC_USB = true;           // 异步USB读取 (固定开启)
}

// ==================== 检测参数 [固定] ====================
namespace Detection {
    constexpr float BOX_CONF_THRESHOLD = 0.5f;     // 人脸检测置信度阈值
    constexpr float NMS_THRESHOLD = 0.45f;         // NMS阈值
}

// ==================== 默认值 [UI 可配置] ====================
// 这些值仅作为 ConfigManager 的初始默认值
// 运行时应从 ConfigManager 读取用户设置
namespace Default {
    // 识别设置
    constexpr float RECOGNITION_THRESHOLD = 0.60f; // 人脸识别相似度阈值
    constexpr int DUPLICATE_CHECK_INTERVAL = 300;  // 防重复打卡间隔 (秒)
    constexpr int RECOGNITION_CONFIRM_COUNT = 5;   // 识别确认次数
    constexpr int USER_CONFIRM_DURATION_MS = 1000; // 用户确认时间 (毫秒)

    // 音频设置
    constexpr int AUDIO_VOLUME = 100;               // 默认音量 (0-100)
    constexpr bool AUDIO_ENABLED = true;           // 默认启用音频

    // 考勤设置
    constexpr int WORK_START_HOUR = 9;             // 上班时间 (时)
    constexpr int WORK_START_MINUTE = 0;           // 上班时间 (分)
    constexpr int WORK_END_HOUR = 18;              // 下班时间 (时)
    constexpr int WORK_END_MINUTE = 0;             // 下班时间 (分)
    constexpr int LATE_THRESHOLD = 30;             // 迟到阈值 (分钟)
    constexpr int EARLY_LEAVE_THRESHOLD = 30;      // 早退阈值 (分钟)

    // 摄像头设置
    constexpr int CAMERA_ID = 21;                  // 默认摄像头ID

    // 设备信息
    constexpr const char* DEVICE_ID = "device_001";    // 设备ID
    constexpr const char* LOCATION = "Main Entrance";  // 设备位置
}

} // namespace Config