#pragma once

#include "system/angband.h"
#include "lore/lore-util.h"

void roff_top(MONRACE_IDX r_idx);
void screen_roff(player_type *player_ptr, MONRACE_IDX r_idx, BIT_FLAGS mode);
void display_roff(player_type *player_ptr);
void output_monster_spoiler(player_type *player_ptr, MONRACE_IDX r_idx, void (*roff_func)(TERM_COLOR attr, concptr str));
void display_kill_numbers(lore_type *lore_ptr);
bool display_where_to_appear(lore_type *lore_ptr);
