#pragma once

#include "system/angband.h"
#include <string>

/*
 * Bit flags for the *_can_enter() and monster_can_cross_terrain()
 */
#define CEM_RIDING 0x0001
#define CEM_P_CAN_ENTER_PATTERN 0x0002
enum class MonsterRaceId : int16_t;
class MonsterRaceInfo;
class MonsterEntity;
class PlayerType;
bool monster_can_cross_terrain(PlayerType *player_ptr, FEAT_IDX feat, const MonsterRaceInfo *r_ptr, BIT_FLAGS16 mode);
bool monster_can_enter(PlayerType *player_ptr, POSITION y, POSITION x, MonsterRaceInfo *r_ptr, BIT_FLAGS16 mode);
bool monster_has_hostile_align(PlayerType *player_ptr, MonsterEntity *m_ptr, int pa_good, int pa_evil, const MonsterRaceInfo *r_ptr);
bool is_original_ap_and_seen(PlayerType *player_ptr, const MonsterEntity *m_ptr);
std::string monster_name(PlayerType *player_ptr, MONSTER_IDX m_idx);
