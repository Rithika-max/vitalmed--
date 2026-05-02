#include "core/Validator.hpp"

namespace vetalmed::core {

bool Validator::validateQuery(const std::string &query, std::string &error) {
    if (query.empty()) {
        error = "Query cannot be empty.";
        return false;
    }
    if (query.size() > 2000) {
        error = "Query is too long.";
        return false;
    }
    return true;
}

}
