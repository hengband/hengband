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

#include "spell/spells-util.h"
#include "object/object-index-list.h"

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
#define place_rubble(F, Y, X) set_cave_feat(F, Y, X, feat_rubble)

/*!
 * @brief 指定座標がFLOOR属性を持ったマスかどうかを返す
 * @param Y 指定Y座標
 * @param X 指定X座標
 * @return FLOOR属性を持っているならばTRUE
 */
#define is_floor_bold(F, Y, X) (F->grid_array[Y][X].info & CAVE_FLOOR)
#define is_extra_bold(F, Y, X) (F->grid_array[Y][X].info & CAVE_EXTRA)

#define is_inner_bold(F, Y, X) (F->grid_array[Y][X].info & CAVE_INNER)
#define is_outer_bold(F, Y, X) (F->grid_array[Y][X].info & CAVE_OUTER)
#define is_solid_bold(F, Y, X) (F->grid_array[Y][X].info & CAVE_SOLID)

#define is_floor_grid(C) ((C)->info & CAVE_FLOOR)
#define is_extra_grid(C) ((C)->info & CAVE_EXTRA)
#define is_inner_grid(C) ((C)->info & CAVE_INNER)
#define is_outer_grid(C) ((C)->info & CAVE_OUTER)
#define is_solid_grid(C) ((C)->info & CAVE_SOLID)

// clang-format off

/* Types of doors */
#define DOOR_DEFAULT    -1
#define DOOR_DOOR        0
#define DOOR_GLASS_DOOR  1
#define DOOR_CURTAIN     2

// clang-format on
struct floor_type;
struct grid_type;
struct player_type;
struct monster_race;
extern bool new_player_spot(player_type *creature_ptr);

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

extern void update_local_illumination(player_type *creature_ptr, POSITION y, POSITION x);
extern bool no_lite(player_type *creature_ptr);
extern void print_rel(player_type *subject_ptr, SYMBOL_CODE c, TERM_COLOR a, POSITION y, POSITION x);
extern void note_spot(player_type *player_ptr, POSITION y, POSITION x);
extern void lite_spot(player_type *player_ptr, POSITION y, POSITION x);
extern void update_flow(player_type *subject_ptr);
extern byte grid_cost(grid_type *g_ptr, monster_race *r_ptr);
extern byte grid_dist(grid_type *g_ptr, monster_race *r_ptr);
extern FEAT_IDX feat_state(floor_type *floor_ptr, FEAT_IDX feat, int action);
extern void cave_alter_feat(player_type *player_ptr, POSITION y, POSITION x, int action);
extern void remove_mirror(player_type *caster_ptr, POSITION y, POSITION x);
extern bool is_open(player_type *player_ptr, FEAT_IDX feat);
extern bool check_local_illumination(player_type *creature_ptr, POSITION y, POSITION x);

extern bool cave_monster_teleportable_bold(player_type *player_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, teleport_flags mode);
extern bool cave_player_teleportable_bold(player_type *player_ptr, POSITION y, POSITION x, teleport_flags mode);

// clang-format off
enum grid_bold_type {
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
// clang-format on

void place_grid(player_type *player_ptr, grid_type *g_ptr, grid_bold_type pg_type);
bool darkened_grid(player_type *player_ptr, grid_type *g_ptr);
void delete_monster(player_type *player_ptr, POSITION y, POSITION x);
void place_bold(player_type *player_ptr, POSITION y, POSITION x, grid_bold_type gh_type);
void set_cave_feat(floor_type *floor_ptr, POSITION y, POSITION x, FEAT_IDX feature_idx);
void add_cave_info(floor_type *floor_ptr, POSITION y, POSITION x, int cave_mask);
FEAT_IDX get_feat_mimic(grid_type *g_ptr);

// clang-format off

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

// clang-format on

int count_dt(player_type *creature_ptr, POSITION *y, POSITION *x, bool (*test)(player_type *, FEAT_IDX), bool under);
