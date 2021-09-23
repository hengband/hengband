#pragma once

#include "system/angband.h"

#include <map>

enum class SmithEssence;

struct smith_data_type {
    std::map<SmithEssence, int> essences;
};
