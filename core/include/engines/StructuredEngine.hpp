#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace vetalmed::engines {

class StructuredEngine {
public:
    static nlohmann::json execute(const std::string &query);
};

}
