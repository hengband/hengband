#pragma once

/* xtra2.c */
extern void panel_bounds_center(void);
extern void verify_panel(void);
extern bool target_able(MONSTER_IDX m_idx);
extern bool target_okay(void);
extern bool target_set(BIT_FLAGS mode);
extern void target_set_prepare_look(void);
extern bool get_aim_dir(DIRECTION *dp);
extern bool get_hack_dir(DIRECTION *dp);
extern bool get_direction(DIRECTION *dp, bool allow_under, bool with_steed);
extern bool get_rep_dir(DIRECTION *dp, bool under);
extern bool tgt_pt(POSITION *x, POSITION *y);
