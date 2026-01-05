/**
 * @file attendance_service.cc
 * @brief 考勤业务逻辑实现
 * @details 包含打卡判定逻辑（签到/签退）、迟到早退判定以及数据持久化触发。
 */

#include "service/attendance_service.h"
#include "database/attendance_dao.h"
#include "database/user_dao.h"
#include "config/config_manager.h" // 新增
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
    
    // 获取配置
    auto& config = ConfigManager::instance();
    int start_h = config.getWorkStartHour();
    int start_m = config.getWorkStartMinute();
    int end_h = config.getWorkEndHour();
    int end_m = config.getWorkEndMinute();
    int late_thresh = config.getLateThreshold();
    int early_thresh = config.getEarlyLeaveThreshold();

    int current_mins = local_tm->tm_hour * 60 + local_tm->tm_min;
    int start_mins = start_h * 60 + start_m;
    int end_mins = end_h * 60 + end_m;

    if (records.empty()) {
        // 今日首次打卡 -> 签到
        new_record.check_type = 1; // 签到
        
        // 判定状态：在 (上班时间 + 迟到阈值) 之前都算正常
        if (current_mins <= start_mins + late_thresh) {
            new_record.status = 1; // 正常
        } else {
            new_record.status = 2; // 迟到
        }
    } else {
        // 已有打卡 -> 签退 (或更新签退)
        // 防抖：如果上一条记录在1分钟内，忽略
        db::AttendanceRecord last = records.back();
        if (now - last.check_time < 60) {
            return -1; // 忽略频繁打卡
        }
        
        new_record.check_type = 2; // 签退
        
        // 判定状态：在 (下班时间 - 早退阈值) 之后都算正常
        if (current_mins >= end_mins - early_thresh) {
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
