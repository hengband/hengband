#pragma once

#include "system/angband.h"
#include <vector>

struct mind_type {
    PLAYER_LEVEL min_lev;
    MANA_POINT mana_cost;
    PERCENTAGE fail;
    concptr name;
};

struct mind_power {
    std::vector<mind_type> info;
};

extern const std::vector<mind_power> mind_powers;
extern const std::vector<std::vector<std::string>> mind_tips;
