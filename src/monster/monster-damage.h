#pragma once

#include "effect/attribute-types.h"
#include "monster-race/race-indice-types.h"
#include "system/angband.h"
#include "util/flag-group.h"
#include <string_view>
#include <tuple>
#include <vector>

enum class MonsterRaceId : int16_t;
class MonsterRaceInfo;
class MonsterEntity;
class PlayerType;
class MonsterDamageProcessor {
public:
    MonsterDamageProcessor(PlayerType *player_ptr, MONSTER_IDX m_idx, int dam, bool *fear, AttributeType type);
    MonsterDamageProcessor(PlayerType *player_ptr, MONSTER_IDX m_idx, int dam, bool *fear, AttributeFlags attribute_flags);
    virtual ~MonsterDamageProcessor() = default;
    bool mon_take_hit(std::string_view note);

private:
    PlayerType *player_ptr;
    MONSTER_IDX m_idx;
    int dam;
    bool *fear;
    AttributeFlags attribute_flags{};
    void get_exp_from_mon(MonsterEntity *m_ptr, int exp_dam);
    bool genocide_chaos_patron();
    bool process_dead_exp_virtue(std::string_view note, MonsterEntity *exp_mon);
    void death_special_flag_monster();
    void death_unique_monster(MonsterRaceId r_idx);
    void increase_kill_numbers();
    void death_amberites(std::string_view m_name);
    void dying_scream(std::string_view m_name);
    void show_kill_message(std::string_view note, std::string_view m_name);
    void show_explosion_message(std::string_view died_mes, std::string_view m_name);
    void show_bounty_message(std::string_view m_name);
    void set_redraw();
    void summon_special_unique();
    void add_monster_fear();
};
