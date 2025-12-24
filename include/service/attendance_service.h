/**
 * @file attendance_service.h
 * @brief 考勤业务服务头文件
 */

#ifndef ATTENDANCE_SERVICE_H
#define ATTENDANCE_SERVICE_H

#include <string>
#include <memory>

namespace service {

class AttendanceService {
public:
    AttendanceService();
    
    /**
     * @brief 记录考勤
     * @param user_id 用户ID
     * @param similarity 相似度
     * @return 记录ID，失败返回 -1
     */
    int64_t record_attendance(int64_t user_id, float similarity);

private:
    // 简单的判定逻辑：早上9点前正常，之后迟到
    // 实际项目中应从配置读取
    const int WORK_START_HOUR = 9; 
};

} // namespace service

#endif // ATTENDANCE_SERVICE_H
