#include "core/ResponseComposer.hpp"

namespace vetalmed::core {

nlohmann::json ResponseComposer::compose(
    const std::string &summary,
    const std::vector<nlohmann::json> &data,
    const std::vector<std::string> &insights,
    const std::string &recommendation,
    double confidence,
    const std::string &sourceType) {

    return nlohmann::json{
        {"summary", summary},
        {"data", data},
        {"insights", insights},
        {"recommendation", recommendation},
        {"confidence", confidence},
        {"source_type", sourceType}
    };
}

}
