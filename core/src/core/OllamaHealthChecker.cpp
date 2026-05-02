#include "core/OllamaHealthChecker.hpp"

namespace vetalmed::core {

bool OllamaHealthChecker::isOllamaOnline(const std::string &ollamaUrl) {
    // Simple check: Ollama typically runs on localhost:11434
    // For now, return false to use rule-based mode
    // TODO: Implement async HTTP health check
    return false;
}

nlohmann::json OllamaHealthChecker::getHealthStatus(const std::string &ollamaUrl) {
    nlohmann::json status;
    status["ollama"] = "offline";
    status["models"] = nlohmann::json::array();
    return status;
}

}
