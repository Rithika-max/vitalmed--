#pragma once
#include "IntentParser.hpp"

namespace vetalmed::core {

class DecisionRouter {
public:
    static IntentType route(const IntentType intent, bool hasStructured, bool hasUnstructured, bool hasApiData);
};

}
