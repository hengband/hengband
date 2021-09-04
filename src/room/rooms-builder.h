#pragma once

#include "system/angband.h"

struct player_type;
void build_lake(player_type *player_ptr, int type);
void build_cavern(player_type *player_ptr);
void build_small_room(player_type *player_ptr, POSITION x0, POSITION y0);
void add_outer_wall(player_type *player_ptr, POSITION x, POSITION y, int light, POSITION x1, POSITION y1, POSITION x2, POSITION y2);
POSITION dist2(POSITION x1, POSITION y1, POSITION x2, POSITION y2, POSITION h1, POSITION h2, POSITION h3, POSITION h4);
void build_recursive_room(player_type *player_ptr, POSITION x1, POSITION y1, POSITION x2, POSITION y2, int power);
void build_room(player_type *player_ptr, POSITION x1, POSITION x2, POSITION y1, POSITION y2);
