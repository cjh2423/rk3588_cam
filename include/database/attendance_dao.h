#ifndef ATTENDANCE_DAO_H
#define ATTENDANCE_DAO_H

#include "database/database_types.h"
#include <vector>
#include <optional>

namespace db {

class AttendanceDao {
public:
    int64_t add_record(const AttendanceRecord& record);
    
    // 获取用户最近一次打卡记录
    std::optional<AttendanceRecord> get_last_record(int64_t user_id);
    
    // 获取指定时间范围内的打卡记录
    std::vector<AttendanceRecord> get_records_by_user(int64_t user_id, std::time_t start_time, std::time_t end_time);
};

} // namespace db

#endif // ATTENDANCE_DAO_H
