/*!
 * @file rooms.h
 * @brief 部屋生成処理の定義ヘッダー / Header file for rooms.c, used only in generate.c
 * @date 2014/09/07
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke<br>
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.<br
 */

#pragma once
#include "floor.h"

#define ALLOW_CAVERNS_AND_LAKES

#define DUN_ROOMS_MAX	40 /*!< 部屋生成処理の最大試行数 / Number of rooms to attempt (was 50) */


/* 池型地形の生成ID / Room types for generate_lake() */
#define LAKE_T_LAVA        1 /*!< 池型地形ID: 溶岩 */
#define LAKE_T_WATER       2 /*!< 池型地形ID: 池 */
#define LAKE_T_CAVE        3 /*!< 池型地形ID: 空洞 */
#define LAKE_T_EARTH_VAULT 4 /*!< 池型地形ID: 地属性VAULT */
#define LAKE_T_AIR_VAULT   5 /*!< 池型地形ID: 風属性VAULT */
#define LAKE_T_WATER_VAULT 6 /*!< 池型地形ID: 水属性VAULT */
#define LAKE_T_FIRE_VAULT  7 /*!< 池型地形ID: 火属性VAULT */


/* 部屋型ID / Room types for room_build() */
#define ROOM_T_NORMAL         0	 /*!<部屋型ID:基本長方形 / Simple (33x11) */
#define ROOM_T_OVERLAP        1	 /*!<部屋型ID:長方形二つ重ね / Overlapping (33x11) */
#define ROOM_T_CROSS          2	 /*!<部屋型ID:十字 / Crossed (33x11) */
#define ROOM_T_INNER_FEAT     3	 /*!<部屋型ID:二重壁 / Large (33x11) */
#define ROOM_T_NEST           4	 /*!<部屋型ID:モンスターNEST / Monster nest (33x11) */
#define ROOM_T_PIT            5	 /*!<部屋型ID:モンスターPIT / Monster pit (33x11) */
#define ROOM_T_LESSER_VAULT   6	 /*!<部屋型ID:小型VAULT / Lesser vault (33x22) */
#define ROOM_T_GREATER_VAULT  7	 /*!<部屋型ID:大型VAULT / Greater vault (66x44) */
#define ROOM_T_FRACAVE        8	 /*!<部屋型ID:フラクタル地形 / Fractal room (42x24) */
#define ROOM_T_RANDOM_VAULT   9	 /*!<部屋型ID:ランダムVAULT / Random vault (44x22) */
#define ROOM_T_OVAL          10	 /*!<部屋型ID:円形部屋 / Circular rooms (22x22) */
#define ROOM_T_CRYPT         11	 /*!<部屋型ID:聖堂 / Crypts (22x22) */
#define ROOM_T_TRAP_PIT      12	 /*!<部屋型ID:トラップつきモンスターPIT / Trapped monster pit */
#define ROOM_T_TRAP          13	 /*!<部屋型ID:トラップ部屋 / Piranha/Armageddon trap room */
#define ROOM_T_GLASS         14	 /*!<部屋型ID:ガラス部屋 / Glass room */
#define ROOM_T_ARCADE        15  /*!<部屋型ID:商店 / Arcade */
#define ROOM_T_FIXED         16  /*!<部屋型ID:固定部屋 / Fixed room */

#define ROOM_T_MAX 17 /*!<部屋型ID最大数 */


/*
 * Room type information
 */
typedef struct room_info_type room_info_type;

struct room_info_type
{
        /* Allocation information. */
        s16b prob[ROOM_T_MAX];

        /* Minimum level on which room can appear. */
        byte min_level;
};

extern void build_lake(player_type *player_ptr, int type);
extern void build_cavern(player_type *player_ptr);

/* Maximum locked/jammed doors */
#define MAX_LJ_DOORS 8

/*
 * A structure type for doors
 */
typedef struct
{
	FEAT_IDX open;
	FEAT_IDX broken;
	FEAT_IDX closed;
	FEAT_IDX locked[MAX_LJ_DOORS];
	FEAT_IDX num_locked;
	FEAT_IDX jammed[MAX_LJ_DOORS];
	FEAT_IDX num_jammed;
} door_type;

door_type feat_door[MAX_DOOR_TYPES];

extern bool generate_rooms(player_type *player_ptr);
extern void build_maze_vault(player_type *player_ptr, POSITION x0, POSITION y0, POSITION xsize, POSITION ysize, bool is_vault);
extern bool find_space(player_type *player_ptr, POSITION *y, POSITION *x, POSITION height, POSITION width);
extern void build_small_room(player_type *player_ptr, POSITION x0, POSITION y0);
extern void add_outer_wall(player_type *player_ptr, POSITION x, POSITION y, int light, POSITION x1, POSITION y1, POSITION x2, POSITION y2);
extern POSITION dist2(POSITION x1, POSITION y1, POSITION x2, POSITION y2, POSITION h1, POSITION h2, POSITION h3, POSITION h4);
extern void generate_room_floor(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, int light);
extern void generate_fill_perm_bold(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
extern void generate_hmap(floor_type *floor_ptr, POSITION y0, POSITION x0, POSITION xsiz, POSITION ysiz, int grd, int roug, int cutoff);
extern bool generate_fracave(player_type *player_ptr, POSITION y0, POSITION x0, POSITION xsize, POSITION ysize, int cutoff, bool light, bool room);
extern void fill_treasure(player_type *player_ptr, POSITION x1, POSITION x2, POSITION y1, POSITION y2, int difficulty);
extern bool generate_lake(player_type *player_ptr, POSITION y0, POSITION x0, POSITION xsize, POSITION ysize, int c1, int c2, int c3, int type);
extern void build_recursive_room(player_type *player_ptr, POSITION x1, POSITION y1, POSITION x2, POSITION y2, int power);
extern void build_room(player_type *player_ptr, POSITION x1, POSITION x2, POSITION y1, POSITION y2);
extern void r_visit(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, int node, DIRECTION dir, int *visited);
