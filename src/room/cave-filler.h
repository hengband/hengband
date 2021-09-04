#pragma once

#include "system/angband.h"

struct floor_type;
struct player_type;
void generate_hmap(floor_type *floor_ptr, POSITION y0, POSITION x0, POSITION xsiz, POSITION ysiz, int grd, int roug, int cutoff);
bool generate_fracave(player_type *player_ptr, POSITION y0, POSITION x0, POSITION xsize, POSITION ysize, int cutoff, bool light, bool room);
bool generate_lake(player_type *player_ptr, POSITION y0, POSITION x0, POSITION xsize, POSITION ysize, int c1, int c2, int c3, int type);
