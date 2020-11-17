#pragma once

#include "system/angband.h"

bool exe_open(player_type *creature_ptr, POSITION y, POSITION x);
bool exe_close(player_type *creature_ptr, POSITION y, POSITION x);
bool easy_open_door(player_type *creature_ptr, POSITION y, POSITION x);
bool exe_disarm(player_type *creature_ptr, POSITION y, POSITION x, DIRECTION dir);
bool exe_disarm_chest(player_type *creature_ptr, POSITION y, POSITION x, OBJECT_IDX o_idx);
bool exe_bash(player_type *creature_ptr, POSITION y, POSITION x, DIRECTION dir);
