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

    bool is_valid() const;
    bool is_bounty(bool unachieved_only) const;
    int calc_power() const;

private:
    MonsterRaceId r_idx;
};
