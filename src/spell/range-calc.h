#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"

class FloorType;
class PlayerType;
class projection_path;
bool in_disintegration_range(FloorType *floor_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
void breath_shape(PlayerType *player_ptr, const projection_path &path, int dist, int *pgrids, POSITION *gx, POSITION *gy, POSITION *gm, POSITION *pgm_rad, POSITION rad, POSITION y1, POSITION x1, POSITION y2, POSITION x2, AttributeType typ);
POSITION dist_to_line(POSITION y, POSITION x, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
