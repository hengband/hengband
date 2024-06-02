#pragma once

#include "system/angband.h"

#include <map>

enum class MonsterRaceId : int16_t;
class MonsterRaceInfo;
extern std::map<MonsterRaceId, MonsterRaceInfo> monraces_info;

class MonsterRace {
public:
    MonsterRace(MonsterRaceId r_idx);

    static MonsterRaceId empty_id();
    static MonsterRaceId pick_one_at_random();

    int calc_power() const;

private:
    MonsterRaceId r_idx;
};
