#pragma once

#include "system/angband.h"

/*
 * Bit flags for the *_can_enter() and monster_can_cross_terrain()
 */
#define CEM_RIDING 0x0001
#define CEM_P_CAN_ENTER_PATTERN 0x0002
struct monster_race;
struct monster_type;
class PlayerType;
bool monster_can_cross_terrain(PlayerType *player_ptr, FEAT_IDX feat, monster_race *r_ptr, BIT_FLAGS16 mode);
bool monster_can_enter(PlayerType *player_ptr, POSITION y, POSITION x, monster_race *r_ptr, BIT_FLAGS16 mode);
bool are_enemies(PlayerType *player_ptr, monster_type *m_ptr1, monster_type *m_ptr2);
bool monster_has_hostile_align(PlayerType *player_ptr, monster_type *m_ptr, int pa_good, int pa_evil, monster_race *r_ptr);
bool is_original_ap_and_seen(PlayerType *player_ptr, monster_type *m_ptr);

bool is_friendly(monster_type *m_ptr);
bool is_pet(monster_type *m_ptr);
bool is_hostile(monster_type *m_ptr);
bool is_original_ap(monster_type *m_ptr);
bool is_mimicry(monster_type *m_ptr);

monster_race *real_r_ptr(monster_type *m_ptr);
MONRACE_IDX real_r_idx(monster_type *m_ptr);
void monster_name(PlayerType *player_ptr, MONSTER_IDX m_idx, char *m_name);
