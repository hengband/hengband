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

#include "system/angband.h"
#include "floor/geometry.h"
#include "spell/spells-util.h"

 /*
  * A single "grid" in a Cave
  *
  * Note that several aspects of the code restrict the actual grid
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
  * ">m_list" arrays, using "zero" when no monster/object is present.
  *
  * Note that "o_idx" is the index of the top object in a stack of
  * objects, using the "next_o_idx" field of objects (see below) to
  * create the singly linked list of objects.  If "o_idx" is zero
  * then there are no objects in the grid.
  *
  * Note the special fields for the "MONSTER_FLOW" code.
  */

typedef struct grid_type {
	BIT_FLAGS info;		/* Hack -- grid flags */

	FEAT_IDX feat;		/* Hack -- feature type */
	OBJECT_IDX o_idx;		/* Object in this grid */
	MONSTER_IDX m_idx;		/* Monster in this grid */

	/*! 地形の特別な情報を保存する / Special grid info
	 * 具体的な使用一覧はクエスト行き階段の移行先クエストID、
	 * 各ダンジョン入口の移行先ダンジョンID、
	 *
	 */
	s16b special;

	FEAT_IDX mimic;		/* Feature to mimic */

	byte cost;		/* Hack -- cost of flowing */
	byte dist;		/* Hack -- distance from player */
	byte when;		/* Hack -- when cost was computed */
} grid_type;

/*  A structure type for terrain template of saving dungeon floor */
typedef struct grid_template_type {
	BIT_FLAGS info;
	FEAT_IDX feat;
	FEAT_IDX mimic;
	s16b special;
	u16b occurrence;
} grid_template_type;

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

/* Used only while floor generation */
#define CAVE_FLOOR      0x0200	/*!< フロア属性のあるマス */
#define CAVE_EXTRA      0x0400
#define CAVE_INNER      0x0800
#define CAVE_OUTER      0x1000
#define CAVE_SOLID      0x2000
#define CAVE_VAULT      0x4000
#define CAVE_MASK (CAVE_FLOOR | CAVE_EXTRA | CAVE_INNER | CAVE_OUTER | CAVE_SOLID | CAVE_VAULT)

/* Used only after floor generation */
#define CAVE_KNOWN      0x0200    /* Directly viewed or map detected flag */
#define CAVE_NOTE       0x0400    /* Flag for delayed visual update (needs note_spot()) */
#define CAVE_REDRAW     0x0800    /* Flag for delayed visual update (needs lite_spot()) */
#define CAVE_OBJECT     0x1000    /* Mirror, rune, etc. */
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

/* Types of doors */
#define DOOR_DEFAULT    -1
#define DOOR_DOOR        0
#define DOOR_GLASS_DOOR  1
#define DOOR_CURTAIN     2

extern bool new_player_spot(player_type *creature_ptr);
extern pos_list tmp_pos;

extern void place_bound_perm_wall(player_type *player_ptr, grid_type *g_ptr);
extern bool is_known_trap(player_type *player_ptr, grid_type *g_ptr);
extern bool is_hidden_door(player_type *player_ptr, grid_type *g_ptr);
extern bool is_mirror_grid(grid_type *g_ptr);
extern bool is_rune_protection_grid(grid_type *g_ptr);
extern bool is_rune_explosion_grid(grid_type *g_ptr);
extern bool player_can_enter(player_type *creature_ptr, FEAT_IDX feature, BIT_FLAGS16 mode);

/*!
 * マス構造体のspecial要素を利用する地形かどうかを判定するマクロ / Is this feature has special meaning (except floor_id) with g_ptr->special?
 */
bool feat_uses_special(FEAT_IDX f_idx);

extern POSITION distance(POSITION y1, POSITION x1, POSITION y2, POSITION x2);
extern void update_local_illumination(player_type *creature_ptr, POSITION y, POSITION x);
extern bool no_lite(player_type *creature_ptr);
extern void print_rel(player_type *subject_ptr, SYMBOL_CODE c, TERM_COLOR a, POSITION y, POSITION x);
extern void note_spot(player_type *player_ptr, POSITION y, POSITION x);
extern void lite_spot(player_type *player_ptr, POSITION y, POSITION x);
extern void update_flow(player_type *subject_ptr);
extern FEAT_IDX feat_state(player_type *player_ptr, FEAT_IDX feat, int action);
extern void cave_alter_feat(player_type *player_ptr, POSITION y, POSITION x, int action);
extern void remove_mirror(player_type *caster_ptr, POSITION y, POSITION x);
extern bool is_open(player_type *player_ptr, FEAT_IDX feat);
extern bool check_local_illumination(player_type *creature_ptr, POSITION y, POSITION x);

extern bool cave_monster_teleportable_bold(player_type *player_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, teleport_flags mode);
extern bool cave_player_teleportable_bold(player_type *player_ptr, POSITION y, POSITION x, teleport_flags mode);

enum grid_bold_type
{
	GB_FLOOR,
	GB_EXTRA,
	GB_EXTRA_PERM,
	GB_INNER,
	GB_INNER_PERM,
	GB_OUTER,
	GB_OUTER_NOPERM,
	GB_SOLID,
	GB_SOLID_PERM,
	GB_SOLID_NOPERM
};

void place_grid(player_type *player_ptr, grid_type *g_ptr, grid_bold_type pg_type);
bool darkened_grid(player_type *player_ptr, grid_type *g_ptr);
void delete_monster(player_type *player_ptr, POSITION y, POSITION x);
void place_bold(player_type *player_ptr, POSITION y, POSITION x, grid_bold_type gh_type);
void set_cave_feat(floor_type *floor_ptr, POSITION y, POSITION x, FEAT_IDX feature_idx);
void add_cave_info(floor_type *floor_ptr, POSITION y, POSITION x, int cave_mask);
FEAT_IDX get_feat_mimic(grid_type *g_ptr);

/*
 * This macro allows us to efficiently add a grid to the "lite" array,
 * note that we are never called for illegal grids, or for grids which
 * have already been placed into the "lite" array, and we are never
 * called when the "lite" array is full.
 */
#define cave_lite_hack(F,Y,X) \
{\
	if (!((F)->grid_array[Y][X].info & (CAVE_LITE))) \
	{ \
		(F)->grid_array[Y][X].info |= (CAVE_LITE); \
		(F)->lite_y[(F)->lite_n] = (Y); \
		(F)->lite_x[(F)->lite_n++] = (X); \
	} \
}

/*
 * For delayed visual update
 */
#define cave_note_and_redraw_later(F,C,Y,X) \
{\
	(C)->info |= CAVE_NOTE; \
	cave_redraw_later((F), (C), (Y), (X)); \
}

/*
* For delayed visual update
*/
#define cave_redraw_later(F,G,Y,X) \
{\
	if (!((G)->info & CAVE_REDRAW)) \
	{ \
		(G)->info |= CAVE_REDRAW; \
		(F)->redraw_y[(F)->redraw_n] = (Y); \
		(F)->redraw_x[(F)->redraw_n++] = (X); \
	} \
}

/*
 * This macro allows us to efficiently add a grid to the "view" array,
 * note that we are never called for illegal grids, or for grids which
 * have already been placed into the "view" array, and we are never
 * called when the "view" array is full.
 */
#define cave_view_hack(F,C,Y,X) \
{\
    if (!((C)->info & (CAVE_VIEW))){\
    (C)->info |= (CAVE_VIEW); \
    (F)->view_y[(F)->view_n] = (Y); \
    (F)->view_x[(F)->view_n] = (X); \
    (F)->view_n++;}\
}

int count_dt(player_type *creature_ptr, POSITION *y, POSITION *x, bool (*test)(player_type *, FEAT_IDX), bool under);
