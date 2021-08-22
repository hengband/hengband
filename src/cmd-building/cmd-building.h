#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

// clang-format off
#define BUILDING_NON_MEMBER 0 /*!< 不明(現在未使用) */
#define BUILDING_MEMBER     1 /*!< 不明(現在未使用) */
#define BUILDING_OWNER      2 /*!< 施設の種族/職業条件が一致している状態値 */
// clang-format on

/*
 * Arena constants
 */
#define ARENA_DEFEATED_OLD_VER (-(MAX_SHORT)) /*<! 旧バージョンの闘技場敗北定義 */

extern bool reinit_wilderness;

extern uint32_t mon_odds[4];
extern int battle_odds;
extern PRICE kakekin;
extern int sel_monster;

typedef struct player_type player_type;
void do_cmd_building(player_type *player_ptr);
