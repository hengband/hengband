#pragma once

#include <vector>

enum tr_type : int;
enum class AttributeType;
struct dragonbreath_type {
    tr_type flag;
    AttributeType type;
    std::string name;
};

extern const std::vector<dragonbreath_type> dragonbreath_info;
