#pragma once

#include "system/angband.h"
#include "effect/attribute-types.h"

enum dap_type : int;
class PlayerType;
bool alloc_stairs(PlayerType *player_ptr, FEAT_IDX feat, int num, int walls);
void alloc_object(PlayerType *player_ptr, dap_type set, EFFECT_ID typ, int num);
