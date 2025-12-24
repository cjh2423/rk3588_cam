/**
 * @file attendance_service.cc
 * @brief 考勤业务逻辑实现
 * @details 包含打卡判定逻辑（签到/签退）、迟到早退判定以及数据持久化触发。
 */

#include "service/attendance_service.h"
#include "database/attendance_dao.h"
#include "database/user_dao.h"
#include <ctime>
#include <iostream>

namespace service {

AttendanceService::AttendanceService() {}

int64_t AttendanceService::record_attendance(int64_t user_id, float similarity) {
    db::AttendanceDao attendance_dao;
    
    std::time_t now = std::time(nullptr);
    std::tm* local_tm = std::localtime(&now);
    
    // 获取今日零点
    std::tm today_tm = *local_tm;
    today_tm.tm_hour = 0;
    today_tm.tm_min = 0;
    today_tm.tm_sec = 0;
    std::time_t today_start = std::mktime(&today_tm);
    
    // 获取今日记录
    auto records = attendance_dao.get_records_by_user(user_id, today_start, now);
    
    db::AttendanceRecord new_record;
    new_record.user_id = user_id;
    new_record.check_time = now;
    new_record.similarity = similarity;
    
    if (records.empty()) {
        // 今日首次打卡 -> 签到
        new_record.check_type = 1; // 签到
        
        // 判定状态 (假设9点上班)
        if (local_tm->tm_hour < WORK_START_HOUR) {
            new_record.status = 1; // 正常
        } else {
            new_record.status = 2; // 迟到
        }
    } else {
        // 已有打卡 -> 签退 (或更新签退)
        // 这里简单处理：只要有签到，之后的都算签退
        // 实际逻辑可能需要防抖（例如1分钟内不重复打卡）
        
        // 防抖：如果上一条记录在1分钟内，忽略
        db::AttendanceRecord last = records.back();
        if (now - last.check_time < 60) {
            return -1; // 忽略频繁打卡
        }
        
        new_record.check_type = 2; // 签退
        
        // 判定状态 (假设18点下班)
        if (local_tm->tm_hour >= 18) {
            new_record.status = 1; // 正常
        } else {
            new_record.status = 3; // 早退
        }
    }
    
    int64_t id = attendance_dao.add_record(new_record);
    if (id != -1) {
        std::cout << "User " << user_id << " attendance recorded: Type=" << new_record.check_type 
                  << ", Status=" << new_record.status << std::endl;
    }
    return id;
}

} // namespace service
