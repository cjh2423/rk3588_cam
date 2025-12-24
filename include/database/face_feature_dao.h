#ifndef FACE_FEATURE_DAO_H
#define FACE_FEATURE_DAO_H

#include "database/database_types.h"
#include <vector>

namespace db {

class FaceFeatureDao {
public:
    int64_t add_feature(const FaceFeature& feature);
    
    std::vector<FaceFeature> get_features_by_user_id(int64_t user_id);
    
    // 获取所有特征（用于系统启动时加载到内存）
    std::vector<FaceFeature> get_all_features();
    
    bool delete_feature(int64_t feature_id);
    bool delete_features_by_user_id(int64_t user_id);
};

} // namespace db

#endif // FACE_FEATURE_DAO_H
