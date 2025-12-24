/**
 * @file user_dao.cc
 * @brief 用户信息数据访问对象实现
 */

#include "database/user_dao.h"
#include "database/database_manager.h"
#include <iostream>

namespace db {

int64_t UserDao::add_user(const User& user) {
    sqlite3* db = DatabaseManager::instance().connection();
    if (!db) return -1;

    const char* sql = "INSERT INTO users (user_name, employee_id, department, status) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    sqlite3_bind_text(stmt, 1, user.user_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user.employee_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, user.department.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, user.status);

    int64_t new_id = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        new_id = sqlite3_last_insert_rowid(db);
    } else {
        std::cerr << "Insert user failed: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return new_id;
}

std::optional<User> UserDao::get_user_by_id(int64_t user_id) {
    sqlite3* db = DatabaseManager::instance().connection();
    if (!db) return std::nullopt;

    const char* sql = "SELECT user_id, user_name, employee_id, department, status FROM users WHERE user_id = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int64(stmt, 1, user_id);

    std::optional<User> result = std::nullopt;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        User u;
        u.user_id = sqlite3_column_int64(stmt, 0);
        u.user_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* emp_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        if (emp_id) u.employee_id = emp_id;
        const char* dept = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (dept) u.department = dept;
        u.status = sqlite3_column_int(stmt, 4);
        result = u;
    }

    sqlite3_finalize(stmt);
    return result;
}

std::optional<User> UserDao::get_user_by_name(const std::string& name) {
    sqlite3* db = DatabaseManager::instance().connection();
    if (!db) return std::nullopt;

    const char* sql = "SELECT user_id, user_name, employee_id, department, status FROM users WHERE user_name = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);

    std::optional<User> result = std::nullopt;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        User u;
        u.user_id = sqlite3_column_int64(stmt, 0);
        u.user_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* emp_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        if (emp_id) u.employee_id = emp_id;
        const char* dept = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (dept) u.department = dept;
        u.status = sqlite3_column_int(stmt, 4);
        result = u;
    }

    sqlite3_finalize(stmt);
    return result;
}

std::vector<User> UserDao::get_all_active_users() {
    std::vector<User> users;
    sqlite3* db = DatabaseManager::instance().connection();
    if (!db) return users;

    const char* sql = "SELECT user_id, user_name, employee_id, department, status FROM users WHERE status = 1";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return users;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        User u;
        u.user_id = sqlite3_column_int64(stmt, 0);
        u.user_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* emp_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        if (emp_id) u.employee_id = emp_id;
        const char* dept = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (dept) u.department = dept;
        u.status = sqlite3_column_int(stmt, 4);
        users.push_back(u);
    }

    sqlite3_finalize(stmt);
    return users;
}

bool UserDao::update_user(const User& user) {
    sqlite3* db = DatabaseManager::instance().connection();
    if (!db) return false;

    const char* sql = "UPDATE users SET user_name=?, employee_id=?, department=?, status=? WHERE user_id=?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, user.user_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user.employee_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, user.department.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, user.status);
    sqlite3_bind_int64(stmt, 5, user.user_id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool UserDao::delete_user(int64_t user_id) {
    sqlite3* db = DatabaseManager::instance().connection();
    if (!db) return false;

    const char* sql = "DELETE FROM users WHERE user_id = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int64(stmt, 1, user_id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

} // namespace db
