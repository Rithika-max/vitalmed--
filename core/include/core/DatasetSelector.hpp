#pragma once
#include <string>

namespace vetalmed::core {

class DatasetSelector {
public:
    static std::string selectBestDataset(const std::string &query);
};

}
