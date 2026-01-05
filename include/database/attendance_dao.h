#ifndef ATTENDANCE_DAO_H
#define ATTENDANCE_DAO_H

#include "database/database_types.h"
#include <vector>
#include <optional>
#include <string>

namespace db {

/**
 * @brief 包含用户信息的考勤记录 (联表查询结果)
 */
struct AttendanceJoinedRecord : public AttendanceRecord {
    std::string user_name;
    std::string department;
};

class AttendanceDao {
public:
    int64_t add_record(const AttendanceRecord& record);
    bool delete_record(int64_t record_id);
    
    // 获取用户最近一次打卡记录
    std::optional<AttendanceRecord> get_last_record(int64_t user_id);
    
    // 获取指定时间范围内的打卡记录
    std::vector<AttendanceRecord> get_records_by_user(int64_t user_id, std::time_t start_time, std::time_t end_time);

    // 获取所有考勤记录（包含用户信息）
    std::vector<AttendanceJoinedRecord> get_all_records_joined();
    
    // 搜索考勤记录（支持姓名、部门、工号搜索）
    std::vector<AttendanceJoinedRecord> search_records_joined(const std::string& keyword);
};

} // namespace db

#endif // ATTENDANCE_DAO_H
