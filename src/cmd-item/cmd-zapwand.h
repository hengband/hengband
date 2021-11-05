#pragma once

#include "system/angband.h"

class PlayerType;
bool wand_effect(PlayerType *player_ptr, OBJECT_SUBTYPE_VALUE sval, DIRECTION dir, bool powerful, bool magic);
void do_cmd_aim_wand(PlayerType *player_ptr);
