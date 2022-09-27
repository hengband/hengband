#pragma once

#include "system/angband.h"

/*
 * Bit flags for the *_can_enter() and monster_can_cross_terrain()
 */
#define CEM_RIDING 0x0001
#define CEM_P_CAN_ENTER_PATTERN 0x0002
enum class MonsterRaceId : int16_t;
struct monster_race;
struct monster_type;
class PlayerType;
bool monster_can_cross_terrain(PlayerType *player_ptr, FEAT_IDX feat, monster_race *r_ptr, BIT_FLAGS16 mode);
bool monster_can_enter(PlayerType *player_ptr, POSITION y, POSITION x, monster_race *r_ptr, BIT_FLAGS16 mode);
bool are_enemies(PlayerType *player_ptr, const monster_type &m1_ref, const monster_type &m2_ref);
bool monster_has_hostile_align(PlayerType *player_ptr, monster_type *m_ptr, int pa_good, int pa_evil, monster_race *r_ptr);
bool is_original_ap_and_seen(PlayerType *player_ptr, const monster_type *m_ptr);
void monster_name(PlayerType *player_ptr, MONSTER_IDX m_idx, char *m_name);
