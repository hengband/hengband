﻿#include "player/player-status-table.h"

/*!
 * @brief 能力値テーブル / Abbreviations of healthy stats
 */
extern const concptr stat_names[A_MAX] = {
#ifdef JP
    "腕力 :", "知能 :", "賢さ :", "器用 :", "耐久 :", "魅力 :"
#else
    "STR : ", "INT : ", "WIS : ", "DEX : ", "CON : ", "CHR : "
#endif
};

/*!
 * @brief 能力値テーブル(能力低下時) / Abbreviations of damaged stats
 */
extern const concptr stat_names_reduced[A_MAX] = {
#ifdef JP
    "腕力x:", "知能x:", "賢さx:", "器用x:", "耐久x:", "魅力x:"
#else
    "Str : ", "Int : ", "Wis : ", "Dex : ", "Con : ", "Chr : "
#endif
};

/*!
 * @brief 基本必要経験値テーブル /
 * Base experience levels, may be adjusted up for race and/or class
 */
extern const int player_exp[PY_MAX_LEVEL] = { 10, 25, 45, 70, 100, 140, 200, 280, 380, /*10*/
    500, 650, 850, 1100, 1400, 1800, 2300, 2900, 3600, 4400, /*20*/
    5400, 6800, 8400, 10200, 12500, 17500, 25000, 35000L, 50000L, 75000L, /*30*/
    100000L, 150000L, 200000L, 275000L, 350000L, 450000L, 550000L, 700000L, 850000L, 1000000L, /*40*/
    1250000L, 1500000L, 1800000L, 2100000L, 2400000L, 2700000L, 3000000L, 3500000L, 4000000L, 4500000L, /*50*/
    5000000L };

/*!
 * @brief 基本必要強化値テーブル(アンドロイド専用)
 */
extern const int player_exp_a[PY_MAX_LEVEL] = { 20, 50, 100, 170, 280, 430, 650, 950, 1400, /*10*/
    1850, 2300, 2900, 3600, 4400, 5400, 6800, 8400, 10400, 12500, /*20*/
    17500, 25000, 35000, 50000L, 75000L, 100000L, 150000L, 200000L, 275000L, 350000L, /*30*/
    450000L, 550000L, 650000L, 800000L, 950000L, 1100000L, 1250000L, 1400000L, 1550000L, 1700000L, /*40*/
    1900000L, 2100000L, 2300000L, 2550000L, 2800000L, 3050000L, 3300000L, 3700000L, 4100000L, 4500000L, /*50*/
    5000000L };

/*!
 * 知力/賢さによるレベル毎の習得可能魔法数テーブル
 * Stat Table (INT/WIS) -- Number of half-spells per level
 */
extern const byte adj_mag_study[] = {
    0 /* 3 */, 0 /* 4 */, 0 /* 5 */, 0 /* 6 */, 0 /* 7 */, 1 /* 8 */, 1 /* 9 */, 1 /* 10 */, 1 /* 11 */, 2 /* 12 */, 2 /* 13 */, 2 /* 14 */, 2 /* 15 */,
    2 /* 16 */, 2 /* 17 */, 2 /* 18/00-18/09 */, 2 /* 18/10-18/19 */, 2 /* 18/20-18/29 */, 2 /* 18/30-18/39 */, 2 /* 18/40-18/49 */, 3 /* 18/50-18/59 */,
    3 /* 18/60-18/69 */, 3 /* 18/70-18/79 */, 3 /* 18/80-18/89 */, 4 /* 18/90-18/99 */, 4 /* 18/100-18/109 */, 4 /* 18/110-18/119 */, 5 /* 18/120-18/129 */,
    5 /* 18/130-18/139 */, 5 /* 18/140-18/149 */, 5 /* 18/150-18/159 */, 5 /* 18/160-18/169 */, 5 /* 18/170-18/179 */, 5 /* 18/180-18/189 */,
    5 /* 18/190-18/199 */, 5 /* 18/200-18/209 */, 6 /* 18/210-18/219 */, 6 /* 18/220+ */
};

/*!
 * 知力/賢さによるMP修正テーブル
 * Stat Table (INT/WIS) -- extra 1/4-mana-points per level
 */
extern const byte adj_mag_mana[] = {
    0 /* 3 */, 0 /* 4 */, 0 /* 5 */, 0 /* 6 */, 0 /* 7 */, 1 /* 8 */, 2 /* 9 */, 3 /* 10 */, 4 /* 11 */, 5 /* 12 */, 5 /* 13 */, 6 /* 14 */, 7 /* 15 */,
    8 /* 16 */, 9 /* 17 */, 10 /* 18/00-18/09 */, 11 /* 18/10-18/19 */, 11 /* 18/20-18/29 */, 12 /* 18/30-18/39 */, 12 /* 18/40-18/49 */, 13 /* 18/50-18/59 */,
    14 /* 18/60-18/69 */, 15 /* 18/70-18/79 */, 16 /* 18/80-18/89 */, 17 /* 18/90-18/99 */, 18 /* 18/100-18/109 */, 19 /* 18/110-18/119 */,
    20 /* 18/120-18/129 */, 21 /* 18/130-18/139 */, 22 /* 18/140-18/149 */, 23 /* 18/150-18/159 */, 24 /* 18/160-18/169 */, 25 /* 18/170-18/179 */,
    26 /* 18/180-18/189 */, 27 /* 18/190-18/199 */, 28 /* 18/200-18/209 */, 29 /* 18/210-18/219 */, 30 /* 18/220+ */
};

/*!
 * 知力/賢さによる最低魔法失敗率テーブル
 * Stat Table (INT/WIS) -- Minimum failure rate (percentage)
 */
extern const byte adj_mag_fail[] = {
    99 /* 3 */, 99 /* 4 */, 99 /* 5 */, 99 /* 6 */, 99 /* 7 */, 50 /* 8 */, 30 /* 9 */, 20 /* 10 */, 15 /* 11 */, 12 /* 12 */, 11 /* 13 */, 10 /* 14 */,
    9 /* 15 */, 8 /* 16 */, 7 /* 17 */, 6 /* 18/00-18/09 */, 6 /* 18/10-18/19 */, 5 /* 18/20-18/29 */, 5 /* 18/30-18/39 */, 5 /* 18/40-18/49 */,
    4 /* 18/50-18/59 */, 4 /* 18/60-18/69 */, 4 /* 18/70-18/79 */, 4 /* 18/80-18/89 */, 3 /* 18/90-18/99 */, 3 /* 18/100-18/109 */, 2 /* 18/110-18/119 */,
    2 /* 18/120-18/129 */, 2 /* 18/130-18/139 */, 2 /* 18/140-18/149 */, 1 /* 18/150-18/159 */, 1 /* 18/160-18/169 */, 1 /* 18/170-18/179 */,
    1 /* 18/180-18/189 */, 1 /* 18/190-18/199 */, 0 /* 18/200-18/209 */, 0 /* 18/210-18/219 */, 0 /* 18/220+ */
};

/*!
 * 知力/賢さによる魔法失敗率修正テーブル
 * Stat Table (INT/WIS) -- Various things
 */
extern const byte adj_mag_stat[] = {
    0 /* 3 */, 0 /* 4 */, 0 /* 5 */, 0 /* 6 */, 0 /* 7 */, 1 /* 8 */, 1 /* 9 */, 1 /* 10 */, 1 /* 11 */, 1 /* 12 */, 1 /* 13 */, 1 /* 14 */, 2 /* 15 */,
    2 /* 16 */, 2 /* 17 */, 3 /* 18/00-18/09 */, 3 /* 18/10-18/19 */, 3 /* 18/20-18/29 */, 3 /* 18/30-18/39 */, 3 /* 18/40-18/49 */, 4 /* 18/50-18/59 */,
    4 /* 18/60-18/69 */, 5 /* 18/70-18/79 */, 6 /* 18/80-18/89 */, 7 /* 18/90-18/99 */, 8 /* 18/100-18/109 */, 9 /* 18/110-18/119 */, 10 /* 18/120-18/129 */,
    11 /* 18/130-18/139 */, 12 /* 18/140-18/149 */, 13 /* 18/150-18/159 */, 14 /* 18/160-18/169 */, 15 /* 18/170-18/179 */, 16 /* 18/180-18/189 */,
    17 /* 18/190-18/199 */, 18 /* 18/200-18/209 */, 19 /* 18/210-18/219 */, 20 /* 18/220+ */
};

/*!
 * 魅力による店での取引修正テーブル
 * Stat Table (CHR) -- payment percentages
 */
extern const byte adj_chr_gold[] = {
    130 /* 3 */, 125 /* 4 */, 122 /* 5 */, 120 /* 6 */, 118 /* 7 */, 116 /* 8 */, 114 /* 9 */, 112 /* 10 */, 110 /* 11 */, 108 /* 12 */, 106 /* 13 */,
    104 /* 14 */, 103 /* 15 */, 102 /* 16 */, 101 /* 17 */, 100 /* 18/00-18/09 */, 99 /* 18/10-18/19 */, 98 /* 18/20-18/29 */, 97 /* 18/30-18/39 */,
    96 /* 18/40-18/49 */, 95 /* 18/50-18/59 */, 94 /* 18/60-18/69 */, 93 /* 18/70-18/79 */, 92 /* 18/80-18/89 */, 91 /* 18/90-18/99 */, 90 /* 18/100-18/109 */,
    89 /* 18/110-18/119 */, 88 /* 18/120-18/129 */, 87 /* 18/130-18/139 */, 86 /* 18/140-18/149 */, 85 /* 18/150-18/159 */, 84 /* 18/160-18/169 */,
    83 /* 18/170-18/179 */, 82 /* 18/180-18/189 */, 81 /* 18/190-18/199 */, 80 /* 18/200-18/209 */, 79 /* 18/210-18/219 */, 78 /* 18/220+ */
};

/*!
 * 知力による魔道具使用修正テーブル
 * Stat Table (INT) -- Magic devices
 */
extern const byte adj_int_dev[] = {
    0 /* 3 */, 0 /* 4 */, 0 /* 5 */, 0 /* 6 */, 0 /* 7 */, 1 /* 8 */, 1 /* 9 */, 1 /* 10 */, 1 /* 11 */, 1 /* 12 */, 1 /* 13 */, 1 /* 14 */, 2 /* 15 */,
    2 /* 16 */, 2 /* 17 */, 3 /* 18/00-18/09 */, 3 /* 18/10-18/19 */, 4 /* 18/20-18/29 */, 4 /* 18/30-18/39 */, 5 /* 18/40-18/49 */, 5 /* 18/50-18/59 */,
    6 /* 18/60-18/69 */, 6 /* 18/70-18/79 */, 7 /* 18/80-18/89 */, 7 /* 18/90-18/99 */, 8 /* 18/100-18/109 */, 9 /* 18/110-18/119 */, 10 /* 18/120-18/129 */,
    11 /* 18/130-18/139 */, 12 /* 18/140-18/149 */, 13 /* 18/150-18/159 */, 14 /* 18/160-18/169 */, 15 /* 18/170-18/179 */, 16 /* 18/180-18/189 */,
    17 /* 18/190-18/199 */, 18 /* 18/200-18/209 */, 19 /* 18/210-18/219 */, 20 /* 18/220+ */
};

/*!
 * 賢さによる魔法防御修正テーブル
 * Stat Table (WIS) -- Saving throw
 */
extern const byte adj_wis_sav[] = {
    0 /* 3 */, 0 /* 4 */, 0 /* 5 */, 0 /* 6 */, 0 /* 7 */, 1 /* 8 */, 1 /* 9 */, 1 /* 10 */, 1 /* 11 */, 1 /* 12 */, 1 /* 13 */, 1 /* 14 */, 2 /* 15 */,
    2 /* 16 */, 2 /* 17 */, 3 /* 18/00-18/09 */, 3 /* 18/10-18/19 */, 3 /* 18/20-18/29 */, 3 /* 18/30-18/39 */, 3 /* 18/40-18/49 */, 4 /* 18/50-18/59 */,
    4 /* 18/60-18/69 */, 5 /* 18/70-18/79 */, 5 /* 18/80-18/89 */, 6 /* 18/90-18/99 */, 7 /* 18/100-18/109 */, 8 /* 18/110-18/119 */, 9 /* 18/120-18/129 */,
    10 /* 18/130-18/139 */, 11 /* 18/140-18/149 */, 12 /* 18/150-18/159 */, 13 /* 18/160-18/169 */, 14 /* 18/170-18/179 */, 15 /* 18/180-18/189 */,
    16 /* 18/190-18/199 */, 17 /* 18/200-18/209 */, 18 /* 18/210-18/219 */, 19 /* 18/220+ */
};

/*!
 * 器用さによるトラップ解除修正テーブル
 * Stat Table (DEX) -- disarming
 */
extern const byte adj_dex_dis[] = {
    0 /* 3 */, 0 /* 4 */, 0 /* 5 */, 0 /* 6 */, 0 /* 7 */, 0 /* 8 */, 0 /* 9 */, 0 /* 10 */, 0 /* 11 */, 0 /* 12 */, 1 /* 13 */, 1 /* 14 */, 1 /* 15 */,
    2 /* 16 */, 2 /* 17 */, 4 /* 18/00-18/09 */, 4 /* 18/10-18/19 */, 4 /* 18/20-18/29 */, 4 /* 18/30-18/39 */, 5 /* 18/40-18/49 */, 5 /* 18/50-18/59 */,
    5 /* 18/60-18/69 */, 6 /* 18/70-18/79 */, 6 /* 18/80-18/89 */, 7 /* 18/90-18/99 */, 8 /* 18/100-18/109 */, 8 /* 18/110-18/119 */, 8 /* 18/120-18/129 */,
    8 /* 18/130-18/139 */, 8 /* 18/140-18/149 */, 9 /* 18/150-18/159 */, 9 /* 18/160-18/169 */, 9 /* 18/170-18/179 */, 9 /* 18/180-18/189 */,
    9 /* 18/190-18/199 */, 10 /* 18/200-18/209 */, 10 /* 18/210-18/219 */, 10 /* 18/220+ */
};

/*!
 * 知力によるトラップ解除修正テーブル
 * Stat Table (INT) -- disarming
 */
extern const byte adj_int_dis[] = {
    0 /* 3 */, 0 /* 4 */, 0 /* 5 */, 0 /* 6 */, 0 /* 7 */, 1 /* 8 */, 1 /* 9 */, 1 /* 10 */, 1 /* 11 */, 1 /* 12 */, 1 /* 13 */, 1 /* 14 */, 2 /* 15 */,
    2 /* 16 */, 2 /* 17 */, 3 /* 18/00-18/09 */, 3 /* 18/10-18/19 */, 3 /* 18/20-18/29 */, 4 /* 18/30-18/39 */, 4 /* 18/40-18/49 */, 5 /* 18/50-18/59 */,
    6 /* 18/60-18/69 */, 7 /* 18/70-18/79 */, 8 /* 18/80-18/89 */, 9 /* 18/90-18/99 */, 10 /* 18/100-18/109 */, 10 /* 18/110-18/119 */, 11 /* 18/120-18/129 */,
    12 /* 18/130-18/139 */, 13 /* 18/140-18/149 */, 14 /* 18/150-18/159 */, 15 /* 18/160-18/169 */, 16 /* 18/170-18/179 */, 17 /* 18/180-18/189 */,
    18 /* 18/190-18/199 */, 19 /* 18/200-18/209 */, 19 /* 18/210-18/219 */, 20 /* 18/220+ */
};

/*!
 * 器用さによるAC修正テーブル
 * Stat Table (DEX) -- bonus to ac (plus 128)
 */
extern const byte adj_dex_ta[] = {
    128 + -4 /*  3 */, 128 + -3 /*  4 */, 128 + -2 /*  5 */, 128 + -1 /*  6 */, 128 + 0 /*  7 */, 128 + 0 /*  8 */, 128 + 0 /*  9 */, 128 + 0 /* 10 */,
    128 + 0 /* 11 */, 128 + 0 /* 12 */, 128 + 0 /* 13 */, 128 + 0 /* 14 */, 128 + 1 /* 15 */, 128 + 1 /* 16 */, 128 + 1 /* 17 */, 128 + 2 /* 18/00-18/09 */,
    128 + 2 /* 18/10-18/19 */, 128 + 2 /* 18/20-18/29 */, 128 + 2 /* 18/30-18/39 */, 128 + 2 /* 18/40-18/49 */, 128 + 3 /* 18/50-18/59 */,
    128 + 3 /* 18/60-18/69 */, 128 + 3 /* 18/70-18/79 */, 128 + 4 /* 18/80-18/89 */, 128 + 5 /* 18/90-18/99 */, 128 + 6 /* 18/100-18/109 */,
    128 + 7 /* 18/110-18/119 */, 128 + 8 /* 18/120-18/129 */, 128 + 9 /* 18/130-18/139 */, 128 + 9 /* 18/140-18/149 */, 128 + 10 /* 18/150-18/159 */,
    128 + 11 /* 18/160-18/169 */, 128 + 12 /* 18/170-18/179 */, 128 + 13 /* 18/180-18/189 */, 128 + 14 /* 18/190-18/199 */, 128 + 15 /* 18/200-18/209 */,
    128 + 15 /* 18/210-18/219 */, 128 + 16 /* 18/220+ */
};

/*!
 * 腕力によるダメージ修正テーブル
 * Stat Table (STR) -- bonus to dam (plus 128)
 */
extern const byte adj_str_td[] = {
    128 + -2 /*  3 */, 128 + -2 /*  4 */, 128 + -1 /*  5 */, 128 + -1 /*  6 */, 128 + 0 /*  7 */, 128 + 0 /*  8 */, 128 + 0 /*  9 */, 128 + 0 /* 10 */,
    128 + 0 /* 11 */, 128 + 0 /* 12 */, 128 + 0 /* 13 */, 128 + 0 /* 14 */, 128 + 0 /* 15 */, 128 + 1 /* 16 */, 128 + 2 /* 17 */, 128 + 2 /* 18/00-18/09 */,
    128 + 2 /* 18/10-18/19 */, 128 + 3 /* 18/20-18/29 */, 128 + 3 /* 18/30-18/39 */, 128 + 3 /* 18/40-18/49 */, 128 + 3 /* 18/50-18/59 */,
    128 + 3 /* 18/60-18/69 */, 128 + 4 /* 18/70-18/79 */, 128 + 5 /* 18/80-18/89 */, 128 + 5 /* 18/90-18/99 */, 128 + 6 /* 18/100-18/109 */,
    128 + 7 /* 18/110-18/119 */, 128 + 8 /* 18/120-18/129 */, 128 + 9 /* 18/130-18/139 */, 128 + 10 /* 18/140-18/149 */, 128 + 11 /* 18/150-18/159 */,
    128 + 12 /* 18/160-18/169 */, 128 + 13 /* 18/170-18/179 */, 128 + 14 /* 18/180-18/189 */, 128 + 15 /* 18/190-18/199 */, 128 + 16 /* 18/200-18/209 */,
    128 + 18 /* 18/210-18/219 */, 128 + 20 /* 18/220+ */
};

/*!
 * 器用度による命中修正テーブル
 * Stat Table (DEX) -- bonus to hit (plus 128)
 */
extern const byte adj_dex_th[] = {
    128 + -3 /* 3 */, 128 + -2 /* 4 */, 128 + -2 /* 5 */, 128 + -1 /* 6 */, 128 + -1 /* 7 */, 128 + 0 /* 8 */, 128 + 0 /* 9 */, 128 + 0 /* 10 */,
    128 + 0 /* 11 */, 128 + 0 /* 12 */, 128 + 0 /* 13 */, 128 + 0 /* 14 */, 128 + 0 /* 15 */, 128 + 1 /* 16 */, 128 + 2 /* 17 */, 128 + 3 /* 18/00-18/09 */,
    128 + 3 /* 18/10-18/19 */, 128 + 3 /* 18/20-18/29 */, 128 + 3 /* 18/30-18/39 */, 128 + 3 /* 18/40-18/49 */, 128 + 4 /* 18/50-18/59 */,
    128 + 4 /* 18/60-18/69 */, 128 + 4 /* 18/70-18/79 */, 128 + 4 /* 18/80-18/89 */, 128 + 5 /* 18/90-18/99 */, 128 + 6 /* 18/100-18/109 */,
    128 + 7 /* 18/110-18/119 */, 128 + 8 /* 18/120-18/129 */, 128 + 9 /* 18/130-18/139 */, 128 + 9 /* 18/140-18/149 */, 128 + 10 /* 18/150-18/159 */,
    128 + 11 /* 18/160-18/169 */, 128 + 12 /* 18/170-18/179 */, 128 + 13 /* 18/180-18/189 */, 128 + 14 /* 18/190-18/199 */, 128 + 15 /* 18/200-18/209 */,
    128 + 15 /* 18/210-18/219 */, 128 + 16 /* 18/220+ */
};

/*!
 * 腕力による命中修正テーブル
 * Stat Table (STR) -- bonus to hit (plus 128)
 */
extern const byte adj_str_th[] = {
    128 + -3 /* 3 */, 128 + -2 /* 4 */, 128 + -1 /* 5 */, 128 + -1 /* 6 */, 128 + 0 /* 7 */, 128 + 0 /* 8 */, 128 + 0 /* 9 */, 128 + 0 /* 10 */,
    128 + 0 /* 11 */, 128 + 0 /* 12 */, 128 + 0 /* 13 */, 128 + 0 /* 14 */, 128 + 0 /* 15 */, 128 + 0 /* 16 */, 128 + 0 /* 17 */, 128 + 1 /* 18/00-18/09 */,
    128 + 1 /* 18/10-18/19 */, 128 + 1 /* 18/20-18/29 */, 128 + 1 /* 18/30-18/39 */, 128 + 1 /* 18/40-18/49 */, 128 + 1 /* 18/50-18/59 */,
    128 + 1 /* 18/60-18/69 */, 128 + 2 /* 18/70-18/79 */, 128 + 3 /* 18/80-18/89 */, 128 + 4 /* 18/90-18/99 */, 128 + 5 /* 18/100-18/109 */,
    128 + 6 /* 18/110-18/119 */, 128 + 7 /* 18/120-18/129 */, 128 + 8 /* 18/130-18/139 */, 128 + 9 /* 18/140-18/149 */, 128 + 10 /* 18/150-18/159 */,
    128 + 11 /* 18/160-18/169 */, 128 + 12 /* 18/170-18/179 */, 128 + 13 /* 18/180-18/189 */, 128 + 14 /* 18/190-18/199 */, 128 + 15 /* 18/200-18/209 */,
    128 + 15 /* 18/210-18/219 */, 128 + 16 /* 18/220+ */
};

/*!
 * 腕力による基本所持重量値テーブル
 * Stat Table (STR) -- weight limit in deca-pounds
 */
extern const byte adj_str_wgt[] = {
    10 /* 3 */, 11 /* 4 */, 12 /* 5 */, 13 /* 6 */, 14 /* 7 */, 15 /* 8 */, 16 /* 9 */, 17 /* 10 */, 18 /* 11 */, 19 /* 12 */, 20 /* 13 */, 21 /* 14 */,
    22 /* 15 */, 23 /* 16 */, 24 /* 17 */, 25 /* 18/00-18/09 */, 26 /* 18/10-18/19 */, 27 /* 18/20-18/29 */, 28 /* 18/30-18/39 */, 29 /* 18/40-18/49 */,
    30 /* 18/50-18/59 */, 31 /* 18/60-18/69 */, 31 /* 18/70-18/79 */, 32 /* 18/80-18/89 */, 32 /* 18/90-18/99 */, 33 /* 18/100-18/109 */,
    33 /* 18/110-18/119 */, 34 /* 18/120-18/129 */, 34 /* 18/130-18/139 */, 35 /* 18/140-18/149 */, 35 /* 18/150-18/159 */, 36 /* 18/160-18/169 */,
    36 /* 18/170-18/179 */, 37 /* 18/180-18/189 */, 37 /* 18/190-18/199 */, 38 /* 18/200-18/209 */, 38 /* 18/210-18/219 */, 39 /* 18/220+ */
};

/*!
 * 腕力による武器重量限界値テーブル
 * Stat Table (STR) -- weapon weight limit in pounds
 */
extern const byte adj_str_hold[] = {
    4 /* 3 */, 5 /* 4 */, 6 /* 5 */, 7 /* 6 */, 8 /* 7 */, 9 /* 8 */, 10 /* 9 */, 11 /* 10 */, 12 /* 11 */, 13 /* 12 */, 14 /* 13 */, 15 /* 14 */, 16 /* 15 */,
    17 /* 16 */, 18 /* 17 */, 19 /* 18/00-18/09 */, 20 /* 18/10-18/19 */, 21 /* 18/20-18/29 */, 22 /* 18/30-18/39 */, 23 /* 18/40-18/49 */,
    24 /* 18/50-18/59 */, 25 /* 18/60-18/69 */, 26 /* 18/70-18/79 */, 27 /* 18/80-18/89 */, 28 /* 18/90-18/99 */, 30 /* 18/100-18/109 */,
    31 /* 18/110-18/119 */, 32 /* 18/120-18/129 */, 33 /* 18/130-18/139 */, 34 /* 18/140-18/149 */, 35 /* 18/150-18/159 */, 37 /* 18/160-18/169 */,
    40 /* 18/170-18/179 */, 44 /* 18/180-18/189 */, 48 /* 18/190-18/199 */, 50 /* 18/200-18/209 */, 50 /* 18/210-18/219 */, 50 /* 18/220+ */
};

/*!
 * 腕力による採掘能力修正値テーブル
 * Stat Table (STR) -- digging value
 */
extern const byte adj_str_dig[] = {
    0 /* 3 */, 0 /* 4 */, 1 /* 5 */, 2 /* 6 */, 3 /* 7 */, 4 /* 8 */, 4 /* 9 */, 5 /* 10 */, 5 /* 11 */, 6 /* 12 */, 6 /* 13 */, 7 /* 14 */, 7 /* 15 */,
    8 /* 16 */, 8 /* 17 */, 9 /* 18/00-18/09 */, 10 /* 18/10-18/19 */, 12 /* 18/20-18/29 */, 15 /* 18/30-18/39 */, 20 /* 18/40-18/49 */, 25 /* 18/50-18/59 */,
    30 /* 18/60-18/69 */, 35 /* 18/70-18/79 */, 40 /* 18/80-18/89 */, 45 /* 18/90-18/99 */, 50 /* 18/100-18/109 */, 55 /* 18/110-18/119 */,
    60 /* 18/120-18/129 */, 65 /* 18/130-18/139 */, 70 /* 18/140-18/149 */, 75 /* 18/150-18/159 */, 80 /* 18/160-18/169 */, 85 /* 18/170-18/179 */,
    90 /* 18/180-18/189 */, 95 /* 18/190-18/199 */, 100 /* 18/200-18/209 */, 100 /* 18/210-18/219 */, 100 /* 18/220+ */
};

/*!
 * 器用さによる盗難防止＆体当たり成功判定修正テーブル
 * Stat Table (DEX) -- chance of avoiding "theft" and "falling"
 */
extern const byte adj_dex_safe[] = {
    0 /* 3 */, 1 /* 4 */, 2 /* 5 */, 3 /* 6 */, 4 /* 7 */, 5 /* 8 */, 5 /* 9 */, 6 /* 10 */, 6 /* 11 */, 7 /* 12 */, 7 /* 13 */, 8 /* 14 */, 8 /* 15 */,
    9 /* 16 */, 9 /* 17 */, 10 /* 18/00-18/09 */, 10 /* 18/10-18/19 */, 15 /* 18/20-18/29 */, 15 /* 18/30-18/39 */, 20 /* 18/40-18/49 */, 25 /* 18/50-18/59 */,
    30 /* 18/60-18/69 */, 35 /* 18/70-18/79 */, 40 /* 18/80-18/89 */, 45 /* 18/90-18/99 */, 50 /* 18/100-18/109 */, 60 /* 18/110-18/119 */,
    70 /* 18/120-18/129 */, 80 /* 18/130-18/139 */, 90 /* 18/140-18/149 */, 100 /* 18/150-18/159 */, 100 /* 18/160-18/169 */, 100 /* 18/170-18/179 */,
    100 /* 18/180-18/189 */, 100 /* 18/190-18/199 */, 100 /* 18/200-18/209 */, 100 /* 18/210-18/219 */, 100 /* 18/220+ */
};

/*!
 * 耐久による基本HP自然治癒値テーブル /
 * Stat Table (CON) -- base regeneration rate
 */
extern const byte adj_con_fix[] = {
    0 /* 3 */, 0 /* 4 */, 0 /* 5 */, 0 /* 6 */, 0 /* 7 */, 0 /* 8 */, 0 /* 9 */, 0 /* 10 */, 0 /* 11 */, 0 /* 12 */, 0 /* 13 */, 1 /* 14 */, 1 /* 15 */,
    1 /* 16 */, 1 /* 17 */, 2 /* 18/00-18/09 */, 2 /* 18/10-18/19 */, 2 /* 18/20-18/29 */, 2 /* 18/30-18/39 */, 2 /* 18/40-18/49 */, 3 /* 18/50-18/59 */,
    3 /* 18/60-18/69 */, 3 /* 18/70-18/79 */, 3 /* 18/80-18/89 */, 3 /* 18/90-18/99 */, 4 /* 18/100-18/109 */, 4 /* 18/110-18/119 */, 5 /* 18/120-18/129 */,
    6 /* 18/130-18/139 */, 6 /* 18/140-18/149 */, 7 /* 18/150-18/159 */, 7 /* 18/160-18/169 */, 8 /* 18/170-18/179 */, 8 /* 18/180-18/189 */,
    8 /* 18/190-18/199 */, 9 /* 18/200-18/209 */, 9 /* 18/210-18/219 */, 9 /* 18/220+ */
};

/*!
 * 耐久による基本HP自然治癒値テーブル /
 * Stat Table (CON) -- extra 1/4-hitpoints per level (plus 128)
 */
extern const byte adj_con_mhp[] = {
    128 + -8 /* 3 */, 128 + -6 /* 4 */, 128 + -4 /* 5 */, 128 + -2 /* 6 */, 128 + -1 /* 7 */, 128 + 0 /* 8 */, 128 + 0 /* 9 */, 128 + 0 /* 10 */,
    128 + 0 /* 11 */, 128 + 0 /* 12 */, 128 + 0 /* 13 */, 128 + 1 /* 14 */, 128 + 1 /* 15 */, 128 + 2 /* 16 */, 128 + 3 /* 17 */, 128 + 4 /* 18/00-18/09 */,
    128 + 5 /* 18/10-18/19 */, 128 + 6 /* 18/20-18/29 */, 128 + 7 /* 18/30-18/39 */, 128 + 8 /* 18/40-18/49 */, 128 + 9 /* 18/50-18/59 */,
    128 + 10 /* 18/60-18/69 */, 128 + 11 /* 18/70-18/79 */, 128 + 12 /* 18/80-18/89 */, 128 + 14 /* 18/90-18/99 */, 128 + 17 /* 18/100-18/109 */,
    128 + 20 /* 18/110-18/119 */, 128 + 23 /* 18/120-18/129 */, 128 + 26 /* 18/130-18/139 */, 128 + 29 /* 18/140-18/149 */, 128 + 32 /* 18/150-18/159 */,
    128 + 35 /* 18/160-18/169 */, 128 + 38 /* 18/170-18/179 */, 128 + 40 /* 18/180-18/189 */, 128 + 42 /* 18/190-18/199 */, 128 + 44 /* 18/200-18/209 */,
    128 + 46 /* 18/210-18/219 */, 128 + 48 /* 18/220+ */
};

/*!
 * 魅力による魅了能力修正テーブル /
 * Stat Table (CHR) -- charm
 */
extern const byte adj_chr_chm[] = {
    0 /* 3 */, 0 /* 4 */, 1 /* 5 */, 2 /* 6 */, 3 /* 7 */, 4 /* 8 */, 4 /* 9 */, 5 /* 10 */, 5 /* 11 */, 6 /* 12 */, 6 /* 13 */, 7 /* 14 */, 7 /* 15 */,
    8 /* 16 */, 8 /* 17 */, 9 /* 18/00-18/09 */, 10 /* 18/10-18/19 */, 12 /* 18/20-18/29 */, 15 /* 18/30-18/39 */, 18 /* 18/40-18/49 */, 21 /* 18/50-18/59 */,
    24 /* 18/60-18/69 */, 28 /* 18/70-18/79 */, 32 /* 18/80-18/89 */, 36 /* 18/90-18/99 */, 39 /* 18/100-18/109 */, 42 /* 18/110-18/119 */,
    45 /* 18/120-18/129 */, 49 /* 18/130-18/139 */, 53 /* 18/140-18/149 */, 57 /* 18/150-18/159 */, 61 /* 18/160-18/169 */, 65 /* 18/170-18/179 */,
    69 /* 18/180-18/189 */, 73 /* 18/190-18/199 */, 77 /* 18/200-18/209 */, 81 /* 18/210-18/219 */, 85 /* 18/220+ */
};
