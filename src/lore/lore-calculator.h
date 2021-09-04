#pragma once

#include "system/angband.h"

enum class RF_ABILITY;

struct lore_type;
struct player_type;
void dice_to_string(int base_damage, int dice_num, int dice_side, int dice_mult, int dice_div, char *msg);
bool know_armour(MONRACE_IDX r_idx, const bool know_everything);
bool know_damage(MONRACE_IDX r_idx, int i);
void set_damage(player_type *player_ptr, lore_type *lore_ptr, RF_ABILITY ms_type, concptr msg);
void set_drop_flags(lore_type *lore_ptr);
