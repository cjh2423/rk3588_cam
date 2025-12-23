#!/bin/bash

# --- 1. 配置区域 ---
# 切换为系统自带的交叉编译器，以匹配系统的 Qt/OpenCV 库
CROSS_PREFIX="aarch64-linux-gnu-"

# 终端输出颜色
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# --- 2. 检查环境 ---
echo -e "${YELLOW}>>> 正在检查系统交叉编译器...${NC}"
if ! command -v ${CROSS_PREFIX}gcc &> /dev/null; then
    echo -e "${RED}[错误] 找不到交叉编译器: ${CROSS_PREFIX}gcc${NC}"
    echo "请执行: sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu"
    exit 1
fi

# --- 3. 清理与准备 ---
echo -e "${YELLOW}>>> 清理旧的编译缓存...${NC}"
rm -rf build
mkdir -p build
cd build

# --- 4. 配置 CMake ---
echo -e "${YELLOW}>>> 正在配置 CMake...${NC}"

# 注意：这里不再指定 SDK 路径，直接使用系统命令名
cmake .. \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
    -DCMAKE_C_COMPILER=${CROSS_PREFIX}gcc \
    -DCMAKE_CXX_COMPILER=${CROSS_PREFIX}g++

# --- 5. 编译 ---
if [ $? -eq 0 ]; then
    echo -e "${YELLOW}>>> 正在进行并行编译 (make -j$(nproc))...${NC}"
    make -j$(nproc)
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}=======================================${NC}"
        echo -e "${GREEN}  编译成功!${NC}"
        echo -e "${GREEN}=======================================${NC}"
    else
        echo -e "${RED}[错误] 编译阶段失败!${NC}"
        exit 1
    fi
else
    echo -e "${RED}[错误] CMake 配置失败!${NC}"
    exit 1
fi