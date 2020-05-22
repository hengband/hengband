#pragma once

#include "combat/combat-options-type.h"

void exe_player_attack_to_monster(player_type *attacker_ptr, POSITION y, POSITION x, bool *fear, bool *mdeath, s16b hand, combat_options mode);
