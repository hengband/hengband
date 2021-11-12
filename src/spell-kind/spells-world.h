#pragma once

#include "system/angband.h"

class PlayerType;
void teleport_level(PlayerType *player_ptr, MONSTER_IDX m_idx);
bool teleport_level_other(PlayerType *player_ptr);
bool tele_town(PlayerType *player_ptr);
void reserve_alter_reality(PlayerType *player_ptr, TIME_EFFECT turns);
bool is_teleport_level_ineffective(PlayerType *player_ptr, MONSTER_IDX idx);
bool recall_player(PlayerType *player_ptr, TIME_EFFECT turns);
bool free_level_recall(PlayerType *player_ptr);
bool reset_recall(PlayerType *player_ptr);
