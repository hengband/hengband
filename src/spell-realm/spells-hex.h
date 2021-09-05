#pragma once

#include "system/angband.h"

struct player_type;
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
bool hex_spelling_any(player_type *caster_type);
#define casting_hex_flags(P_PTR) ((P_PTR)->magic_num1[0])
#define casting_hex_num(P_PTR) ((P_PTR)->magic_num2[0])
#define hex_revenge_power(P_PTR) ((P_PTR)->magic_num1[2])
#define hex_revenge_turn(P_PTR) ((P_PTR)->magic_num2[2])
#define hex_revenge_type(P_PTR) ((P_PTR)->magic_num2[1])
