#pragma once

#include "combat/combat-options-type.h"
#include "system/angband.h"

typedef struct player_type player_type;
bool do_cmd_attack(player_type *attacker_ptr, POSITION y, POSITION x, combat_options mode);
