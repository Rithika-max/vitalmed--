#pragma once
#include <string>

namespace vetalmed::upload {

class StructureDetector {
public:
    static bool isStructured(const std::string &fileType, const std::string &content);
};

}
