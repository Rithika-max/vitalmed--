#pragma once
#include <string>

namespace vetalmed::core {

class Validator {
public:
    static bool validateQuery(const std::string &query, std::string &error);
};

}
