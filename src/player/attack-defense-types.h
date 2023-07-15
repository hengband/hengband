#pragma once

enum special_attack_type {
    ATTACK_CONFUSE = 0x00000001, /*!< プレイヤーのステータス:混乱打撃 */
    ATTACK_XXX1 = 0x00000002, /*!< プレイヤーのステータス:未使用1 */
    ATTACK_XXX2 = 0x00000004, /*!< プレイヤーのステータス:未使用2 */
    ATTACK_XXX3 = 0x00000008, /*!< プレイヤーのステータス:未使用3 */
    ATTACK_ACID = 0x00000010, /*!< プレイヤーのステータス:魔法剣/溶解 */
    ATTACK_ELEC = 0x00000020, /*!< プレイヤーのステータス:魔法剣/電撃 */
    ATTACK_FIRE = 0x00000040, /*!< プレイヤーのステータス:魔法剣/火炎 */
    ATTACK_COLD = 0x00000080, /*!< プレイヤーのステータス:魔法剣/冷凍 */
    ATTACK_POIS = 0x00000100, /*!< プレイヤーのステータス:魔法剣/毒殺 */
    ATTACK_HOLY = 0x00000200, /*!< プレイヤーのステータス:対邪?(未使用) */
    ATTACK_SUIKEN = 0x00000400, /*!< プレイヤーのステータス:酔拳 */
};

enum special_defense_type {
    ACTION_NONE = 0, /*!< 持続行動: なし */
    ACTION_SEARCH = 1, /*!< 持続行動: 探索 */
    ACTION_REST = 2, /*!< 持続行動: 休憩 */
    ACTION_LEARN = 3, /*!< 持続行動: 青魔法ラーニング */
    ACTION_FISH = 4, /*!< 持続行動: 釣り */
    ACTION_MONK_STANCE = 5, /*!< 持続行動: 修行僧の構え */
    ACTION_SAMURAI_STANCE = 6, /*!< 持続行動: 剣術家の型 */
    ACTION_SING = 7, /*!< 持続行動: 歌 */
    ACTION_HAYAGAKE = 8, /*!< 持続行動: 早駆け */
    ACTION_SPELL = 9, /*!< 持続行動: 呪術 */
};
