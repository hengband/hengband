#pragma once
#include "realm/realm.h"

bool in_disintegration_range(floor_type *floor_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
void breath_shape(player_type *caster_ptr, u16b *path_g, int dist, int *pgrids, POSITION *gx, POSITION *gy, POSITION *gm, POSITION *pgm_rad, POSITION rad, POSITION y1, POSITION x1, POSITION y2, POSITION x2, EFFECT_ID typ);
POSITION dist_to_line(POSITION y, POSITION x, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
bool binding_field(player_type *caster_ptr, HIT_POINT dam);
void seal_of_mirror(player_type *caster_ptr, HIT_POINT dam);
concptr spell_category_name(OBJECT_TYPE_VALUE tval);
