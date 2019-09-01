#pragma once

extern void wiz_lite(player_type *caster_ptr, bool ninja);
extern void wiz_dark(void);
extern bool warding_glyph(void);
extern bool explosive_rune(void);
extern bool place_mirror(void);
extern void stair_creation(void);
extern void map_area(POSITION range);
extern bool destroy_area(POSITION y1, POSITION x1, POSITION r, bool in_generate);
extern bool earthquake(POSITION cy, POSITION cx, POSITION r, MONSTER_IDX m_idx);
