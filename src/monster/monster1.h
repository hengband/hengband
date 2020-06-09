#pragma once

#include "system/angband.h"
#include "system/monster-type-definition.h"

void set_friendly(monster_type *m_ptr);
void set_pet(player_type *player_ptr, monster_type *m_ptr);
void set_hostile(player_type *player_ptr, monster_type *m_ptr);
void anger_monster(player_type *player_ptr, monster_type *m_ptr);

/*
 * Bit flags for the *_can_enter() and monster_can_cross_terrain()
 */
#define CEM_RIDING 0x0001
#define CEM_P_CAN_ENTER_PATTERN 0x0002
bool monster_can_cross_terrain(player_type *player_ptr, FEAT_IDX feat, monster_race *r_ptr, BIT_FLAGS16 mode);
bool monster_can_enter(player_type *player_ptr, POSITION y, POSITION x, monster_race *r_ptr, BIT_FLAGS16 mode);
bool are_enemies(player_type *player_ptr, monster_type *m_ptr1, monster_type *m_ptr2);
bool monster_has_hostile_align(player_type *player_ptr, monster_type *m_ptr, int pa_good, int pa_evil, monster_race *r_ptr);
bool is_original_ap_and_seen(player_type *player_ptr, monster_type *m_ptr);
