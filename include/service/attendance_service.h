/**
 * @file attendance_service.h
 * @brief 考勤业务服务头文件
 */

#ifndef ATTENDANCE_SERVICE_H
#define ATTENDANCE_SERVICE_H

#include <string>
#include <memory>
#include <optional>
#include "database/database_types.h"

namespace service {

class AttendanceService {
public:
    AttendanceService();
    
    /**
     * @brief 记录考勤
     * @param user_id 用户ID
     * @param similarity 相似度
     * @return 成功返回记录详情，失败或防抖过滤返回 nullopt
     */
    std::optional<db::AttendanceRecord> record_attendance(int64_t user_id, float similarity);

private:
};

} // namespace service

#endif // ATTENDANCE_SERVICE_H
