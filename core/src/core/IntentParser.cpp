#include "core/IntentParser.hpp"
#include <algorithm>

namespace vetalmed::core {

static std::string lower(const std::string &text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

IntentType IntentParser::parseIntent(const std::string &query) {
    auto normalized = lower(query);
    if (normalized.find("erp") != std::string::npos || normalized.find("api") != std::string::npos || normalized.find("order") != std::string::npos || normalized.find("invoice") != std::string::npos || normalized.find("purchase") != std::string::npos || normalized.find("vendor") != std::string::npos) {
        return IntentType::API;
    }
    if (normalized.find("report") != std::string::npos || normalized.find("sales") != std::string::npos || normalized.find("inventory") != std::string::npos || normalized.find("balance") != std::string::npos || normalized.find("total") != std::string::npos || normalized.find("sum") != std::string::npos || normalized.find("average") != std::string::npos || normalized.find("count") != std::string::npos || normalized.find("quantity") != std::string::npos || normalized.find("revenue") != std::string::npos || normalized.find("expense") != std::string::npos) {
        return IntentType::STRUCTURED;
    }
    if (normalized.find("document") != std::string::npos || normalized.find("contract") != std::string::npos || normalized.find("pdf") != std::string::npos || normalized.find("note") != std::string::npos || normalized.find("agreement") != std::string::npos || normalized.find("policy") != std::string::npos) {
        return IntentType::RAG;
    }
    if (normalized.find("combine") != std::string::npos || normalized.find("hybrid") != std::string::npos || normalized.find("insight") != std::string::npos || normalized.find("analysis") != std::string::npos) {
        return IntentType::HYBRID;
    }
    return IntentType::UNKNOWN;
}

}
