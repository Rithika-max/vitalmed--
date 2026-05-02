#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace vetalmed::models {

struct DatasetMetadata {
    int id;
    std::string filename;
    std::string file_type;
    std::string uploaded_at;
    std::string status;
    std::string schema;
    std::string source;
    long long size_bytes = 0;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(DatasetMetadata, id, filename, file_type, uploaded_at, status, schema, source, size_bytes)
};

}
