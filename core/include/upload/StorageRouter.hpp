#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace vetalmed::upload {

class StorageRouter {
public:
    static bool persist(const std::string &filename,
                        const std::string &fileType,
                        const std::string &content,
                        const std::string &schema,
                        int &metadataId,
                        std::string &error);
};

}
