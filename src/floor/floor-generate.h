#pragma once

#include "system/angband.h"

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

bool place_quest_monsters(player_type *creature_ptr);
void wipe_generate_random_floor_flags(floor_type *floor_ptr);
void clear_cave(player_type *player_ptr);
void generate_floor(player_type *player_ptr);
bool build_tunnel(player_type *player_ptr, POSITION row1, POSITION col1, POSITION row2, POSITION col2);
bool build_tunnel2(player_type *player_ptr, POSITION x1, POSITION y1, POSITION x2, POSITION y2, int type, int cutoff);
