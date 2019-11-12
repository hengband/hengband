#pragma once

/*!
 * @file grid.h
 * @brief ダンジョンの生成処理の基幹部分ヘッダーファイル
 * @date 2014/08/15
 * @details
 * Purpose: header file for grid.c, used only in dungeon generation
 * files (generate.c, rooms.c)
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */


 /*
  * A single "grid" in a Cave
  *
  * Note that several aspects of the code restrict the actual p_ptr->current_floor_ptr->grid_array
  * to a max size of 256 by 256.  In partcular, locations are often
  * saved as bytes, limiting each coordinate to the 0-255 range.
  *
  * The "o_idx" and "m_idx" fields are very interesting.  There are
  * many places in the code where we need quick access to the actual
  * monster or object(s) in a given grid.  The easiest way to
  * do this is to simply keep the index of the monster and object
  * (if any) with the grid, but this takes 198*66*4 bytes of memory.
  * Several other methods come to mind, which require only half this
  * amound of memory, but they all seem rather complicated, and would
  * probably add enough code that the savings would be lost.  So for
  * these reasons, we simply store an index into the "o_list" and
  * "p_ptr->current_floor_ptr->m_list" arrays, using "zero" when no monster/object is present.
  *
  * Note that "o_idx" is the index of the top object in a stack of
  * objects, using the "next_o_idx" field of objects (see below) to
  * create the singly linked list of objects.  If "o_idx" is zero
  * then there are no objects in the grid.
  *
  * Note the special fields for the "MONSTER_FLOW" code.
  */

typedef struct player_type player_type; // TODO: Delete Finally.

typedef struct grid_type grid_type;

struct grid_type
{
	BIT_FLAGS info;		/* Hack -- p_ptr->current_floor_ptr->grid_array flags */

	FEAT_IDX feat;		/* Hack -- feature type */
	OBJECT_IDX o_idx;		/* Object in this grid */
	MONSTER_IDX m_idx;		/* Monster in this grid */

	/*! 地形の特別な情報を保存する / Special p_ptr->current_floor_ptr->grid_array info
	 * 具体的な使用一覧はクエスト行き階段の移行先クエストID、
	 * 各ダンジョン入口の移行先ダンジョンID、
	 *
	 */
	s16b special;

	FEAT_IDX mimic;		/* Feature to mimic */

	byte cost;		/* Hack -- cost of flowing */
	byte dist;		/* Hack -- distance from player */
	byte when;		/* Hack -- when cost was computed */
};

/*
 *  A structure type for terrain template of saving dungeon floor
 */
typedef struct
{
	BIT_FLAGS info;
	FEAT_IDX feat;
	FEAT_IDX mimic;
	s16b special;
	u16b occurrence;
} grid_template_type;

/* This should not be used */
/*#define set_cave_info(Y,X,I)    (p_ptr->current_floor_ptr->grid_array[(Y)][(X)].info = (I)) */

#define feat_locked_door_random(DOOR_TYPE) \
	(feat_door[(DOOR_TYPE)].num_locked ? \
	 feat_door[(DOOR_TYPE)].locked[randint0(feat_door[(DOOR_TYPE)].num_locked)] : feat_none)

#define feat_jammed_door_random(DOOR_TYPE) \
	(feat_door[(DOOR_TYPE)].num_jammed ? \
	 feat_door[(DOOR_TYPE)].jammed[randint0(feat_door[(DOOR_TYPE)].num_jammed)] : feat_none)

/* Macros */
#define set_cave_feat(FL,Y,X,F)    ((FL)->grid_array[(Y)][(X)].feat = (F))
#define add_cave_info(FL,Y,X,I)    ((FL)->grid_array[(Y)][(X)].info |= (I))

/*!
 * @brief 指定座標に瓦礫を配置する
 * @param Y 指定Y座標
 * @param X 指定X座標
 */
#define place_rubble(F,Y,X)       set_cave_feat(F,Y,X,feat_rubble)

/*!
 * @brief 指定座標がFLOOR属性を持ったマスかどうかを返す
 * @param Y 指定Y座標
 * @param X 指定X座標
 * @return FLOOR属性を持っているならばTRUE
 */
#define is_floor_bold(F,Y,X) (F->grid_array[Y][X].info & CAVE_FLOOR)
#define is_extra_bold(F,Y,X) (F->grid_array[Y][X].info & CAVE_EXTRA)

#define is_inner_bold(F,Y,X) (F->grid_array[Y][X].info & CAVE_INNER)
#define is_outer_bold(F,Y,X) (F->grid_array[Y][X].info & CAVE_OUTER)
#define is_solid_bold(F,Y,X) (F->grid_array[Y][X].info & CAVE_SOLID)

#define is_floor_grid(C) ((C)->info & CAVE_FLOOR)
#define is_extra_grid(C) ((C)->info & CAVE_EXTRA)
#define is_inner_grid(C) ((C)->info & CAVE_INNER)
#define is_outer_grid(C) ((C)->info & CAVE_OUTER)
#define is_solid_grid(C) ((C)->info & CAVE_SOLID)

#define place_floor_bold(Y, X) \
{ \
	set_cave_feat(p_ptr->current_floor_ptr, Y,X,feat_ground_type[randint0(100)]); \
	p_ptr->current_floor_ptr->grid_array[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(p_ptr->current_floor_ptr, Y,X,CAVE_FLOOR); \
	delete_monster(Y, X); \
}

#define place_floor_grid(C) \
{ \
	(C)->feat = feat_ground_type[randint0(100)]; \
	(C)->info &= ~(CAVE_MASK); \
	(C)->info |= CAVE_FLOOR; \
	if ((C)->m_idx) delete_monster_idx((C)->m_idx); \
}

#define place_extra_bold(Y, X) \
{ \
	set_cave_feat(p_ptr->current_floor_ptr, Y,X,feat_wall_type[randint0(100)]); \
	p_ptr->current_floor_ptr->grid_array[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(p_ptr->current_floor_ptr, Y,X,CAVE_EXTRA); \
	delete_monster(Y, X); \
}

#define place_extra_grid(C) \
{ \
	(C)->feat = feat_wall_type[randint0(100)]; \
	(C)->info &= ~(CAVE_MASK); \
	(C)->info |= CAVE_EXTRA; \
	if ((C)->m_idx) delete_monster_idx((C)->m_idx); \
}

#define place_extra_perm_bold(Y, X) \
{ \
	set_cave_feat(floor_ptr, Y,X,feat_permanent); \
	p_ptr->current_floor_ptr->grid_array[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(p_ptr->current_floor_ptr, Y,X,CAVE_EXTRA); \
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
	set_cave_feat(floor_ptr, Y,X,feat_wall_type[randint0(100)]); \
	_f_ptr = &f_info[p_ptr->current_floor_ptr->grid_array[Y][X].feat]; \
	if (permanent_wall(_f_ptr)) p_ptr->current_floor_ptr->grid_array[Y][X].feat = feat_state(p_ptr->current_floor_ptr->grid_array[Y][X].feat, FF_UNPERM); \
	p_ptr->current_floor_ptr->grid_array[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(p_ptr->current_floor_ptr, Y,X,CAVE_EXTRA); \
	delete_monster(Y, X); \
}

#define place_inner_bold(Y, X) \
{ \
	set_cave_feat(p_ptr->current_floor_ptr, Y,X,feat_wall_inner); \
	p_ptr->current_floor_ptr->grid_array[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(p_ptr->current_floor_ptr, Y,X,CAVE_INNER); \
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
	set_cave_feat(p_ptr->current_floor_ptr, Y,X,feat_permanent); \
	p_ptr->current_floor_ptr->grid_array[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(p_ptr->current_floor_ptr, Y,X,CAVE_INNER); \
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
	set_cave_feat(p_ptr->current_floor_ptr, Y,X,feat_wall_outer); \
	p_ptr->current_floor_ptr->grid_array[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(p_ptr->current_floor_ptr, Y,X,CAVE_OUTER); \
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
	set_cave_feat(floor_ptr, Y,X,feat_permanent); \
	p_ptr->current_floor_ptr->grid_array[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(p_ptr->current_floor_ptr, Y,X,CAVE_OUTER); \
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
	if (permanent_wall(_f_ptr)) set_cave_feat(p_ptr->current_floor_ptr, Y, X, (s16b)feat_state(feat_wall_outer, FF_UNPERM)); \
	else set_cave_feat(p_ptr->current_floor_ptr, Y,X,feat_wall_outer); \
	p_ptr->current_floor_ptr->grid_array[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(p_ptr->current_floor_ptr, Y,X,(CAVE_OUTER | CAVE_VAULT)); \
	delete_monster(Y, X); \
}

#define place_outer_noperm_grid(C) \
{ \
	feature_type *_f_ptr = &f_info[feat_wall_outer]; \
	if (permanent_wall(_f_ptr)) (C)->feat = (s16b)feat_state(feat_wall_outer, FF_UNPERM); \
	else (C)->feat = feat_wall_outer; \
	(C)->info &= ~(CAVE_MASK); \
	(C)->info |= (CAVE_OUTER | CAVE_VAULT); \
	if ((C)->m_idx) delete_monster_idx((C)->m_idx); \
}

#define place_solid_bold(Y, X) \
{ \
	set_cave_feat(p_ptr->current_floor_ptr, Y,X,feat_wall_solid); \
	p_ptr->current_floor_ptr->grid_array[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(p_ptr->current_floor_ptr, Y,X,CAVE_SOLID); \
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
	set_cave_feat(floor_ptr, Y,X,feat_permanent); \
	p_ptr->current_floor_ptr->grid_array[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(p_ptr->current_floor_ptr, Y,X,CAVE_SOLID); \
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
	if ((p_ptr->current_floor_ptr->grid_array[Y][X].info & CAVE_VAULT) && permanent_wall(_f_ptr)) \
		set_cave_feat(p_ptr->current_floor_ptr, Y, X, feat_state(feat_wall_solid, FF_UNPERM)); \
	else set_cave_feat(p_ptr->current_floor_ptr, Y,X,feat_wall_solid); \
	p_ptr->current_floor_ptr->grid_array[Y][X].info &= ~(CAVE_MASK); \
	add_cave_info(p_ptr->current_floor_ptr, Y,X,CAVE_SOLID); \
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


/*
 * 特殊なマス状態フラグ / Special grid flags
 */
#define CAVE_MARK       0x0001    /*!< 現在プレイヤーの記憶に収まっている / memorized feature */
#define CAVE_GLOW       0x0002    /*!< マス自体が光源を持っている / self-illuminating */
#define CAVE_ICKY       0x0004    /*!< 生成されたVaultの一部である / part of a vault */
#define CAVE_ROOM       0x0008    /*!< 生成された部屋の一部である / part of a room */
#define CAVE_LITE       0x0010    /*!< 現在光に照らされている / lite flag  */
#define CAVE_VIEW       0x0020    /*!< 現在プレイヤーの視界に収まっている / view flag */
#define CAVE_TEMP       0x0040    /*!< 光源に関する処理のアルゴリズム用記録フラグ / temp flag */
#define CAVE_XTRA       0x0080    /*!< 視界に関する処理のアルゴリズム用記録フラグ(update_view()等参照) / misc flag */
#define CAVE_MNLT       0x0100    /*!< モンスターの光源によって照らされている / Illuminated by monster */
#define CAVE_MNDK       0x8000    /*!< モンスターの暗源によって暗闇になっている / Darken by monster */

 /* Used only while p_ptr->current_floor_ptr->grid_array generation */
#define CAVE_FLOOR      0x0200	/*!< フロア属性のあるマス */
#define CAVE_EXTRA      0x0400
#define CAVE_INNER      0x0800
#define CAVE_OUTER      0x1000
#define CAVE_SOLID      0x2000
#define CAVE_VAULT      0x4000
#define CAVE_MASK (CAVE_FLOOR | CAVE_EXTRA | CAVE_INNER | CAVE_OUTER | CAVE_SOLID | CAVE_VAULT)

/* Used only after p_ptr->current_floor_ptr->grid_array generation */
#define CAVE_KNOWN      0x0200    /* Directly viewed or map detected flag */
#define CAVE_NOTE       0x0400    /* Flag for delayed visual update (needs note_spot()) */
#define CAVE_REDRAW     0x0800    /* Flag for delayed visual update (needs lite_spot()) */
#define CAVE_OBJECT     0x1000    /* Mirror, glyph, etc. */
#define CAVE_UNSAFE     0x2000    /* Might have trap */
#define CAVE_IN_DETECT  0x4000    /* trap detected area (inner circle only) */

/* Types of conversions */
#define CONVERT_TYPE_FLOOR   0
#define CONVERT_TYPE_WALL    1
#define CONVERT_TYPE_INNER   2
#define CONVERT_TYPE_OUTER   3
#define CONVERT_TYPE_SOLID   4
#define CONVERT_TYPE_STREAM1 5
#define CONVERT_TYPE_STREAM2 6

/* Externs */

extern bool new_player_spot(void);

extern void place_random_stairs(POSITION y, POSITION x);
extern void place_random_door(POSITION y, POSITION x, bool room);
extern void add_door(POSITION x, POSITION y);

/* Types of doors */
#define DOOR_DEFAULT    -1
#define DOOR_DOOR        0
#define DOOR_GLASS_DOOR  1
#define DOOR_CURTAIN     2

#define MAX_DOOR_TYPES   3
extern void place_closed_door(POSITION y, POSITION x, int type);


extern void try_door(POSITION y, POSITION x);
extern void place_floor(POSITION x1, POSITION x2, POSITION y1, POSITION y2, bool light);
extern void place_room(POSITION x1, POSITION x2, POSITION y1, POSITION y2, bool light);
extern void vault_monsters(POSITION y1, POSITION x1, int num);
extern void vault_objects(POSITION y, POSITION x, int num);
extern void vault_trap_aux(POSITION y, POSITION x, POSITION yd, POSITION xd);
extern void vault_traps(POSITION y, POSITION x, POSITION yd, POSITION xd, int num);

extern bool get_is_floor(POSITION x, POSITION y);
extern void set_floor(POSITION x, POSITION y);
extern void place_bound_perm_wall(grid_type *g_ptr);

extern bool is_known_trap(grid_type *g_ptr);
extern bool is_hidden_door(grid_type *g_ptr);
extern bool is_mirror_grid(grid_type *g_ptr);
extern bool is_glyph_grid(grid_type *g_ptr);
extern bool is_explosive_rune_grid(grid_type *g_ptr);

extern bool player_can_enter(FEAT_IDX feature, BIT_FLAGS16 mode);

/*!
 * マス構造体のspecial要素を利用する地形かどうかを判定するマクロ / Is this feature has special meaning (except floor_id) with g_ptr->special?
 */
#define feat_uses_special(F) (have_flag(f_info[(F)].flags, FF_SPECIAL))

/* grids.c */
extern POSITION distance(POSITION y1, POSITION x1, POSITION y2, POSITION x2);
extern bool los(POSITION y1, POSITION x1, POSITION y2, POSITION x2);
extern void update_local_illumination(player_type *creature_ptr, POSITION y, POSITION x);
extern bool player_can_see_bold(POSITION y, POSITION x);
extern bool cave_valid_bold(POSITION y, POSITION x);
extern bool no_lite(void);
extern void map_info(POSITION y, POSITION x, TERM_COLOR *ap, SYMBOL_CODE *cp, TERM_COLOR *tap, SYMBOL_CODE *tcp);
extern void print_rel(SYMBOL_CODE c, TERM_COLOR a, TERM_LEN y, TERM_LEN x);
extern void note_spot(POSITION y, POSITION x);
extern void lite_spot(POSITION y, POSITION x);
extern void delayed_visual_update(void);
extern void update_flow(void);
extern void cave_set_feat(POSITION y, POSITION x, FEAT_IDX feat);
extern FEAT_IDX conv_dungeon_feat(FEAT_IDX newfeat);
extern FEAT_IDX feat_state(FEAT_IDX feat, int action);
extern void cave_alter_feat(POSITION y, POSITION x, int action);
extern void remove_mirror(POSITION y, POSITION x);
extern bool is_open(FEAT_IDX feat);
extern bool check_local_illumination(POSITION y, POSITION x);

extern bool cave_monster_teleportable_bold(MONSTER_IDX m_idx, POSITION y, POSITION x, BIT_FLAGS mode);
extern bool cave_player_teleportable_bold(POSITION y, POSITION x, BIT_FLAGS mode);


/*!
 * モンスターにより照明が消されている地形か否かを判定する。 / Is this grid "darkened" by monster?
 */
#define darkened_grid(C) \
	((((C)->info & (CAVE_VIEW | CAVE_LITE | CAVE_MNLT | CAVE_MNDK)) == (CAVE_VIEW | CAVE_MNDK)) && \
	!p_ptr->see_nocto)

/*
 * Get feature mimic from f_info[] (applying "mimic" field)
 */
#define get_feat_mimic(C) \
	(f_info[(C)->mimic ? (C)->mimic : (C)->feat].mimic)

/*
 * This macro allows us to efficiently add a grid to the "lite" array,
 * note that we are never called for illegal grids, or for grids which
 * have already been placed into the "lite" array, and we are never
 * called when the "lite" array is full.
 */
#define cave_lite_hack(F, Y,X) \
{\
	if (!((F)->grid_array[Y][X].info & (CAVE_LITE))) \
	{ \
		(F)->grid_array[Y][X].info |= (CAVE_LITE); \
		(F)->lite_y[p_ptr->current_floor_ptr->lite_n] = (Y); \
		(F)->lite_x[p_ptr->current_floor_ptr->lite_n++] = (X); \
	} \
}

/*
 * For delayed visual update
 */
#define cave_note_and_redraw_later(C,Y,X) \
{\
	(C)->info |= CAVE_NOTE; \
	cave_redraw_later((C), (Y), (X)); \
}

/*
 * For delayed visual update
 */
#define cave_redraw_later(C,Y,X) \
{\
	if (!((C)->info & CAVE_REDRAW)) \
	{ \
		(C)->info |= CAVE_REDRAW; \
		p_ptr->current_floor_ptr->redraw_y[p_ptr->current_floor_ptr->redraw_n] = (Y); \
		p_ptr->current_floor_ptr->redraw_x[p_ptr->current_floor_ptr->redraw_n++] = (X); \
	} \
}

 /*
  * This macro allows us to efficiently add a grid to the "view" array,
  * note that we are never called for illegal grids, or for grids which
  * have already been placed into the "view" array, and we are never
  * called when the "view" array is full.
  */
#define cave_view_hack(C,Y,X) \
{\
    if (!((C)->info & (CAVE_VIEW))){\
    (C)->info |= (CAVE_VIEW); \
    p_ptr->current_floor_ptr->view_y[p_ptr->current_floor_ptr->view_n] = (Y); \
    p_ptr->current_floor_ptr->view_x[p_ptr->current_floor_ptr->view_n] = (X); \
    p_ptr->current_floor_ptr->view_n++;}\
}

