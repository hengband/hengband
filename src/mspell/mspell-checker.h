#pragma once

#include "system/angband.h"
#include "system/monster-type-definition.h"

bool clean_shot(player_type *target_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, bool is_friend);
bool summon_possible(player_type *target_ptr, POSITION y1, POSITION x1);
bool raise_possible(player_type *target_ptr, monster_type *m_ptr);
bool spell_is_inate(SPELL_IDX spell);
void beam(player_type *target_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, spell_type typ, int dam_hp, int monspell, int target_type);
void bolt(player_type *target_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, spell_type typ, int dam_hp, int monspell, int target_type);
void breath(
    player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, spell_type typ, int dam_hp, POSITION rad, bool breath, int monspell, int target_type);
