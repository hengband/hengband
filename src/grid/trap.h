#pragma once

#include "system/angband.h"
#include "util/flag-group.h"

// clang-format off
/*!
 * 箱のトラップ定義 Chest trap flags
 */
enum class ChestTrapType : ushort {
    LOSE_STR = 0,       /*!< 箱のトラップ: 腕力減少の毒針 */
    LOSE_CON = 1,       /*!< 箱のトラップ: 器用さ減少の毒針 */
    POISON = 2,         /*!< 箱のトラップ: 毒針 */
    PARALYZE = 3,       /*!< 箱のトラップ: 麻痺ガス */
    EXPLODE = 4,        /*!< 箱のトラップ: 爆発 */
    SUMMON = 5,         /*!< 箱のトラップ: 召喚のルーン(モンスター) */
    SCATTER = 6,        /*!< 箱のトラップ: アイテム散乱 */
    E_SUMMON = 7,       /*!< 箱のトラップ: 召喚のルーン(エレメンタル) */
    BIRD_STORM = 8,     /*!< 箱のトラップ: 召喚のルーン(鳥) */
    H_SUMMON = 9,      /*!< 箱のトラップ: 召喚のルーン(強敵) */
    RUNES_OF_EVIL = 10, /*!< 箱のトラップ: 邪悪なルーン */
    ALARM = 11,         /*!< 箱のトラップ: 警報装置 */
    MAX,
};
// clang-format on

/* Types of normal traps */
enum trap_type {
    NOT_TRAP = -1,
    TRAP_TRAPDOOR = 0,
    TRAP_PIT = 1,
    TRAP_SPIKED_PIT = 2,
    TRAP_POISON_PIT = 3,
    TRAP_TY_CURSE = 4,
    TRAP_TELEPORT = 5,
    TRAP_FIRE = 6,
    TRAP_ACID = 7,
    TRAP_SLOW = 8,

    TRAP_LOSE_STR = 9,
    TRAP_LOSE_DEX = 10,
    TRAP_LOSE_CON = 11,
    TRAP_BLIND = 12,
    TRAP_CONFUSE = 13,
    TRAP_POISON = 14,
    TRAP_SLEEP = 15,
    TRAP_TRAPS = 16,
    TRAP_ALARM = 17,

    /* Types of special traps */
    TRAP_OPEN = 18,
    TRAP_ARMAGEDDON = 19,
    TRAP_PIRANHA = 20,

};

extern const std::vector<EnumClassFlagGroup<ChestTrapType>> chest_traps;

class PlayerType;
void init_normal_traps(void);
FEAT_IDX choose_random_trap(PlayerType *player_ptr);
void disclose_grid(PlayerType *player_ptr, POSITION y, POSITION x);
void place_trap(PlayerType *player_ptr, POSITION y, POSITION x);
void hit_trap(PlayerType *player_ptr, bool break_trap);
