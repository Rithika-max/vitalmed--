#pragma once
#include <string>

namespace vetalmed::upload {

class SchemaInferenceEngine {
public:
    static std::string infer(const std::string &fileType, const std::string &content);
};

}
