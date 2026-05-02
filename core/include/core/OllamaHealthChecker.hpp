#pragma once
#include <string>
#include <json/json.h>

namespace vetalmed::core {

class OllamaHealthChecker {
public:
    static bool isOllamaOnline(const std::string &ollamaUrl = "http://localhost:11434");
    static Json::Value getHealthStatus(const std::string &ollamaUrl = "http://localhost:11434");
};

}
