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

#define place_rubble(Y,X)       set_cave_feat(Y,X,FEAT_RUBBLE)
#define place_up_stairs(Y,X)    set_cave_feat(Y,X,FEAT_LESS)
#define place_down_stairs(Y,X)  set_cave_feat(Y,X,FEAT_MORE)
#define place_invis_wall(Y,X)   set_cave_feat(Y,X,FEAT_WALL_INVIS)

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
}

#define place_floor_grid(C) \
{ \
  (C)->feat = floor_type[randint0(100)]; \
  (C)->info &= ~(CAVE_MASK); \
  (C)->info |= CAVE_FLOOR; \
}

#define place_extra_bold(Y, X) \
{ \
  set_cave_feat(Y,X,fill_type[randint0(100)]); \
  cave[Y][X].info &= ~(CAVE_MASK); \
  add_cave_info(Y,X,CAVE_EXTRA); \
}

#define place_extra_grid(C) \
{ \
  (C)->feat = fill_type[randint0(100)]; \
  (C)->info &= ~(CAVE_MASK); \
  (C)->info |= CAVE_EXTRA; \
}

#define place_extra_noperm_bold(Y, X) \
{ \
  set_cave_feat(Y,X,fill_type[randint0(100)]); \
  if ((cave[Y][X].feat >= FEAT_PERM_EXTRA) && (cave[Y][X].feat <= FEAT_PERM_SOLID)) cave[Y][X].feat -= 0x04; \
  else if (cave[Y][X].feat == FEAT_MOUNTAIN) cave[Y][X].feat = feat_wall_inner; \
  cave[Y][X].info &= ~(CAVE_MASK); \
  add_cave_info(Y,X,CAVE_EXTRA); \
}

#define place_inner_bold(Y, X) \
{ \
  set_cave_feat(Y,X,feat_wall_inner); \
  cave[Y][X].info &= ~(CAVE_MASK); \
  add_cave_info(Y,X,CAVE_INNER); \
}

#define place_inner_grid(C) \
{ \
  (C)->feat = feat_wall_inner; \
  (C)->info &= ~(CAVE_MASK); \
  (C)->info |= CAVE_INNER; \
}

#define place_outer_bold(Y, X) \
{ \
  set_cave_feat(Y,X,feat_wall_outer); \
  cave[Y][X].info &= ~(CAVE_MASK); \
  add_cave_info(Y,X,CAVE_OUTER); \
}

#define place_outer_grid(C) \
{ \
  (C)->feat = feat_wall_outer; \
  (C)->info &= ~(CAVE_MASK); \
  (C)->info |= CAVE_OUTER; \
}

#define place_outer_noperm_bold(Y, X) \
{ \
  if ((feat_wall_outer >= FEAT_PERM_EXTRA) && (feat_wall_outer <= FEAT_PERM_SOLID)) set_cave_feat(Y, X, feat_wall_outer-0x04); \
  else if (feat_wall_outer == FEAT_MOUNTAIN) cave[Y][X].feat = feat_wall_inner; \
  else set_cave_feat(Y,X,feat_wall_outer); \
  cave[Y][X].info &= ~(CAVE_MASK); \
  add_cave_info(Y,X,(CAVE_OUTER | CAVE_VAULT)); \
}

#define place_outer_noperm_grid(C) \
{ \
  if ((feat_wall_outer >= FEAT_PERM_EXTRA) && (feat_wall_outer <= FEAT_PERM_SOLID)) (C)->feat = feat_wall_outer-0x04; \
  else if (feat_wall_outer == FEAT_MOUNTAIN) (C)->feat = feat_wall_inner; \
  else (C)->feat = feat_wall_outer; \
  (C)->info &= ~(CAVE_MASK); \
  (C)->info |= (CAVE_OUTER | CAVE_VAULT); \
}

#define place_solid_bold(Y, X) \
{ \
  set_cave_feat(Y,X,feat_wall_solid); \
  cave[Y][X].info &= ~(CAVE_MASK); \
  add_cave_info(Y,X,CAVE_SOLID); \
}

#define place_solid_grid(C) \
{ \
  (C)->feat = feat_wall_solid; \
  (C)->info &= ~(CAVE_MASK); \
  (C)->info |= CAVE_SOLID; \
}

#define place_solid_noperm_bold(Y, X) \
{ \
  if ((cave[Y][X].info & CAVE_VAULT) && (feat_wall_solid >= FEAT_PERM_EXTRA) && (feat_wall_solid <= FEAT_PERM_SOLID)) set_cave_feat(Y, X, feat_wall_solid-0x04); \
  else set_cave_feat(Y,X,feat_wall_solid); \
  cave[Y][X].info &= ~(CAVE_MASK); \
  add_cave_info(Y,X,CAVE_SOLID); \
}

#define place_solid_noperm_grid(C) \
{ \
  if ((c_ptr->info & CAVE_VAULT) && (feat_wall_solid >= FEAT_PERM_EXTRA) && (feat_wall_solid <= FEAT_PERM_SOLID)) (C)->feat = feat_wall_solid-0x04; \
  else (C)->feat = feat_wall_solid; \
  (C)->info &= ~(CAVE_MASK); \
  (C)->info |= CAVE_SOLID; \
}


/* Externs */

extern bool new_player_spot(void);

extern void place_random_stairs(int y, int x);
extern void place_random_door(int y, int x, bool room);
extern void place_closed_door(int y, int x);
extern void place_floor(int x1, int x2, int y1, int y2, bool light);
extern void place_room(int x1, int x2, int y1, int y2, bool light);
extern void vault_monsters(int y1, int x1, int num);
extern void vault_objects(int y, int x, int num);
extern void vault_trap_aux(int y, int x, int yd, int xd);
extern void vault_traps(int y, int x, int yd, int xd, int num);

extern int next_to_walls(int y, int x);
extern void correct_dir(int *rdir, int *cdir, int y1, int x1, int y2, int x2);

extern void rand_dir(int *rdir, int *cdir);

extern bool get_is_floor(int x, int y);
extern void set_floor(int x, int y);

extern void build_tunnel(int row1, int col1, int row2, int col2);
extern bool build_tunnel2(int x1, int y1, int x2, int y2, int type, int cutoff);
