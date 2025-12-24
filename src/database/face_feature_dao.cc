/**
 * @file face_feature_dao.cc
 * @brief 人脸特征向量数据访问对象实现
 */

#include "database/face_feature_dao.h"
#include "database/database_manager.h"
#include <iostream>
#include <cstring>

namespace db {

int64_t FaceFeatureDao::add_feature(const FaceFeature& feature) {
    sqlite3* db = DatabaseManager::instance().connection();
    if (!db) return -1;

    const char* sql = "INSERT INTO face_features (user_id, feature_vector, feature_quality) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, feature.user_id);
    
    // Bind BLOB
    // feature.feature_vector is std::vector<float>
    // size in bytes = size() * sizeof(float)
    sqlite3_bind_blob(stmt, 2, feature.feature_vector.data(), 
                      feature.feature_vector.size() * sizeof(float), SQLITE_STATIC);
    
    sqlite3_bind_double(stmt, 3, feature.feature_quality);

    int64_t new_id = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        new_id = sqlite3_last_insert_rowid(db);
    } else {
        std::cerr << "Insert feature failed: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return new_id;
}

std::vector<FaceFeature> FaceFeatureDao::get_features_by_user_id(int64_t user_id) {
    std::vector<FaceFeature> features;
    sqlite3* db = DatabaseManager::instance().connection();
    if (!db) return features;

    const char* sql = "SELECT feature_id, user_id, feature_vector, feature_quality FROM face_features WHERE user_id = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return features;

    sqlite3_bind_int64(stmt, 1, user_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FaceFeature f;
        f.feature_id = sqlite3_column_int64(stmt, 0);
        f.user_id = sqlite3_column_int64(stmt, 1);
        
        // Read BLOB
        const void* blob_data = sqlite3_column_blob(stmt, 2);
        int blob_size = sqlite3_column_bytes(stmt, 2);
        
        int float_count = blob_size / sizeof(float);
        f.feature_vector.resize(float_count);
        if (blob_data && blob_size > 0) {
            std::memcpy(f.feature_vector.data(), blob_data, blob_size);
        }
        
        f.feature_quality = sqlite3_column_double(stmt, 3);
        features.push_back(f);
    }

    sqlite3_finalize(stmt);
    return features;
}

std::vector<FaceFeature> FaceFeatureDao::get_all_features() {
    std::vector<FaceFeature> features;
    sqlite3* db = DatabaseManager::instance().connection();
    if (!db) return features;

    const char* sql = "SELECT feature_id, user_id, feature_vector, feature_quality FROM face_features";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return features;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FaceFeature f;
        f.feature_id = sqlite3_column_int64(stmt, 0);
        f.user_id = sqlite3_column_int64(stmt, 1);
        
        const void* blob_data = sqlite3_column_blob(stmt, 2);
        int blob_size = sqlite3_column_bytes(stmt, 2);
        
        int float_count = blob_size / sizeof(float);
        f.feature_vector.resize(float_count);
        if (blob_data && blob_size > 0) {
            std::memcpy(f.feature_vector.data(), blob_data, blob_size);
        }
        
        f.feature_quality = sqlite3_column_double(stmt, 3);
        features.push_back(f);
    }

    sqlite3_finalize(stmt);
    return features;
}

bool FaceFeatureDao::delete_feature(int64_t feature_id) {
    sqlite3* db = DatabaseManager::instance().connection();
    if (!db) return false;

    const char* sql = "DELETE FROM face_features WHERE feature_id = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int64(stmt, 1, feature_id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool FaceFeatureDao::delete_features_by_user_id(int64_t user_id) {
    sqlite3* db = DatabaseManager::instance().connection();
    if (!db) return false;

    const char* sql = "DELETE FROM face_features WHERE user_id = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int64(stmt, 1, user_id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

} // namespace db
