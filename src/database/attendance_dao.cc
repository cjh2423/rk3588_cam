/**
 * @file attendance_dao.cc
 * @brief 考勤记录数据访问对象实现
 */

#include "database/attendance_dao.h"
#include "database/database_manager.h"
#include <iostream>

namespace db {

int64_t AttendanceDao::add_record(const AttendanceRecord& record) {
    sqlite3* db = DatabaseManager::instance().connection();
    if (!db) return -1;

    const char* sql = "INSERT INTO attendance_records (user_id, check_time, check_type, status, similarity) VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, record.user_id);
    sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(record.check_time));
    sqlite3_bind_int(stmt, 3, record.check_type);
    sqlite3_bind_int(stmt, 4, record.status);
    sqlite3_bind_double(stmt, 5, record.similarity);

    int64_t new_id = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        new_id = sqlite3_last_insert_rowid(db);
    } else {
        std::cerr << "Insert record failed: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return new_id;
}

std::optional<AttendanceRecord> AttendanceDao::get_last_record(int64_t user_id) {
    sqlite3* db = DatabaseManager::instance().connection();
    if (!db) return std::nullopt;

    // 按时间倒序取第一条
    const char* sql = "SELECT record_id, user_id, check_time, check_type, status, similarity FROM attendance_records WHERE user_id = ? ORDER BY check_time DESC LIMIT 1";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int64(stmt, 1, user_id);

    std::optional<AttendanceRecord> result = std::nullopt;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        AttendanceRecord r;
        r.record_id = sqlite3_column_int64(stmt, 0);
        r.user_id = sqlite3_column_int64(stmt, 1);
        r.check_time = static_cast<std::time_t>(sqlite3_column_int64(stmt, 2));
        r.check_type = sqlite3_column_int(stmt, 3);
        r.status = sqlite3_column_int(stmt, 4);
        r.similarity = sqlite3_column_double(stmt, 5);
        result = r;
    }

    sqlite3_finalize(stmt);
    return result;
}

std::vector<AttendanceRecord> AttendanceDao::get_records_by_user(int64_t user_id, std::time_t start_time, std::time_t end_time) {
    std::vector<AttendanceRecord> records;
    sqlite3* db = DatabaseManager::instance().connection();
    if (!db) return records;

    const char* sql = "SELECT record_id, user_id, check_time, check_type, status, similarity FROM attendance_records WHERE user_id = ? AND check_time BETWEEN ? AND ? ORDER BY check_time ASC";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return records;

    sqlite3_bind_int64(stmt, 1, user_id);
    sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(start_time));
    sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(end_time));

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        AttendanceRecord r;
        r.record_id = sqlite3_column_int64(stmt, 0);
        r.user_id = sqlite3_column_int64(stmt, 1);
        r.check_time = static_cast<std::time_t>(sqlite3_column_int64(stmt, 2));
        r.check_type = sqlite3_column_int(stmt, 3);
        r.status = sqlite3_column_int(stmt, 4);
        r.similarity = sqlite3_column_double(stmt, 5);
        records.push_back(r);
    }

    sqlite3_finalize(stmt);
    return records;
}

} // namespace db
