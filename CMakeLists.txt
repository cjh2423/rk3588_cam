cmake_minimum_required(VERSION 3.14)
project(cam_demo LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)

# --- 1. Qt 自动处理配置 ---
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# --- 2. 交叉编译工具链设置 (切换到系统编译器以解决 glibc 冲突) ---
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# 使用系统 aarch64 交叉编译器，它能完美匹配 /usr/lib/aarch64-linux-gnu 目录下的库
set(CMAKE_C_COMPILER "aarch64-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "aarch64-linux-gnu-g++")

# --- 3. 依赖路径与库定义 ---
set(SDK_3RDPARTY_DIR "${CMAKE_SOURCE_DIR}/3rdparty")
set(RKNN_LIBS "${SDK_3RDPARTY_DIR}/librknn_api/aarch64/librknnrt.so")
set(RGA_LIBS  "${SDK_3RDPARTY_DIR}/rga/lib/Linux/aarch64/librga.so")

# 定位 Qt5 (使用 apt 安装在系统的路径)
set(Qt5_DIR "/usr/lib/aarch64-linux-gnu/cmake/Qt5")
find_package(Qt5 COMPONENTS Core Widgets Gui REQUIRED)

# 系统动态库列表 (不再依赖 3rdparty/opencv 的静态库)
link_directories(/usr/lib/aarch64-linux-gnu)
set(SYSTEM_LIBS 
    opencv_core 
    opencv_imgproc 
    opencv_imgcodecs 
    opencv_videoio 
    opencv_video
    opencv_highgui
    sqlite3         # sqlite3
    z 
    pthread 
    dl 
    m
)

# --- 4. 文件扫描 (包含所有源代码以修复 Undefined Reference) ---
# 扫描源文件
file(GLOB_RECURSE SRC_FILES 
    "src/*.cc" "src/*.cpp" "src/*.c"
    "gui/src/*.cc" "gui/src/*.cpp"
)
# 扫描头文件 (AUTOMOC 依赖头文件扫描)
file(GLOB_RECURSE HDR_FILES 
    "include/*.h" "include/*.hpp"
    "gui/include/*.h" "gui/include/*.hpp"
)

# 包含目录
include_directories(
    include 
    gui/include
    "${SDK_3RDPARTY_DIR}/librknn_api/include"
    "${SDK_3RDPARTY_DIR}/rga/include"
    "/usr/include/opencv4"
)

# --- 5. 生成目标 ---
# 将头文件也加入 add_executable 是触发 Qt MOC 的最佳实践
add_executable(${PROJECT_NAME} ${SRC_FILES} ${HDR_FILES})

# --- 6. 链接配置 ---
target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--allow-shlib-undefined")

target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets 
    Qt5::Gui 
    Qt5::Core
    ${SYSTEM_LIBS}
    ${RKNN_LIBS}
    ${RGA_LIBS}
)

# 设置运行时库搜索路径
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN/lib")