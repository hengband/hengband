#include "combat/attack-power-table.h"

/*!
 * @brief 修行僧のターンダメージ算出テーブル
 */
const int monk_ave_damage[PY_MAX_LEVEL + 1][3] = {
    { 0, 0, 0 },
    { 249, 249, 249 },
    { 324, 324, 324 },
    { 382, 438, 382 },
    { 382, 439, 382 },
    { 390, 446, 390 },
    { 394, 473, 394 },
    { 425, 528, 425 },
    { 430, 535, 430 },
    { 505, 560, 435 },
    { 517, 575, 444 },
    { 566, 655, 474 },
    { 585, 713, 486 },
    { 653, 843, 527 },
    { 678, 890, 544 },
    { 703, 973, 558 },
    { 765, 1096, 596 },
    { 914, 1146, 614 },
    { 943, 1240, 629 },
    { 971, 1276, 643 },
    { 1018, 1350, 667 },
    { 1063, 1464, 688 },
    { 1099, 1515, 705 },
    { 1128, 1559, 721 },
    { 1153, 1640, 735 },
    { 1336, 1720, 757 },
    { 1387, 1789, 778 },
    { 1430, 1893, 794 },
    { 1610, 2199, 863 },
    { 1666, 2280, 885 },
    { 1713, 2401, 908 },
    { 1755, 2465, 925 },
    { 1909, 2730, 984 },
    { 2156, 2891, 1009 },
    { 2218, 2970, 1031 },
    { 2319, 3107, 1063 },
    { 2404, 3290, 1098 },
    { 2477, 3389, 1125 },
    { 2544, 3483, 1150 },
    { 2771, 3899, 1228 },
    { 2844, 3982, 1259 },
    { 3129, 4064, 1287 },
    { 3200, 4190, 1313 },
    { 3554, 4674, 1432 },
    { 3614, 4738, 1463 },
    { 3679, 4853, 1485 },
    { 3741, 4905, 1512 },
    { 3785, 4943, 1538 },
    { 4141, 5532, 1652 },
    { 4442, 5581, 1679 },
    { 4486, 5636, 1702 },
};

/*!
 * 腕力による攻撃回数算定値テーブル
 * Stat Table (STR) -- help index into the "blow" table
 */
const byte adj_str_blow[MAX_ADJ_STR] = {
    3 /* 3 */,
    4 /* 4 */,
    5 /* 5 */,
    6 /* 6 */,
    7 /* 7 */,
    8 /* 8 */,
    9 /* 9 */,
    10 /* 10 */,
    11 /* 11 */,
    12 /* 12 */,
    13 /* 13 */,
    14 /* 14 */,
    15 /* 15 */,
    16 /* 16 */,
    17 /* 17 */,
    20 /* 18/00-18/09 */,
    30 /* 18/10-18/19 */,
    40 /* 18/20-18/29 */,
    50 /* 18/30-18/39 */,
    60 /* 18/40-18/49 */,
    70 /* 18/50-18/59 */,
    80 /* 18/60-18/69 */,
    90 /* 18/70-18/79 */,
    100 /* 18/80-18/89 */,
    110 /* 18/90-18/99 */,
    120 /* 18/100-18/109 */,
    130 /* 18/110-18/119 */,
    140 /* 18/120-18/129 */,
    150 /* 18/130-18/139 */,
    160 /* 18/140-18/149 */,
    170 /* 18/150-18/159 */,
    180 /* 18/160-18/169 */,
    190 /* 18/170-18/179 */,
    200 /* 18/180-18/189 */,
    210 /* 18/190-18/199 */,
    220 /* 18/200-18/209 */,
    230 /* 18/210-18/219 */,
    240 /* 18/220+ */
};

/*!
 * 器用さによる攻撃回数インデックステーブル
 * Stat Table (DEX) -- index into the "blow" table
 */
const byte adj_dex_blow[MAX_ADJ_DEX] = {
    0 /* 3 */,
    0 /* 4 */,
    0 /* 5 */,
    0 /* 6 */,
    0 /* 7 */,
    0 /* 8 */,
    0 /* 9 */,
    1 /* 10 */,
    1 /* 11 */,
    1 /* 12 */,
    1 /* 13 */,
    1 /* 14 */,
    2 /* 15 */,
    2 /* 16 */,
    2 /* 17 */,
    2 /* 18/00-18/09 */,
    3 /* 18/10-18/19 */,
    3 /* 18/20-18/29 */,
    3 /* 18/30-18/39 */,
    4 /* 18/40-18/49 */,
    4 /* 18/50-18/59 */,
    5 /* 18/60-18/69 */,
    5 /* 18/70-18/79 */,
    6 /* 18/80-18/89 */,
    6 /* 18/90-18/99 */,
    7 /* 18/100-18/109 */,
    7 /* 18/110-18/119 */,
    8 /* 18/120-18/129 */,
    8 /* 18/130-18/139 */,
    9 /* 18/140-18/149 */,
    9 /* 18/150-18/159 */,
    10 /* 18/160-18/169 */,
    10 /* 18/170-18/179 */,
    11 /* 18/180-18/189 */,
    11 /* 18/190-18/199 */,
    12 /* 18/200-18/209 */,
    12 /* 18/210-18/219 */,
    13 /* 18/220+ */
};

/*!
 * @brief
 * 腕力、器用さに応じた攻撃回数テーブル /
 * This table is used to help calculate the number of blows the player can
 * make in a single round of attacks (one player turn) with a normal weapon.
 * @details
 * <pre>
 * This number ranges from a single blow/round for weak players to up to six
 * blows/round for powerful warriors.
 *
 * Note that certain artifacts and ego-items give "bonus" blows/round.
 *
 * First, from the player class, we extract some values:
 *
 * Warrior       num = 6; mul = 5; div = std::max(70, weapon_weight);
 * Berserker     num = 6; mul = 7; div = std::max(70, weapon_weight);
 * Mage          num = 3; mul = 2; div = std::max(100, weapon_weight);
 * Priest        num = 5; mul = 3; div = std::max(100, weapon_weight);
 * Mindcrafter   num = 5; mul = 3; div = std::max(100, weapon_weight);
 * Rogue         num = 5; mul = 3; div = std::max(40, weapon_weight);
 * Ranger        num = 5; mul = 4; div = std::max(70, weapon_weight);
 * Paladin       num = 5; mul = 4; div = std::max(70, weapon_weight);
 * Weaponsmith   num = 5; mul = 5; div = std::max(150, weapon_weight);
 * Warrior-Mage  num = 5; mul = 3; div = std::max(70, weapon_weight);
 * Chaos Warrior num = 5; mul = 4; div = std::max(70, weapon_weight);
 * Monk          num = 5; mul = 3; div = std::max(60, weapon_weight);
 * Tourist       num = 4; mul = 3; div = std::max(100, weapon_weight);
 * Imitator      num = 5; mul = 4; div = std::max(70, weapon_weight);
 * Beastmaster   num = 5; mul = 3; div = std::max(70, weapon_weight);
 * Cavalry(Ride) num = 5; mul = 4; div = std::max(70, weapon_weight);
 * Cavalry(Walk) num = 5; mul = 3; div = std::max(100, weapon_weight);
 * Sorcerer      num = 1; mul = 1; div = std::max(1, weapon_weight);
 * Archer        num = 4; mul = 2; div = std::max(70, weapon_weight);
 * Magic eater   num = 4; mul = 2; div = std::max(70, weapon_weight);
 * ForceTrainer  num = 4; mul = 2; div = std::max(60, weapon_weight);
 * Mirror Master num = 3; mul = 3; div = std::max(100, weapon_weight);
 * Ninja         num = 4; mul = 1; div = std::max(20, weapon_weight);
 *
 * To get "P", we look up the relevant "adj_str_blow[]" (see above),
 * multiply it by "mul", and then divide it by "div".
 * Increase P by 1 if you wield a weapon two-handed.
 * Decrease P by 1 if you are a Ninja.
 *
 * To get "D", we look up the relevant "adj_dex_blow[]" (see above),
 *
 * The player gets "blows_table[P][D]" blows/round, as shown below,
 * up to a maximum of "num" blows/round, plus any "bonus" blows/round.
 * </pre>
 */
const byte blows_table[12][12] = {
    /* P/D */
    /*      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11+ */
    /*      3   10   15  /10  /40  /60  /80 /100 /120 /140 /160 /180  */
    /* 0 */ { 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4 },
    /* 1 */ { 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4 },
    /* 2 */ { 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 5 },
    /* 3 */ { 1, 1, 2, 3, 3, 4, 4, 4, 5, 5, 5, 5 },
    /* 4 */ { 1, 1, 2, 3, 3, 4, 4, 5, 5, 5, 5, 5 },
    /* 5 */ { 1, 1, 2, 3, 4, 4, 4, 5, 5, 5, 5, 6 },
    /* 6 */ { 1, 1, 2, 3, 4, 4, 4, 5, 5, 5, 5, 6 },
    /* 7 */ { 1, 2, 2, 3, 4, 4, 4, 5, 5, 5, 5, 6 },
    /* 8 */ { 1, 2, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6 },
    /* 9 */ { 1, 2, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6 },
    /* 10*/ { 2, 2, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6 },
    /*11+*/ { 2, 2, 3, 4, 4, 4, 5, 5, 6, 6, 6, 6 },
};
