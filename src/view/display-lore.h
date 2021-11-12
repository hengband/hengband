#pragma once

#include "lore/lore-util.h"
#include "system/angband.h"

struct lore_type;
class PlayerType;
void roff_top(MONRACE_IDX r_idx);
void screen_roff(PlayerType *player_ptr, MONRACE_IDX r_idx, monster_lore_mode mode);
void display_roff(PlayerType *player_ptr);
void output_monster_spoiler(MONRACE_IDX r_idx, void (*roff_func)(TERM_COLOR attr, concptr str));
void display_kill_numbers(lore_type *lore_ptr);
bool display_where_to_appear(lore_type *lore_ptr);
void display_random_move(lore_type *lore_ptr);
void display_monster_move(lore_type *lore_ptr);
void display_monster_never_move(lore_type *lore_ptr);
void display_monster_kind(lore_type *lore_ptr);
void display_monster_alignment(lore_type *lore_ptr);
void display_monster_exp(PlayerType *player_ptr, lore_type *lore_ptr);
void display_monster_aura(lore_type *lore_ptr);
void display_lore_this(PlayerType *player_ptr, lore_type *lore_ptr);
void display_monster_collective(lore_type *lore_ptr);
void display_monster_launching(PlayerType *player_ptr, lore_type *lore_ptr);
void display_monster_sometimes(lore_type *lore_ptr);
void display_monster_guardian(lore_type *lore_ptr);
