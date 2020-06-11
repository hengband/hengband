#pragma once

#include "system/angband.h"

void dice_to_string(int base_damage, int dice_num, int dice_side, int dice_mult, int dice_div, char *msg);
void process_monster_lore(player_type *player_ptr, MONRACE_IDX r_idx, BIT_FLAGS mode);
