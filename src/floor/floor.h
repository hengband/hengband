#pragma once

#include "system/angband.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "floor/floor-save.h"


/*!
 * @brief ダンジョンの最深層 / Maximum dungeon level.
 * @details
 * The player can never reach this level
 * in the dungeon, and this value is used for various calculations
 * involving object and monster creation.  It must be at least 100.
 * Setting it below 128 may prevent the creation of some objects.
 */
#define MAX_DEPTH       128 

/*!
 * @brief generate.cで用いられる基本的なブロック数単位(垂直方向)
 * Number of grids in each block (vertically) Probably hard-coded to 11, see "generate.c"
 */
#define BLOCK_HGT 11

/*!
 * @brief generate.cで用いられる基本的なブロック数単位(水平方向)
 * Number of grids in each block (horizontally) Probably hard-coded to 11, see "generate.c"
 */
#define BLOCK_WID 11

/*!
 * @brief 表示上の基本的なブロック単位(垂直方向、PANEL_HGTの倍数で設定すること)
 * Number of grids used to display the dungeon (vertically). Must be a multiple of 11, probably hard-coded to 22.
 */
#define SCREEN_HGT 22

/*!
 * @brief 表示上の基本的なブロック単位(水平方向、PANEL_WIDの倍数で設定すること)
 * Number of grids used to display the dungeon (horizontally). Must be a multiple of 33, probably hard-coded to 66.
 */
#define SCREEN_WID 66

/*!
 * @brief 表示上のダンジョンの最大垂直サイズ(SCREEN_HGTの3倍が望ましい)
 * Maximum dungeon height in grids, must be a multiple of SCREEN_HGT, probably hard-coded to SCREEN_HGT * 3.
 */
#define MAX_HGT 66

/*!
 * @brief 表示上のダンジョンの最大水平サイズ(SCREEN_WIDの3倍が望ましい)
 * Maximum dungeon width in grids, must be a multiple of SCREEN_WID, probably hard-coded to SCREEN_WID * 3.
 */
#define MAX_WID 198

/*!
 * @brief プレイヤー用光源処理配列サイズ / Maximum size of the "lite" array (see "grid.c")
 * @details Note that the "lite radius" will NEVER exceed 14, and we would
 * never require more than 581 entries in the array for circular "lite".
 */
#define LITE_MAX 600

/*!
 * @brief モンスター用光源処理配列サイズ / Maximum size of the "mon_lite" array (see ">grid.c")
 * @details Note that the "view radius" will NEVER exceed 20, monster illumination
 * flags are dependent on CAVE_VIEW, and even if the "view" was octagonal,
 * we would never require more than 1520 entries in the array.
 */
#define MON_LITE_MAX 1536

/*!
 * @brief 視界処理配列サイズ / Maximum size of the "view" array
 * @details Note that the "view radius" will NEVER exceed 20, and even if the "view"
 * was octagonal, we would never require more than 1520 entries in the array.
 */
#define VIEW_MAX 1536

/*!
 * @brief 再描画処理用配列サイズ / Maximum size of the "redraw" array
 * @details We must be large for proper functioning of delayed redrawing.
 * We must also be as large as two times of the largest view area.
 * Note that maximum view grids are 1149 entries.
 */
#define REDRAW_MAX 2298


typedef struct floor_type {
	DUNGEON_IDX dungeon_idx;
	grid_type *grid_array[MAX_HGT];
	DEPTH dun_level;		/*!< 現在の実ダンジョン階層 base_level の参照元となる / Current dungeon level */
	DEPTH base_level;		/*!< 基本生成レベル、後述のobject_level, monster_levelの参照元となる / Base dungeon level */
	DEPTH object_level;		/*!< アイテムの生成レベル、 base_level を起点に一時変更する時に参照 / Current object creation level */
	DEPTH monster_level;	/*!< モンスターの生成レベル、 base_level を起点に一時変更する時に参照 / Current monster creation level */
	POSITION width;			/*!< Current dungeon width */
	POSITION height;		/*!< Current dungeon height */
	MONSTER_NUMBER num_repro; /*!< Current reproducer count */

	GAME_TURN generated_turn; /* Turn when level began */

	object_type *o_list; /*!< The array of dungeon items [max_o_idx] */
	OBJECT_IDX o_max; /* Number of allocated objects */
	OBJECT_IDX o_cnt; /* Number of live objects */

	monster_type *m_list; /*!< The array of dungeon monsters [max_m_idx] */
	MONSTER_IDX m_max; /* Number of allocated monsters */
	MONSTER_IDX m_cnt; /* Number of live monsters */

	s16b *mproc_list[MAX_MTIMED]; /*!< The array to process dungeon monsters[max_m_idx] */
	s16b mproc_max[MAX_MTIMED]; /*!< Number of monsters to be processed */

	POSITION_IDX lite_n; //!< Array of grids lit by player lite
	POSITION lite_y[LITE_MAX];
	POSITION lite_x[LITE_MAX];

	POSITION_IDX mon_lite_n; //!< Array of grids lit by player lite
	POSITION mon_lite_y[MON_LITE_MAX];
	POSITION mon_lite_x[MON_LITE_MAX];

	POSITION_IDX view_n; //!< Array of grids viewable to the player
	POSITION view_y[VIEW_MAX];
	POSITION view_x[VIEW_MAX];

	POSITION_IDX redraw_n; //!< Array of grids for delayed visual updating
	POSITION redraw_y[REDRAW_MAX];
	POSITION redraw_x[REDRAW_MAX];

	bool monster_noise;
	QUEST_IDX inside_quest;		/* Inside quest level */
	bool inside_arena;		/* Is character inside arena? */

} floor_type;

extern floor_type floor_info;

#define DUNGEON_MODE_NONE       0
#define DUNGEON_MODE_AND        1
#define DUNGEON_MODE_NAND       2
#define DUNGEON_MODE_OR         3
#define DUNGEON_MODE_NOR        4

/*** Dungeon type flags -- DG ***/
#define DF1_WINNER              0x00000001L
#define DF1_MAZE                0x00000002L
#define DF1_SMALLEST            0x00000004L
#define DF1_BEGINNER            0x00000008L
#define DF1_BIG                 0x00000010L
#define DF1_NO_DOORS            0x00000020L
#define DF1_WATER_RIVER         0x00000040L
#define DF1_LAVA_RIVER          0x00000080L
#define DF1_CURTAIN             0x00000100L
#define DF1_GLASS_DOOR          0x00000200L
#define DF1_CAVE                0x00000400L
#define DF1_CAVERN              0x00000800L
#define DF1_ARCADE              0x00001000L
#define DF1_LAKE_ACID           0x00002000L
#define DF1_LAKE_POISONOUS      0x00004000L
#define DF1_XXX15               0x00008000L
#define DF1_FORGET              0x00010000L
#define DF1_LAKE_WATER          0x00020000L
#define DF1_LAKE_LAVA           0x00040000L
#define DF1_LAKE_RUBBLE         0x00080000L
#define DF1_LAKE_TREE           0x00100000L
#define DF1_NO_VAULT            0x00200000L
#define DF1_ARENA               0x00400000L
#define DF1_DESTROY             0x00800000L
#define DF1_GLASS_ROOM          0x01000000L
#define DF1_NO_CAVE             0x02000000L
#define DF1_NO_MAGIC            0x04000000L
#define DF1_NO_MELEE            0x08000000L
#define DF1_CHAMELEON           0x10000000L
#define DF1_DARKNESS            0x20000000L
#define DF1_ACID_RIVER          0x40000000L
#define DF1_POISONOUS_RIVER     0x80000000L

#define DF1_LAKE_MASK (DF1_LAKE_WATER | DF1_LAKE_LAVA | DF1_LAKE_RUBBLE | DF1_LAKE_TREE | DF1_LAKE_POISONOUS | DF1_LAKE_ACID)

#define HAS_RIVER_FLAG(D_PTR) ((D_PTR)->flags1 & (DF1_WATER_RIVER | DF1_LAVA_RIVER | DF1_ACID_RIVER | DF1_POISONOUS_RIVER))

/*
 * Determines if a map location is fully inside the outer walls
 */
#define in_bounds(F,Y,X) \
   (((Y) > 0) && ((X) > 0) && ((Y) < (F)->height-1) && ((X) < (F)->width-1))

/*
 * Determines if a map location is on or inside the outer walls
 */
#define in_bounds2(F,Y,X) \
   (((Y) >= 0) && ((X) >= 0) && ((Y) < (F)->height) && ((X) < (F)->width))

/*
 * Determines if a map location is on or inside the outer walls
 * (unsigned version)
 */
#define in_bounds2u(F,Y,X) \
   (((Y) < (F)->height) && ((X) < (F)->width))


/*
 * Determine if player is on this grid
 */
#define player_bold(C,Y,X) \
	(((Y) == (C)->y) && ((X) == (C)->x))

/*
 * Grid based version of "creature_bold()"
 */
#define player_grid(C, G) \
	((G) == &(C)->current_floor_ptr->grid_array[(C)->y][(C)->x])


#define cave_have_flag_bold(F,Y,X,INDEX) \
	(have_flag(f_info[(F)->grid_array[(Y)][(X)].feat].flags, (INDEX)))


#define cave_have_flag_grid(C,INDEX) \
	(have_flag(f_info[(C)->feat].flags, (INDEX)))


/*
 * Determine if a "feature" supports "los"
 */
#define feat_supports_los(F) \
	(have_flag(f_info[(F)].flags, FF_LOS))


#define cave_los_grid(C) \
	(feat_supports_los((C)->feat))


/*
 * Determine if a "legal" grid is a "clean" floor grid
 * Determine if terrain-change spells are allowed in a grid.
 *
 * Line 1 -- forbid non-floors
 * Line 2 -- forbid object terrains
 * Line 3 -- forbid normal objects
 */
#define cave_clean_bold(F,Y,X) \
	(cave_have_flag_bold((F), (Y), (X), FF_FLOOR) && \
	 !((F)->grid_array[Y][X].info & CAVE_OBJECT) && \
	  ((F)->grid_array[Y][X].o_idx == 0))


/*
 * Determine if an object can be dropped on a "legal" grid
 *
 * Line 1 -- forbid non-drops
 * Line 2 -- forbid object terrains
 */
#define cave_drop_bold(F,Y,X) \
	(cave_have_flag_bold((F), (Y), (X), FF_DROP) && \
	 !((F)->grid_array[Y][X].info & CAVE_OBJECT))


/*
 * Determine if a "legal" grid is an "naked" floor grid
 *
 * Line 1 -- forbid non-clean gird
 * Line 2 -- forbid monsters
 * Line 3 -- forbid the player
 */
#define cave_naked_bold(C,F,Y,X) \
	(cave_clean_bold(F,Y,X) && \
	 !((F)->grid_array[Y][X].m_idx) && \
	 !player_bold(C,Y,X))


/*
 * Determine if a "legal" grid is "permanent"
 *
 * Line 1 -- permanent flag
 */
#define cave_perma_bold(F,Y,X) \
	(cave_have_flag_bold((F), (Y), (X), FF_PERMANENT))


/*
 * Grid based version of "cave_perma_bold()"
 */
#define cave_perma_grid(C) \
	(cave_have_flag_grid((C), FF_PERMANENT))


/*
 * Does the grid stop disintegration?
 */
#define cave_stop_disintegration(F,Y,X) \
	(!cave_have_flag_bold((F), (Y), (X), FF_PROJECT) && \
	 (!cave_have_flag_bold((F), (Y), (X), FF_HURT_DISI) || \
	  cave_have_flag_bold((F), (Y), (X), FF_PERMANENT)))


/*
 * Determine if a "legal" grid is within "los" of the player
 *
 * Note the use of comparison to zero to force a "boolean" result
 */
#define player_has_los_grid(C) \
    (((C)->info & (CAVE_VIEW)) != 0)

/*
 * Determine if a "legal" grid is within "los" of the player
 *
 * Note the use of comparison to zero to force a "boolean" result
 */
#define player_has_los_bold(C,Y,X) \
    ((((C)->current_floor_ptr->grid_array[Y][X].info & (CAVE_VIEW)) != 0) || (C)->phase_out)


/*
 * Determine if a "feature" is "permanent wall"
 */
#define permanent_wall(F) \
	(have_flag((F)->flags, FF_WALL) && \
	 have_flag((F)->flags, FF_PERMANENT))

extern saved_floor_type saved_floors[MAX_SAVED_FLOORS];

/*
 * Convert a "location" (Y,X) into a "grid" (G)
 */
#define GRID(Y,X) \
	(256 * (Y) + (X))

/*
 * Convert a "grid" (G) into a "location" (Y)
 */
#define GRID_Y(G) \
	((int)((G) / 256U))

/*
 * Convert a "grid" (G) into a "location" (X)
 */
#define GRID_X(G) \
	((int)((G) % 256U))

extern bool pattern_tile(floor_type *floor_ptr, POSITION y, POSITION x);
extern bool is_cave_empty_bold(player_type *player_ptr, POSITION x, POSITION y);
extern bool is_cave_empty_bold2(player_type *player_ptr, POSITION x, POSITION y);
extern void update_smell(floor_type *floor_ptr, player_type *subject_ptr);

extern void add_door(player_type *player_ptr, POSITION x, POSITION y);
extern void place_secret_door(player_type *player_ptr, POSITION y, POSITION x, int type);
extern void place_locked_door(player_type *player_ptr, POSITION y, POSITION x);
extern void forget_flow(floor_type *floor_ptr);
extern void place_random_stairs(player_type *player_ptr, POSITION y, POSITION x);

extern bool los(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
extern bool projectable(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);

extern void vault_monsters(player_type *player_ptr, POSITION y1, POSITION x1, int num);
extern bool cave_valid_bold(floor_type *floor_ptr, POSITION y, POSITION x);
extern void cave_set_feat(player_type *player_ptr, POSITION y, POSITION x, FEAT_IDX feat);
extern void place_random_door(player_type *player_ptr, POSITION y, POSITION x, bool room);
extern void place_closed_door(player_type *player_ptr, POSITION y, POSITION x, int type);

extern void wipe_o_list(floor_type *floor_ptr);
extern void vault_trap_aux(player_type *player_ptr, POSITION y, POSITION x, POSITION yd, POSITION xd);

extern bool get_is_floor(floor_type *floor_ptr, POSITION x, POSITION y);
extern void try_door(player_type *player_ptr, POSITION y, POSITION x);

extern FEAT_IDX conv_dungeon_feat(floor_type *floor_ptr, FEAT_IDX newfeat);
extern void vault_objects(player_type *player_ptr, POSITION y, POSITION x, int num);

extern sint project_path(player_type *player_ptr, u16b *gp, POSITION range, POSITION y1, POSITION x1, POSITION y2, POSITION x2, BIT_FLAGS flg);

extern void set_floor(player_type *player_ptr, POSITION x, POSITION y);
extern void place_object(player_type *owner_ptr, POSITION y, POSITION x, BIT_FLAGS mode);
extern void place_gold(player_type *player_ptr, POSITION y, POSITION x);
extern void delete_monster(player_type *player_ptr, POSITION y, POSITION x);
extern void compact_objects(player_type *owner_ptr, int size);
extern void vault_traps(player_type *player_ptr, POSITION y, POSITION x, POSITION yd, POSITION xd, int num);
extern void scatter(player_type *player_ptr, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION d, BIT_FLAGS mode);

extern bool cave_los_bold(floor_type *floor_ptr, POSITION y, POSITION x);
