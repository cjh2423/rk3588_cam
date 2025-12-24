cmake_minimum_required(VERSION 3.14)
project(db_tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# 使用系统 aarch64 交叉编译器
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER "aarch64-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "aarch64-linux-gnu-g++")

include_directories(../../include)

set(SYSTEM_LIBS 
    sqlite3
    pthread 
    dl
)

add_executable(db_tool 
    db_tool.cc
    ../../src/database/database_manager.cc
    ../../src/database/user_dao.cc
    ../../src/database/attendance_dao.cc
    ../../src/database/face_feature_dao.cc
)

target_link_libraries(db_tool ${SYSTEM_LIBS})
