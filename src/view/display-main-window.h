#pragma once

#include "system/angband.h"

void get_screen_size(TERM_LEN *wid_p, TERM_LEN *hgt_p);
void move_cursor_relative(int row, int col);
void print_path(player_type *player_ptr, POSITION y, POSITION x);
void resize_map(void);
bool change_panel(player_type *player_ptr, POSITION dy, POSITION dx);
bool panel_contains(POSITION y, POSITION x);
