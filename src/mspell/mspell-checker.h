#pragma once

#include "system/angband.h"

struct ProjectResult;
enum class RF_ABILITY;

struct monster_type;
struct player_type;
bool clean_shot(player_type *target_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, bool is_friend);
bool summon_possible(player_type *target_ptr, POSITION y1, POSITION x1);
bool raise_possible(player_type *target_ptr, monster_type *m_ptr);
bool spell_is_inate(RF_ABILITY spell);
ProjectResult beam(player_type *target_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, EFFECT_ID typ, int dam_hp, int target_type);
ProjectResult bolt(player_type *target_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, EFFECT_ID typ, int dam_hp, int target_type);
ProjectResult breath(
    player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, EFFECT_ID typ, int dam_hp, POSITION rad, bool breath, int target_type);
