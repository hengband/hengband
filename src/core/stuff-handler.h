#pragma once

#include "system/angband.h"

class PlayerType;
void handle_stuff(PlayerType *player_ptr);
void monster_race_track(PlayerType *player_ptr, MONRACE_IDX r_idx);
void object_kind_track(PlayerType *player_ptr, KIND_OBJECT_IDX k_idx);
void health_track(PlayerType *player_ptr, MONSTER_IDX m_idx);

bool update_player(PlayerType *player_ptr);
bool redraw_player(PlayerType *player_ptr);
