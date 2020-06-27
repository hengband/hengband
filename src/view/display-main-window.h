#pragma once

#include "system/angband.h"

void get_screen_size(TERM_LEN *wid_p, TERM_LEN *hgt_p);
void apply_default_feat_lighting(TERM_COLOR *f_attr, SYMBOL_CODE *f_char);
void health_track(player_type *player_ptr, MONSTER_IDX m_idx);
void print_time(player_type *player_ptr);
concptr map_name(player_type *creature_ptr);
void move_cursor_relative(int row, int col);
void print_path(player_type *player_ptr, POSITION y, POSITION x);
void monster_race_track(player_type *player_ptr, MONRACE_IDX r_idx);
void object_kind_track(player_type *player_ptr, KIND_OBJECT_IDX k_idx);
void resize_map(void);
bool change_panel(player_type *player_ptr, POSITION dy, POSITION dx);
bool panel_contains(POSITION y, POSITION x);
void delayed_visual_update(player_type *player_ptr);
