﻿#pragma once

#include "system/angband.h"

enum dap_type : int;
class player_type;
bool alloc_stairs(player_type *player_ptr, FEAT_IDX feat, int num, int walls);
void alloc_object(player_type *player_ptr, dap_type set, EFFECT_ID typ, int num);
