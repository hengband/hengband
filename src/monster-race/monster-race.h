#pragma once

#include "system/angband.h"

#include <map>

enum class MonsterRaceId : int16_t;
class MonsterRaceInfo;
extern std::map<MonsterRaceId, MonsterRaceInfo> monraces_info;

class MonsterRace {
public:
    MonsterRace(MonsterRaceId r_idx);

    int calc_power() const;

private:
    MonsterRaceId r_idx;
};
