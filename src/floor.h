#pragma once

#include "feature.h"
#include "grid.h"

typedef struct {
	grid_type *grid_array[MAX_HGT];
	DEPTH dun_level;		/*!< 現在の実ダンジョン階層base_levelの参照元となる / Current dungeon level */
	DEPTH base_level;		/*!< 基本生成レベル、後述のobject_level, monster_levelの参照元となる / Base dungeon level */
	DEPTH object_level;		/*!< アイテムの生成レベル、current_floor_ptr->base_levelを起点に一時変更する時に参照 / Current object creation level */
	DEPTH monster_level;	/*!< モンスターの生成レベル、current_floor_ptr->base_levelを起点に一時変更する時に参照 / Current monster creation level */
	POSITION width;			/* Current dungeon width */
	POSITION height;		/* Current dungeon height */
	MONSTER_NUMBER num_repro; /*!< Current reproducer count */

	GAME_TURN generated_turn; /* Turn when level began */

	object_type *o_list; /*!< The array of dungeon items [current_floor_ptr->max_o_idx] */
	OBJECT_IDX max_o_idx; /*!< Maximum number of objects in the level */
	OBJECT_IDX o_max; /* Number of allocated objects */
	OBJECT_IDX o_cnt; /* Number of live objects */

	monster_type *m_list; /*!< The array of dungeon monsters [current_floor_ptr->max_m_idx] */
	MONSTER_IDX max_m_idx; /*!< Maximum number of monsters in the level */
	MONSTER_IDX m_max; /* Number of allocated monsters */
	MONSTER_IDX m_cnt; /* Number of live monsters */

	s16b *mproc_list[MAX_MTIMED]; /*!< The array to process dungeon monsters[max_m_idx] */
	s16b mproc_max[MAX_MTIMED]; /*!< Number of monsters to be processed */

	POSITION_IDX lite_n; //!< Array of grids lit by player lite (see "current_floor_ptr->grid_array.c")
	POSITION lite_y[LITE_MAX];
	POSITION lite_x[LITE_MAX];

	POSITION_IDX mon_lite_n; //!< Array of grids lit by player lite (see "current_floor_ptr->grid_array.c")
	POSITION mon_lite_y[MON_LITE_MAX];
	POSITION mon_lite_x[MON_LITE_MAX];

	POSITION_IDX view_n; //!< Array of grids viewable to the player (see "grid_array")
	POSITION view_y[VIEW_MAX];
	POSITION view_x[VIEW_MAX];

	POSITION_IDX redraw_n; //!< Array of grids for delayed visual updating (see "current_floor_ptr->grid_array.c")
	POSITION redraw_y[REDRAW_MAX];
	POSITION redraw_x[REDRAW_MAX];

	bool monster_noise;

} floor_type;

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

#define DUNGEON_ANGBAND  1
#define DUNGEON_GALGALS  2
#define DUNGEON_ORC      3
#define DUNGEON_MAZE     4
#define DUNGEON_DRAGON   5
#define DUNGEON_GRAVE    6
#define DUNGEON_WOOD     7
#define DUNGEON_VOLCANO  8
#define DUNGEON_HELL     9
#define DUNGEON_HEAVEN   10
#define DUNGEON_OCEAN    11
#define DUNGEON_CASTLE   12
#define DUNGEON_CTH      13
#define DUNGEON_MOUNTAIN 14
#define DUNGEON_GOLD     15
#define DUNGEON_NO_MAGIC 16
#define DUNGEON_NO_MELEE 17
#define DUNGEON_CHAMELEON 18
#define DUNGEON_DARKNESS 19

/*
 * Flags for change floor mode
 */
#define CFM_UP        	 0x0001  /* Move up */
#define CFM_DOWN      	 0x0002  /* Move down */
#define CFM_LONG_STAIRS  0x0004  /* Randomly occurred long stairs/shaft */
#define CFM_XXX  	 0x0008  /* XXX */
#define CFM_SHAFT     	 0x0010  /* Shaft */
#define CFM_RAND_PLACE   0x0020  /* Arrive at random grid */
#define CFM_RAND_CONNECT 0x0040  /* Connect with random stairs */
#define CFM_SAVE_FLOORS  0x0080  /* Save floors */
#define CFM_NO_RETURN    0x0100  /* Flee from random quest etc... */
#define CFM_FIRST_FLOOR  0x0200  /* Create exit from the dungeon */

/*
 * Determines if a map location is fully inside the outer walls
 */
#define in_bounds(Y,X) \
   (((Y) > 0) && ((X) > 0) && ((Y) < current_floor_ptr->height-1) && ((X) < current_floor_ptr->width-1))

/*
 * Determines if a map location is on or inside the outer walls
 */
#define in_bounds2(Y,X) \
   (((Y) >= 0) && ((X) >= 0) && ((Y) < current_floor_ptr->height) && ((X) < current_floor_ptr->width))

/*
 * Determines if a map location is on or inside the outer walls
 * (unsigned version)
 */
#define in_bounds2u(Y,X) \
   (((Y) < current_floor_ptr->height) && ((X) < current_floor_ptr->width))


/*
 * Determine if player is on this grid
 */
#define player_bold(Y,X) \
	(((Y) == p_ptr->y) && ((X) == p_ptr->x))

/*
 * Grid based version of "player_bold()"
 */
#define player_grid(C) \
	((C) == &current_floor_ptr->grid_array[p_ptr->y][p_ptr->x])


#define cave_have_flag_bold(Y,X,INDEX) \
	(have_flag(f_info[current_floor_ptr->grid_array[(Y)][(X)].feat].flags, (INDEX)))


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
#define cave_los_bold(Y,X) \
	(feat_supports_los(current_floor_ptr->grid_array[(Y)][(X)].feat))

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
#define cave_clean_bold(Y,X) \
	(cave_have_flag_bold((Y), (X), FF_FLOOR) && \
	 !(current_floor_ptr->grid_array[Y][X].info & CAVE_OBJECT) && \
	  (current_floor_ptr->grid_array[Y][X].o_idx == 0))


/*
 * Determine if an object can be dropped on a "legal" grid
 *
 * Line 1 -- forbid non-drops
 * Line 2 -- forbid object terrains
 */
#define cave_drop_bold(Y,X) \
	(cave_have_flag_bold((Y), (X), FF_DROP) && \
	 !(current_floor_ptr->grid_array[Y][X].info & CAVE_OBJECT))


/*
 * Determine if a "legal" grid is an "empty" floor grid
 * Determine if monsters are allowed to move into a grid
 *
 * Line 1 -- forbid non-placement grids
 * Line 2 -- forbid normal monsters
 * Line 3 -- forbid the player
 */
#define cave_empty_bold(Y,X) \
	(cave_have_flag_bold((Y), (X), FF_PLACE) && \
	 !(current_floor_ptr->grid_array[Y][X].m_idx) && \
	 !player_bold(Y,X))


/*
 * Determine if a "legal" grid is an "empty" floor grid
 * Determine if monster generation is allowed in a grid
 *
 * Line 1 -- forbid non-empty grids
 * Line 2 -- forbid trees while dungeon generation
 */
#define cave_empty_bold2(Y,X) \
	(cave_empty_bold(Y,X) && \
	 (character_dungeon || !cave_have_flag_bold((Y), (X), FF_TREE)))


/*
 * Determine if a "legal" grid is an "naked" floor grid
 *
 * Line 1 -- forbid non-clean gird
 * Line 2 -- forbid monsters
 * Line 3 -- forbid the player
 */
#define cave_naked_bold(Y,X) \
	(cave_clean_bold(Y,X) && \
	 !(current_floor_ptr->grid_array[Y][X].m_idx) && \
	 !player_bold(Y,X))


/*
 * Determine if a "legal" grid is "permanent"
 *
 * Line 1 -- permanent flag
 */
#define cave_perma_bold(Y,X) \
	(cave_have_flag_bold((Y), (X), FF_PERMANENT))


/*
 * Grid based version of "cave_empty_bold()"
 */
#define cave_empty_grid(C) \
	(cave_have_flag_grid((C), FF_PLACE) && \
	 !((C)->m_idx) && \
	 !player_grid(C))


/*
 * Grid based version of "cave_perma_bold()"
 */
#define cave_perma_grid(C) \
	(cave_have_flag_grid((C), FF_PERMANENT))


#define pattern_tile(Y,X) \
	(cave_have_flag_bold((Y), (X), FF_PATTERN))

/*
 * Does the grid stop disintegration?
 */
#define cave_stop_disintegration(Y,X) \
	(!cave_have_flag_bold((Y), (X), FF_PROJECT) && \
	 (!cave_have_flag_bold((Y), (X), FF_HURT_DISI) || \
	  cave_have_flag_bold((Y), (X), FF_PERMANENT)))


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
#define player_has_los_bold(Y,X) \
    (((current_floor_ptr->grid_array[Y][X].info & (CAVE_VIEW)) != 0) || p_ptr->inside_battle)


/*
 * Determine if a "feature" is "permanent wall"
 */
#define permanent_wall(F) \
	(have_flag((F)->flags, FF_WALL) && \
	 have_flag((F)->flags, FF_PERMANENT))

extern floor_type *current_floor_ptr;
extern saved_floor_type saved_floors[MAX_SAVED_FLOORS];