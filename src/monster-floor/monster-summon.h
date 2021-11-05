#pragma once

#include "system/angband.h"

extern int summon_specific_who;
extern bool summon_unique_okay;

enum summon_type : int;
class PlayerType;
bool summon_specific(PlayerType *player_ptr, MONSTER_IDX who, POSITION y1, POSITION x1, DEPTH lev, summon_type type, BIT_FLAGS mode);
bool summon_named_creature(PlayerType *player_ptr, MONSTER_IDX who, POSITION oy, POSITION ox, MONRACE_IDX r_idx, BIT_FLAGS mode);
