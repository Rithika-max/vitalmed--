#include "engines/HybridEngine.hpp"
#include "engines/StructuredEngine.hpp"
#include "engines/RagEngine.hpp"
#include "engines/ReasoningEngine.hpp"

namespace vetalmed::engines {

nlohmann::json HybridEngine::execute(const std::string &query) {
    auto structured = StructuredEngine::execute(query);
    auto rag = RagEngine::execute(query);
    return ReasoningEngine::combine(structured, rag);
}

}
