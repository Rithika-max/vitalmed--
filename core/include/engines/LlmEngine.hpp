#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace vetalmed::engines {

class LlmEngine {
public:
    static nlohmann::json summarize(const std::string &text);
};

}
