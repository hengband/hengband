#pragma once

#include "system/angband.h"

extern int wild_regen;

typedef struct player_type player_type;
void regenhp(player_type *creature_ptr, int percent);
void regenmana(player_type* creature_ptr, MANA_POINT upkeep_factor, MANA_POINT regen_amount);
void regenmagic(player_type* creature_ptr, int regen_amount);
void regenerate_monsters(player_type* player_ptr);
void regenerate_captured_monsters(player_type* creature_ptr);
