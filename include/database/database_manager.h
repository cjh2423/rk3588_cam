#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <sqlite3.h>
#include <string>
#include <mutex>
#include <functional>

namespace db {

/**
 * @brief 数据库管理器 (单例)
 * 负责 SQLite 连接的生命周期管理
 */
class DatabaseManager {
public:
    static DatabaseManager& instance();

    ~DatabaseManager();

    // 打开数据库
    bool open(const std::string& path);
    
    // 关闭数据库
    void close();

    // 执行无返回值的 SQL (建表、插入、更新等)
    bool execute(const std::string& sql);

    // 事务支持
    bool begin_transaction();
    bool commit_transaction();
    bool rollback_transaction();

    // 获取原始句柄 (供 DAO 使用)
    sqlite3* connection() const { return db_; }

    // 检查是否已连接
    bool is_open() const { return db_ != nullptr; }

private:
    DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    // 创建必要的表结构
    bool create_tables();

    sqlite3* db_ = nullptr;
    std::mutex mutex_;
};

} // namespace db

#endif // DATABASE_MANAGER_H
