#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace vetalmed::core {

class FallbackResponseComposer {
public:
    static nlohmann::json composeFromData(const std::string &query, 
                                         const nlohmann::json &searchResults);
    static nlohmann::json composeNoData(const std::string &query);
};

}
