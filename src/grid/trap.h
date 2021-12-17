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
enum class TrapType {
    NOT_TRAP = -1,
    TRAPDOOR = 0,
    PIT = 1,
    SPIKED_PIT = 2,
    POISON_PIT = 3,
    TY_CURSE = 4,
    TELEPORT = 5,
    FIRE = 6,
    ACID = 7,
    SLOW = 8,

    LOSE_STR = 9,
    LOSE_DEX = 10,
    LOSE_CON = 11,
    BLIND = 12,
    CONFUSE = 13,
    POISON = 14,
    SLEEP = 15,
    TRAPS = 16,
    ALARM = 17,

    /* Types of special traps */
    OPEN = 18,
    ARMAGEDDON = 19,
    PIRANHA = 20,
};

extern const std::vector<EnumClassFlagGroup<ChestTrapType>> chest_traps;

class PlayerType;
void init_normal_traps(void);
FEAT_IDX choose_random_trap(PlayerType *player_ptr);
void disclose_grid(PlayerType *player_ptr, POSITION y, POSITION x);
void place_trap(PlayerType *player_ptr, POSITION y, POSITION x);
void hit_trap(PlayerType *player_ptr, bool break_trap);
