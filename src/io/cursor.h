#pragma once

#include "system/angband.h"

class PlayerType;
void move_cursor_relative(int row, int col);
void print_path(PlayerType *player_ptr, POSITION y, POSITION x);
bool change_panel(PlayerType *player_ptr, POSITION dy, POSITION dx);
void panel_bounds_center(void);
