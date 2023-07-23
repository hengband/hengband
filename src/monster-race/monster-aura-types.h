#pragma once

/*!
 * @brief モンスターがまとっているオーラ定義.
 * @details 火炎、冷気、電撃以外は全て予約領域.
 */
enum class MonsterAuraType {
    FIRE = 0, // 火炎.
    COLD = 1, // 冷気.
    ELEC = 2, // 電撃.
    ACID = 3, // 酸.
    POISON = 4, // 毒.
    NUKE = 5, // 放射性廃棄物.
    PLASMA = 6, // プラズマ.
    WATER = 7, // 水.
    ICEE = 8, // 極寒.
    LITE = 9, // 光.
    DARK = 10, // 闇.
    SHARDS = 11, // 破片.
    FORCE = 12, // フォース.
    MANA = 13, // 魔力.
    METEOR = 14, // 隕石.
    CHAOS = 15, // カオス.
    HOLINESS = 16, // 神聖.
    NETHER = 17, // 地獄.
    DISENCHANT = 18, // 劣化.
    NEXUS = 19, // 因果混乱.
    TIME = 20, // 時間.
    GRAVITY = 21, // 重力.
    VOIDS = 22, // 虚無.
    ABYSS = 23, // 深淵.
    MAX,
};
