#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace vetalmed::core {

class OllamaHealthChecker {
public:
    static bool isOllamaOnline(const std::string &ollamaUrl = "http://localhost:11434");
    static nlohmann::json getHealthStatus(const std::string &ollamaUrl = "http://localhost:11434");
};

}
