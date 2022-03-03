#pragma once

#include "system/angband.h"

#include <map>

enum class MonsterRaceId;
struct monster_race;
extern std::map<MonsterRaceId, monster_race> r_info;
int calc_monrace_power(monster_race *r_ptr);

class MonsterRace {
public:
    MonsterRace(MonsterRaceId r_idx);
    bool is_valid() const;

private:
    const MonsterRaceId r_idx;
};
