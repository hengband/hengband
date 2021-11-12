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

class PlayerType;
void regenhp(PlayerType *player_ptr, int percent);
void regenmana(PlayerType *player_ptr, MANA_POINT upkeep_factor, MANA_POINT regen_amount);
void regenmagic(PlayerType *player_ptr, int regen_amount);
void regenerate_monsters(PlayerType *player_ptr);
void regenerate_captured_monsters(PlayerType *player_ptr);
