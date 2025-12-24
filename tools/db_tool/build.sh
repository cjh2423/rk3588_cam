#!/bin/bash

# --- 配置 ---
CROSS_PREFIX="aarch64-linux-gnu-"

# 颜色定义
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}>>> 正在编译 db_tool (交叉编译: aarch64)...${NC}"

# 检查编译器
if ! command -v ${CROSS_PREFIX}g++ &> /dev/null; then
    echo -e "${RED}[错误] 找不到交叉编译器: ${CROSS_PREFIX}g++${NC}"
    exit 1
fi

# 创建并进入构建目录
mkdir -p build
cd build

# 执行 CMake
cmake .. \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
    -DCMAKE_C_COMPILER=${CROSS_PREFIX}gcc \
    -DCMAKE_CXX_COMPILER=${CROSS_PREFIX}g++

# 编译
if [ $? -eq 0 ]; then
    make -j$(nproc)
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}=======================================${NC}"
        echo -e "${GREEN}  db_tool 编译成功!${NC}"
        echo -e "${GREEN}  可执行文件位于: tools/db_tool/build/db_tool${NC}"
        echo -e "${GREEN}=======================================${NC}"
    else
        echo -e "${RED}[错误] 编译失败!${NC}"
        exit 1
    fi
else
    echo -e "${RED}[错误] CMake 配置失败!${NC}"
    exit 1
fi
