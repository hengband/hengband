#pragma once

#include "combat/combat-options-type.h"
#include "system/angband.h"

struct player_type;
bool do_cmd_attack(player_type *player_ptr, POSITION y, POSITION x, combat_options mode);
