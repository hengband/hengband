#pragma once

#include <string>
#include <vector>

enum class AttributeType;
struct named_num {
    std::string name; /* The name of this thing */
    AttributeType num; /* A number associated with it */
};

extern const std::vector<named_num> gf_descriptions;
