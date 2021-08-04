#pragma once

#include "system/angband.h"

struct monster_type;
struct player_type;
class MonsterDamageProcessor {
public:
    MonsterDamageProcessor(player_type *target_ptr, MONSTER_IDX m_idx, HIT_POINT dam, bool *fear);
    MonsterDamageProcessor() = delete;
    virtual ~MonsterDamageProcessor() = default;
    bool mon_take_hit(concptr note);

private:
    player_type *target_ptr;
    MONSTER_IDX m_idx;
    HIT_POINT dam;
    bool *fear;
    void get_exp_from_mon(monster_type *m_ptr, HIT_POINT exp_dam);
};
