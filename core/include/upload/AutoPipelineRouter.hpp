#pragma once
#include <string>

namespace vetalmed::upload {

enum class PipelineType {
    STRUCTURED,
    UNSTRUCTURED,
    HYBRID
};

class AutoPipelineRouter {
public:
    static PipelineType route(bool hasStructured, bool hasUnstructured);
};

}
