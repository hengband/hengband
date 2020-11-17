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

extern u32b mon_odds[4];
extern int battle_odds;
extern PRICE kakekin;
extern int sel_monster;

void do_cmd_building(player_type *player_ptr);
