# RK3588 AI Camera Demo (cam_demo)

## Project Overview
This project is a decoupled, high-performance asynchronous video processing framework designed specifically for the **Rockchip RK3588** platform. It combines **standard C++11** threading for compute-intensive tasks with **Qt 5** for UI rendering.

### Key Features
*   **Architecture**: "Business Logic" (Capture/AI) is completely separated from "Display Framework" (Qt).
*   **Zero-Copy UI**: The Qt UI thread does not perform any image processing, ensuring a smooth interface even under 100% CPU/NPU load.
*   **Hardware Acceleration**:
    *   **NPU**: RKNN for deep learning inference (YOLOv8-face, FaceNet).
    *   **RGA**: Hardware-accelerated 2D graphics (resize, format conversion) using `librga`.
*   **Input**: V4L2 camera capture (MJPG/YUY2).

## Directory Structure

```text
/
├── 3rdparty/           # External libraries (librknn_api, rga, stb)
├── build.sh            # Automated cross-compilation script
├── CMakeLists.txt      # CMake build configuration
├── doc/                # Documentation (Architecture, Modules, etc.)
├── gui/                # Qt UI implementation
│   ├── src/            # UI source files (cameraview.cpp)
│   └── include/        # UI headers
├── include/            # Core headers
│   ├── app/            # Application logic (Controller, Monitor)
│   ├── core/           # AI Engine (Facenet, YOLOv8, Model Manager)
│   └── hardware/       # Hardware abstraction (CameraDevice)
└── src/                # Source code
    ├── main.cc         # Entry point
    ├── app/            # Application logic implementation
    ├── core/           # Core AI/Processing implementation
    └── hardware/       # Camera implementation
```

## Prerequisites

The project is configured for **cross-compilation** on a Linux host for an **aarch64** target (RK3588).

### Host System Requirements
*   **Cross-Compiler**: `aarch64-linux-gnu-gcc` / `g++`
*   **Qt5**: `qt5-default` (or similar package for aarch64)
*   **CMake**: Version 3.14+

### Target System Requirements (RK3588)
*   **Libraries**: `libopencv`, `librga`, `librknnrt`, `libqt5`
*   **Drivers**: V4L2 camera drivers

## Build Instructions

1.  **Install Cross-Compiler (Debian/Ubuntu)**:
    ```bash
    sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
    ```

2.  **Run Build Script**:
    The project includes a helper script to clean, configure, and build:
    ```bash
    ./build.sh
    ```
    *This script sets up the `aarch64-linux-gnu` toolchain and runs `make -j$(nproc)`.*

    **Manual Build**:
    ```bash
    mkdir build && cd build
    cmake .. -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
    make -j4
    ```

## Running the Application

The application is designed to run on the RK3588 target board.

```bash
./cam_test [yolov8_model_path] [facenet_model_path] [camera_source] [camera_id] [db_path]
```

*   **Default Configuration**: Loaded from `config.h` if arguments are not provided.
*   **Arguments**:
    1.  `yolov8_model_path`: Path to YOLOv8 face detection model.
    2.  `facenet_model_path`: Path to FaceNet recognition model.
    3.  `camera_source`: Camera type (default "usb").
    4.  `camera_id`: Camera device ID (integer, default is typically 21 or similar in config).
    5.  `db_path`: Path to the SQLite database.

## Architecture Details

### Threading Model
*   **Main Thread (Qt)**: Handles the Event Loop. Receives `QImage` objects via `updateFrame` slot and renders them.
*   **Inference Worker (std::thread)**:
    1.  Captures `cv::Mat` from Camera.
    2.  Performs Preprocessing (RGA/OpenCV).
    3.  Runs NPU Inference (RKNN).
    4.  Post-processing (Drawing boxes/text).
    5.  Converts to `QImage` and pushes to Main Thread via `QMetaObject::invokeMethod`.

### Key Files
*   `src/main.cc`: Initializes `QApplication`, `AppController`, and loads configuration.
*   `src/core/algoengine.cpp`: (Mentioned in docs, possibly refactored to `model_manager` or similar in current tree) Central place for NPU execution.
*   `gui/src/cameraview.cpp`: Qt Widget for displaying the video feed.
