#pragma once

#include "system/angband.h"
#include "grid/feature.h"

extern void get_screen_size(TERM_LEN *wid_p, TERM_LEN *hgt_p);
extern int panel_col_of(int col);
extern void apply_default_feat_lighting(TERM_COLOR f_attr[F_LIT_MAX], SYMBOL_CODE f_char[F_LIT_MAX]);
extern void map_info(player_type *player_ptr, POSITION y, POSITION x, TERM_COLOR *ap, SYMBOL_CODE *cp, TERM_COLOR *tap, SYMBOL_CODE *tcp);
extern void do_cmd_view_map(player_type *player_ptr);

extern void health_track(player_type *player_ptr, MONSTER_IDX m_idx);
extern void print_time(player_type *player_ptr);
extern concptr map_name(player_type *creature_ptr);
extern void move_cursor_relative(int row, int col);
extern void print_path(player_type *player_ptr, POSITION y, POSITION x);
extern void monster_race_track(player_type *player_ptr, MONRACE_IDX r_idx);
extern void object_kind_track(player_type *player_ptr, KIND_OBJECT_IDX k_idx);
extern void resize_map(void);
extern bool change_panel(player_type *player_ptr, POSITION dy, POSITION dx);

extern void update_playtime(void);

/*
 * Determines if a map location is currently "on screen" -RAK-
 * Note that "panel_contains(Y,X)" always implies "in_bounds2(Y,X)".
 */
bool panel_contains(POSITION y, POSITION x);
void delayed_visual_update(player_type *player_ptr);
