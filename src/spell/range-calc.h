#pragma once

#include "system/angband.h"

typedef struct floor_type floor_type;
typedef struct player_type player_type;
bool in_disintegration_range(floor_type *floor_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
void breath_shape(player_type *caster_ptr, uint16_t *path_g, int dist, int *pgrids, POSITION *gx, POSITION *gy, POSITION *gm, POSITION *pgm_rad, POSITION rad, POSITION y1, POSITION x1, POSITION y2, POSITION x2, EFFECT_ID typ);
POSITION dist_to_line(POSITION y, POSITION x, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
