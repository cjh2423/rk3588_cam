# RK3588 AI 相机演示 (cam_demo)

## 项目概览
本项目是一个专为 **Rockchip RK3588** 设计的高效异步视频处理框架。它实现了采集、预处理（RGA）、AI 推理（RKNN）、数据库管理（SQLite）与 UI 渲染（Qt5）的完全解耦。

### 核心特性
*   **双异步线程**：
    1.  **PreprocessingThread**: 负责 V4L2 采集与 RGA 硬件加速（缩放、翻转）。
    2.  **InferenceThread**: 负责 NPU 推理（YOLOv8-face 检测 + FaceNet 特征提取）与识别逻辑。
*   **人脸识别 & 考勤**：
    *   实时检测人脸并与数据库特征比对。
    *   **可视化反馈**：已知用户显示**绿框+姓名**，未知用户显示**红框**。
    *   **自动考勤**：识别成功后自动记录打卡时间与状态（签到/签退）。
*   **交互界面 (Qt)**：
    *   **实时预览**：显示带有 AI 标注的摄像头画面（中文界面）。
    *   **用户管理**：查看、删除已注册用户。
    *   **人脸注册**：支持手动 3 次采集模式，通过多帧平均提高特征质量。
*   **数据库支持**：使用 SQLite 存储人员信息、特征向量和识别记录。
*   **硬件加速**：
    *   **RGA**: 替代 CPU 进行图像预处理。
    *   **NPU**: 执行 YOLOv8 和 FaceNet 模型。

## 目录结构
```text
/
├── 3rdparty/           # RKNN/RGA 库与头文件
├── tools/              # 辅助工具（如 db_tool 数据库运维工具）
├── doc/                # 详细开发、部署与数据库文档
├── model/              # 模型文件目录
├── include/
│   ├── app/            # 应用逻辑（Controller, Threads）
│   ├── core/           # 算法实现（YOLO, FaceNet,ModelManager）
│   ├── database/       # 数据库访问对象 (DAO)
│   ├── service/        # 业务服务（特征库、考勤服务）
│   └── hardware/       # 硬件封装（CameraDevice）
├── src/                # 源代码实现
├── build.sh            # 虚拟机交叉编译脚本
├── run.sh              # 开发板运行脚本
└── include/config.h    # 全局配置（路径、阈值等）
```

## 快速开始 (部署)
1.  **编译程序**: `./build.sh`
2.  **编译工具**: `cd tools/db_tool && ./build.sh`
3.  **传输**: 使用 `adb push` 将可执行文件、`run.sh`、`3rdparty`、`model` 推送至开发板。
4.  **初始化数据库**: 在板端运行 `./db_tool init ./data.db`。
5.  **运行**: 执行 `./run.sh`。

详细步骤请参阅 `doc/部署.md` 和 `tools/数据库工具使用.md`。