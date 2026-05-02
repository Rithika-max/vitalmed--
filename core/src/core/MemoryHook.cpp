#include "core/MemoryHook.hpp"

namespace vetalmed::core {

void MemoryHook::addMemory(const std::string &sessionId, const std::string &entry) {
    memories_[sessionId] += entry + "\n";
}

std::string MemoryHook::retrieveMemory(const std::string &sessionId) const {
    auto it = memories_.find(sessionId);
    return it != memories_.end() ? it->second : "";
}

}
