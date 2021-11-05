#pragma once

#include "system/angband.h"

struct ProjectResult;
enum class RF_ABILITY;

struct monster_type;
class PlayerType;
bool clean_shot(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, bool is_friend);
bool summon_possible(PlayerType *player_ptr, POSITION y1, POSITION x1);
bool raise_possible(PlayerType *player_ptr, monster_type *m_ptr);
bool spell_is_inate(RF_ABILITY spell);
ProjectResult beam(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, EFFECT_ID typ, int dam_hp, int target_type);
ProjectResult bolt(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, EFFECT_ID typ, int dam_hp, int target_type);
ProjectResult breath(
    PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, EFFECT_ID typ, int dam_hp, POSITION rad, bool breath, int target_type);
