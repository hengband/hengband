#pragma once

#include "system/angband.h"

#include <map>

enum class SmithEssence : int16_t;

struct smith_data_type {
    std::map<SmithEssence, int16_t> essences;
};
