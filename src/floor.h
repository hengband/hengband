#pragma once

#include "feature.h"
#include "grid.h"
#include "object.h"
#include "monster.h"

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


typedef struct {
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


/*
 * Determine if a "legal" grid supports "los"
 */
#define cave_los_bold(F,Y,X) \
	(feat_supports_los((F)->grid_array[(Y)][(X)].feat))

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
 * Determine if a "legal" grid is an "empty" floor grid
 * Determine if monsters are allowed to move into a grid
 *
 * Line 1 -- forbid non-placement grids
 * Line 2 -- forbid normal monsters
 * Line 3 -- forbid the player
 */
#define cave_empty_bold(F,Y,X) \
	(cave_have_flag_bold((F), (Y), (X), FF_PLACE) && \
	 !((F)->grid_array[Y][X].m_idx) && \
	 !player_bold(p_ptr, Y,X))


/*
 * Determine if a "legal" grid is an "empty" floor grid
 * Determine if monster generation is allowed in a grid
 *
 * Line 1 -- forbid non-empty grids
 * Line 2 -- forbid trees while dungeon generation
 */
#define cave_empty_bold2(F,Y,X) \
	(cave_empty_bold(F,Y,X) && \
	 (current_world_ptr->character_dungeon || !cave_have_flag_bold((F), (Y), (X), FF_TREE)))


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
 * Grid based version of "cave_empty_bold()"
 */
#define cave_empty_grid(C) \
	(cave_have_flag_grid((C), FF_PLACE) && \
	 !((C)->m_idx) && !player_grid(p_ptr, C))


/*
 * Grid based version of "cave_perma_bold()"
 */
#define cave_perma_grid(C) \
	(cave_have_flag_grid((C), FF_PERMANENT))


#define pattern_tile(Y,X) \
	(cave_have_flag_bold(p_ptr->current_floor_ptr, (Y), (X), FF_PATTERN))

/*
 * Does the grid stop disintegration?
 */
#define cave_stop_disintegration(Y,X) \
	(!cave_have_flag_bold(p_ptr->current_floor_ptr, (Y), (X), FF_PROJECT) && \
	 (!cave_have_flag_bold(p_ptr->current_floor_ptr, (Y), (X), FF_HURT_DISI) || \
	  cave_have_flag_bold(p_ptr->current_floor_ptr, (Y), (X), FF_PERMANENT)))


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

extern void update_smell(floor_type *floor_ptr, player_type *subject_ptr);

extern void add_door(floor_type *floor_ptr, POSITION x, POSITION y);
extern void place_secret_door(floor_type *floor_ptr, POSITION y, POSITION x, int type);
extern void place_locked_door(floor_type *floor_ptr, POSITION y, POSITION x);
extern void forget_flow(floor_type *floor_ptr);
extern void place_random_stairs(floor_type *floor_ptr, POSITION y, POSITION x);

extern bool los(floor_type* floor_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
extern bool projectable(floor_type *floor_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
extern int project_length;

extern void vault_monsters(floor_type *floor_ptr, POSITION y1, POSITION x1, int num);
extern bool cave_valid_bold(floor_type *floor_ptr, POSITION y, POSITION x);
extern void cave_set_feat(floor_type *floor_ptr, POSITION y, POSITION x, FEAT_IDX feat);
extern void place_random_door(floor_type *floor_ptr, POSITION y, POSITION x, bool room);
extern void place_closed_door(floor_type *floor_ptr, POSITION y, POSITION x, int type);

extern void wipe_o_list(floor_type *floor_ptr);
extern void vault_trap_aux(floor_type *floor_ptr, POSITION y, POSITION x, POSITION yd, POSITION xd);

extern bool get_is_floor(floor_type *floor_ptr, POSITION x, POSITION y);
extern void try_door(floor_type *floor_ptr, POSITION y, POSITION x);

extern FEAT_IDX conv_dungeon_feat(floor_type *floor_ptr, FEAT_IDX newfeat);
extern void vault_objects(floor_type *floor_ptr, POSITION y, POSITION x, int num);

/*
 * project()関数に用いられる、遠隔攻撃特性ビットフラグ / Bit flags for the "project()" function
 */
#define PROJECT_JUMP        0x0001 /*!< 遠隔攻撃特性: 発動者からの軌跡を持たず、指定地点に直接発生する(予め置いたトラップ、上空からの発生などのイメージ) / Jump directly to the target location (this is a hack) */
#define PROJECT_BEAM        0x0002 /*!< 遠隔攻撃特性: ビーム範囲を持つ。 / Work as a beam weapon (affect every grid passed through) */
#define PROJECT_THRU        0x0004 /*!< 遠隔攻撃特性: 目標地点に到達しても射程と遮蔽の限り引き延ばす。 / Continue "through" the target (used for "bolts"/"beams") */
#define PROJECT_STOP        0x0008 /*!< 遠隔攻撃特性: 道中にプレイヤーかモンスターがいた時点で到達地点を更新して停止する(壁や森はPROJECT_DISIがない限り最初から貫通しない) */
#define PROJECT_GRID        0x0010 /*!< 遠隔攻撃特性: 射程内の地形に影響を及ぼす / Affect each grid in the "blast area" in some way */
#define PROJECT_ITEM        0x0020 /*!< 遠隔攻撃特性: 射程内のアイテムに影響を及ぼす / Affect each object in the "blast area" in some way */
#define PROJECT_KILL        0x0040 /*!< 遠隔攻撃特性: 射程内のモンスターに影響を及ぼす / Affect each monster in the "blast area" in some way */
#define PROJECT_HIDE        0x0080 /*!< 遠隔攻撃特性: / Hack -- disable "visual" feedback from projection */
#define PROJECT_DISI        0x0100 /*!< 遠隔攻撃特性: / Disintegrate non-permanent features */
#define PROJECT_PLAYER      0x0200 /*!< 遠隔攻撃特性: / Main target is player (used for riding player) */
#define PROJECT_AIMED       0x0400 /*!< 遠隔攻撃特性: / Target is only player or monster, so don't affect another. Depend on PROJECT_PLAYER. (used for minimum (rad == 0) balls on riding player) */
#define PROJECT_REFLECTABLE 0x0800 /*!< 遠隔攻撃特性: 反射可能(ボルト系魔法に利用) / Refrectable spell attacks (used for "bolts") */
#define PROJECT_NO_HANGEKI  0x1000 /*!< 遠隔攻撃特性: / Avoid counter attacks of monsters */
#define PROJECT_PATH        0x2000 /*!< 遠隔攻撃特性: / Only used for printing project path */
#define PROJECT_FAST        0x4000 /*!< 遠隔攻撃特性: / Hide "visual" of flying bolts until blast */
#define PROJECT_LOS         0x8000 /*!< 遠隔攻撃特性: /  */
extern sint project_path(floor_type *floor_ptr, u16b *gp, POSITION range, POSITION y1, POSITION x1, POSITION y2, POSITION x2, BIT_FLAGS flg);

extern void set_floor(floor_type *floor_ptr, POSITION x, POSITION y);
extern void place_object(floor_type *floor_ptr, POSITION y, POSITION x, BIT_FLAGS mode);

