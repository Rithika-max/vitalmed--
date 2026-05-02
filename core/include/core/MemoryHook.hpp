#pragma once
#include <string>
#include <unordered_map>

namespace vetalmed::core {

class MemoryHook {
public:
    void addMemory(const std::string &sessionId, const std::string &entry);
    std::string retrieveMemory(const std::string &sessionId) const;

private:
    std::unordered_map<std::string, std::string> memories_;
};

}
