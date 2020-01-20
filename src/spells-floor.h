#pragma once

extern void wiz_lite(player_type *caster_ptr, bool ninja);
extern void wiz_dark(player_type *caster_ptr);
extern bool warding_glyph(player_type *caster_ptr);
extern bool explosive_rune(player_type *caster_ptr, POSITION y, POSITION x);
extern bool place_mirror(player_type *caster_ptr);
extern void stair_creation(player_type *caster_ptr);
extern void map_area(player_type *caster_ptr, POSITION range);
extern bool destroy_area(player_type *caster_ptr, POSITION y1, POSITION x1, POSITION r, bool in_generate);
extern bool earthquake(player_type *caster_ptr, POSITION cy, POSITION cx, POSITION r, MONSTER_IDX m_idx);
