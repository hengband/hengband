#pragma once

#include "system/angband.h"

struct player_type;
void handle_stuff(player_type *player_ptr);
void monster_race_track(player_type *player_ptr, MONRACE_IDX r_idx);
void object_kind_track(player_type *player_ptr, KIND_OBJECT_IDX k_idx);
void health_track(player_type *player_ptr, MONSTER_IDX m_idx);

bool update_player(player_type *player_ptr);
bool redraw_player(player_type *player_ptr);
