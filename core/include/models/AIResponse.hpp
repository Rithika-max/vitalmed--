#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace vetalmed::models {

struct AIResponse {
    std::string summary;
    std::vector<nlohmann::json> data;
    std::vector<std::string> insights;
    std::string recommendation;
    double confidence = 0.0;
    std::string source_type;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AIResponse, summary, data, insights, recommendation, confidence, source_type)
};

}
