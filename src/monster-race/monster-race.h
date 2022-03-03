#pragma once

#include "system/angband.h"

#include <map>

enum class MonsterRaceId;
struct monster_race;
extern std::map<MonsterRaceId, monster_race> r_info;
int calc_monrace_power(monster_race *r_ptr);
bool is_valid_monster_race(MonsterRaceId r_idx);
