/**
 * @file feature_library.cc
 * @brief 人脸特征库管理实现
 * @details 负责从数据库加载已注册的人脸特征，并提供基于向量相似度的 1:N 检索功能。
 */

#include "service/feature_library.h"
#include "database/face_feature_dao.h"
#include <cmath>
#include <iostream>

namespace service {

FeatureLibrary& FeatureLibrary::instance() {
    static FeatureLibrary instance;
    return instance;
}

void FeatureLibrary::load_from_database() {
    std::lock_guard<std::mutex> lock(mutex_);
    db::FaceFeatureDao dao;
    auto db_features = dao.get_all_features();
    
    features_.clear();
    for (const auto& df : db_features) {
        LoadedFeature lf;
        lf.user_id = df.user_id;
        lf.feature = df.feature_vector;
        normalize(lf.feature);
        features_.push_back(lf);
    }
    
    std::cout << "Loaded " << features_.size() << " face features from database." << std::endl;
}

int64_t FeatureLibrary::search(const std::vector<float>& feature, float threshold, float& out_similarity) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 输入特征也需要归一化
    std::vector<float> input_norm = feature;
    normalize(input_norm);
    
    int64_t best_user_id = -1;
    float max_sim = -1.0f;
    
    for (const auto& lf : features_) {
        float sim = cosine_similarity(input_norm, lf.feature);
        if (sim > max_sim) {
            max_sim = sim;
            best_user_id = lf.user_id;
        }
    }
    
    out_similarity = max_sim;
    if (max_sim >= threshold) {
        return best_user_id;
    }
    
    return -1;
}

float FeatureLibrary::cosine_similarity(const std::vector<float>& f1, const std::vector<float>& f2) {
    if (f1.size() != f2.size() || f1.empty()) return 0.0f;
    
    // 假设都已经归一化了，dot product 就是 cosine similarity
    float dot = 0.0f;
    for (size_t i = 0; i < f1.size(); ++i) {
        dot += f1[i] * f2[i];
    }
    return dot;
}

void FeatureLibrary::normalize(std::vector<float>& feature) {
    float sq_sum = 0.0f;
    for (float v : feature) {
        sq_sum += v * v;
    }
    float norm = std::sqrt(sq_sum);
    if (norm > 1e-6) {
        for (float& v : feature) {
            v /= norm;
        }
    }
}

} // namespace service
