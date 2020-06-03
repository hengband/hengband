#pragma once

#include "system/angband.h"

bool stop_hex_spell_all(player_type *caster_ptr);
bool stop_hex_spell(player_type *caster_ptr);
void check_hex(player_type *caster_ptr);
bool hex_spell_fully(player_type *caster_ptr);
void revenge_spell(player_type *caster_ptr);
void revenge_store(player_type *caster_ptr, HIT_POINT dam);
bool teleport_barrier(player_type *caster_ptr, MONSTER_IDX m_idx);
bool magic_barrier(player_type *target_ptr, MONSTER_IDX m_idx);
bool multiply_barrier(player_type *caster_ptr, MONSTER_IDX m_idx);
bool hex_spelling(player_type *caster_type, int hex);
