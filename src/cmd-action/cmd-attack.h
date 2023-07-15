#pragma once

#include "combat/combat-options-type.h"
#include "system/angband.h"

class PlayerType;
bool do_cmd_attack(PlayerType *player_ptr, POSITION y, POSITION x, combat_options mode);
