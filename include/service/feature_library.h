/**
 * @file feature_library.h
 * @brief 人脸特征库管理头文件
 */

#ifndef FEATURE_LIBRARY_H
#define FEATURE_LIBRARY_H

#include "database/database_types.h"
#include <vector>
#include <mutex>

namespace service {

struct LoadedFeature {
    int64_t user_id;
    std::vector<float> feature; // 归一化后的特征
};

class FeatureLibrary {
public:
    static FeatureLibrary& instance();
    
    // 从数据库加载特征
    void load_from_database();
    
    // 搜索最相似的人脸
    // 返回 user_id, 没找到返回 -1
    int64_t search(const std::vector<float>& feature, float threshold, float& out_similarity);

private:
    FeatureLibrary() = default;
    
    // 计算余弦相似度
    float cosine_similarity(const std::vector<float>& f1, const std::vector<float>& f2);
    
    // 归一化向量
    void normalize(std::vector<float>& feature);

    std::vector<LoadedFeature> features_;
    std::mutex mutex_;
};

} // namespace service

#endif // FEATURE_LIBRARY_H
