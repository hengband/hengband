#pragma once

#include "system/angband.h"

struct player_type;
void move_cursor_relative(int row, int col);
void print_path(player_type *player_ptr, POSITION y, POSITION x);
bool change_panel(player_type *player_ptr, POSITION dy, POSITION dx);
void panel_bounds_center(void);
