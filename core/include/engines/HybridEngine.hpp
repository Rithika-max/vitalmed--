#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace vetalmed::engines {

class HybridEngine {
public:
    static nlohmann::json execute(const std::string &query);
};

}
