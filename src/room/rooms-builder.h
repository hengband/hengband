#pragma once

#include "system/angband.h"
#include "util/point-2d.h"

class PlayerType;
void build_lake(PlayerType *player_ptr, int type);
void build_cavern(PlayerType *player_ptr);
void build_small_room(PlayerType *player_ptr, POSITION x0, POSITION y0);
void add_outer_wall(PlayerType *player_ptr, POSITION x, POSITION y, int light, POSITION x1, POSITION y1, POSITION x2, POSITION y2);
POSITION dist2(POSITION x1, POSITION y1, POSITION x2, POSITION y2, POSITION h1, POSITION h2, POSITION h3, POSITION h4);
void build_recursive_room(PlayerType *player_ptr, POSITION x1, POSITION y1, POSITION x2, POSITION y2, int power);
void build_room(PlayerType *player_ptr, POSITION x1, POSITION x2, POSITION y1, POSITION y2);
void place_room_floor(PlayerType *player_ptr, const Rect2D &rectangle, bool should_brighten = false);
void place_room_outer_walls(PlayerType *player_ptr, const Rect2D &rectangle);
void place_room_with_outer_walls(PlayerType *player_ptr, const Rect2D &rectangle, bool should_brighten = false);
void place_room_inner_walls(PlayerType *player_ptr, const Rect2D &rectangle);
