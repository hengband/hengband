#pragma once

#include "system/item-entity.h"
#include <string>
#include <vector>

struct self_info_type {
    self_info_type() = default;
    TrFlags flags{};
    std::vector<std::string> info_list{};
};
