#pragma once
#include <string>

namespace vetalmed::upload {

class DataQualityChecker {
public:
    static bool check(const std::string &content, std::string &report);
};

}
