#pragma once

#include "system/angband.h"

struct player_type;
bool wand_effect(player_type *player_ptr, OBJECT_SUBTYPE_VALUE sval, DIRECTION dir, bool powerful, bool magic);
void exe_aim_wand(player_type *player_ptr, INVENTORY_IDX item);
void do_cmd_aim_wand(player_type *player_ptr);
