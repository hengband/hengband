#pragma once

#include "system/angband.h"

/*
 * Player regeneration constants
 */
#define PY_REGEN_NORMAL 197 /* Regen factor*2^16 when full */
#define PY_REGEN_WEAK 98 /* Regen factor*2^16 when weak */
#define PY_REGEN_FAINT 33 /* Regen factor*2^16 when fainting */
#define PY_REGEN_HPBASE 1442 /* Min amount hp regen*2^16 */
#define PY_REGEN_MNBASE 524 /* Min amount mana regen*2^16 */

extern int wild_regen;

struct player_type;
void regenhp(player_type *creature_ptr, int percent);
void regenmana(player_type *creature_ptr, MANA_POINT upkeep_factor, MANA_POINT regen_amount);
void regenmagic(player_type *creature_ptr, int regen_amount);
void regenerate_monsters(player_type *player_ptr);
void regenerate_captured_monsters(player_type *creature_ptr);
