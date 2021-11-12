#pragma once

#include "system/angband.h"

enum summon_type : int;
class PlayerType;
typedef bool (*summon_specific_pf)(PlayerType *, MONSTER_IDX, POSITION, POSITION, DEPTH, summon_type, BIT_FLAGS);

bool mon_scatter(PlayerType *player_ptr, MONRACE_IDX r_idx, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION max_dist);
bool multiply_monster(PlayerType *player_ptr, MONSTER_IDX m_idx, bool clone, BIT_FLAGS mode);
bool place_monster_aux(PlayerType *player_ptr, MONSTER_IDX who, POSITION y, POSITION x, MONRACE_IDX r_idx, BIT_FLAGS mode);
bool place_monster(PlayerType *player_ptr, POSITION y, POSITION x, BIT_FLAGS mode);
bool alloc_horde(PlayerType *player_ptr, POSITION y, POSITION x, summon_specific_pf summon_specific);
bool alloc_guardian(PlayerType *player_ptr, bool def_val);
bool alloc_monster(PlayerType *player_ptr, POSITION dis, BIT_FLAGS mode, summon_specific_pf summon_specific);
