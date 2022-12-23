#pragma once

#include "effect/attribute-types.h"
#include "monster-race/race-indice-types.h"
#include "system/angband.h"
#include "util/flag-group.h"
#include <tuple>
#include <vector>

enum class MonsterRaceId : int16_t;
class MonsterRaceInfo;
class MonsterEntity;
class PlayerType;
typedef std::vector<std::tuple<MonsterRaceId, MonsterRaceId, MonsterRaceId>> combined_uniques;
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
    void get_exp_from_mon(MonsterEntity *m_ptr, int exp_dam);
    bool genocide_chaos_patron();
    bool process_dead_exp_virtue(concptr note, MonsterEntity *exp_mon);
    void death_special_flag_monster();
    void death_unique_monster(MonsterRaceId r_idx);
    bool check_combined_unique(const MonsterRaceId r_idx, std::vector<MonsterRaceId> *combined_uniques);
    void death_combined_uniques(const MonsterRaceId r_idx, const combined_uniques &combined_uniques);
    void increase_kill_numbers();
    void death_amberites(concptr m_name);
    void dying_scream(concptr m_name);
    void show_kill_message(concptr note, concptr m_name);
    void show_bounty_message(concptr m_name);
    void set_redraw();
    void summon_special_unique();
    void add_monster_fear();
};
