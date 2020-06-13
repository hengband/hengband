#pragma once

#include "system/angband.h"

bool mon_scatter(player_type *player_ptr, MONRACE_IDX r_idx, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION max_dist);
bool multiply_monster(player_type *player_ptr, MONSTER_IDX m_idx, bool clone, BIT_FLAGS mode);
bool place_monster_aux(player_type *player_ptr, MONSTER_IDX who, POSITION y, POSITION x, MONRACE_IDX r_idx, BIT_FLAGS mode);
bool place_monster(player_type *player_ptr, POSITION y, POSITION x, BIT_FLAGS mode);
bool alloc_horde(player_type *player_ptr, POSITION y, POSITION x);
bool alloc_guardian(player_type *player_ptr, bool def_val);
bool alloc_monster(player_type *player_ptr, POSITION dis, BIT_FLAGS mode);
