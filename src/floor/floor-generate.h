#pragma once

#include "system/angband.h"
#include "floor/floor-base-definitions.h"
#include "floor/geometry.h"

/*!
 * @file generate.h
 * @brief ダンジョン生成処理のヘッダーファイル
 * @date 2014/08/08
 * @author
 * 不明(変愚蛮怒スタッフ？)
 */

#define SAFE_MAX_ATTEMPTS 5000 /*!< 生成処理基本試行回数 */

extern int dun_tun_rnd; /*!< ダンジョンの通路方向を掻き回す頻度(一回の試行ごとに%で判定している) */
extern int dun_tun_chg; /*!< ダンジョンの通路をクランクさせる頻度(一回の試行ごとに%で判定している) */
extern int dun_tun_con; /*!< ダンジョンの通路を継続して引き延ばす頻度(一回の試行ごとに%で判定している) */
extern int dun_tun_pen; /*!< ダンジョンの部屋入口にドアを設置する頻度(一回の試行ごとに%で判定している) */
extern int dun_tun_jct; /*!< ダンジョンの通路交差地点付近にドアを設置する頻度(一回の試行ごとに%で判定している) */

/*
 * Hack -- Dungeon allocation "places"
 */
#define ALLOC_SET_CORR		1	/* Hallway */
#define ALLOC_SET_ROOM		2	/* Room */
#define ALLOC_SET_BOTH		3	/* Anywhere */

/*
 * Hack -- Dungeon allocation "types"
 */
#define ALLOC_TYP_RUBBLE	1	/* Rubble */
#define ALLOC_TYP_TRAP		3	/* Trap */
#define ALLOC_TYP_GOLD		4	/* Gold */
#define ALLOC_TYP_OBJECT	5	/* Object */
#define ALLOC_TYP_INVIS		6	/* Invisible wall */



/*
 * The "size" of a "generation block" in grids
 */
#define BLOCK_HGT	11
#define BLOCK_WID	11

/*
 * Maximum numbers of rooms along each axis (currently 6x6)
 */
#define MAX_ROOMS_ROW	(MAX_HGT / BLOCK_HGT)
#define MAX_ROOMS_COL	(MAX_WID / BLOCK_WID)


/*
 * Bounds on some arrays used in the "dun_data" structure.
 * These bounds are checked, though usually this is a formality.
 */
#define CENT_MAX	100
#define DOOR_MAX	200
#define WALL_MAX	500
#define TUNN_MAX	900


/*
 * Structure to hold all "dungeon generation" data
 */
typedef struct dun_data {
	/* Array of centers of rooms */
	int cent_n;
	coord cent[CENT_MAX];

	/* Array of possible door locations */
	int door_n;
	coord door[DOOR_MAX];

	/* Array of wall piercing locations */
	int wall_n;
	coord wall[WALL_MAX];

	/* Array of tunnel grids */
	int tunn_n;
	coord tunn[TUNN_MAX];

	/* Number of blocks along each axis */
	int row_rooms;
	int col_rooms;

	/* Array of which blocks are used */
	bool room_map[MAX_ROOMS_ROW][MAX_ROOMS_COL];

	/* Various type of dungeon floors */
	bool destroyed;
	bool empty_level;
	bool cavern;
	int laketype;
} dun_data;

extern dun_data *dun;

extern bool place_quest_monsters(player_type *creature_ptr);
extern void wipe_generate_random_floor_flags(floor_type *floor_ptr);
extern void clear_cave(player_type *player_ptr);
extern void generate_floor(player_type *player_ptr);

extern bool build_tunnel(player_type *player_ptr, POSITION row1, POSITION col1, POSITION row2, POSITION col2);
extern bool build_tunnel2(player_type *player_ptr, POSITION x1, POSITION y1, POSITION x2, POSITION y2, int type, int cutoff);
