#pragma once

#include "system/angband.h"

typedef void (*hook_c_roff_pf)(TERM_COLOR attr, concptr str);
extern hook_c_roff_pf hook_c_roff;

void dice_to_string(int base_damage, int dice_num, int dice_side, int dice_mult, int dice_div, char *msg);
void display_monster_lore(player_type *player_ptr, MONRACE_IDX r_idx, BIT_FLAGS mode);
