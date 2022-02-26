#pragma once

#include "spell/spells-util.h"
#include "system/angband.h"

class PlayerType;
bool teleport_monster(PlayerType *player_ptr, DIRECTION dir, int distance);
bool teleport_swap(PlayerType *player_ptr, DIRECTION dir);
bool teleport_away(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION dis, teleport_flags mode);
void teleport_monster_to(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION ty, POSITION tx, int power, teleport_flags mode);
bool teleport_player_aux(PlayerType *player_ptr, POSITION dis, bool is_quantum_effect, teleport_flags mode);
void teleport_player(PlayerType *player_ptr, POSITION dis, BIT_FLAGS mode);
void teleport_player_away(MONSTER_IDX m_idx, PlayerType *player_ptr, POSITION dis, bool is_quantum_effect);
void teleport_player_to(PlayerType *player_ptr, POSITION ny, POSITION nx, teleport_flags mode);
void teleport_away_followable(PlayerType *player_ptr, MONSTER_IDX m_idx);
bool dimension_door(PlayerType *player_ptr);
bool exe_dimension_door(PlayerType *player_ptr, POSITION x, POSITION y);
