#pragma once

#include "object-enchant/tr-flags.h"
#include <string>
#include <utility>
#include <vector>

enum tr_type : int;
enum class AttributeType;
class DragonBreathType {
public:
    tr_type flag;
    AttributeType type;
    std::string name;
};

class DragonBreaths {
public:
    static std::vector<std::pair<AttributeType, std::string>> get_breaths(const TrFlags &flags);
    static std::string build_description(const TrFlags &flags);
};
