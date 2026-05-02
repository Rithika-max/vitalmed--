#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace vetalmed::engines {

class RagEngine {
public:
    static nlohmann::json execute(const std::string &query);
};

}
