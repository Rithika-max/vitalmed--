#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace vetalmed::models {

struct UploadResult {
    bool success = false;
    std::string message;
    std::string filename;
    std::string file_type;
    std::string status;
    int metadata_id = -1;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(UploadResult, success, message, filename, file_type, status, metadata_id)
};

}
