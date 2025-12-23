# RK3588 AI Camera Demo (cam_demo)

## Project Overview
本项目是一个专为 **Rockchip RK3588** 设计的高效异步视频处理框架。它实现了采集、预处理（RGA）、AI 推理（RKNN）与 UI 渲染（Qt5）的完全解耦。

### 核心架构
*   **双异步线程**：
    1.  **PreprocessingThread**: 负责 V4L2 采集与 RGA 硬件加速（缩放、翻转）。
    2.  **InferenceThread**: 负责 NPU 推理（YOLOv8-face 检测）与结果后处理。
*   **低耦合设计**：Core 层保持算法纯净，业务逻辑封装在 App 层。
*   **硬件加速**：
    *   **RGA**: 替代 CPU 进行图像预处理。
    *   **NPU**: 执行 YOLOv8 和 FaceNet 模型。

## Directory Structure
```text
/
├── 3rdparty/           # RKNN/RGA 库与头文件
├── build.sh            # 虚拟机交叉编译脚本
├── run.sh              # 开发板运行脚本（含环境设置与日志过滤）
├── doc/                # 详细开发与部署文档
├── include/
│   ├── app/            # 应用逻辑（Controller, Threads）
│   ├── core/           # 算法实现（YOLO, FaceNet, ModelManager）
│   └── hardware/       # 硬件封装（CameraDevice）
├── src/                # 源代码实现
└── config.h            # 全局配置（含模型绝对路径）
```

## Quick Start (Deploy)
1.  **编译**: `./build.sh`
2.  **传输**: 使用 `adb push` 将 `cam_demo`, `run.sh`, `3rdparty` 传至板子。
3.  **运行**: `adb shell "cd /home/firefly/cjh/cam_demo/ && ./run.sh"`

详细步骤请参阅 `doc/部署.md`。