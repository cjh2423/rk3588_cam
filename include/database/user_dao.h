#ifndef USER_DAO_H
#define USER_DAO_H

#include "database/database_types.h"
#include <optional>
#include <vector>

namespace db {

class UserDao {
public:
    // 添加用户，返回生成的 user_id，失败返回 -1
    int64_t add_user(const User& user);
    
    // 查询
    std::optional<User> get_user_by_id(int64_t user_id);
    std::optional<User> get_user_by_name(const std::string& name);
    
    // 获取所有启用用户
    std::vector<User> get_all_active_users();
    
    // 更新
    bool update_user(const User& user);
    
    // 删除
    bool delete_user(int64_t user_id);
};

} // namespace db

#endif // USER_DAO_H
