#pragma once

#include "system/angband.h"

class FloorType;
class PlayerType;
void generate_hmap(FloorType &floor, POSITION y0, POSITION x0, POSITION xsiz, POSITION ysiz, int grd, int roug, int cutoff);
bool generate_fracave(PlayerType *player_ptr, POSITION y0, POSITION x0, POSITION xsize, POSITION ysize, int cutoff, bool light, bool room);
bool generate_lake(PlayerType *player_ptr, POSITION y0, POSITION x0, POSITION xsize, POSITION ysize, int c1, int c2, int c3, int type);
