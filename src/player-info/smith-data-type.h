#pragma once

#include "system/angband.h"

#include <map>

enum class SmithEssenceType : int16_t;

struct smith_data_type {
    std::map<SmithEssenceType, int16_t> essences{};
};
