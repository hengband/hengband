#pragma once

#include "system/angband.h"
#include "object/tval-types.h"

#define BUILDING_NON_MEMBER 0 /*!< 不明(現在未使用) */
#define BUILDING_MEMBER     1 /*!< 不明(現在未使用) */
#define BUILDING_OWNER      2 /*!< 施設の種族/職業条件が一致している状態値 */

/*
 * Arena constants
 */
#define ARENA_DEFEATED_OLD_VER (-(MAX_SHORT)) /*<! 旧バージョンの闘技場敗北定義 */

extern bool reinit_wilderness;
extern MONRACE_IDX today_mon;

extern MONRACE_IDX battle_mon[4];
extern u32b mon_odds[4];
extern int battle_odds;
extern PRICE kakekin;
extern int sel_monster;

/*!
 * @struct arena_type
 * @brief 闘技場のモンスターエントリー構造体 / A structure type for arena entry
 */
typedef struct arena_type {
	MONRACE_IDX r_idx; /*!< 闘技場のモンスター種族ID(0ならば表彰式) / Monster (0 means victory prizing) */
	tval_type tval;  /*!< モンスター打倒後に得られるアイテムの大カテゴリID / tval of prize (0 means no prize) */
	OBJECT_SUBTYPE_VALUE sval;  /*!< モンスター打倒後に得られるアイテムの小カテゴリID / sval of prize */
} arena_type;

void do_cmd_building(player_type *player_ptr);
