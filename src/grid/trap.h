#pragma once

#include "system/angband.h"

/*!
 * 箱のトラップ定義 Chest trap flags
 */
#define CHEST_LOSE_STR          0x0001 /*!< 箱のトラップ: STR減少の毒針 */
#define CHEST_LOSE_CON          0x0002 /*!< 箱のトラップ: CON減少の毒針 */
#define CHEST_POISON            0x0004 /*!< 箱のトラップ: 毒針 */
#define CHEST_PARALYZE          0x0008 /*!< 箱のトラップ: 麻痺ガス */
#define CHEST_EXPLODE           0x0010 /*!< 箱のトラップ: 爆発 */
#define CHEST_SUMMON            0x0020 /*!< 箱のトラップ: 召喚のルーン(モンスター) */
#define CHEST_SCATTER           0x0040 /*!< 箱のトラップ: アイテム散乱 */
#define CHEST_E_SUMMON          0x0080 /*!< 箱のトラップ: 召喚のルーン(エレメンタル) */
#define CHEST_BIRD_STORM        0x0100 /*!< 箱のトラップ: 召喚のルーン(鳥) */
#define CHEST_H_SUMMON          0x0200 /*!< 箱のトラップ: 召喚のルーン(強敵)  */
#define CHEST_RUNES_OF_EVIL     0x0400 /*!< 箱のトラップ: 邪悪なルーン */
#define CHEST_ALARM             0x0800 /*!< 箱のトラップ: 警報装置 */

/* Types of normal traps */
#define NOT_TRAP        -1
#define TRAP_TRAPDOOR    0
#define TRAP_PIT         1
#define TRAP_SPIKED_PIT  2
#define TRAP_POISON_PIT  3
#define TRAP_TY_CURSE    4
#define TRAP_TELEPORT    5
#define TRAP_FIRE        6
#define TRAP_ACID        7
#define TRAP_SLOW        8

#define TRAP_LOSE_STR    9
#define TRAP_LOSE_DEX   10
#define TRAP_LOSE_CON   11
#define TRAP_BLIND      12
#define TRAP_CONFUSE    13
#define TRAP_POISON     14
#define TRAP_SLEEP      15
#define TRAP_TRAPS      16
#define TRAP_ALARM      17

#define MAX_NORMAL_TRAPS 18

/* Types of special traps */
#define TRAP_OPEN       18
#define TRAP_ARMAGEDDON 19
#define TRAP_PIRANHA    20

extern const int chest_traps[64];

typedef struct player_type player_type;
void init_normal_traps(void);
FEAT_IDX choose_random_trap(player_type *trapped_ptr);
void disclose_grid(player_type *trapped_ptr, POSITION y, POSITION x);
void place_trap(player_type *trapped_ptr, POSITION y, POSITION x);
void hit_trap(player_type *trapped_ptr, bool break_trap);
