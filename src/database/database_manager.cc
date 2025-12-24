/**
 * @file database_manager.cc
 * @brief 数据库连接管理实现
 * @details 负责 SQLite 数据库的打开、关闭、事务处理以及基础表结构的自动创建。
 */

#include "database/database_manager.h"
#include <iostream>

namespace db {

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager() {}

DatabaseManager::~DatabaseManager() {
    close();
}

bool DatabaseManager::open(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (db_) {
        return true; // 已经打开
    }

    int rc = sqlite3_open(path.c_str(), &db_);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    // 开启外键约束支持
    execute("PRAGMA foreign_keys = ON;");

    // 创建表结构
    if (!create_tables()) {
        close();
        return false;
    }

    return true;
}

void DatabaseManager::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool DatabaseManager::execute(const std::string& sql) {
    if (!db_) return false;

    char* zErrMsg = 0;
    int rc = sqlite3_exec(db_, sql.c_str(), 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << zErrMsg << "\nSQL: " << sql << std::endl;
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

bool DatabaseManager::begin_transaction() {
    return execute("BEGIN TRANSACTION;");
}

bool DatabaseManager::commit_transaction() {
    return execute("COMMIT;");
}

bool DatabaseManager::rollback_transaction() {
    return execute("ROLLBACK;");
}

bool DatabaseManager::create_tables() {
    const char* sql_users = 
        "CREATE TABLE IF NOT EXISTS users ("
        "user_id INTEGER PRIMARY KEY AUTOINCREMENT," 
        "user_name TEXT UNIQUE NOT NULL," 
        "employee_id TEXT," 
        "department TEXT," 
        "status INTEGER DEFAULT 1"
        ");";

    const char* sql_features = 
        "CREATE TABLE IF NOT EXISTS face_features (" 
        "feature_id INTEGER PRIMARY KEY AUTOINCREMENT," 
        "user_id INTEGER," 
        "feature_vector BLOB," 
        "feature_quality REAL," 
        "FOREIGN KEY(user_id) REFERENCES users(user_id) ON DELETE CASCADE"
        ");";

    const char* sql_records = 
        "CREATE TABLE IF NOT EXISTS attendance_records (" 
        "record_id INTEGER PRIMARY KEY AUTOINCREMENT," 
        "user_id INTEGER," 
        "check_time INTEGER," 
        "check_type INTEGER," 
        "status INTEGER," 
        "similarity REAL," 
        "FOREIGN KEY(user_id) REFERENCES users(user_id) ON DELETE CASCADE"
        ");";
    
    // 索引
    const char* sql_idx_name = "CREATE INDEX IF NOT EXISTS idx_users_name ON users(user_name);";
    const char* sql_idx_time = "CREATE INDEX IF NOT EXISTS idx_records_time ON attendance_records(check_time);";
    const char* sql_idx_user = "CREATE INDEX IF NOT EXISTS idx_records_user ON attendance_records(user_id);";

    return execute(sql_users) && 
           execute(sql_features) && 
           execute(sql_records) &&
           execute(sql_idx_name) &&
           execute(sql_idx_time) &&
           execute(sql_idx_user);
}

} // namespace db
