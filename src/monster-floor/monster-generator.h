#pragma once

#include "system/angband.h"

enum summon_type : int;
struct player_type;
typedef bool (*summon_specific_pf)(player_type *, MONSTER_IDX, POSITION, POSITION, DEPTH, summon_type, BIT_FLAGS);

bool mon_scatter(player_type *player_ptr, MONRACE_IDX r_idx, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION max_dist);
bool multiply_monster(player_type *player_ptr, MONSTER_IDX m_idx, bool clone, BIT_FLAGS mode);
bool place_monster_aux(player_type *player_ptr, MONSTER_IDX who, POSITION y, POSITION x, MONRACE_IDX r_idx, BIT_FLAGS mode);
bool place_monster(player_type *player_ptr, POSITION y, POSITION x, BIT_FLAGS mode);
bool alloc_horde(player_type *player_ptr, POSITION y, POSITION x, summon_specific_pf summon_specific);
bool alloc_guardian(player_type *player_ptr, bool def_val);
bool alloc_monster(player_type *player_ptr, POSITION dis, BIT_FLAGS mode, summon_specific_pf summon_specific);
