#pragma once

#include "floor/floor-allocation-types.h"
#include "system/angband.h"

enum dap_type : int;
class PlayerType;
bool alloc_stairs(PlayerType *player_ptr, FEAT_IDX feat, int num, int walls);
void alloc_object(PlayerType *player_ptr, dap_type set, dungeon_allocation_type typ, int num);
