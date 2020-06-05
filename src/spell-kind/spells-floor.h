#pragma once

#include "system/angband.h"

void wiz_lite(player_type *caster_ptr, bool ninja);
void wiz_dark(player_type *caster_ptr);
bool warding_glyph(player_type *caster_ptr);
bool explosive_rune(player_type *caster_ptr, POSITION y, POSITION x);
void stair_creation(player_type *caster_ptr);
void map_area(player_type *caster_ptr, POSITION range);
bool destroy_area(player_type *caster_ptr, POSITION y1, POSITION x1, POSITION r, bool in_generate);
bool earthquake(player_type *caster_ptr, POSITION cy, POSITION cx, POSITION r, MONSTER_IDX m_idx);
void cast_meteor(player_type *caster_ptr, HIT_POINT dam, POSITION rad);
