#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace vetalmed::models {

struct AIRequest {
    std::string query;
    std::string session_id;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AIRequest, query, session_id)
};

}
