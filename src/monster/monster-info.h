#pragma once

#include "system/angband.h"
#include <string>

/*
 * Bit flags for the *_can_enter() and monster_can_cross_terrain()
 */
#define CEM_RIDING 0x0001
#define CEM_P_CAN_ENTER_PATTERN 0x0002
enum class MonraceId : short;
class MonraceDefinition;
class MonsterEntity;
class PlayerType;
bool monster_can_cross_terrain(PlayerType *player_ptr, FEAT_IDX feat, const MonraceDefinition &monrace, BIT_FLAGS16 mode);
bool monster_can_enter(PlayerType *player_ptr, POSITION y, POSITION x, const MonraceDefinition &monrace, BIT_FLAGS16 mode);
bool monster_has_hostile_to_player(PlayerType *player_ptr, int pa_good, int pa_evil, const MonraceDefinition &monrace);
bool monster_has_hostile_to_other_monster(const MonsterEntity &monster_other, const MonraceDefinition &monrace);
bool is_original_ap_and_seen(PlayerType *player_ptr, const MonsterEntity *m_ptr);
std::string monster_name(PlayerType *player_ptr, MONSTER_IDX m_idx);
