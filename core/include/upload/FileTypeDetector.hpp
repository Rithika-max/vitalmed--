#pragma once
#include <string>

namespace vetalmed::upload {

class FileTypeDetector {
public:
    static std::string detectType(const std::string &filename, const std::string &path);
};

}
