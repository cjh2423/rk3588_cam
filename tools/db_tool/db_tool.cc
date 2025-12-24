#include <iostream>
#include <string>
#include <vector>
#include "database/database_manager.h"
#include "database/user_dao.h"
#include "database/attendance_dao.h"

void print_usage() {
    std::cout << "Usage:" << std::endl;
    std::cout << "  db_tool init <db_path>" << std::endl;
    std::cout << "  db_tool add_user <db_path> <name> <dept>" << std::endl;
    std::cout << "  db_tool list_users <db_path>" << std::endl;
    std::cout << "  db_tool stats <db_path>" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage();
        return 1;
    }

    std::string command = argv[1];
    std::string db_path = argv[2];

    if (!db::DatabaseManager::instance().open(db_path)) {
        std::cerr << "Failed to open database: " << db_path << std::endl;
        return 1;
    }

    if (command == "init") {
        std::cout << "Database initialized successfully at " << db_path << std::endl;
    } 
    else if (command == "add_user") {
        if (argc < 5) {
            std::cout << "Usage: db_tool add_user <db_path> <name> <dept>" << std::endl;
            return 1;
        }
        std::string name = argv[3];
        std::string dept = argv[4];
        
        db::User user;
        user.user_name = name;
        user.department = dept;
        user.status = 1;
        
        db::UserDao dao;
        int64_t id = dao.add_user(user);
        if (id != -1) {
            std::cout << "User added. ID: " << id << std::endl;
        } else {
            std::cerr << "Failed to add user." << std::endl;
        }
    }
    else if (command == "list_users") {
        db::UserDao dao;
        auto users = dao.get_all_active_users();
        std::cout << "ID\tName\tDept" << std::endl;
        for (const auto& u : users) {
            std::cout << u.user_id << "\t" << u.user_name << "\t" << u.department << std::endl;
        }
    }
    else if (command == "stats") {
        db::UserDao udao;
        auto users = udao.get_all_active_users();
        std::cout << "Total Active Users: " << users.size() << std::endl;
    }
    else {
        print_usage();
        return 1;
    }

    return 0;
}
