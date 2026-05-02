#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace vetalmed::core {

class ResponseComposer {
public:
    static nlohmann::json compose(
        const std::string &summary,
        const std::vector<nlohmann::json> &data,
        const std::vector<std::string> &insights,
        const std::string &recommendation,
        double confidence,
        const std::string &sourceType);
};

}
