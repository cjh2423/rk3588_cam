#ifndef DATABASE_TYPES_H
#define DATABASE_TYPES_H

#include <string>
#include <vector>
#include <cstdint>
#include <ctime>

namespace db {

/**
 * @brief 用户基本信息
 */
struct User {
    int64_t user_id = -1;           // 主键
    std::string user_name;          // 姓名
    std::string employee_id;        // 工号
    std::string department;         // 部门
    int status = 1;                 // 1=启用, 0=禁用
};

/**
 * @brief 人脸特征
 */
struct FaceFeature {
    int64_t feature_id = -1;        // 主键
    int64_t user_id = -1;           // 外键
    std::vector<float> feature_vector; // 512维特征向量
    float feature_quality = 0.0f;   // 注册时的画质评分
};

/**
 * @brief 考勤记录
 */
struct AttendanceRecord {
    int64_t record_id = -1;         // 主键
    int64_t user_id = -1;           // 用户ID
    std::time_t check_time;         // 打卡时间
    int check_type = 1;             // 1=签到, 2=签退
    int status = 1;                 // 1=正常, 2=迟到, 3=早退
    float similarity = 0.0f;        // 相似度
};

} // namespace db

#endif // DATABASE_TYPES_H
