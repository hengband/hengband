#pragma once

#include "monster-race/race-indice-types.h"
#include "system/angband.h"
#include "effect/attribute-types.h"
#include "util/flag-group.h"
#include <tuple>
#include <vector>

struct monster_race;
struct monster_type;
class PlayerType;
typedef std::vector<std::tuple<monster_race_type, monster_race_type, monster_race_type>> combined_uniques;
class MonsterDamageProcessor {
public:
    MonsterDamageProcessor(PlayerType *player_ptr, MONSTER_IDX m_idx, int dam, bool *fear, AttributeType type);
    MonsterDamageProcessor(PlayerType *player_ptr, MONSTER_IDX m_idx, int dam, bool *fear, AttributeFlags attribute_flags);
    virtual ~MonsterDamageProcessor() = default;
    bool mon_take_hit(concptr note);

private:
    PlayerType *player_ptr;
    MONSTER_IDX m_idx;
    int dam;
    bool *fear;
    AttributeFlags attribute_flags{};
    void get_exp_from_mon(monster_type *m_ptr, int exp_dam);
    bool genocide_chaos_patron();
    bool process_dead_exp_virtue(concptr note, monster_type *exp_mon);
    void death_special_flag_monster();
    void death_unique_monster(monster_race_type r_idx);
    bool check_combined_unique(const monster_race_type r_idx, std::vector<monster_race_type> *combined_uniques);
    void death_combined_uniques(const monster_race_type r_idx, combined_uniques *combined_uniques);
    void increase_kill_numbers();
    void death_amberites(GAME_TEXT *m_name);
    void dying_scream(GAME_TEXT *m_name);
    void show_kill_message(concptr note, GAME_TEXT *m_name);
    void show_bounty_message(GAME_TEXT *m_name);
    void set_redraw();
    void summon_special_unique();
    void add_monster_fear();
};
