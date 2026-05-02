#include "core/DecisionRouter.hpp"

namespace vetalmed::core {

IntentType DecisionRouter::route(const IntentType intent, bool hasStructured, bool hasUnstructured, bool hasApiData) {
    switch (intent) {
        case IntentType::API:
            return hasApiData ? IntentType::API : IntentType::UNKNOWN;
        case IntentType::STRUCTURED:
            return hasStructured ? IntentType::STRUCTURED : IntentType::UNKNOWN;
        case IntentType::RAG:
            return hasUnstructured ? IntentType::RAG : IntentType::UNKNOWN;
        case IntentType::HYBRID:
            if (hasStructured && hasUnstructured) {
                return IntentType::HYBRID;
            }
            if (hasStructured) {
                return IntentType::STRUCTURED;
            }
            if (hasUnstructured) {
                return IntentType::RAG;
            }
            if (hasApiData) {
                return IntentType::API;
            }
            return IntentType::UNKNOWN;
        default:
            if (hasStructured && hasUnstructured) {
                return IntentType::HYBRID;
            }
            if (hasStructured) {
                return IntentType::STRUCTURED;
            }
            if (hasApiData) {
                return IntentType::API;
            }
            if (hasUnstructured) {
                return IntentType::RAG;
            }
            return IntentType::UNKNOWN;
    }
}

}
