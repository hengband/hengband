#pragma once

extern void wiz_lite(player_type *caster_ptr, bool ninja);
extern void wiz_dark(void);
extern bool warding_glyph(player_type *caster_ptr);
extern bool explosive_rune(void);
extern bool place_mirror(player_type *caster_ptr);
extern void stair_creation(player_type *caster_ptr);
extern void map_area(POSITION range);
extern bool destroy_area(POSITION y1, POSITION x1, POSITION r, bool in_generate);
extern bool earthquake(POSITION cy, POSITION cx, POSITION r, MONSTER_IDX m_idx);
