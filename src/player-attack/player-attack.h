﻿#pragma once

#include "system/angband.h"
#include "combat/combat-options-type.h"

typedef struct player_type player_type;
void exe_player_attack_to_monster(player_type *attacker_ptr, POSITION y, POSITION x, bool *fear, bool *mdeath, short hand, combat_options mode);
void massacre(player_type *caster_ptr);
