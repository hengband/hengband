﻿#pragma once

#include "system/angband.h"
#include "combat/combat-options-type.h"

class player_type;
void exe_player_attack_to_monster(player_type *player_ptr, POSITION y, POSITION x, bool *fear, bool *mdeath, int16_t hand, combat_options mode);
void massacre(player_type *player_ptr);
