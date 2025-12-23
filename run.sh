#!/bin/bash

# 1. 获取当前脚本所在目录
PROJECT_DIR=$(cd "$(dirname "$0")"; pwd)
cd $PROJECT_DIR

echo ">>> 运行目录: $PROJECT_DIR"

# 2. 设置库路径
export LD_LIBRARY_PATH=$PROJECT_DIR/3rdparty/rga/lib/Linux/aarch64:$PROJECT_DIR/3rdparty/librknn_api/aarch64:$LD_LIBRARY_PATH

# 3. 启动程序并过滤烦人的日志
echo ">>> 正在启动 cam_demo (配置由 config.h 提供)..."

# 使用 grep -v 过滤掉包含 "rk-debug" 的行
# 注意：这可能会导致程序输出有缓冲延迟，但能保持清爽
# 2>&1 将 stderr 合并到 stdout 一起过滤
./cam_demo 2>&1 | grep -v "rk-debug"