#pragma once

#include "monster-race/race-indice-types.h"
#include "system/angband.h"
#include <tuple>
#include <vector>

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
    bool genocide_chaos_patron(monster_type *m_ptr);
    void death_special_flag_monster(monster_type *m_ptr);
    void split_unite_uniques(monster_type *m_ptr, std::vector<std::tuple<monster_race_type, monster_race_type, monster_race_type>> uniques);
    void set_redraw();
    void summon_special_unique(monster_type *m_ptr);
};
