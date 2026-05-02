#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace vetalmed::engines {

class ReasoningEngine {
public:
    static nlohmann::json combine(const nlohmann::json &structuredResult,
                                  const nlohmann::json &ragResult);
};

}
