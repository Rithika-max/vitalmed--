#pragma once
#include <string>

namespace vetalmed::upload {

class FileValidator {
public:
    static bool validate(const std::string &filename, const std::string &path, std::string &error);
};

}
