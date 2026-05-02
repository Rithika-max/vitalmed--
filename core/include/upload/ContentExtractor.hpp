#pragma once
#include <string>

namespace vetalmed::upload {

class ContentExtractor {
public:
    static std::string extract(const std::string &fileType, const std::string &path);
private:
    static std::string extractCsvContent(const std::string &content);
    static std::string extractJsonContent(const std::string &content);
};

}
