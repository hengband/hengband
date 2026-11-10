#pragma once

#include "system/angband.h"
#include "util/point-2d.h"
#include <vector>

class PlayerType;
void move_cursor_relative(int row, int col);
void erase_path(PlayerType *player_ptr, std::vector<Pos2D> &path_grids);
void print_path(PlayerType *player_ptr, const Pos2D &pos, std::vector<Pos2D> &path_grids);
bool change_panel(PlayerType *player_ptr, POSITION dy, POSITION dx);
void panel_bounds_center();
