#pragma once
#include <string>

namespace vetalmed::core {

enum class IntentType {
    API,
    STRUCTURED,
    RAG,
    HYBRID,
    UNKNOWN
};

class IntentParser {
public:
    static IntentType parseIntent(const std::string &query);
};

}
