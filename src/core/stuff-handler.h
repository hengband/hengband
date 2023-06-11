#pragma once

#include "system/angband.h"

enum class MonsterRaceId : int16_t;
class PlayerType;
void handle_stuff(PlayerType *player_ptr);
void monster_race_track(PlayerType *player_ptr, MonsterRaceId r_idx);
void object_kind_track(PlayerType *player_ptr, short bi_id);
void health_track(PlayerType *player_ptr, MONSTER_IDX m_idx);

bool update_player();
bool redraw_player(PlayerType *player_ptr);
