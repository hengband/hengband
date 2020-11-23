#pragma once

#include "lore/lore-util.h"
#include "mspell/mspell-type.h"
#include "system/angband.h"

void dice_to_string(int base_damage, int dice_num, int dice_side, int dice_mult, int dice_div, char *msg);
bool know_armour(MONRACE_IDX r_idx);
bool know_damage(MONRACE_IDX r_idx, int i);
void set_damage(player_type *player_ptr, MONRACE_IDX r_idx, monster_spell_type ms_type, char *msg, char *tmp);
void set_drop_flags(lore_type *lore_ptr);
