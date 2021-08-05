#pragma once

#include "monster-race/race-indice-types.h"
#include "system/angband.h"
#include <tuple>
#include <vector>

struct monster_race;
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
    bool genocide_chaos_patron();
    void death_special_flag_monster();
    void death_unique_monster(monster_race_type r_idx);
    bool check_combined_unique(const monster_race_type r_idx, std::vector<monster_race_type> *combined_uniques);
    void death_combined_uniques(const monster_race_type r_idx, std::vector<std::tuple<monster_race_type, monster_race_type, monster_race_type>> *combined_uniques);
    void increase_kill_numbers();
    void death_amberites(GAME_TEXT *m_name);
    void dying_scream(GAME_TEXT *m_name);
    void change_virtue_non_beginner();
    void change_virtue_unique();
    void change_virtue_good_evil();
    void change_virtue_revenge();
    void change_virtue_wild_thief();
    void set_redraw();
    void summon_special_unique();
};
