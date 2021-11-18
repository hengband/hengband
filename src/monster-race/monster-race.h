#pragma once

#include "system/angband.h"

#include <vector>

struct monster_race;
extern std::vector<monster_race> r_info;
int calc_monrace_power(monster_race *r_ptr);
