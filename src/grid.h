/*
 * File: grid.h
 * Purpose: header file for grid.c, used only in dungeon generation
 * files (generate.c, rooms.c)
 */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */


/* Macros */

#define set_cave_feat(Y,X,F)    (cave[(Y)][(X)].feat = (F))
#define add_cave_info(Y,X,I)    (cave[(Y)][(X)].info |= (I))

/* This should not be used */
/*#define set_cave_info(Y,X,I)    (cave[(Y)][(X)].info = (I)) */

#define place_rubble(Y,X)       set_cave_feat(Y,X,feat_rubble)
#define place_up_stairs(Y,X)    set_cave_feat(Y,X,feat_up_stair)
#define place_down_stairs(Y,X)  set_cave_feat(Y,X,feat_down_stair)

#define is_floor_bold(Y,X) (cave[Y][X].info & CAVE_FLOOR)
#define is_extra_bold(Y,X) (cave[Y][X].info & CAVE_EXTRA)
#define is_inner_bold(Y,X) (cave[Y][X].info & CAVE_INNER)
#define is_outer_bold(Y,X) (cave[Y][X].info & CAVE_OUTER)
#define is_solid_bold(Y,X) (cave[Y][X].info & CAVE_SOLID)

#define is_floor_grid(C) ((C)->info & CAVE_FLOOR)
#define is_extra_grid(C) ((C)->info & CAVE_EXTRA)
#define is_inner_grid(C) ((C)->info & CAVE_INNER)
#define is_outer_grid(C) ((C)->info & CAVE_OUTER)
#define is_solid_grid(C) ((C)->info & CAVE_SOLID)

#define place_floor_bold(Y, X) \
{ \
	set_cave_feat(Y,X,floor_type[randint0(100)]); \
	cave[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(Y,X,CAVE_FLOOR); \
	delete_monster(Y, X); \
}

#define place_floor_grid(C) \
{ \
	(C)->feat = floor_type[randint0(100)]; \
	(C)->info &= ~(CAVE_MASK); \
	(C)->info |= CAVE_FLOOR; \
	if ((C)->m_idx) delete_monster_idx((C)->m_idx); \
}

#define place_extra_bold(Y, X) \
{ \
	set_cave_feat(Y,X,fill_type[randint0(100)]); \
	cave[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(Y,X,CAVE_EXTRA); \
	delete_monster(Y, X); \
}

#define place_extra_grid(C) \
{ \
	(C)->feat = fill_type[randint0(100)]; \
	(C)->info &= ~(CAVE_MASK); \
	(C)->info |= CAVE_EXTRA; \
	if ((C)->m_idx) delete_monster_idx((C)->m_idx); \
}

#define place_extra_perm_bold(Y, X) \
{ \
	set_cave_feat(Y,X,feat_permanent); \
	cave[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(Y,X,CAVE_EXTRA); \
	delete_monster(Y, X); \
}

#define place_extra_perm_grid(C) \
{ \
	(C)->feat = feat_permanent; \
	(C)->info &= ~(CAVE_MASK); \
	(C)->info |= CAVE_EXTRA; \
	if ((C)->m_idx) delete_monster_idx((C)->m_idx); \
}

#define place_extra_noperm_bold(Y, X) \
{ \
	feature_type *_f_ptr; \
	set_cave_feat(Y,X,fill_type[randint0(100)]); \
	_f_ptr = &f_info[cave[Y][X].feat]; \
	if (permanent_wall(_f_ptr)) cave[Y][X].feat = feat_state(cave[Y][X].feat, FF_UNPERM); \
	cave[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(Y,X,CAVE_EXTRA); \
	delete_monster(Y, X); \
}

#define place_inner_bold(Y, X) \
{ \
	set_cave_feat(Y,X,feat_wall_inner); \
	cave[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(Y,X,CAVE_INNER); \
	delete_monster(Y, X); \
}

#define place_inner_grid(C) \
{ \
	(C)->feat = feat_wall_inner; \
	(C)->info &= ~(CAVE_MASK); \
	(C)->info |= CAVE_INNER; \
	if ((C)->m_idx) delete_monster_idx((C)->m_idx); \
}

#define place_inner_perm_bold(Y, X) \
{ \
	set_cave_feat(Y,X,feat_permanent); \
	cave[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(Y,X,CAVE_INNER); \
	delete_monster(Y, X); \
}

#define place_inner_perm_grid(C) \
{ \
	(C)->feat = feat_permanent; \
	(C)->info &= ~(CAVE_MASK); \
	(C)->info |= CAVE_INNER; \
	if ((C)->m_idx) delete_monster_idx((C)->m_idx); \
}

#define place_outer_bold(Y, X) \
{ \
	set_cave_feat(Y,X,feat_wall_outer); \
	cave[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(Y,X,CAVE_OUTER); \
	delete_monster(Y, X); \
}

#define place_outer_grid(C) \
{ \
	(C)->feat = feat_wall_outer; \
	(C)->info &= ~(CAVE_MASK); \
	(C)->info |= CAVE_OUTER; \
	if ((C)->m_idx) delete_monster_idx((C)->m_idx); \
}

#define place_outer_perm_bold(Y, X) \
{ \
	set_cave_feat(Y,X,feat_permanent); \
	cave[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(Y,X,CAVE_OUTER); \
	delete_monster(Y, X); \
}

#define place_outer_perm_grid(C) \
{ \
	(C)->feat = feat_permanent; \
	(C)->info &= ~(CAVE_MASK); \
	(C)->info |= CAVE_OUTER; \
	if ((C)->m_idx) delete_monster_idx((C)->m_idx); \
}

#define place_outer_noperm_bold(Y, X) \
{ \
	feature_type *_f_ptr = &f_info[feat_wall_outer]; \
	if (permanent_wall(_f_ptr)) set_cave_feat(Y, X, feat_state(feat_wall_outer, FF_UNPERM)); \
	else set_cave_feat(Y,X,feat_wall_outer); \
	cave[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(Y,X,(CAVE_OUTER | CAVE_VAULT)); \
	delete_monster(Y, X); \
}

#define place_outer_noperm_grid(C) \
{ \
	feature_type *_f_ptr = &f_info[feat_wall_outer]; \
	if (permanent_wall(_f_ptr)) (C)->feat = feat_state(feat_wall_outer, FF_UNPERM); \
	else (C)->feat = feat_wall_outer; \
	(C)->info &= ~(CAVE_MASK); \
	(C)->info |= (CAVE_OUTER | CAVE_VAULT); \
	if ((C)->m_idx) delete_monster_idx((C)->m_idx); \
}

#define place_solid_bold(Y, X) \
{ \
	set_cave_feat(Y,X,feat_wall_solid); \
	cave[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(Y,X,CAVE_SOLID); \
	delete_monster(Y, X); \
}

#define place_solid_grid(C) \
{ \
	(C)->feat = feat_wall_solid; \
	(C)->info &= ~(CAVE_MASK); \
	(C)->info |= CAVE_SOLID; \
	if ((C)->m_idx) delete_monster_idx((C)->m_idx); \
}

#define place_solid_perm_bold(Y, X) \
{ \
	set_cave_feat(Y,X,feat_permanent); \
	cave[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(Y,X,CAVE_SOLID); \
	delete_monster(Y, X); \
}

#define place_solid_perm_grid(C) \
{ \
	(C)->feat = feat_permanent; \
	(C)->info &= ~(CAVE_MASK); \
	(C)->info |= CAVE_SOLID; \
	if ((C)->m_idx) delete_monster_idx((C)->m_idx); \
}

#define place_solid_noperm_bold(Y, X) \
{ \
	feature_type *_f_ptr = &f_info[feat_wall_solid]; \
	if ((cave[Y][X].info & CAVE_VAULT) && permanent_wall(_f_ptr)) \
		set_cave_feat(Y, X, feat_state(feat_wall_solid, FF_UNPERM)); \
	else set_cave_feat(Y,X,feat_wall_solid); \
	cave[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(Y,X,CAVE_SOLID); \
	delete_monster(Y, X); \
}

#define place_solid_noperm_grid(C) \
{ \
	feature_type *_f_ptr = &f_info[feat_wall_solid]; \
	if (((C)->info & CAVE_VAULT) && permanent_wall(_f_ptr)) \
		(C)->feat = feat_state(feat_wall_solid, FF_UNPERM); \
	else (C)->feat = feat_wall_solid; \
	(C)->info &= ~(CAVE_MASK); \
	(C)->info |= CAVE_SOLID; \
	if ((C)->m_idx) delete_monster_idx((C)->m_idx); \
}


/* Externs */

extern bool new_player_spot(void);

extern void place_random_stairs(int y, int x);
extern void place_random_door(int y, int x, bool room);
extern void place_closed_door(int y, int x, int type);
extern void place_floor(int x1, int x2, int y1, int y2, bool light);
extern void place_room(int x1, int x2, int y1, int y2, bool light);
extern void vault_monsters(int y1, int x1, int num);
extern void vault_objects(int y, int x, int num);
extern void vault_trap_aux(int y, int x, int yd, int xd);
extern void vault_traps(int y, int x, int yd, int xd, int num);

extern void correct_dir(int *rdir, int *cdir, int y1, int x1, int y2, int x2);

extern void rand_dir(int *rdir, int *cdir);

extern bool get_is_floor(int x, int y);
extern void set_floor(int x, int y);

extern bool build_tunnel(int row1, int col1, int row2, int col2);
extern bool build_tunnel2(int x1, int y1, int x2, int y2, int type, int cutoff);
