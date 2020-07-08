#include "player/player-status.h"
#include "art-definition/art-sword-types.h"
#include "art-definition/art-weapon-types.h"
#include "autopick/autopick-reader-writer.h"
#include "autopick/autopick.h"
#include "cmd-action/cmd-pet.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-building/cmd-building.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-item/cmd-magiceat.h"
#include "combat/attack-power-table.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/floor-events.h"
#include "floor/floor.h"
#include "game-option/birth-options.h"
#include "grid/feature.h"
#include "inventory/inventory-object.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "market/arena-info-table.h"
#include "mind/mind-force-trainer.h"
#include "monster-floor/monster-remover.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/smart-learn-types.h"
#include "mutation/mutation.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object/object-flags.h"
#include "object/object-hook.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "object/object-mark-types.h"
#include "perception/object-perception.h"
#include "pet/pet-util.h"
#include "player/avatar.h"
#include "player/mimic-info-table.h"
#include "player/patron.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-effects.h"
#include "player/player-move.h"
#include "player/player-personalities-types.h"
#include "player/player-personality.h"
#include "player/player-race-types.h"
#include "player/player-skill.h"
#include "player/race-info-table.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-song-numbers.h"
#include "specific-object/bow.h"
#include "spell-realm/spells-hex.h"
#include "spell/range-calc.h"
#include "spell/spells-describer.h"
#include "spell/spells-execution.h"
#include "spell/spells-status.h"
#include "spell/spells-util.h"
#include "spell/technic-info-table.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/angband.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#include "view/display-main-window.h"
#include "view/display-messages.h"
#include "world/world.h"

static void calc_intra_vision(player_type *creature_ptr);
static void calc_stealth(player_type *creature_ptr);
static void calc_disarming(player_type *creature_ptr);
static void calc_device_ability(player_type *creature_ptr);
static void calc_saving_throw(player_type *creature_ptr);
static void calc_search(player_type *creature_ptr);
static void calc_search_freq(player_type *creature_ptr);
static void calc_to_hit_melee(player_type *creature_ptr);
static void calc_to_hit_shoot(player_type *creature_ptr);
static void calc_to_hit_throw(player_type *creature_ptr);
static void calc_dig(player_type *creature_ptr);
static void calc_num_blow(player_type *creature_ptr, int i);
static void calc_strength_addition(player_type *creature_ptr);
static void calc_intelligence_addition(player_type *creature_ptr);
static void calc_wisdom_addition(player_type *creature_ptr);
static void calc_dexterity_addition(player_type *creature_ptr);
static void calc_constitution_addition(player_type *creature_ptr);
static void calc_charisma_addition(player_type *creature_ptr);
static void calc_to_magic_chance(player_type *creature_ptr);
static void calc_base_ac(player_type *creature_ptr);
static void calc_base_ac_display(player_type *creature_ptr);
static void calc_to_ac(player_type *creature_ptr);
static void calc_to_ac_display(player_type *creature_ptr);
static void calc_speed(player_type *creature_ptr);

/*!
 * @brief 能力値テーブル / Abbreviations of healthy stats
 */
const concptr stat_names[6] = {
#ifdef JP
    "腕力 :", "知能 :", "賢さ :", "器用 :", "耐久 :", "魅力 :"
#else
    "STR : ", "INT : ", "WIS : ", "DEX : ", "CON : ", "CHR : "
#endif
};

/*!
 * @brief 能力値テーブル(能力低下時) / Abbreviations of damaged stats
 */
const concptr stat_names_reduced[6] = {
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
const s32b player_exp[PY_MAX_LEVEL] = { 10, 25, 45, 70, 100, 140, 200, 280, 380, /*10*/
    500, 650, 850, 1100, 1400, 1800, 2300, 2900, 3600, 4400, /*20*/
    5400, 6800, 8400, 10200, 12500, 17500, 25000, 35000L, 50000L, 75000L, /*30*/
    100000L, 150000L, 200000L, 275000L, 350000L, 450000L, 550000L, 700000L, 850000L, 1000000L, /*40*/
    1250000L, 1500000L, 1800000L, 2100000L, 2400000L, 2700000L, 3000000L, 3500000L, 4000000L, 4500000L, /*50*/
    5000000L };

/*!
 * @brief 基本必要強化値テーブル(アンドロイド専用)
 */
const s32b player_exp_a[PY_MAX_LEVEL] = { 20, 50, 100, 170, 280, 430, 650, 950, 1400, /*10*/
    1850, 2300, 2900, 3600, 4400, 5400, 6800, 8400, 10400, 12500, /*20*/
    17500, 25000, 35000, 50000L, 75000L, 100000L, 150000L, 200000L, 275000L, 350000L, /*30*/
    450000L, 550000L, 650000L, 800000L, 950000L, 1100000L, 1250000L, 1400000L, 1550000L, 1700000L, /*40*/
    1900000L, 2100000L, 2300000L, 2550000L, 2800000L, 3050000L, 3300000L, 3700000L, 4100000L, 4500000L, /*50*/
    5000000L };

/*!
 * 知力/賢さによるレベル毎の習得可能魔法数テーブル
 * Stat Table (INT/WIS) -- Number of half-spells per level
 */
const byte adj_mag_study[] = {
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
const byte adj_mag_mana[] = {
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
const byte adj_mag_fail[] = {
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
const byte adj_mag_stat[] = {
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
const byte adj_chr_gold[] = {
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
const byte adj_int_dev[] = {
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
const byte adj_wis_sav[] = {
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
const byte adj_dex_dis[] = {
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
const byte adj_int_dis[] = {
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
const byte adj_dex_ta[] = {
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
const byte adj_str_td[] = {
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
const byte adj_dex_th[] = {
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
const byte adj_str_th[] = {
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
const byte adj_str_wgt[] = {
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
const byte adj_str_hold[] = {
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
const byte adj_str_dig[] = {
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
const byte adj_dex_safe[] = {
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
const byte adj_con_fix[] = {
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
const byte adj_con_mhp[] = {
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
const byte adj_chr_chm[] = {
    0 /* 3 */, 0 /* 4 */, 1 /* 5 */, 2 /* 6 */, 3 /* 7 */, 4 /* 8 */, 4 /* 9 */, 5 /* 10 */, 5 /* 11 */, 6 /* 12 */, 6 /* 13 */, 7 /* 14 */, 7 /* 15 */,
    8 /* 16 */, 8 /* 17 */, 9 /* 18/00-18/09 */, 10 /* 18/10-18/19 */, 12 /* 18/20-18/29 */, 15 /* 18/30-18/39 */, 18 /* 18/40-18/49 */, 21 /* 18/50-18/59 */,
    24 /* 18/60-18/69 */, 28 /* 18/70-18/79 */, 32 /* 18/80-18/89 */, 36 /* 18/90-18/99 */, 39 /* 18/100-18/109 */, 42 /* 18/110-18/119 */,
    45 /* 18/120-18/129 */, 49 /* 18/130-18/139 */, 53 /* 18/140-18/149 */, 57 /* 18/150-18/159 */, 61 /* 18/160-18/169 */, 65 /* 18/170-18/179 */,
    69 /* 18/180-18/189 */, 73 /* 18/190-18/199 */, 77 /* 18/200-18/209 */, 81 /* 18/210-18/219 */, 85 /* 18/220+ */
};

/*** Player information ***/

/*
 * Static player info record
 */
player_type p_body;

/*
 * Pointer to the player info
 */
player_type *p_ptr = &p_body;

/*
 * Return alignment title
 */
concptr your_alignment(player_type *creature_ptr)
{
    if (creature_ptr->align > 150)
        return _("大善", "Lawful");
    else if (creature_ptr->align > 50)
        return _("中善", "Good");
    else if (creature_ptr->align > 10)
        return _("小善", "Neutral Good");
    else if (creature_ptr->align > -11)
        return _("中立", "Neutral");
    else if (creature_ptr->align > -51)
        return _("小悪", "Neutral Evil");
    else if (creature_ptr->align > -151)
        return _("中悪", "Evil");
    else
        return _("大悪", "Chaotic");
}

/*
 * Return proficiency level of weapons and misc. skills (except riding)
 */
int weapon_exp_level(int weapon_exp)
{
    if (weapon_exp < WEAPON_EXP_BEGINNER)
        return EXP_LEVEL_UNSKILLED;
    else if (weapon_exp < WEAPON_EXP_SKILLED)
        return EXP_LEVEL_BEGINNER;
    else if (weapon_exp < WEAPON_EXP_EXPERT)
        return EXP_LEVEL_SKILLED;
    else if (weapon_exp < WEAPON_EXP_MASTER)
        return EXP_LEVEL_EXPERT;
    else
        return EXP_LEVEL_MASTER;
}

/*
 * Return proficiency level of riding
 */
int riding_exp_level(int riding_exp)
{
    if (riding_exp < RIDING_EXP_BEGINNER)
        return EXP_LEVEL_UNSKILLED;
    else if (riding_exp < RIDING_EXP_SKILLED)
        return EXP_LEVEL_BEGINNER;
    else if (riding_exp < RIDING_EXP_EXPERT)
        return EXP_LEVEL_SKILLED;
    else if (riding_exp < RIDING_EXP_MASTER)
        return EXP_LEVEL_EXPERT;
    else
        return EXP_LEVEL_MASTER;
}

/*
 * Return proficiency level of spells
 */
int spell_exp_level(int spell_exp)
{
    if (spell_exp < SPELL_EXP_BEGINNER)
        return EXP_LEVEL_UNSKILLED;
    else if (spell_exp < SPELL_EXP_SKILLED)
        return EXP_LEVEL_BEGINNER;
    else if (spell_exp < SPELL_EXP_EXPERT)
        return EXP_LEVEL_SKILLED;
    else if (spell_exp < SPELL_EXP_MASTER)
        return EXP_LEVEL_EXPERT;
    else
        return EXP_LEVEL_MASTER;
}

/*!
 * @brief プレイヤー構造体の全ステータスを初期化する
 */
static void clear_creature_bonuses(player_type *creature_ptr)
{
    creature_ptr->dis_to_h[0] = creature_ptr->to_h[0] = 0;
    creature_ptr->dis_to_h[1] = creature_ptr->to_h[1] = 0;
    creature_ptr->dis_to_d[0] = creature_ptr->to_d[0] = 0;
    creature_ptr->dis_to_d[1] = creature_ptr->to_d[1] = 0;
    creature_ptr->dis_to_h_b = creature_ptr->to_h_b = 0;
    creature_ptr->to_h_m = 0;
    creature_ptr->to_d_m = 0;
    creature_ptr->to_dd[0] = creature_ptr->to_ds[0] = 0;
    creature_ptr->to_dd[1] = creature_ptr->to_ds[1] = 0;

    creature_ptr->extra_blows[0] = creature_ptr->extra_blows[1] = 0;
    creature_ptr->num_fire = 100;
    creature_ptr->tval_xtra = 0;
    creature_ptr->tval_ammo = 0;
    creature_ptr->cursed = 0L;
    creature_ptr->bless_blade = FALSE;
    creature_ptr->xtra_might = FALSE;
    creature_ptr->impact[0] = FALSE;
    creature_ptr->impact[1] = FALSE;
    creature_ptr->pass_wall = FALSE;
    creature_ptr->kill_wall = FALSE;
    creature_ptr->dec_mana = FALSE;
    creature_ptr->easy_spell = FALSE;
    creature_ptr->heavy_spell = FALSE;
    creature_ptr->see_inv = FALSE;
    creature_ptr->free_act = FALSE;
    creature_ptr->slow_digest = FALSE;
    creature_ptr->regenerate = FALSE;
    creature_ptr->can_swim = FALSE;
    creature_ptr->levitation = FALSE;
    creature_ptr->hold_exp = FALSE;
    creature_ptr->telepathy = FALSE;
    creature_ptr->esp_animal = FALSE;
    creature_ptr->esp_undead = FALSE;
    creature_ptr->esp_demon = FALSE;
    creature_ptr->esp_orc = FALSE;
    creature_ptr->esp_troll = FALSE;
    creature_ptr->esp_giant = FALSE;
    creature_ptr->esp_dragon = FALSE;
    creature_ptr->esp_human = FALSE;
    creature_ptr->esp_evil = FALSE;
    creature_ptr->esp_good = FALSE;
    creature_ptr->esp_nonliving = FALSE;
    creature_ptr->esp_unique = FALSE;
    creature_ptr->lite = FALSE;
    creature_ptr->sustain_str = FALSE;
    creature_ptr->sustain_int = FALSE;
    creature_ptr->sustain_wis = FALSE;
    creature_ptr->sustain_con = FALSE;
    creature_ptr->sustain_dex = FALSE;
    creature_ptr->sustain_chr = FALSE;
    creature_ptr->resist_acid = FALSE;
    creature_ptr->resist_elec = FALSE;
    creature_ptr->resist_fire = FALSE;
    creature_ptr->resist_cold = FALSE;
    creature_ptr->resist_pois = FALSE;
    creature_ptr->resist_conf = FALSE;
    creature_ptr->resist_sound = FALSE;
    creature_ptr->resist_lite = FALSE;
    creature_ptr->resist_dark = FALSE;
    creature_ptr->resist_chaos = FALSE;
    creature_ptr->resist_disen = FALSE;
    creature_ptr->resist_shard = FALSE;
    creature_ptr->resist_nexus = FALSE;
    creature_ptr->resist_blind = FALSE;
    creature_ptr->resist_neth = FALSE;
    creature_ptr->resist_time = FALSE;
    creature_ptr->resist_water = FALSE;
    creature_ptr->resist_fear = FALSE;
    creature_ptr->reflect = FALSE;
    creature_ptr->sh_fire = FALSE;
    creature_ptr->sh_elec = FALSE;
    creature_ptr->sh_cold = FALSE;
    creature_ptr->anti_magic = FALSE;
    creature_ptr->anti_tele = FALSE;
    creature_ptr->warning = FALSE;
    creature_ptr->mighty_throw = FALSE;
    creature_ptr->see_nocto = FALSE;
    creature_ptr->immune_acid = FALSE;
    creature_ptr->immune_elec = FALSE;
    creature_ptr->immune_fire = FALSE;
    creature_ptr->immune_cold = FALSE;
    creature_ptr->ryoute = FALSE;
    creature_ptr->migite = FALSE;
    creature_ptr->hidarite = FALSE;
    creature_ptr->no_flowed = FALSE;
    creature_ptr->yoiyami = FALSE;
    creature_ptr->easy_2weapon = FALSE;
    creature_ptr->down_saving = FALSE;
}

/*!
 * @brief プレイヤーの全ステータスを更新する /
 * Calculate the players current "state", taking into account
 * not only race/class intrinsics, but also objects being worn
 * and temporary spell effects.
 * @return なし
 * @details
 * <pre>
 * See also calc_mana() and calc_hitpoints().
 *
 * Take note of the new "speed code", in particular, a very strong
 * player will start slowing down as soon as he reaches 150 pounds,
 * but not until he reaches 450 pounds will he be half as fast as
 * a normal kobold.  This both hurts and helps the player, hurts
 * because in the old days a player could just avoid 300 pounds,
 * and helps because now carrying 300 pounds is not very painful.
 *
 * The "weapon" and "bow" do *not* add to the bonuses to hit or to
 * damage, since that would affect non-combat things.  These values
 * are actually added in later, at the appropriate place.
 *
 * This function induces various "status" messages.
 * </pre>
 */
void calc_bonuses(player_type *creature_ptr)
{
    int hold;
    int default_hand = 0;
    int empty_hands_status = empty_hands(creature_ptr, TRUE);
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    bool omoi = FALSE;
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;

    bool have_sw = FALSE, have_kabe = FALSE;
    bool riding_levitation = FALSE;
    OBJECT_IDX this_o_idx, next_o_idx = 0;

    /* Save the old vision stuff */
    bool old_telepathy = creature_ptr->telepathy;
    bool old_esp_animal = creature_ptr->esp_animal;
    bool old_esp_undead = creature_ptr->esp_undead;
    bool old_esp_demon = creature_ptr->esp_demon;
    bool old_esp_orc = creature_ptr->esp_orc;
    bool old_esp_troll = creature_ptr->esp_troll;
    bool old_esp_giant = creature_ptr->esp_giant;
    bool old_esp_dragon = creature_ptr->esp_dragon;
    bool old_esp_human = creature_ptr->esp_human;
    bool old_esp_evil = creature_ptr->esp_evil;
    bool old_esp_good = creature_ptr->esp_good;
    bool old_esp_nonliving = creature_ptr->esp_nonliving;
    bool old_esp_unique = creature_ptr->esp_unique;
    bool old_see_inv = creature_ptr->see_inv;
    bool old_mighty_throw = creature_ptr->mighty_throw;
    s16b old_speed = creature_ptr->pspeed;

    ARMOUR_CLASS old_dis_ac = creature_ptr->dis_ac;
    ARMOUR_CLASS old_dis_to_a = creature_ptr->dis_to_a;

    clear_creature_bonuses(creature_ptr);
    calc_race_status(creature_ptr);

    if (has_melee_weapon(creature_ptr, INVEN_RARM))
        creature_ptr->migite = TRUE;
    if (has_melee_weapon(creature_ptr, INVEN_LARM)) {
        creature_ptr->hidarite = TRUE;
        if (!creature_ptr->migite)
            default_hand = 1;
    }

    if (can_two_hands_wielding(creature_ptr)) {
        if (creature_ptr->migite && (empty_hands(creature_ptr, FALSE) == EMPTY_HAND_LARM)
            && object_allow_two_hands_wielding(&creature_ptr->inventory_list[INVEN_RARM])) {
            creature_ptr->ryoute = TRUE;
        } else if (creature_ptr->hidarite && (empty_hands(creature_ptr, FALSE) == EMPTY_HAND_RARM)
            && object_allow_two_hands_wielding(&creature_ptr->inventory_list[INVEN_LARM])) {
            creature_ptr->ryoute = TRUE;
        } else {
            switch (creature_ptr->pclass) {
            case CLASS_MONK:
            case CLASS_FORCETRAINER:
            case CLASS_BERSERKER:
                if (empty_hands(creature_ptr, FALSE) == (EMPTY_HAND_RARM | EMPTY_HAND_LARM)) {
                    creature_ptr->migite = TRUE;
                    creature_ptr->ryoute = TRUE;
                }
                break;
            }
        }
    }

    if (!creature_ptr->migite && !creature_ptr->hidarite) {
        if (empty_hands_status & EMPTY_HAND_RARM)
            creature_ptr->migite = TRUE;
        else if (empty_hands_status == EMPTY_HAND_LARM) {
            creature_ptr->hidarite = TRUE;
            default_hand = 1;
        }
    }

    if (creature_ptr->special_defense & KAMAE_MASK) {
        if (!(empty_hands_status & EMPTY_HAND_RARM)) {
            set_action(creature_ptr, ACTION_NONE);
        }
    }

    calc_class_status(creature_ptr);
    calc_timelimit_status(creature_ptr);
    set_personality_flags(creature_ptr);

    if (music_singing(creature_ptr, MUSIC_WALL)) {
        creature_ptr->kill_wall = TRUE;
    }

    set_mutation_flags(creature_ptr);

    calc_equipment_status(creature_ptr);

    if (old_mighty_throw != creature_ptr->mighty_throw) {
        creature_ptr->window |= PW_INVEN;
    }

    if (creature_ptr->cursed & TRC_TELEPORT)
        creature_ptr->cursed &= ~(TRC_TELEPORT_SELF);

    if (creature_ptr->sh_fire)
        creature_ptr->lite = TRUE;

    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(creature_ptr, HEX_DETECT_EVIL))
            creature_ptr->esp_evil = TRUE;

        if (hex_spelling(creature_ptr, HEX_DEMON_AURA)) {
            creature_ptr->sh_fire = TRUE;
            creature_ptr->regenerate = TRUE;
        }

        if (hex_spelling(creature_ptr, HEX_ICE_ARMOR)) {
            creature_ptr->sh_cold = TRUE;
        }

        if (hex_spelling(creature_ptr, HEX_SHOCK_CLOAK)) {
            creature_ptr->sh_elec = TRUE;
        }
    }

    calc_strength_addition(creature_ptr);
    calc_intelligence_addition(creature_ptr);
    calc_wisdom_addition(creature_ptr);
    calc_dexterity_addition(creature_ptr);
    calc_constitution_addition(creature_ptr);
    calc_charisma_addition(creature_ptr);
    calc_to_magic_chance(creature_ptr);
    calc_base_ac(creature_ptr);
    calc_to_ac(creature_ptr);
    calc_base_ac_display(creature_ptr);
    calc_to_ac_display(creature_ptr);

    int count = 0;
    for (int i = 0; i < A_MAX; i++) {
        int ind;
        int top = modify_stat_value(creature_ptr->stat_max[i], creature_ptr->stat_add[i]);

        if (creature_ptr->stat_top[i] != top) {
            creature_ptr->stat_top[i] = (s16b)top;
            creature_ptr->redraw |= (PR_STATS);
            creature_ptr->window |= (PW_PLAYER);
        }

        int use = modify_stat_value(creature_ptr->stat_cur[i], creature_ptr->stat_add[i]);

        if ((i == A_CHR) && (creature_ptr->muta3 & MUT3_ILL_NORM)) {
            /* 10 to 18/90 charisma, guaranteed, based on level */
            if (use < 8 + 2 * creature_ptr->lev) {
                use = 8 + 2 * creature_ptr->lev;
            }
        }

        if (creature_ptr->stat_use[i] != use) {
            creature_ptr->stat_use[i] = (s16b)use;
            creature_ptr->redraw |= (PR_STATS);
            creature_ptr->window |= (PW_PLAYER);
        }

        if (use <= 18)
            ind = (use - 3);
        else if (use <= 18 + 219)
            ind = (15 + (use - 18) / 10);
        else
            ind = (37);

        if (creature_ptr->stat_ind[i] == ind)
            continue;
        creature_ptr->stat_ind[i] = (s16b)ind;
        if (i == A_CON) {
            creature_ptr->update |= (PU_HP);
        } else if (i == A_INT) {
            if (mp_ptr->spell_stat == A_INT) {
                creature_ptr->update |= (PU_MANA | PU_SPELLS);
            }
        } else if (i == A_WIS) {
            if (mp_ptr->spell_stat == A_WIS) {
                creature_ptr->update |= (PU_MANA | PU_SPELLS);
            }
        } else if (i == A_CHR) {
            if (mp_ptr->spell_stat == A_CHR) {
                creature_ptr->update |= (PU_MANA | PU_SPELLS);
            }
        }

        creature_ptr->window |= (PW_PLAYER);
        count++;
    }

    if (creature_ptr->telepathy != old_telepathy) {
        creature_ptr->update |= (PU_MONSTERS);
    }

    if ((creature_ptr->esp_animal != old_esp_animal) || (creature_ptr->esp_undead != old_esp_undead) || (creature_ptr->esp_demon != old_esp_demon)
        || (creature_ptr->esp_orc != old_esp_orc) || (creature_ptr->esp_troll != old_esp_troll) || (creature_ptr->esp_giant != old_esp_giant)
        || (creature_ptr->esp_dragon != old_esp_dragon) || (creature_ptr->esp_human != old_esp_human) || (creature_ptr->esp_evil != old_esp_evil)
        || (creature_ptr->esp_good != old_esp_good) || (creature_ptr->esp_nonliving != old_esp_nonliving) || (creature_ptr->esp_unique != old_esp_unique)) {
        creature_ptr->update |= (PU_MONSTERS);
    }

    if (creature_ptr->see_inv != old_see_inv) {
        creature_ptr->update |= (PU_MONSTERS);
    }

    if ((creature_ptr->migite && (empty_hands_status & EMPTY_HAND_RARM)) || (creature_ptr->hidarite && (empty_hands_status & EMPTY_HAND_LARM))) {
        creature_ptr->to_h[default_hand] += (creature_ptr->skill_exp[GINOU_SUDE] - WEAPON_EXP_BEGINNER) / 200;
        creature_ptr->dis_to_h[default_hand] += (creature_ptr->skill_exp[GINOU_SUDE] - WEAPON_EXP_BEGINNER) / 200;
    }

    if (has_melee_weapon(creature_ptr, INVEN_RARM) && has_melee_weapon(creature_ptr, INVEN_LARM)) {
        int penalty1, penalty2;
        penalty1 = ((100 - creature_ptr->skill_exp[GINOU_NITOURYU] / 160) - (130 - creature_ptr->inventory_list[INVEN_RARM].weight) / 8);
        penalty2 = ((100 - creature_ptr->skill_exp[GINOU_NITOURYU] / 160) - (130 - creature_ptr->inventory_list[INVEN_LARM].weight) / 8);
        if ((creature_ptr->inventory_list[INVEN_RARM].name1 == ART_QUICKTHORN) && (creature_ptr->inventory_list[INVEN_LARM].name1 == ART_TINYTHORN)) {
            penalty1 = penalty1 / 2 - 5;
            penalty2 = penalty2 / 2 - 5;
            creature_ptr->pspeed += 7;
            creature_ptr->to_a += 10;
            creature_ptr->dis_to_a += 10;
        }
        if (creature_ptr->easy_2weapon) {
            if (penalty1 > 0)
                penalty1 /= 2;
            if (penalty2 > 0)
                penalty2 /= 2;
        } else if ((creature_ptr->inventory_list[INVEN_LARM].tval == TV_SWORD)
            && ((creature_ptr->inventory_list[INVEN_LARM].sval == SV_MAIN_GAUCHE) || (creature_ptr->inventory_list[INVEN_LARM].sval == SV_WAKIZASHI))) {
            penalty1 = MAX(0, penalty1 - 10);
            penalty2 = MAX(0, penalty2 - 10);
        }
        if ((creature_ptr->inventory_list[INVEN_RARM].name1 == ART_MUSASI_KATANA) && (creature_ptr->inventory_list[INVEN_LARM].name1 == ART_MUSASI_WAKIZASI)) {
            penalty1 = MIN(0, penalty1);
            penalty2 = MIN(0, penalty2);
            creature_ptr->to_a += 10;
            creature_ptr->dis_to_a += 10;
        } else {
            if ((creature_ptr->inventory_list[INVEN_RARM].name1 == ART_MUSASI_KATANA) && (penalty1 > 0))
                penalty1 /= 2;
            if ((creature_ptr->inventory_list[INVEN_LARM].name1 == ART_MUSASI_WAKIZASI) && (penalty2 > 0))
                penalty2 /= 2;
        }

        if (creature_ptr->inventory_list[INVEN_RARM].tval == TV_POLEARM)
            penalty1 += 10;
        if (creature_ptr->inventory_list[INVEN_LARM].tval == TV_POLEARM)
            penalty2 += 10;
        creature_ptr->to_h[0] -= (s16b)penalty1;
        creature_ptr->to_h[1] -= (s16b)penalty2;
        creature_ptr->dis_to_h[0] -= (s16b)penalty1;
        creature_ptr->dis_to_h[1] -= (s16b)penalty2;
    }

    int j = creature_ptr->total_weight;
    if (!creature_ptr->riding) {
        count = (int)weight_limit(creature_ptr);
    } else {
        monster_type *riding_m_ptr = &floor_ptr->m_list[creature_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        riding_levitation = (riding_r_ptr->flags7 & RF7_CAN_FLY) ? TRUE : FALSE;
        if (riding_r_ptr->flags7 & (RF7_CAN_SWIM | RF7_AQUATIC))
            creature_ptr->can_swim = TRUE;

        if (!(riding_r_ptr->flags2 & RF2_PASS_WALL))
            creature_ptr->pass_wall = FALSE;
        if (riding_r_ptr->flags2 & RF2_KILL_WALL)
            creature_ptr->kill_wall = TRUE;

        if (creature_ptr->skill_exp[GINOU_RIDING] < RIDING_EXP_SKILLED)
            j += (creature_ptr->wt * 3 * (RIDING_EXP_SKILLED - creature_ptr->skill_exp[GINOU_RIDING])) / RIDING_EXP_SKILLED;

        count = 1500 + riding_r_ptr->level * 25;
    }

    if (j > count)
        creature_ptr->pspeed -= ((j - count) / (count / 5));

    creature_ptr->to_d[0] += ((int)(adj_str_td[creature_ptr->stat_ind[A_STR]]) - 128);
    creature_ptr->to_d[1] += ((int)(adj_str_td[creature_ptr->stat_ind[A_STR]]) - 128);
    creature_ptr->to_d_m += ((int)(adj_str_td[creature_ptr->stat_ind[A_STR]]) - 128);
    creature_ptr->to_h[0] += ((int)(adj_dex_th[creature_ptr->stat_ind[A_DEX]]) - 128);
    creature_ptr->to_h[1] += ((int)(adj_dex_th[creature_ptr->stat_ind[A_DEX]]) - 128);
    creature_ptr->to_h_b += ((int)(adj_dex_th[creature_ptr->stat_ind[A_DEX]]) - 128);
    creature_ptr->to_h_m += ((int)(adj_dex_th[creature_ptr->stat_ind[A_DEX]]) - 128);
    creature_ptr->to_h[0] += ((int)(adj_str_th[creature_ptr->stat_ind[A_STR]]) - 128);
    creature_ptr->to_h[1] += ((int)(adj_str_th[creature_ptr->stat_ind[A_STR]]) - 128);
    creature_ptr->to_h_b += ((int)(adj_str_th[creature_ptr->stat_ind[A_STR]]) - 128);
    creature_ptr->to_h_m += ((int)(adj_str_th[creature_ptr->stat_ind[A_STR]]) - 128);
    creature_ptr->dis_to_d[0] += ((int)(adj_str_td[creature_ptr->stat_ind[A_STR]]) - 128);
    creature_ptr->dis_to_d[1] += ((int)(adj_str_td[creature_ptr->stat_ind[A_STR]]) - 128);
    creature_ptr->dis_to_h[0] += ((int)(adj_dex_th[creature_ptr->stat_ind[A_DEX]]) - 128);
    creature_ptr->dis_to_h[1] += ((int)(adj_dex_th[creature_ptr->stat_ind[A_DEX]]) - 128);
    creature_ptr->dis_to_h_b += ((int)(adj_dex_th[creature_ptr->stat_ind[A_DEX]]) - 128);
    creature_ptr->dis_to_h[0] += ((int)(adj_str_th[creature_ptr->stat_ind[A_STR]]) - 128);
    creature_ptr->dis_to_h[1] += ((int)(adj_str_th[creature_ptr->stat_ind[A_STR]]) - 128);
    creature_ptr->dis_to_h_b += ((int)(adj_str_th[creature_ptr->stat_ind[A_STR]]) - 128);

    hold = adj_str_hold[creature_ptr->stat_ind[A_STR]];
    o_ptr = &creature_ptr->inventory_list[INVEN_BOW];
    creature_ptr->heavy_shoot = is_heavy_shoot(creature_ptr, o_ptr);
    if (creature_ptr->heavy_shoot) {
        creature_ptr->to_h_b += 2 * (hold - o_ptr->weight / 10);
        creature_ptr->dis_to_h_b += 2 * (hold - o_ptr->weight / 10);
    }

    if (o_ptr->k_idx) {
        creature_ptr->tval_ammo = (byte)bow_tval_ammo(o_ptr);
        if (o_ptr->k_idx && !creature_ptr->heavy_shoot) {
            creature_ptr->num_fire = calc_num_fire(creature_ptr, o_ptr);
            if ((creature_ptr->pclass == CLASS_SNIPER) && (creature_ptr->tval_ammo == TV_BOLT)) {
                creature_ptr->to_h_b += (10 + (creature_ptr->lev / 5));
                creature_ptr->dis_to_h_b += (10 + (creature_ptr->lev / 5));
            }
        }
    }

    if (creature_ptr->ryoute)
        hold *= 2;
    for (int i = 0; i < 2; i++) {
        o_ptr = &creature_ptr->inventory_list[INVEN_RARM + i];
        object_flags(o_ptr, flgs);

        calc_num_blow(creature_ptr, i);

        if ((creature_ptr->pclass == CLASS_PRIEST) && (!(have_flag(flgs, TR_BLESSED))) && ((o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM))) {
            creature_ptr->to_h[i] -= 2;
            creature_ptr->to_d[i] -= 2;
            creature_ptr->dis_to_h[i] -= 2;
            creature_ptr->dis_to_d[i] -= 2;
            creature_ptr->icky_wield[i] = TRUE;
        } else if (creature_ptr->pclass == CLASS_BERSERKER) {
            creature_ptr->to_h[i] += creature_ptr->lev / 5;
            creature_ptr->to_d[i] += creature_ptr->lev / 6;
            creature_ptr->dis_to_h[i] += creature_ptr->lev / 5;
            creature_ptr->dis_to_d[i] += creature_ptr->lev / 6;
            if (((i == 0) && !creature_ptr->hidarite) || creature_ptr->ryoute) {
                creature_ptr->to_h[i] += creature_ptr->lev / 5;
                creature_ptr->to_d[i] += creature_ptr->lev / 6;
                creature_ptr->dis_to_h[i] += creature_ptr->lev / 5;
                creature_ptr->dis_to_d[i] += creature_ptr->lev / 6;
            }
        } else if (creature_ptr->pclass == CLASS_SORCERER) {
            if (!((o_ptr->tval == TV_HAFTED) && ((o_ptr->sval == SV_WIZSTAFF) || (o_ptr->sval == SV_NAMAKE_HAMMER)))) {
                creature_ptr->to_h[i] -= 200;
                creature_ptr->to_d[i] -= 200;
                creature_ptr->dis_to_h[i] -= 200;
                creature_ptr->dis_to_d[i] -= 200;
                creature_ptr->icky_wield[i] = TRUE;
            } else {
                creature_ptr->to_h[i] -= 30;
                creature_ptr->to_d[i] -= 10;
                creature_ptr->dis_to_h[i] -= 30;
                creature_ptr->dis_to_d[i] -= 10;
            }
        }

        if ((creature_ptr->realm1 == REALM_HEX) && object_is_cursed(o_ptr)) {
            if (o_ptr->curse_flags & (TRC_CURSED)) {
                creature_ptr->to_h[i] += 5;
                creature_ptr->dis_to_h[i] += 5;
            }
            if (o_ptr->curse_flags & (TRC_HEAVY_CURSE)) {
                creature_ptr->to_h[i] += 7;
                creature_ptr->dis_to_h[i] += 7;
            }
            if (o_ptr->curse_flags & (TRC_PERMA_CURSE)) {
                creature_ptr->to_h[i] += 13;
                creature_ptr->dis_to_h[i] += 13;
            }
            if (o_ptr->curse_flags & (TRC_TY_CURSE)) {
                creature_ptr->to_h[i] += 5;
                creature_ptr->dis_to_h[i] += 5;
            }
            if (hex_spelling(creature_ptr, HEX_RUNESWORD)) {
                if (o_ptr->curse_flags & (TRC_CURSED)) {
                    creature_ptr->to_d[i] += 5;
                    creature_ptr->dis_to_d[i] += 5;
                }
                if (o_ptr->curse_flags & (TRC_HEAVY_CURSE)) {
                    creature_ptr->to_d[i] += 7;
                    creature_ptr->dis_to_d[i] += 7;
                }
                if (o_ptr->curse_flags & (TRC_PERMA_CURSE)) {
                    creature_ptr->to_d[i] += 13;
                    creature_ptr->dis_to_d[i] += 13;
                }
            }
        }

        if (creature_ptr->riding == 0)
            continue;

        if ((o_ptr->tval == TV_POLEARM) && ((o_ptr->sval == SV_LANCE) || (o_ptr->sval == SV_HEAVY_LANCE))) {
            creature_ptr->to_h[i] += 15;
            creature_ptr->dis_to_h[i] += 15;
            creature_ptr->to_dd[i] += 2;
            continue;
        }

        if (have_flag(flgs, TR_RIDING))
            continue;

        int penalty;
        if ((creature_ptr->pclass == CLASS_BEASTMASTER) || (creature_ptr->pclass == CLASS_CAVALRY)) {
            penalty = 5;
        } else {
            penalty = r_info[floor_ptr->m_list[creature_ptr->riding].r_idx].level - creature_ptr->skill_exp[GINOU_RIDING] / 80;
            penalty += 30;
            if (penalty < 30)
                penalty = 30;
        }

        creature_ptr->to_h[i] -= (s16b)penalty;
        creature_ptr->dis_to_h[i] -= (s16b)penalty;
        creature_ptr->riding_wield[i] = TRUE;
    }

    if (creature_ptr->riding) {
        int penalty = 0;

        creature_ptr->riding_ryoute = FALSE;

        if (creature_ptr->ryoute || (empty_hands(creature_ptr, FALSE) == EMPTY_HAND_NONE))
            creature_ptr->riding_ryoute = TRUE;
        else if (creature_ptr->pet_extra_flags & PF_TWO_HANDS) {
            switch (creature_ptr->pclass) {
            case CLASS_MONK:
            case CLASS_FORCETRAINER:
            case CLASS_BERSERKER:
                if ((empty_hands(creature_ptr, FALSE) != EMPTY_HAND_NONE) && !has_melee_weapon(creature_ptr, INVEN_RARM)
                    && !has_melee_weapon(creature_ptr, INVEN_LARM))
                    creature_ptr->riding_ryoute = TRUE;
                break;
            }
        }

        if ((creature_ptr->pclass == CLASS_BEASTMASTER) || (creature_ptr->pclass == CLASS_CAVALRY)) {
            if (creature_ptr->tval_ammo != TV_ARROW)
                penalty = 5;
        } else {
            penalty = r_info[floor_ptr->m_list[creature_ptr->riding].r_idx].level - creature_ptr->skill_exp[GINOU_RIDING] / 80;
            penalty += 30;
            if (penalty < 30)
                penalty = 30;
        }

        if (creature_ptr->tval_ammo == TV_BOLT)
            penalty *= 2;
        creature_ptr->to_h_b -= (s16b)penalty;
        creature_ptr->dis_to_h_b -= (s16b)penalty;
    }

    /* Different calculation for monks with empty hands */
    if (((creature_ptr->pclass == CLASS_MONK) || (creature_ptr->pclass == CLASS_FORCETRAINER) || (creature_ptr->pclass == CLASS_BERSERKER))
        && (empty_hands_status & EMPTY_HAND_RARM) && !creature_ptr->hidarite) {
        int blow_base = creature_ptr->lev + adj_dex_blow[creature_ptr->stat_ind[A_DEX]];
        creature_ptr->num_blow[0] = 0;

        if (creature_ptr->pclass == CLASS_FORCETRAINER) {
            if (blow_base > 18)
                creature_ptr->num_blow[0]++;
            if (blow_base > 31)
                creature_ptr->num_blow[0]++;
            if (blow_base > 44)
                creature_ptr->num_blow[0]++;
            if (blow_base > 58)
                creature_ptr->num_blow[0]++;

            MAGIC_NUM1 current_ki = get_current_ki(creature_ptr);
            if (current_ki != 0) {
                creature_ptr->to_d[0] += current_ki / 5;
                creature_ptr->dis_to_d[0] += current_ki / 5;
            }
        } else {
            if (blow_base > 12)
                creature_ptr->num_blow[0]++;
            if (blow_base > 22)
                creature_ptr->num_blow[0]++;
            if (blow_base > 31)
                creature_ptr->num_blow[0]++;
            if (blow_base > 39)
                creature_ptr->num_blow[0]++;
            if (blow_base > 46)
                creature_ptr->num_blow[0]++;
            if (blow_base > 53)
                creature_ptr->num_blow[0]++;
            if (blow_base > 59)
                creature_ptr->num_blow[0]++;
        }

        if (heavy_armor(creature_ptr) && (creature_ptr->pclass != CLASS_BERSERKER))
            creature_ptr->num_blow[0] /= 2;
        else {
            creature_ptr->to_h[0] += (creature_ptr->lev / 3);
            creature_ptr->dis_to_h[0] += (creature_ptr->lev / 3);

            creature_ptr->to_d[0] += (creature_ptr->lev / 6);
            creature_ptr->dis_to_d[0] += (creature_ptr->lev / 6);
        }

        if (creature_ptr->special_defense & KAMAE_SEIRYU) {
            creature_ptr->resist_acid = TRUE;
            creature_ptr->resist_fire = TRUE;
            creature_ptr->resist_elec = TRUE;
            creature_ptr->resist_cold = TRUE;
            creature_ptr->resist_pois = TRUE;
            creature_ptr->sh_fire = TRUE;
            creature_ptr->sh_elec = TRUE;
            creature_ptr->sh_cold = TRUE;
            creature_ptr->levitation = TRUE;
        } else if (creature_ptr->special_defense & KAMAE_GENBU) {
            creature_ptr->to_a += (creature_ptr->lev * creature_ptr->lev) / 50;
            creature_ptr->dis_to_a += (creature_ptr->lev * creature_ptr->lev) / 50;
            creature_ptr->reflect = TRUE;
            creature_ptr->num_blow[0] -= 2;
            if ((creature_ptr->pclass == CLASS_MONK) && (creature_ptr->lev > 42))
                creature_ptr->num_blow[0]--;
            if (creature_ptr->num_blow[0] < 0)
                creature_ptr->num_blow[0] = 0;
        } else if (creature_ptr->special_defense & KAMAE_SUZAKU) {
            creature_ptr->to_h[0] -= (creature_ptr->lev / 3);
            creature_ptr->to_d[0] -= (creature_ptr->lev / 6);

            creature_ptr->dis_to_h[0] -= (creature_ptr->lev / 3);
            creature_ptr->dis_to_d[0] -= (creature_ptr->lev / 6);
            creature_ptr->num_blow[0] /= 2;
            creature_ptr->levitation = TRUE;
        }

        creature_ptr->num_blow[0] += 1 + creature_ptr->extra_blows[0];
    }

    if (creature_ptr->riding)
        creature_ptr->levitation = riding_levitation;

    creature_ptr->monk_armour_aux = FALSE;

    if (heavy_armor(creature_ptr)) {
        creature_ptr->monk_armour_aux = TRUE;
    }

    for (int i = 0; i < 2; i++) {
        if (!has_melee_weapon(creature_ptr, INVEN_RARM + i))
            continue;

        tval_type tval = creature_ptr->inventory_list[INVEN_RARM + i].tval - TV_WEAPON_BEGIN;
        OBJECT_SUBTYPE_VALUE sval = creature_ptr->inventory_list[INVEN_RARM + i].sval;

        creature_ptr->to_h[i] += (creature_ptr->weapon_exp[tval][sval] - WEAPON_EXP_BEGINNER) / 200;
        creature_ptr->dis_to_h[i] += (creature_ptr->weapon_exp[tval][sval] - WEAPON_EXP_BEGINNER) / 200;
        if ((creature_ptr->pclass == CLASS_MONK) || (creature_ptr->pclass == CLASS_FORCETRAINER)) {
            if (!s_info[creature_ptr->pclass].w_max[tval][sval]) {
                creature_ptr->to_h[i] -= 40;
                creature_ptr->dis_to_h[i] -= 40;
                creature_ptr->icky_wield[i] = TRUE;
            }

            continue;
        }

        if (creature_ptr->pclass != CLASS_NINJA)
            continue;

        if ((s_info[CLASS_NINJA].w_max[tval][sval] > WEAPON_EXP_BEGINNER) && (creature_ptr->inventory_list[INVEN_LARM - i].tval != TV_SHIELD))
            continue;

        creature_ptr->to_h[i] -= 40;
        creature_ptr->dis_to_h[i] -= 40;
        creature_ptr->icky_wield[i] = TRUE;
        creature_ptr->num_blow[i] /= 2;
        if (creature_ptr->num_blow[i] < 1)
            creature_ptr->num_blow[i] = 1;
    }

    calc_speed(creature_ptr);

    if (creature_ptr->pspeed != old_speed) {
        creature_ptr->redraw |= (PR_SPEED);
    }

    if ((creature_ptr->dis_ac != old_dis_ac) || (creature_ptr->dis_to_a != old_dis_to_a)) {
        creature_ptr->redraw |= (PR_ARMOR);
        creature_ptr->window |= (PW_PLAYER);
    }

    if (creature_ptr->ryoute && !omoi) {
        int bonus_to_h = 0, bonus_to_d = 0;
        bonus_to_d = ((int)(adj_str_td[creature_ptr->stat_ind[A_STR]]) - 128) / 2;
        bonus_to_h = ((int)(adj_str_th[creature_ptr->stat_ind[A_STR]]) - 128) + ((int)(adj_dex_th[creature_ptr->stat_ind[A_DEX]]) - 128);

        creature_ptr->to_h[default_hand] += MAX(bonus_to_h, 1);
        creature_ptr->dis_to_h[default_hand] += MAX(bonus_to_h, 1);
        creature_ptr->to_d[default_hand] += MAX(bonus_to_d, 1);
        creature_ptr->dis_to_d[default_hand] += MAX(bonus_to_d, 1);
    }

    bool is_special_class = creature_ptr->pclass == CLASS_MONK;
    is_special_class |= creature_ptr->pclass == CLASS_FORCETRAINER;
    is_special_class |= creature_ptr->pclass == CLASS_BERSERKER;
    if (is_special_class && (empty_hands(creature_ptr, FALSE) == (EMPTY_HAND_RARM | EMPTY_HAND_LARM)))
        creature_ptr->ryoute = FALSE;

    if (creature_ptr->immune_acid)
        creature_ptr->resist_acid = TRUE;
    if (creature_ptr->immune_elec)
        creature_ptr->resist_elec = TRUE;
    if (creature_ptr->immune_fire)
        creature_ptr->resist_fire = TRUE;
    if (creature_ptr->immune_cold)
        creature_ptr->resist_cold = TRUE;

    calc_intra_vision(creature_ptr);
    calc_stealth(creature_ptr);
    calc_disarming(creature_ptr);
    calc_device_ability(creature_ptr);
    calc_saving_throw(creature_ptr);
    calc_search(creature_ptr);
    calc_search_freq(creature_ptr);
    calc_to_hit_melee(creature_ptr);
    calc_to_hit_shoot(creature_ptr);
    calc_to_hit_throw(creature_ptr);
    calc_dig(creature_ptr);

    if (current_world_ptr->character_xtra)
        return;

    if (creature_ptr->old_heavy_shoot != creature_ptr->heavy_shoot) {
        if (creature_ptr->heavy_shoot) {
            msg_print(_("こんな重い弓を装備しているのは大変だ。", "You have trouble wielding such a heavy bow."));
        } else if (creature_ptr->inventory_list[INVEN_BOW].k_idx) {
            msg_print(_("この弓なら装備していても辛くない。", "You have no trouble wielding your bow."));
        } else {
            msg_print(_("重い弓を装備からはずして体が楽になった。", "You feel relieved to put down your heavy bow."));
        }

        creature_ptr->old_heavy_shoot = creature_ptr->heavy_shoot;
    }

    for (int i = 0; i < 2; i++) {
        if (creature_ptr->old_heavy_wield[i] != creature_ptr->heavy_wield[i]) {
            if (creature_ptr->heavy_wield[i]) {
                msg_print(_("こんな重い武器を装備しているのは大変だ。", "You have trouble wielding such a heavy weapon."));
            } else if (has_melee_weapon(creature_ptr, INVEN_RARM + i)) {
                msg_print(_("これなら装備していても辛くない。", "You have no trouble wielding your weapon."));
            } else if (creature_ptr->heavy_wield[1 - i]) {
                msg_print(_("まだ武器が重い。", "You have still trouble wielding a heavy weapon."));
            } else {
                msg_print(_("重い武器を装備からはずして体が楽になった。", "You feel relieved to put down your heavy weapon."));
            }

            creature_ptr->old_heavy_wield[i] = creature_ptr->heavy_wield[i];
        }

        if (creature_ptr->old_riding_wield[i] != creature_ptr->riding_wield[i]) {
            if (creature_ptr->riding_wield[i]) {
                msg_print(_("この武器は乗馬中に使うにはむかないようだ。", "This weapon is not suitable for use while riding."));
            } else if (!creature_ptr->riding) {
                msg_print(_("この武器は徒歩で使いやすい。", "This weapon is suitable for use on foot."));
            } else if (has_melee_weapon(creature_ptr, INVEN_RARM + i)) {
                msg_print(_("これなら乗馬中にぴったりだ。", "This weapon is suitable for use while riding."));
            }

            creature_ptr->old_riding_wield[i] = creature_ptr->riding_wield[i];
        }

        if (creature_ptr->old_icky_wield[i] == creature_ptr->icky_wield[i])
            continue;

        if (creature_ptr->icky_wield[i]) {
            msg_print(_("今の装備はどうも自分にふさわしくない気がする。", "You do not feel comfortable with your weapon."));
            if (current_world_ptr->is_loading_now) {
                chg_virtue(creature_ptr, V_FAITH, -1);
            }
        } else if (has_melee_weapon(creature_ptr, INVEN_RARM + i)) {
            msg_print(_("今の装備は自分にふさわしい気がする。", "You feel comfortable with your weapon."));
        } else {
            msg_print(_("装備をはずしたら随分と気が楽になった。", "You feel more comfortable after removing your weapon."));
        }

        creature_ptr->old_icky_wield[i] = creature_ptr->icky_wield[i];
    }

    if (creature_ptr->riding && (creature_ptr->old_riding_ryoute != creature_ptr->riding_ryoute)) {
        if (creature_ptr->riding_ryoute) {
#ifdef JP
            msg_format("%s馬を操れない。", (empty_hands(creature_ptr, FALSE) == EMPTY_HAND_NONE) ? "両手がふさがっていて" : "");
#else
            msg_print("You are using both hand for fighting, and you can't control the pet you're riding.");
#endif
        } else {
#ifdef JP
            msg_format("%s馬を操れるようになった。", (empty_hands(creature_ptr, FALSE) == EMPTY_HAND_NONE) ? "手が空いて" : "");
#else
            msg_print("You began to control the pet you're riding with one hand.");
#endif
        }

        creature_ptr->old_riding_ryoute = creature_ptr->riding_ryoute;
    }

    if (((creature_ptr->pclass == CLASS_MONK) || (creature_ptr->pclass == CLASS_FORCETRAINER) || (creature_ptr->pclass == CLASS_NINJA))
        && (creature_ptr->monk_armour_aux != creature_ptr->monk_notify_aux)) {
        if (heavy_armor(creature_ptr)) {
            msg_print(_("装備が重くてバランスを取れない。", "The weight of your armor disrupts your balance."));
            if (current_world_ptr->is_loading_now) {
                chg_virtue(creature_ptr, V_HARMONY, -1);
            }
        } else {
            msg_print(_("バランスがとれるようになった。", "You regain your balance."));
        }

        creature_ptr->monk_notify_aux = creature_ptr->monk_armour_aux;
    }

    for (int i = 0; i < INVEN_PACK; i++) {
        if ((creature_ptr->inventory_list[i].tval == TV_NATURE_BOOK) && (creature_ptr->inventory_list[i].sval == 2))
            have_sw = TRUE;
        if ((creature_ptr->inventory_list[i].tval == TV_CRAFT_BOOK) && (creature_ptr->inventory_list[i].sval == 2))
            have_kabe = TRUE;
    }

    for (this_o_idx = floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].o_idx; this_o_idx; this_o_idx = next_o_idx) {
        o_ptr = &floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;

        if ((o_ptr->tval == TV_NATURE_BOOK) && (o_ptr->sval == 2))
            have_sw = TRUE;
        if ((o_ptr->tval == TV_CRAFT_BOOK) && (o_ptr->sval == 2))
            have_kabe = TRUE;
    }

    if (creature_ptr->pass_wall && !creature_ptr->kill_wall)
        creature_ptr->no_flowed = TRUE;

    if (have_sw && ((creature_ptr->realm1 == REALM_NATURE) || (creature_ptr->realm2 == REALM_NATURE) || (creature_ptr->pclass == CLASS_SORCERER))) {
        const magic_type *s_ptr = &mp_ptr->info[REALM_NATURE - 1][SPELL_SW];
        if (creature_ptr->lev >= s_ptr->slevel)
            creature_ptr->no_flowed = TRUE;
    }

    if (have_kabe && ((creature_ptr->realm1 == REALM_CRAFT) || (creature_ptr->realm2 == REALM_CRAFT) || (creature_ptr->pclass == CLASS_SORCERER))) {
        const magic_type *s_ptr = &mp_ptr->info[REALM_CRAFT - 1][SPELL_KABE];
        if (creature_ptr->lev >= s_ptr->slevel)
            creature_ptr->no_flowed = TRUE;
    }
}

static void calc_alignment(player_type *creature_ptr)
{
    creature_ptr->align = 0;
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    for (MONSTER_IDX m_idx = floor_ptr->m_max - 1; m_idx >= 1; m_idx--) {
        monster_type *m_ptr;
        monster_race *r_ptr;
        m_ptr = &floor_ptr->m_list[m_idx];
        if (!monster_is_valid(m_ptr))
            continue;
        r_ptr = &r_info[m_ptr->r_idx];

        if (!is_pet(m_ptr))
            continue;

        if (r_ptr->flags3 & RF3_GOOD)
            creature_ptr->align += r_ptr->level;
        if (r_ptr->flags3 & RF3_EVIL)
            creature_ptr->align -= r_ptr->level;
    }

    if (creature_ptr->mimic_form) {
        switch (creature_ptr->mimic_form) {
        case MIMIC_DEMON:
            creature_ptr->align -= 200;
            break;
        case MIMIC_DEMON_LORD:
            creature_ptr->align -= 200;
            break;
        }
    } else {
        switch (creature_ptr->prace) {
        case RACE_ARCHON:
            creature_ptr->align += 200;
            break;
        case RACE_BALROG:
            creature_ptr->align -= 200;
            break;
        }
    }

    for (int i = 0; i < 2; i++) {
        if (!has_melee_weapon(creature_ptr, INVEN_RARM + i))
            continue;
        if (creature_ptr->inventory_list[INVEN_RARM + i].name1 != ART_IRON_BALL)
            continue;
        creature_ptr->align -= 1000;
    }

    int j = 0;
    int neutral[2];
    for (int i = 0; i < 8; i++) {
        switch (creature_ptr->vir_types[i]) {
        case V_JUSTICE:
            creature_ptr->align += creature_ptr->virtues[i] * 2;
            break;
        case V_CHANCE:
            break;
        case V_NATURE:
        case V_HARMONY:
            neutral[j++] = i;
            break;
        case V_UNLIFE:
            creature_ptr->align -= creature_ptr->virtues[i];
            break;
        default:
            creature_ptr->align += creature_ptr->virtues[i];
            break;
        }
    }

    for (int i = 0; i < j; i++) {
        if (creature_ptr->align > 0) {
            creature_ptr->align -= creature_ptr->virtues[neutral[i]] / 2;
            if (creature_ptr->align < 0)
                creature_ptr->align = 0;
        } else if (creature_ptr->align < 0) {
            creature_ptr->align += creature_ptr->virtues[neutral[i]] / 2;
            if (creature_ptr->align > 0)
                creature_ptr->align = 0;
        }
    }
}

/*!
 * @brief プレイヤーの最大HPを計算する /
 * Calculate the players (maximal) hit points
 * Adjust current hitpoints if necessary
 * @return なし
 * @details
 */
static void calc_hitpoints(player_type *creature_ptr)
{
    int bonus = ((int)(adj_con_mhp[creature_ptr->stat_ind[A_CON]]) - 128) * creature_ptr->lev / 4;
    int mhp = creature_ptr->player_hp[creature_ptr->lev - 1];

    byte tmp_hitdie;
    if (creature_ptr->mimic_form) {
        if (creature_ptr->pclass == CLASS_SORCERER)
            tmp_hitdie = mimic_info[creature_ptr->mimic_form].r_mhp / 2 + cp_ptr->c_mhp + ap_ptr->a_mhp;
        else
            tmp_hitdie = mimic_info[creature_ptr->mimic_form].r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;
        mhp = mhp * tmp_hitdie / creature_ptr->hitdie;
    }

    if (creature_ptr->pclass == CLASS_SORCERER) {
        if (creature_ptr->lev < 30)
            mhp = (mhp * (45 + creature_ptr->lev) / 100);
        else
            mhp = (mhp * 75 / 100);
        bonus = (bonus * 65 / 100);
    }

    mhp += bonus;

    if (creature_ptr->pclass == CLASS_BERSERKER) {
        mhp = mhp * (110 + (((creature_ptr->lev + 40) * (creature_ptr->lev + 40) - 1550) / 110)) / 100;
    }

    if (mhp < creature_ptr->lev + 1)
        mhp = creature_ptr->lev + 1;
    if (IS_HERO(creature_ptr))
        mhp += 10;
    if (creature_ptr->shero && (creature_ptr->pclass != CLASS_BERSERKER))
        mhp += 30;
    if (creature_ptr->tsuyoshi)
        mhp += 50;
    if (hex_spelling(creature_ptr, HEX_XTRA_MIGHT))
        mhp += 15;
    if (hex_spelling(creature_ptr, HEX_BUILDING))
        mhp += 60;
    if (creature_ptr->mhp == mhp)
        return;

    if (creature_ptr->chp >= mhp) {
        creature_ptr->chp = mhp;
        creature_ptr->chp_frac = 0;
    }

#ifdef JP
    if (creature_ptr->level_up_message && (mhp > creature_ptr->mhp)) {
        msg_format("最大ヒット・ポイントが %d 増加した！", (mhp - creature_ptr->mhp));
    }
#endif
    creature_ptr->mhp = mhp;

    creature_ptr->redraw |= PR_HP;
    creature_ptr->window |= PW_PLAYER;
}

/*!
 * @brief プレイヤーの光源半径を計算する / Extract and set the current "lite radius"
 * @return なし
 * @details
 * SWD: Experimental modification: multiple light sources have additive effect.
 */
static void calc_torch(player_type *creature_ptr)
{
    creature_ptr->cur_lite = 0;
    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        if (o_ptr->name2 == EGO_LITE_SHINE)
            creature_ptr->cur_lite++;
        if (o_ptr->name2 != EGO_LITE_DARKNESS) {
            if (o_ptr->tval == TV_LITE) {
                if ((o_ptr->sval == SV_LITE_TORCH) && !(o_ptr->xtra4 > 0))
                    continue;
                if ((o_ptr->sval == SV_LITE_LANTERN) && !(o_ptr->xtra4 > 0))
                    continue;
            }
        }

        BIT_FLAGS flgs[TR_FLAG_SIZE];
        object_flags(o_ptr, flgs);

        POSITION rad = 0;
        if (have_flag(flgs, TR_LITE_1) && o_ptr->name2 != EGO_LITE_DARKNESS)
            rad += 1;
        if (have_flag(flgs, TR_LITE_2) && o_ptr->name2 != EGO_LITE_DARKNESS)
            rad += 2;
        if (have_flag(flgs, TR_LITE_3) && o_ptr->name2 != EGO_LITE_DARKNESS)
            rad += 3;
        if (have_flag(flgs, TR_LITE_M1))
            rad -= 1;
        if (have_flag(flgs, TR_LITE_M2))
            rad -= 2;
        if (have_flag(flgs, TR_LITE_M3))
            rad -= 3;
        creature_ptr->cur_lite += rad;
    }

    if (d_info[creature_ptr->dungeon_idx].flags1 & DF1_DARKNESS && creature_ptr->cur_lite > 1)
        creature_ptr->cur_lite = 1;

    if (creature_ptr->cur_lite <= 0 && creature_ptr->lite)
        creature_ptr->cur_lite++;

    if (creature_ptr->cur_lite > 14)
        creature_ptr->cur_lite = 14;

    if (creature_ptr->cur_lite < 0)
        creature_ptr->cur_lite = 0;

    if (creature_ptr->old_lite == creature_ptr->cur_lite)
        return;

    creature_ptr->update |= (PU_LITE | PU_MON_LITE | PU_MONSTERS);
    creature_ptr->old_lite = creature_ptr->cur_lite;

    if ((creature_ptr->cur_lite > 0) && (creature_ptr->special_defense & NINJA_S_STEALTH))
        set_superstealth(creature_ptr, FALSE);
}

/*!
 * @brief プレイヤーの現在学習可能な魔法数を計算し、増減に応じて魔法の忘却、再学習を処置する。 /
 * Calculate number of spells player should have, and forget,
 * or remember, spells until that number is properly reflected.
 * @return なし
 * @details
 * Note that this function induces various "status" messages,
 * which must be bypasses until the character is created.
 */
static void calc_spells(player_type *creature_ptr)
{
    if (!mp_ptr->spell_book)
        return;
    if (!current_world_ptr->character_generated)
        return;
    if (current_world_ptr->character_xtra)
        return;
    if ((creature_ptr->pclass == CLASS_SORCERER) || (creature_ptr->pclass == CLASS_RED_MAGE)) {
        creature_ptr->new_spells = 0;
        return;
    }

    concptr p = spell_category_name(mp_ptr->spell_book);
    int levels = creature_ptr->lev - mp_ptr->spell_first + 1;
    if (levels < 0)
        levels = 0;

    int num_allowed = (adj_mag_study[creature_ptr->stat_ind[mp_ptr->spell_stat]] * levels / 2);
    int bonus = 0;
    if ((creature_ptr->pclass != CLASS_SAMURAI) && (mp_ptr->spell_book != TV_LIFE_BOOK)) {
        bonus = 4;
    }

    if (creature_ptr->pclass == CLASS_SAMURAI) {
        num_allowed = 32;
    } else if (creature_ptr->realm2 == REALM_NONE) {
        num_allowed = (num_allowed + 1) / 2;
        if (num_allowed > (32 + bonus))
            num_allowed = 32 + bonus;
    } else if ((creature_ptr->pclass == CLASS_MAGE) || (creature_ptr->pclass == CLASS_PRIEST)) {
        if (num_allowed > (96 + bonus))
            num_allowed = 96 + bonus;
    } else {
        if (num_allowed > (80 + bonus))
            num_allowed = 80 + bonus;
    }

    int num_boukyaku = 0;
    for (int j = 0; j < 64; j++) {
        if ((j < 32) ? (creature_ptr->spell_forgotten1 & (1L << j)) : (creature_ptr->spell_forgotten2 & (1L << (j - 32)))) {
            num_boukyaku++;
        }
    }

    creature_ptr->new_spells = num_allowed + creature_ptr->add_spells + num_boukyaku - creature_ptr->learned_spells;
    for (int i = 63; i >= 0; i--) {
        if (!creature_ptr->spell_learned1 && !creature_ptr->spell_learned2)
            break;

        int j = creature_ptr->spell_order[i];
        if (j >= 99)
            continue;

        const magic_type *s_ptr;
        if (!is_magic((j < 32) ? creature_ptr->realm1 : creature_ptr->realm2)) {
            if (j < 32)
                s_ptr = &technic_info[creature_ptr->realm1 - MIN_TECHNIC][j];
            else
                s_ptr = &technic_info[creature_ptr->realm2 - MIN_TECHNIC][j % 32];
        } else if (j < 32)
            s_ptr = &mp_ptr->info[creature_ptr->realm1 - 1][j];
        else
            s_ptr = &mp_ptr->info[creature_ptr->realm2 - 1][j % 32];

        if (s_ptr->slevel <= creature_ptr->lev)
            continue;

        bool is_spell_learned = (j < 32) ? (creature_ptr->spell_learned1 & (1L << j)) : (creature_ptr->spell_learned2 & (1L << (j - 32)));
        if (!is_spell_learned)
            continue;

        REALM_IDX which;
        if (j < 32) {
            creature_ptr->spell_forgotten1 |= (1L << j);
            which = creature_ptr->realm1;
        } else {
            creature_ptr->spell_forgotten2 |= (1L << (j - 32));
            which = creature_ptr->realm2;
        }

        if (j < 32) {
            creature_ptr->spell_learned1 &= ~(1L << j);
            which = creature_ptr->realm1;
        } else {
            creature_ptr->spell_learned2 &= ~(1L << (j - 32));
            which = creature_ptr->realm2;
        }

#ifdef JP
        msg_format("%sの%sを忘れてしまった。", exe_spell(creature_ptr, which, j % 32, SPELL_NAME), p);
#else
        msg_format("You have forgotten the %s of %s.", p, exe_spell(creature_ptr, which, j % 32, SPELL_NAME));
#endif
        creature_ptr->new_spells++;
    }

    /* Forget spells if we know too many spells */
    for (int i = 63; i >= 0; i--) {
        if (creature_ptr->new_spells >= 0)
            break;
        if (!creature_ptr->spell_learned1 && !creature_ptr->spell_learned2)
            break;

        int j = creature_ptr->spell_order[i];
        if (j >= 99)
            continue;

        bool is_spell_learned = (j < 32) ? (creature_ptr->spell_learned1 & (1L << j)) : (creature_ptr->spell_learned2 & (1L << (j - 32)));
        if (!is_spell_learned)
            continue;

        REALM_IDX which;
        if (j < 32) {
            creature_ptr->spell_forgotten1 |= (1L << j);
            which = creature_ptr->realm1;
        } else {
            creature_ptr->spell_forgotten2 |= (1L << (j - 32));
            which = creature_ptr->realm2;
        }

        if (j < 32) {
            creature_ptr->spell_learned1 &= ~(1L << j);
            which = creature_ptr->realm1;
        } else {
            creature_ptr->spell_learned2 &= ~(1L << (j - 32));
            which = creature_ptr->realm2;
        }

#ifdef JP
        msg_format("%sの%sを忘れてしまった。", exe_spell(creature_ptr, which, j % 32, SPELL_NAME), p);
#else
        msg_format("You have forgotten the %s of %s.", p, exe_spell(creature_ptr, which, j % 32, SPELL_NAME));
#endif
        creature_ptr->new_spells++;
    }

    /* Check for spells to remember */
    for (int i = 0; i < 64; i++) {
        if (creature_ptr->new_spells <= 0)
            break;
        if (!creature_ptr->spell_forgotten1 && !creature_ptr->spell_forgotten2)
            break;
        int j = creature_ptr->spell_order[i];
        if (j >= 99)
            break;

        const magic_type *s_ptr;
        if (!is_magic((j < 32) ? creature_ptr->realm1 : creature_ptr->realm2)) {
            if (j < 32)
                s_ptr = &technic_info[creature_ptr->realm1 - MIN_TECHNIC][j];
            else
                s_ptr = &technic_info[creature_ptr->realm2 - MIN_TECHNIC][j % 32];
        } else if (j < 32)
            s_ptr = &mp_ptr->info[creature_ptr->realm1 - 1][j];
        else
            s_ptr = &mp_ptr->info[creature_ptr->realm2 - 1][j % 32];

        if (s_ptr->slevel > creature_ptr->lev)
            continue;

        bool is_spell_learned = (j < 32) ? (creature_ptr->spell_forgotten1 & (1L << j)) : (creature_ptr->spell_forgotten2 & (1L << (j - 32)));
        if (!is_spell_learned)
            continue;

        REALM_IDX which;
        if (j < 32) {
            creature_ptr->spell_forgotten1 &= ~(1L << j);
            which = creature_ptr->realm1;
        } else {
            creature_ptr->spell_forgotten2 &= ~(1L << (j - 32));
            which = creature_ptr->realm2;
        }

        if (j < 32) {
            creature_ptr->spell_learned1 |= (1L << j);
            which = creature_ptr->realm1;
        } else {
            creature_ptr->spell_learned2 |= (1L << (j - 32));
            which = creature_ptr->realm2;
        }

#ifdef JP
        msg_format("%sの%sを思い出した。", exe_spell(creature_ptr, which, j % 32, SPELL_NAME), p);
#else
        msg_format("You have remembered the %s of %s.", p, exe_spell(creature_ptr, which, j % 32, SPELL_NAME));
#endif
        creature_ptr->new_spells--;
    }

    if (creature_ptr->realm2 == REALM_NONE) {
        int k = 0;
        for (int j = 0; j < 32; j++) {
            const magic_type *s_ptr;
            if (!is_magic(creature_ptr->realm1))
                s_ptr = &technic_info[creature_ptr->realm1 - MIN_TECHNIC][j];
            else
                s_ptr = &mp_ptr->info[creature_ptr->realm1 - 1][j];

            if (s_ptr->slevel > creature_ptr->lev)
                continue;

            if (creature_ptr->spell_learned1 & (1L << j)) {
                continue;
            }

            k++;
        }

        if (k > 32)
            k = 32;
        if ((creature_ptr->new_spells > k) && ((mp_ptr->spell_book == TV_LIFE_BOOK) || (mp_ptr->spell_book == TV_HISSATSU_BOOK))) {
            creature_ptr->new_spells = (s16b)k;
        }
    }

    if (creature_ptr->new_spells < 0)
        creature_ptr->new_spells = 0;

    if (creature_ptr->old_spells == creature_ptr->new_spells)
        return;

    if (creature_ptr->new_spells) {
#ifdef JP
        if (creature_ptr->new_spells < 10) {
            msg_format("あと %d つの%sを学べる。", creature_ptr->new_spells, p);
        } else {
            msg_format("あと %d 個の%sを学べる。", creature_ptr->new_spells, p);
        }
#else
        msg_format("You can learn %d more %s%s.", creature_ptr->new_spells, p, (creature_ptr->new_spells != 1) ? "s" : "");
#endif
    }

    creature_ptr->old_spells = creature_ptr->new_spells;
    creature_ptr->redraw |= PR_STUDY;
    creature_ptr->window |= PW_OBJECT;
}

/*!
 * @brief プレイヤーの最大MPを計算する /
 * Calculate maximum mana.  You do not need to know any spells.
 * Note that mana is lowered by heavy (or inappropriate) armor.
 * @return なし
 * @details
 * This function induces status messages.
 */
static void calc_mana(player_type *creature_ptr)
{
    if (!mp_ptr->spell_book)
        return;

    int levels;
    if ((creature_ptr->pclass == CLASS_MINDCRAFTER) || (creature_ptr->pclass == CLASS_MIRROR_MASTER) || (creature_ptr->pclass == CLASS_BLUE_MAGE)) {
        levels = creature_ptr->lev;
    } else {
        if (mp_ptr->spell_first > creature_ptr->lev) {
            creature_ptr->msp = 0;
            creature_ptr->redraw |= (PR_MANA);
            return;
        }

        levels = (creature_ptr->lev - mp_ptr->spell_first) + 1;
    }

    int msp;
    if (creature_ptr->pclass == CLASS_SAMURAI) {
        msp = (adj_mag_mana[creature_ptr->stat_ind[mp_ptr->spell_stat]] + 10) * 2;
        if (msp)
            msp += (msp * rp_ptr->r_adj[mp_ptr->spell_stat] / 20);
    } else {
        msp = adj_mag_mana[creature_ptr->stat_ind[mp_ptr->spell_stat]] * (levels + 3) / 4;
        if (msp)
            msp++;
        if (msp)
            msp += (msp * rp_ptr->r_adj[mp_ptr->spell_stat] / 20);
        if (msp && (creature_ptr->pseikaku == PERSONALITY_MUNCHKIN))
            msp += msp / 2;
        if (msp && (creature_ptr->pclass == CLASS_HIGH_MAGE))
            msp += msp / 4;
        if (msp && (creature_ptr->pclass == CLASS_SORCERER))
            msp += msp * (25 + creature_ptr->lev) / 100;
    }

    if (mp_ptr->spell_xtra & MAGIC_GLOVE_REDUCE_MANA) {
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        creature_ptr->cumber_glove = FALSE;
        object_type *o_ptr;
        o_ptr = &creature_ptr->inventory_list[INVEN_HANDS];
        object_flags(o_ptr, flgs);
        if (o_ptr->k_idx && !(have_flag(flgs, TR_FREE_ACT)) && !(have_flag(flgs, TR_DEC_MANA)) && !(have_flag(flgs, TR_EASY_SPELL))
            && !((have_flag(flgs, TR_MAGIC_MASTERY)) && (o_ptr->pval > 0)) && !((have_flag(flgs, TR_DEX)) && (o_ptr->pval > 0))) {
            creature_ptr->cumber_glove = TRUE;
            msp = (3 * msp) / 4;
        }
    }

    creature_ptr->cumber_armor = FALSE;

    int cur_wgt = 0;
    if (creature_ptr->inventory_list[INVEN_RARM].tval > TV_SWORD)
        cur_wgt += creature_ptr->inventory_list[INVEN_RARM].weight;
    if (creature_ptr->inventory_list[INVEN_LARM].tval > TV_SWORD)
        cur_wgt += creature_ptr->inventory_list[INVEN_LARM].weight;
    cur_wgt += creature_ptr->inventory_list[INVEN_BODY].weight;
    cur_wgt += creature_ptr->inventory_list[INVEN_HEAD].weight;
    cur_wgt += creature_ptr->inventory_list[INVEN_OUTER].weight;
    cur_wgt += creature_ptr->inventory_list[INVEN_HANDS].weight;
    cur_wgt += creature_ptr->inventory_list[INVEN_FEET].weight;

    switch (creature_ptr->pclass) {
    case CLASS_MAGE:
    case CLASS_HIGH_MAGE:
    case CLASS_BLUE_MAGE:
    case CLASS_MONK:
    case CLASS_FORCETRAINER:
    case CLASS_SORCERER: {
        if (creature_ptr->inventory_list[INVEN_RARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_RARM].weight;
        if (creature_ptr->inventory_list[INVEN_LARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_LARM].weight;
        break;
    }
    case CLASS_PRIEST:
    case CLASS_BARD:
    case CLASS_TOURIST: {
        if (creature_ptr->inventory_list[INVEN_RARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_RARM].weight * 2 / 3;
        if (creature_ptr->inventory_list[INVEN_LARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_LARM].weight * 2 / 3;
        break;
    }
    case CLASS_MINDCRAFTER:
    case CLASS_BEASTMASTER:
    case CLASS_MIRROR_MASTER: {
        if (creature_ptr->inventory_list[INVEN_RARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_RARM].weight / 2;
        if (creature_ptr->inventory_list[INVEN_LARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_LARM].weight / 2;
        break;
    }
    case CLASS_ROGUE:
    case CLASS_RANGER:
    case CLASS_RED_MAGE:
    case CLASS_WARRIOR_MAGE: {
        if (creature_ptr->inventory_list[INVEN_RARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_RARM].weight / 3;
        if (creature_ptr->inventory_list[INVEN_LARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_LARM].weight / 3;
        break;
    }
    case CLASS_PALADIN:
    case CLASS_CHAOS_WARRIOR: {
        if (creature_ptr->inventory_list[INVEN_RARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_RARM].weight / 5;
        if (creature_ptr->inventory_list[INVEN_LARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_LARM].weight / 5;
        break;
    }
    default: {
        break;
    }
    }

    int max_wgt = mp_ptr->spell_weight;
    if ((cur_wgt - max_wgt) > 0) {
        creature_ptr->cumber_armor = TRUE;
        switch (creature_ptr->pclass) {
        case CLASS_MAGE:
        case CLASS_HIGH_MAGE:
        case CLASS_BLUE_MAGE: {
            msp -= msp * (cur_wgt - max_wgt) / 600;
            break;
        }
        case CLASS_PRIEST:
        case CLASS_MINDCRAFTER:
        case CLASS_BEASTMASTER:
        case CLASS_BARD:
        case CLASS_FORCETRAINER:
        case CLASS_TOURIST:
        case CLASS_MIRROR_MASTER: {
            msp -= msp * (cur_wgt - max_wgt) / 800;
            break;
        }
        case CLASS_SORCERER: {
            msp -= msp * (cur_wgt - max_wgt) / 900;
            break;
        }
        case CLASS_ROGUE:
        case CLASS_RANGER:
        case CLASS_MONK:
        case CLASS_RED_MAGE: {
            msp -= msp * (cur_wgt - max_wgt) / 1000;
            break;
        }
        case CLASS_PALADIN:
        case CLASS_CHAOS_WARRIOR:
        case CLASS_WARRIOR_MAGE: {
            msp -= msp * (cur_wgt - max_wgt) / 1200;
            break;
        }
        case CLASS_SAMURAI: {
            creature_ptr->cumber_armor = FALSE;
            break;
        }
        default: {
            msp -= msp * (cur_wgt - max_wgt) / 800;
            break;
        }
        }
    }

    if (msp < 0)
        msp = 0;

    if (creature_ptr->msp != msp) {
        if ((creature_ptr->csp >= msp) && (creature_ptr->pclass != CLASS_SAMURAI)) {
            creature_ptr->csp = msp;
            creature_ptr->csp_frac = 0;
        }

#ifdef JP
        if (creature_ptr->level_up_message && (msp > creature_ptr->msp)) {
            msg_format("最大マジック・ポイントが %d 増加した！", (msp - creature_ptr->msp));
        }
#endif
        creature_ptr->msp = msp;
        creature_ptr->redraw |= (PR_MANA);
        creature_ptr->window |= (PW_PLAYER | PW_SPELL);
    }

    if (current_world_ptr->character_xtra)
        return;

    if (creature_ptr->old_cumber_glove != creature_ptr->cumber_glove) {
        if (creature_ptr->cumber_glove)
            msg_print(_("手が覆われて呪文が唱えにくい感じがする。", "Your covered hands feel unsuitable for spellcasting."));
        else
            msg_print(_("この手の状態なら、ぐっと呪文が唱えやすい感じだ。", "Your hands feel more suitable for spellcasting."));

        creature_ptr->old_cumber_glove = creature_ptr->cumber_glove;
    }

    if (creature_ptr->old_cumber_armor == creature_ptr->cumber_armor)
        return;

    if (creature_ptr->cumber_armor)
        msg_print(_("装備の重さで動きが鈍くなってしまっている。", "The weight of your equipment encumbers your movement."));
    else
        msg_print(_("ぐっと楽に体を動かせるようになった。", "You feel able to move more freely."));

    creature_ptr->old_cumber_armor = creature_ptr->cumber_armor;
}

/*!
 * @brief 装備中の射撃武器の威力倍率を返す /
 * calcurate the fire rate of target object
 * @param o_ptr 計算する射撃武器のアイテム情報参照ポインタ
 * @return 射撃倍率の値(100で1.00倍)
 */
s16b calc_num_fire(player_type *creature_ptr, object_type *o_ptr)
{
    int extra_shots = 0;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *q_ptr;
        q_ptr = &creature_ptr->inventory_list[i];
        if (!q_ptr->k_idx)
            continue;

        if (i == INVEN_BOW)
            continue;

        object_flags(q_ptr, flgs);
        if (have_flag(flgs, TR_XTRA_SHOTS))
            extra_shots++;
    }

    object_flags(o_ptr, flgs);
    if (have_flag(flgs, TR_XTRA_SHOTS))
        extra_shots++;

    int num = 0;
    if (o_ptr->k_idx == 0 || is_heavy_shoot(creature_ptr, o_ptr))
        return (s16b)num;

    num = 100;
    num += (extra_shots * 100);

    tval_type tval_ammo = bow_tval_ammo(o_ptr);
    if ((creature_ptr->pclass == CLASS_RANGER) && (tval_ammo == TV_ARROW)) {
        num += (creature_ptr->lev * 4);
    }

    if ((creature_ptr->pclass == CLASS_CAVALRY) && (tval_ammo == TV_ARROW)) {
        num += (creature_ptr->lev * 3);
    }

    if (creature_ptr->pclass == CLASS_ARCHER) {
        if (tval_ammo == TV_ARROW)
            num += ((creature_ptr->lev * 5) + 50);
        else if ((tval_ammo == TV_BOLT) || (tval_ammo == TV_SHOT))
            num += (creature_ptr->lev * 4);
    }

    if (creature_ptr->pclass == CLASS_WARRIOR && (tval_ammo <= TV_BOLT) && (tval_ammo >= TV_SHOT)) {
        num += (creature_ptr->lev * 2);
    }

    if ((creature_ptr->pclass == CLASS_ROGUE) && (tval_ammo == TV_SHOT)) {
        num += (creature_ptr->lev * 4);
    }

    return (s16b)num;
}

/*!
 * @brief プレイヤーの赤外線視力値を計算する
 * @return なし
 * @details
 * This function induces status messages.
 */
static void calc_intra_vision(player_type *creature_ptr)
{

    const player_race *tmp_rp_ptr;

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];

    creature_ptr->see_infra = tmp_rp_ptr->infra;

    if (creature_ptr->muta3 & MUT3_INFRAVIS) {
        creature_ptr->see_infra += 3;
    }

    if (creature_ptr->tim_infra) {
        creature_ptr->see_infra += 3;
    }
}

/*!
 * @brief プレイヤーの隠密値を計算する
 * @return なし
 * @details
 * This function induces status messages.
 */
static void calc_stealth(player_type *creature_ptr)
{
    const player_race *tmp_rp_ptr;

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];

    creature_ptr->skill_stl = tmp_rp_ptr->r_stl + c_ptr->c_stl + a_ptr->a_stl;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(o_ptr, flgs);
        if (have_flag(flgs, TR_STEALTH))
            creature_ptr->skill_stl += o_ptr->pval;
    }

    if (creature_ptr->muta3 & MUT3_XTRA_NOIS) {
        creature_ptr->skill_stl -= 3;
    }
    if (creature_ptr->muta3 & MUT3_MOTION) {
        creature_ptr->skill_stl += 1;
    }
    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling_any(creature_ptr))
            creature_ptr->skill_stl -= (1 + casting_hex_num(creature_ptr));
    }
    creature_ptr->skill_stl += 1;
    creature_ptr->skill_stl += (c_ptr->x_stl * creature_ptr->lev / 10);
    if ((is_specific_player_race(creature_ptr, RACE_S_FAIRY)) && (creature_ptr->pseikaku != PERSONALITY_SEXY) && (creature_ptr->cursed & TRC_AGGRAVATE)) {
        creature_ptr->cursed &= ~(TRC_AGGRAVATE);
        creature_ptr->skill_stl = MIN(creature_ptr->skill_stl - 3, (creature_ptr->skill_stl + 2) / 2);
    }

    if (creature_ptr->shero) {
        creature_ptr->skill_stl -= 7;
    }

    if (is_time_limit_stealth(creature_ptr))
        creature_ptr->skill_stl += 99;

    if (creature_ptr->skill_stl > 30)
        creature_ptr->skill_stl = 30;
    if (creature_ptr->skill_stl < 0)
        creature_ptr->skill_stl = 0;
}

/*!
 * @brief プレイヤーの解除能力値を計算する
 * @return なし
 * @details
 * This function induces status messages.
 */
static void calc_disarming(player_type *creature_ptr)
{
    const player_race *tmp_rp_ptr;

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];

    creature_ptr->skill_dis = tmp_rp_ptr->r_dis + c_ptr->c_dis + a_ptr->a_dis;
    creature_ptr->skill_dis += adj_dex_dis[creature_ptr->stat_ind[A_DEX]];
    creature_ptr->skill_dis += adj_int_dis[creature_ptr->stat_ind[A_INT]];
    creature_ptr->skill_dis += ((cp_ptr->x_dis * creature_ptr->lev / 10) + (ap_ptr->a_dis * creature_ptr->lev / 50));
}

/*!
 * @brief プレイヤーの魔道具使用能力値を計算する
 * @return なし
 * @details
 * This function induces status messages.
 */
static void calc_device_ability(player_type *creature_ptr)
{
    const player_race *tmp_rp_ptr;

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];

    creature_ptr->skill_dev = tmp_rp_ptr->r_dev + c_ptr->c_dev + a_ptr->a_dev;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(o_ptr, flgs);
        if (have_flag(flgs, TR_MAGIC_MASTERY))
            creature_ptr->skill_dev += 8 * o_ptr->pval;
    }

    creature_ptr->skill_dev += adj_int_dev[creature_ptr->stat_ind[A_INT]];
    creature_ptr->skill_dev += ((cp_ptr->x_dev * creature_ptr->lev / 10) + (ap_ptr->a_dev * creature_ptr->lev / 50));

    if (creature_ptr->shero) {
        creature_ptr->skill_dev -= 20;
    }
}

static void calc_saving_throw(player_type *creature_ptr)
{
    const player_race *tmp_rp_ptr;

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];

    creature_ptr->skill_sav = tmp_rp_ptr->r_sav + c_ptr->c_sav + a_ptr->a_sav;
    if (creature_ptr->shero)
        creature_ptr->skill_sav -= 30;
    if (creature_ptr->muta3 & MUT3_MAGIC_RES)
        creature_ptr->skill_sav += (15 + (creature_ptr->lev / 5));
    creature_ptr->skill_sav += adj_wis_sav[creature_ptr->stat_ind[A_WIS]];
    creature_ptr->skill_sav += ((cp_ptr->x_sav * creature_ptr->lev / 10) + (ap_ptr->a_sav * creature_ptr->lev / 50));
    if (creature_ptr->anti_magic && (creature_ptr->skill_sav < (90 + creature_ptr->lev)))
        creature_ptr->skill_sav = 90 + creature_ptr->lev;

    if (creature_ptr->tsubureru)
        creature_ptr->skill_sav = 10;

    if ((creature_ptr->ult_res || creature_ptr->resist_magic || creature_ptr->magicdef) && (creature_ptr->skill_sav < (95 + creature_ptr->lev)))
        creature_ptr->skill_sav = 95 + creature_ptr->lev;

    if (creature_ptr->down_saving)
        creature_ptr->skill_sav /= 2;
}

static void calc_search(player_type *creature_ptr)
{
    const player_race *tmp_rp_ptr;

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];

    creature_ptr->skill_srh = tmp_rp_ptr->r_srh + c_ptr->c_srh + a_ptr->a_srh;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(o_ptr, flgs);
        if (have_flag(flgs, TR_SEARCH))
            creature_ptr->skill_srh += (o_ptr->pval * 5);
    }

    if (creature_ptr->muta3 & MUT3_XTRA_EYES) {
        creature_ptr->skill_srh += 15;
    }
    creature_ptr->skill_srh += (cp_ptr->x_srh * creature_ptr->lev / 10);

    if (creature_ptr->shero) {
        creature_ptr->skill_srh -= 15;
    }
}

static void calc_search_freq(player_type *creature_ptr)
{
    const player_race *tmp_rp_ptr;

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];

    creature_ptr->skill_fos = tmp_rp_ptr->r_fos + c_ptr->c_fos + a_ptr->a_fos;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(o_ptr, flgs);
        if (have_flag(flgs, TR_SEARCH))
            creature_ptr->skill_fos += (o_ptr->pval * 5);
    }

    if (creature_ptr->shero) {
        creature_ptr->skill_fos -= 15;
    }

    if (creature_ptr->muta3 & MUT3_XTRA_EYES) {
        creature_ptr->skill_fos += 15;
    }

    creature_ptr->skill_fos += (cp_ptr->x_fos * creature_ptr->lev / 10);
}

static void calc_to_hit_melee(player_type *creature_ptr)
{
    const player_race *tmp_rp_ptr;
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];

    creature_ptr->skill_thn = tmp_rp_ptr->r_thn + c_ptr->c_thn + a_ptr->a_thn;
    creature_ptr->skill_thn += ((c_ptr->x_thn * creature_ptr->lev / 10) + (a_ptr->a_thn * creature_ptr->lev / 50));
}

static void calc_to_hit_shoot(player_type *creature_ptr)
{
    const player_race *tmp_rp_ptr;
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];

    creature_ptr->skill_thb = tmp_rp_ptr->r_thb + c_ptr->c_thb + a_ptr->a_thb;
    creature_ptr->skill_thb += ((c_ptr->x_thb * creature_ptr->lev / 10) + (a_ptr->a_thb * creature_ptr->lev / 50));
}

static void calc_to_hit_throw(player_type *creature_ptr)
{
    const player_race *tmp_rp_ptr;
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];

    if (creature_ptr->shero) {
        creature_ptr->skill_tht -= 20;
    }

    creature_ptr->skill_tht = tmp_rp_ptr->r_thb + c_ptr->c_thb + a_ptr->a_thb;
    creature_ptr->skill_tht += ((c_ptr->x_thb * creature_ptr->lev / 10) + (a_ptr->a_thb * creature_ptr->lev / 50));
}

static void calc_dig(player_type *creature_ptr)
{
    creature_ptr->skill_dig = 0;
    if (creature_ptr->shero)
        creature_ptr->skill_dig += 30;
    creature_ptr->skill_dig += adj_str_dig[creature_ptr->stat_ind[A_STR]];

    for (int i = 0; i < 2; i++) {
        object_type *o_ptr;
        o_ptr = &creature_ptr->inventory_list[INVEN_RARM + i];

        if (has_melee_weapon(creature_ptr, INVEN_RARM + i) && !creature_ptr->heavy_wield[i]) {
            creature_ptr->skill_dig += (o_ptr->weight / 10);
        }
    }

    if (creature_ptr->skill_dig < 1)
        creature_ptr->skill_dig = 1;
}

static void calc_num_blow(player_type *creature_ptr, int i)
{
    int hold = adj_str_hold[creature_ptr->stat_ind[A_STR]];
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    bool omoi = FALSE;
    if (creature_ptr->ryoute)
        hold *= 2;

    o_ptr = &creature_ptr->inventory_list[INVEN_RARM + i];
    object_flags(o_ptr, flgs);
    creature_ptr->heavy_wield[i] = FALSE;
    creature_ptr->icky_wield[i] = FALSE;
    creature_ptr->riding_wield[i] = FALSE;
    if (!has_melee_weapon(creature_ptr, INVEN_RARM + i)) {
        creature_ptr->num_blow[i] = 1;
        return;
    }

    if (hold < o_ptr->weight / 10) {
        creature_ptr->to_h[i] += 2 * (hold - o_ptr->weight / 10);
        creature_ptr->dis_to_h[i] += 2 * (hold - o_ptr->weight / 10);
        creature_ptr->heavy_wield[i] = TRUE;
    } else if (creature_ptr->ryoute && (hold < o_ptr->weight / 5))
        omoi = TRUE;

    if ((i == 1) && (o_ptr->tval == TV_SWORD) && ((o_ptr->sval == SV_MAIN_GAUCHE) || (o_ptr->sval == SV_WAKIZASHI))) {
        creature_ptr->to_a += 5;
        creature_ptr->dis_to_a += 5;
    }

    if (o_ptr->k_idx && !creature_ptr->heavy_wield[i]) {
        int str_index, dex_index;
        int num = 0, wgt = 0, mul = 0, div = 0;
        switch (creature_ptr->pclass) {
        case CLASS_WARRIOR:
            num = 6;
            wgt = 70;
            mul = 5;
            break;

        case CLASS_BERSERKER:
            num = 6;
            wgt = 70;
            mul = 7;
            break;

        case CLASS_MAGE:
        case CLASS_HIGH_MAGE:
        case CLASS_BLUE_MAGE:
            num = 3;
            wgt = 100;
            mul = 2;
            break;

        case CLASS_PRIEST:
        case CLASS_MAGIC_EATER:
        case CLASS_MINDCRAFTER:
            num = 5;
            wgt = 100;
            mul = 3;
            break;

        case CLASS_ROGUE:
            num = 5;
            wgt = 40;
            mul = 3;
            break;

        case CLASS_RANGER:
            num = 5;
            wgt = 70;
            mul = 4;
            break;

        case CLASS_PALADIN:
        case CLASS_SAMURAI:
            num = 5;
            wgt = 70;
            mul = 4;
            break;

        case CLASS_SMITH:
            num = 5;
            wgt = 150;
            mul = 5;
            break;

        case CLASS_WARRIOR_MAGE:
        case CLASS_RED_MAGE:
            num = 5;
            wgt = 70;
            mul = 3;
            break;

        case CLASS_CHAOS_WARRIOR:
            num = 5;
            wgt = 70;
            mul = 4;
            break;

        case CLASS_MONK:
            num = 5;
            wgt = 60;
            mul = 3;
            break;

        case CLASS_TOURIST:
            num = 4;
            wgt = 100;
            mul = 3;
            break;

        case CLASS_IMITATOR:
            num = 5;
            wgt = 70;
            mul = 4;
            break;

        case CLASS_BEASTMASTER:
            num = 5;
            wgt = 70;
            mul = 3;
            break;

        case CLASS_CAVALRY:
            if ((creature_ptr->riding) && (have_flag(flgs, TR_RIDING))) {
                num = 5;
                wgt = 70;
                mul = 4;
            } else {
                num = 5;
                wgt = 100;
                mul = 3;
            }
            break;

        case CLASS_SORCERER:
            num = 1;
            wgt = 1;
            mul = 1;
            break;

        case CLASS_ARCHER:
        case CLASS_BARD:
        case CLASS_SNIPER:
            num = 4;
            wgt = 70;
            mul = 2;
            break;

        case CLASS_FORCETRAINER:
            num = 4;
            wgt = 60;
            mul = 2;
            break;

        case CLASS_MIRROR_MASTER:
            num = 3;
            wgt = 100;
            mul = 3;
            break;

        case CLASS_NINJA:
            num = 4;
            wgt = 20;
            mul = 1;
            break;
        }

        if (hex_spelling(creature_ptr, HEX_XTRA_MIGHT) || hex_spelling(creature_ptr, HEX_BUILDING)) {
            num++;
            wgt /= 2;
            mul += 2;
        }

        div = ((o_ptr->weight < wgt) ? wgt : o_ptr->weight);
        str_index = (adj_str_blow[creature_ptr->stat_ind[A_STR]] * mul / div);

        if (creature_ptr->ryoute && !omoi)
            str_index++;
        if (creature_ptr->pclass == CLASS_NINJA)
            str_index = MAX(0, str_index - 1);
        if (str_index > 11)
            str_index = 11;

        dex_index = (adj_dex_blow[creature_ptr->stat_ind[A_DEX]]);
        if (dex_index > 11)
            dex_index = 11;

        creature_ptr->num_blow[i] = blows_table[str_index][dex_index];
        if (creature_ptr->num_blow[i] > num)
            creature_ptr->num_blow[i] = (s16b)num;

        creature_ptr->num_blow[i] += (s16b)creature_ptr->extra_blows[i];
        if (creature_ptr->pclass == CLASS_WARRIOR)
            creature_ptr->num_blow[i] += (creature_ptr->lev / 40);
        else if (creature_ptr->pclass == CLASS_BERSERKER)
            creature_ptr->num_blow[i] += (creature_ptr->lev / 23);
        else if ((creature_ptr->pclass == CLASS_ROGUE) && (o_ptr->weight < 50) && (creature_ptr->stat_ind[A_DEX] >= 30))
            creature_ptr->num_blow[i]++;

        if (creature_ptr->special_defense & KATA_FUUJIN)
            creature_ptr->num_blow[i] -= 1;

        if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE))
            creature_ptr->num_blow[i] = 1;

        if (creature_ptr->num_blow[i] < 1)
            creature_ptr->num_blow[i] = 1;
    }
}

static void calc_strength_addition(player_type *creature_ptr)
{
    const player_race *tmp_rp_ptr;
    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];
    creature_ptr->stat_add[A_STR] = tmp_rp_ptr->r_adj[A_STR] + c_ptr->c_adj[A_STR] + a_ptr->a_adj[A_STR];

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_ENT) {
        if (creature_ptr->lev > 25)
            creature_ptr->stat_add[A_STR]++;
        if (creature_ptr->lev > 40)
            creature_ptr->stat_add[A_STR]++;
        if (creature_ptr->lev > 45)
            creature_ptr->stat_add[A_STR]++;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(o_ptr, flgs);
        if (have_flag(flgs, TR_STR)) {
            creature_ptr->stat_add[A_STR] += o_ptr->pval;
        }
    }

    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(creature_ptr, HEX_XTRA_MIGHT)) {
            creature_ptr->stat_add[A_STR] += 4;
        }
        if (hex_spelling(creature_ptr, HEX_BUILDING)) {
            creature_ptr->stat_add[A_STR] += 4;
        }
    }

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        creature_ptr->stat_add[A_STR] += 5;
    }

    if (creature_ptr->special_defense & KAMAE_BYAKKO) {
        creature_ptr->stat_add[A_STR] += 2;
    } else if (creature_ptr->special_defense & KAMAE_SUZAKU) {
        creature_ptr->stat_add[A_STR] -= 2;
    }

    if (creature_ptr->muta3) {

        if (creature_ptr->muta3 & MUT3_HYPER_STR) {
            creature_ptr->stat_add[A_STR] += 4;
        }

        if (creature_ptr->muta3 & MUT3_PUNY) {
            creature_ptr->stat_add[A_STR] -= 4;
        }
    }

    if (creature_ptr->tsuyoshi) {
        creature_ptr->stat_add[A_STR] += 4;
    }
}

void calc_intelligence_addition(player_type *creature_ptr)
{
    const player_race *tmp_rp_ptr;
    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];
    creature_ptr->stat_add[A_INT] = tmp_rp_ptr->r_adj[A_INT] + c_ptr->c_adj[A_INT] + a_ptr->a_adj[A_INT];

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(o_ptr, flgs);
        if (have_flag(flgs, TR_INT)) {
            creature_ptr->stat_add[A_INT] += o_ptr->pval;
        }
    }

    if (creature_ptr->special_defense & KAMAE_GENBU) {
        creature_ptr->stat_add[A_INT] -= 1;
    } else if (creature_ptr->special_defense & KAMAE_SUZAKU) {
        creature_ptr->stat_add[A_INT] += 1;
    }

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        creature_ptr->stat_add[A_INT] += 5;
    }

    if (creature_ptr->muta3) {
        if (creature_ptr->muta3 & MUT3_HYPER_INT) {
            creature_ptr->stat_add[A_INT] += 4;
        }

        if (creature_ptr->muta3 & MUT3_MORONIC) {
            creature_ptr->stat_add[A_INT] -= 4;
        }
    }
}

static void calc_wisdom_addition(player_type *creature_ptr)
{
    const player_race *tmp_rp_ptr;
    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];
    creature_ptr->stat_add[A_WIS] = tmp_rp_ptr->r_adj[A_WIS] + c_ptr->c_adj[A_WIS] + a_ptr->a_adj[A_WIS];

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(o_ptr, flgs);
        if (have_flag(flgs, TR_WIS)) {
            creature_ptr->stat_add[A_WIS] += o_ptr->pval;
        }
    }

    if (creature_ptr->muta3) {

        if (creature_ptr->muta3 & MUT3_HYPER_INT) {
            creature_ptr->stat_add[A_WIS] += 4;
        }

        if (creature_ptr->muta3 & MUT3_MORONIC) {
            creature_ptr->stat_add[A_WIS] -= 4;
        }
    }

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        creature_ptr->stat_add[A_WIS] += 5;
    }

    if (creature_ptr->special_defense & KAMAE_GENBU) {
        creature_ptr->stat_add[A_WIS] -= 1;
    } else if (creature_ptr->special_defense & KAMAE_SUZAKU) {
        creature_ptr->stat_add[A_WIS] += 1;
    }
}

static void calc_dexterity_addition(player_type *creature_ptr)
{

    const player_race *tmp_rp_ptr;
    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];
    creature_ptr->stat_add[A_DEX] = tmp_rp_ptr->r_adj[A_DEX] + c_ptr->c_adj[A_DEX] + a_ptr->a_adj[A_DEX];

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_ENT) {
        if (creature_ptr->lev > 25)
            creature_ptr->stat_add[A_DEX]--;
        if (creature_ptr->lev > 40)
            creature_ptr->stat_add[A_DEX]--;
        if (creature_ptr->lev > 45)
            creature_ptr->stat_add[A_DEX]--;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(o_ptr, flgs);
        if (have_flag(flgs, TR_DEX)) {
            creature_ptr->stat_add[A_DEX] += o_ptr->pval;
        }
    }

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        creature_ptr->stat_add[A_DEX] += 5;
    }

    if (creature_ptr->muta3 & MUT3_IRON_SKIN) {
        creature_ptr->stat_add[A_DEX] -= 1;
    }

    if (creature_ptr->muta3 & MUT3_LIMBER) {
        creature_ptr->stat_add[A_DEX] += 3;
    }

    if (creature_ptr->muta3 & MUT3_ARTHRITIS) {
        creature_ptr->stat_add[A_DEX] -= 3;
    }

    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(creature_ptr, HEX_BUILDING)) {
            creature_ptr->stat_add[A_DEX] += 4;
        }
    }

    if (creature_ptr->special_defense & KAMAE_BYAKKO) {
        creature_ptr->stat_add[A_DEX] += 2;
    } else if (creature_ptr->special_defense & KAMAE_GENBU) {
        creature_ptr->stat_add[A_DEX] -= 2;
    } else if (creature_ptr->special_defense & KAMAE_SUZAKU) {
        creature_ptr->stat_add[A_DEX] += 2;
    }
}

static void calc_constitution_addition(player_type *creature_ptr)
{
    const player_race *tmp_rp_ptr;
    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];
    creature_ptr->stat_add[A_CON] = tmp_rp_ptr->r_adj[A_CON] + c_ptr->c_adj[A_CON] + a_ptr->a_adj[A_CON];

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_ENT) {
        if (creature_ptr->lev > 25)
            creature_ptr->stat_add[A_CON]++;
        if (creature_ptr->lev > 40)
            creature_ptr->stat_add[A_CON]++;
        if (creature_ptr->lev > 45)
            creature_ptr->stat_add[A_CON]++;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(o_ptr, flgs);
        if (have_flag(flgs, TR_CON))
            creature_ptr->stat_add[A_CON] += o_ptr->pval;
    }

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        creature_ptr->stat_add[A_CON] += 5;
    }

    if (creature_ptr->muta3) {
        if (creature_ptr->muta3 & MUT3_RESILIENT) {
            creature_ptr->stat_add[A_CON] += 4;
        }

        if (creature_ptr->muta3 & MUT3_ALBINO) {
            creature_ptr->stat_add[A_CON] -= 4;
        }

        if (creature_ptr->muta3 & MUT3_XTRA_FAT) {
            creature_ptr->stat_add[A_CON] += 2;
        }

        if (creature_ptr->muta3 & MUT3_FLESH_ROT) {
            creature_ptr->stat_add[A_CON] -= 2;
        }
    }

    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(creature_ptr, HEX_BUILDING)) {
            creature_ptr->stat_add[A_CON] += 4;
        }
    }

    if (creature_ptr->tsuyoshi) {
        creature_ptr->stat_add[A_CON] += 4;
    }

    if (creature_ptr->special_defense & KAMAE_BYAKKO) {
        creature_ptr->stat_add[A_CON] -= 3;
    } else if (creature_ptr->special_defense & KAMAE_GENBU) {
        creature_ptr->stat_add[A_CON] += 3;
    } else if (creature_ptr->special_defense & KAMAE_SUZAKU) {
        creature_ptr->stat_add[A_CON] -= 2;
    }
}

static void calc_charisma_addition(player_type *creature_ptr)
{
    const player_race *tmp_rp_ptr;
    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];
    creature_ptr->stat_add[A_CHR] = tmp_rp_ptr->r_adj[A_CHR] + c_ptr->c_adj[A_CHR] + a_ptr->a_adj[A_CHR];

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(o_ptr, flgs);
        if (have_flag(flgs, TR_CHR))
            creature_ptr->stat_add[A_CHR] += o_ptr->pval;
    }

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        creature_ptr->stat_add[A_CHR] += 5;
    }

    if (creature_ptr->muta3) {
        if (creature_ptr->muta3 & MUT3_FLESH_ROT) {
            creature_ptr->stat_add[A_CHR] -= 1;
        }
        if (creature_ptr->muta3 & MUT3_SILLY_VOI) {
            creature_ptr->stat_add[A_CHR] -= 4;
        }
        if (creature_ptr->muta3 & MUT3_BLANK_FAC) {
            creature_ptr->stat_add[A_CHR] -= 1;
        }
        if (creature_ptr->muta3 & MUT3_WART_SKIN) {
            creature_ptr->stat_add[A_CHR] -= 2;
        }
        if (creature_ptr->muta3 & MUT3_SCALES) {
            creature_ptr->stat_add[A_CHR] -= 1;
        }
        if (creature_ptr->muta3 & MUT3_ILL_NORM) {
            creature_ptr->stat_add[A_CHR] = 0;
        }
    }
}

static void calc_to_magic_chance(player_type *creature_ptr)
{
    creature_ptr->to_m_chance = 0;

    if (creature_ptr->pseikaku == PERSONALITY_LAZY)
        creature_ptr->to_m_chance += 10;
    if (creature_ptr->pseikaku == PERSONALITY_SHREWD)
        creature_ptr->to_m_chance -= 3;
    if ((creature_ptr->pseikaku == PERSONALITY_PATIENT) || (creature_ptr->pseikaku == PERSONALITY_MIGHTY))
        creature_ptr->to_m_chance++;
    if (creature_ptr->pseikaku == PERSONALITY_CHARGEMAN)
        creature_ptr->to_m_chance += 5;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(o_ptr, flgs);
        if (o_ptr->curse_flags & TRC_LOW_MAGIC) {
            if (o_ptr->curse_flags & TRC_HEAVY_CURSE) {
                creature_ptr->to_m_chance += 10;
            } else {
                creature_ptr->to_m_chance += 3;
            }
        }
    }
}

static void calc_base_ac(player_type *creature_ptr)
{
    creature_ptr->ac = 0;
    if (creature_ptr->yoiyami)
        return;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        creature_ptr->ac += o_ptr->ac;
    }
    if (object_is_armour(&creature_ptr->inventory_list[INVEN_RARM]) || object_is_armour(&creature_ptr->inventory_list[INVEN_LARM])) {
        creature_ptr->ac += creature_ptr->skill_exp[GINOU_SHIELD] * (1 + creature_ptr->lev / 22) / 2000;
    }
}

static void calc_to_ac(player_type *creature_ptr)
{
    creature_ptr->to_a = 0;
    if (creature_ptr->yoiyami)
        return;

    creature_ptr->to_a += ((int)(adj_dex_ta[creature_ptr->stat_ind[A_DEX]]) - 128);

    if (creature_ptr->mimic_form) {
        switch (creature_ptr->mimic_form) {
        case MIMIC_DEMON:
            creature_ptr->to_a += 10;
            break;
        case MIMIC_DEMON_LORD:
            creature_ptr->to_a += 20;
            break;
        case MIMIC_VAMPIRE:
            creature_ptr->to_a += 10;
        }
    }
	
	if (creature_ptr->pclass == CLASS_BERSERKER) {
        creature_ptr->to_a += 10 + creature_ptr->lev / 2;
    }
	if (creature_ptr->pclass == CLASS_SORCERER) {
        creature_ptr->to_a -= 50;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        creature_ptr->to_a += o_ptr->to_a;

        if (o_ptr->curse_flags & TRC_LOW_AC) {
            if (o_ptr->curse_flags & TRC_HEAVY_CURSE) {
                creature_ptr->to_a -= 30;
            } else {
                creature_ptr->to_a -= 10;
            }
        }
	
	}

    if (is_specific_player_race(creature_ptr, RACE_GOLEM) || is_specific_player_race(creature_ptr, RACE_ANDROID)) {
        creature_ptr->to_a += 10 + (creature_ptr->lev * 2 / 5);
    }

    if (creature_ptr->muta3 & MUT3_WART_SKIN) {
        creature_ptr->to_a += 5;
    }

    if (creature_ptr->muta3 & MUT3_SCALES) {
        creature_ptr->to_a += 10;
    }

    if (creature_ptr->muta3 & MUT3_IRON_SKIN) {
        creature_ptr->to_a += 25;
    }

    if (((creature_ptr->pclass == CLASS_MONK) || (creature_ptr->pclass == CLASS_FORCETRAINER)) && !heavy_armor(creature_ptr)) {
        if (!(creature_ptr->inventory_list[INVEN_BODY].k_idx)) {
            creature_ptr->to_a += (creature_ptr->lev * 3) / 2;
        }
        if (!(creature_ptr->inventory_list[INVEN_OUTER].k_idx) && (creature_ptr->lev > 15)) {
            creature_ptr->to_a += ((creature_ptr->lev - 13) / 3);
        }
        if (!(creature_ptr->inventory_list[INVEN_LARM].k_idx) && (creature_ptr->lev > 10)) {
            creature_ptr->to_a += ((creature_ptr->lev - 8) / 3);
        }
        if (!(creature_ptr->inventory_list[INVEN_HEAD].k_idx) && (creature_ptr->lev > 4)) {
            creature_ptr->to_a += (creature_ptr->lev - 2) / 3;
        }
        if (!(creature_ptr->inventory_list[INVEN_HANDS].k_idx)) {
            creature_ptr->to_a += (creature_ptr->lev / 2);
        }
        if (!(creature_ptr->inventory_list[INVEN_FEET].k_idx)) {
            creature_ptr->to_a += (creature_ptr->lev / 3);
        }
    }

    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(creature_ptr, HEX_ICE_ARMOR)) {
            creature_ptr->to_a += 30;
        }

        for (int i = INVEN_RARM; i <= INVEN_FEET; i++) {
            ARMOUR_CLASS ac = 0;
            object_type *o_ptr = &creature_ptr->inventory_list[i];
            if (!o_ptr->k_idx)
                continue;
            if (!object_is_armour(o_ptr))
                continue;
            if (!object_is_cursed(o_ptr))
                continue;
            ac += 5;
            if (o_ptr->curse_flags & TRC_HEAVY_CURSE)
                ac += 7;
            if (o_ptr->curse_flags & TRC_PERMA_CURSE)
                ac += 13;
            creature_ptr->to_a += ac;
        }
    }

    if (creature_ptr->special_defense & KAMAE_BYAKKO) {
        creature_ptr->to_a -= 40;
    } else if (creature_ptr->special_defense & KAMAE_SEIRYU) {
        creature_ptr->to_a -= 50;
    } else if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        creature_ptr->to_a -= 50;
    }

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->to_a += 100;
    } else if (creature_ptr->tsubureru || creature_ptr->shield || creature_ptr->magicdef) {
        creature_ptr->to_a += 50;
    }

    if (is_blessed(creature_ptr)) {
        creature_ptr->to_a += 5;
    }

    if (creature_ptr->shero) {
        creature_ptr->to_a -= 10;
    }
}

static void calc_base_ac_display(player_type *creature_ptr)
{
    creature_ptr->dis_ac = 0;
    if (creature_ptr->yoiyami)
        return;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        creature_ptr->dis_ac += o_ptr->ac;
    }
    if (object_is_armour(&creature_ptr->inventory_list[INVEN_RARM]) || object_is_armour(&creature_ptr->inventory_list[INVEN_LARM])) {
        creature_ptr->dis_ac += creature_ptr->skill_exp[GINOU_SHIELD] * (1 + creature_ptr->lev / 22) / 2000;
    }
}

static void calc_to_ac_display(player_type *creature_ptr)
{
    creature_ptr->dis_to_a = 0;
    if (creature_ptr->yoiyami)
        return;

    creature_ptr->dis_to_a += ((int)(adj_dex_ta[creature_ptr->stat_ind[A_DEX]]) - 128);

    if (creature_ptr->mimic_form) {
        switch (creature_ptr->mimic_form) {
        case MIMIC_DEMON:
            creature_ptr->dis_to_a += 10;
            break;
        case MIMIC_DEMON_LORD:
            creature_ptr->dis_to_a += 20;
            break;
        case MIMIC_VAMPIRE:
            creature_ptr->dis_to_a += 10;
        }
    }
	
	if (creature_ptr->pclass == CLASS_BERSERKER) {
		creature_ptr->dis_to_a += 10 + creature_ptr->lev / 2;
    }

	if (creature_ptr->pclass == CLASS_SORCERER) {
        creature_ptr->dis_to_a -= 50;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        if (object_is_known(o_ptr))
            creature_ptr->dis_to_a += o_ptr->to_a;

        if (o_ptr->curse_flags & TRC_LOW_AC) {
            if (o_ptr->curse_flags & TRC_HEAVY_CURSE) {
                if (object_is_fully_known(o_ptr))
                    creature_ptr->dis_to_a -= 30;
            } else {
                if (object_is_fully_known(o_ptr))
                    creature_ptr->dis_to_a -= 10;
            }
        }	
	}

    if (is_specific_player_race(creature_ptr, RACE_GOLEM) || is_specific_player_race(creature_ptr, RACE_ANDROID)) {
        creature_ptr->dis_to_a += 10 + (creature_ptr->lev * 2 / 5);
    }

    if (((creature_ptr->pclass == CLASS_MONK) || (creature_ptr->pclass == CLASS_FORCETRAINER)) && !heavy_armor(creature_ptr)) {
        if (!(creature_ptr->inventory_list[INVEN_BODY].k_idx)) {
            creature_ptr->dis_to_a += (creature_ptr->lev * 3) / 2;
        }
        if (!(creature_ptr->inventory_list[INVEN_OUTER].k_idx) && (creature_ptr->lev > 15)) {
            creature_ptr->dis_to_a += ((creature_ptr->lev - 13) / 3);
        }
        if (!(creature_ptr->inventory_list[INVEN_LARM].k_idx) && (creature_ptr->lev > 10)) {
            creature_ptr->dis_to_a += ((creature_ptr->lev - 8) / 3);
        }
        if (!(creature_ptr->inventory_list[INVEN_HEAD].k_idx) && (creature_ptr->lev > 4)) {
            creature_ptr->dis_to_a += (creature_ptr->lev - 2) / 3;
        }
        if (!(creature_ptr->inventory_list[INVEN_HANDS].k_idx)) {
            creature_ptr->dis_to_a += (creature_ptr->lev / 2);
        }
        if (!(creature_ptr->inventory_list[INVEN_FEET].k_idx)) {
            creature_ptr->dis_to_a += (creature_ptr->lev / 3);
        }
    }

    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(creature_ptr, HEX_ICE_ARMOR)) {
            creature_ptr->dis_to_a += 30;
        }

        for (int i = INVEN_RARM; i <= INVEN_FEET; i++) {
            ARMOUR_CLASS ac = 0;
            object_type *o_ptr = &creature_ptr->inventory_list[i];
            if (!o_ptr->k_idx)
                continue;
            if (!object_is_armour(o_ptr))
                continue;
            if (!object_is_cursed(o_ptr))
                continue;
            ac += 5;
            if (o_ptr->curse_flags & TRC_HEAVY_CURSE)
                ac += 7;
            if (o_ptr->curse_flags & TRC_PERMA_CURSE)
                ac += 13;
            creature_ptr->dis_to_a += ac;
        }
    }

    if (creature_ptr->muta3) {
        if (creature_ptr->muta3 & MUT3_WART_SKIN) {
            creature_ptr->dis_to_a += 5;
        }

        if (creature_ptr->muta3 & MUT3_SCALES) {
            creature_ptr->dis_to_a += 10;
        }

        if (creature_ptr->muta3 & MUT3_IRON_SKIN) {
            creature_ptr->dis_to_a += 25;
        }
    }

    if (creature_ptr->special_defense & KAMAE_BYAKKO) {
        creature_ptr->dis_to_a -= 40;
    } else if (creature_ptr->special_defense & KAMAE_SEIRYU) {
        creature_ptr->dis_to_a -= 50;
    } else if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        creature_ptr->dis_to_a -= 50;
    }

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->dis_to_a += 100;
    } else if (creature_ptr->tsubureru || creature_ptr->shield || creature_ptr->magicdef) {
        creature_ptr->dis_to_a += 50;
    }

    if (is_blessed(creature_ptr)) {
        creature_ptr->dis_to_a += 5;
    }

    if (creature_ptr->shero) {
        creature_ptr->dis_to_a -= 10;
    }
}

static void calc_speed(player_type *creature_ptr)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    feature_type *f_ptr = &f_info[floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].feat];

	creature_ptr->pspeed = 110;

    if (!creature_ptr->riding) {
        const player_race *tmp_rp_ptr;
        if (creature_ptr->mimic_form)
            tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
        else
            tmp_rp_ptr = &race_info[creature_ptr->prace];

        if (is_specific_player_race(creature_ptr, RACE_KLACKON) || is_specific_player_race(creature_ptr, RACE_SPRITE))
            creature_ptr->pspeed += (creature_ptr->lev) / 10;

        for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
            object_type *o_ptr = &creature_ptr->inventory_list[i];
            BIT_FLAGS flgs[TR_FLAG_SIZE];
            object_flags(o_ptr, flgs);

            if (!o_ptr->k_idx)
                continue;
            if (have_flag(flgs, TR_SPEED))
                creature_ptr->pspeed += o_ptr->pval;
        }

        if (creature_ptr->mimic_form) {
            switch (creature_ptr->mimic_form) {
            case MIMIC_DEMON:
                creature_ptr->pspeed += 3;
                break;
            case MIMIC_DEMON_LORD:
                creature_ptr->pspeed += 5;
                break;
            case MIMIC_VAMPIRE:
                creature_ptr->pspeed += 3;
                break;
            }
        }

        if (creature_ptr->pclass == CLASS_FORCETRAINER && !(heavy_armor(creature_ptr))) {
            if (!(is_specific_player_race(creature_ptr, RACE_KLACKON) || is_specific_player_race(creature_ptr, RACE_SPRITE)
                    || (creature_ptr->pseikaku == PERSONALITY_MUNCHKIN)))
                creature_ptr->pspeed += (creature_ptr->lev) / 10;
        }

        if (creature_ptr->pclass == CLASS_BERSERKER) {
            creature_ptr->pspeed += 2;
            if (creature_ptr->lev > 29)
                creature_ptr->pspeed++;
            if (creature_ptr->lev > 39)
                creature_ptr->pspeed++;
            if (creature_ptr->lev > 44)
                creature_ptr->pspeed++;
            if (creature_ptr->lev > 49)
                creature_ptr->pspeed++;
        }

        if (creature_ptr->pseikaku == PERSONALITY_MUNCHKIN && creature_ptr->prace != RACE_KLACKON && creature_ptr->prace != RACE_SPRITE) {
            creature_ptr->pspeed += (creature_ptr->lev) / 10 + 5;
        }

        if (IS_FAST(creature_ptr)) {
            creature_ptr->pspeed += 10;
        }

        if (creature_ptr->slow) {
            creature_ptr->pspeed -= 10;
        }

        if (creature_ptr->realm1 == REALM_HEX) {
            if (hex_spelling(creature_ptr, HEX_SHOCK_CLOAK)) {
                creature_ptr->pspeed += 3;
            }
        }

        if (creature_ptr->food >= PY_FOOD_MAX)
            creature_ptr->pspeed -= 10;

        if (creature_ptr->special_defense & KAMAE_SUZAKU)
            creature_ptr->pspeed += 10;

        if (creature_ptr->muta3) {

            if (creature_ptr->muta3 & MUT3_XTRA_FAT) {
                creature_ptr->pspeed -= 2;
            }

            if (creature_ptr->muta3 & MUT3_XTRA_LEGS) {
                creature_ptr->pspeed += 3;
            }

            if (creature_ptr->muta3 & MUT3_SHORT_LEG) {
                creature_ptr->pspeed -= 3;
            }
        }

        if (creature_ptr->prace == RACE_MERFOLK) {
            if (have_flag(f_ptr->flags, FF_WATER)) {
                creature_ptr->pspeed += (2 + creature_ptr->lev / 10);
            } else if (!creature_ptr->levitation) {
                creature_ptr->pspeed -= 2;
            }
        }

    } else {
        monster_type *riding_m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        SPEED speed = riding_m_ptr->mspeed;

        if (riding_m_ptr->mspeed > 110) {
            creature_ptr->pspeed = 110 + (s16b)((speed - 110) * (creature_ptr->skill_exp[GINOU_RIDING] * 3 + creature_ptr->lev * 160L - 10000L) / (22000L));
            if (creature_ptr->pspeed < 110)
                creature_ptr->pspeed = 110;
        } else {
            creature_ptr->pspeed = speed;
        }

        creature_ptr->pspeed += (creature_ptr->skill_exp[GINOU_RIDING] + creature_ptr->lev * 160L) / 3200;
        if (monster_fast_remaining(riding_m_ptr))
            creature_ptr->pspeed += 10;
        if (monster_slow_remaining(riding_m_ptr))
            creature_ptr->pspeed -= 10;
    }

	if (creature_ptr->action == ACTION_SEARCH)
        creature_ptr->pspeed -= 10;

    /* Maximum speed is (+99). (internally it's 110 + 99) */
    /* Temporary lightspeed forces to be maximum speed */
    if ((creature_ptr->lightspeed && !creature_ptr->riding) || (creature_ptr->pspeed > 209)) {
        creature_ptr->pspeed = 209;
    }

    /* Minimum speed is (-99). (internally it's 110 - 99) */
    if (creature_ptr->pspeed < 11)
        creature_ptr->pspeed = 11;
}

/*!
 * @brief プレイヤーの所持重量制限を計算する /
 * Computes current weight limit.
 * @return 制限重量(ポンド)
 */
WEIGHT weight_limit(player_type *creature_ptr)
{
    WEIGHT i = (WEIGHT)adj_str_wgt[creature_ptr->stat_ind[A_STR]] * 50;
    if (creature_ptr->pclass == CLASS_BERSERKER)
        i = i * 3 / 2;
    return i;
}

/*!
 * @brief プレイヤーが現在右手/左手に武器を持っているか判定する /
 * @param i 判定する手のID(右手:0 左手:1)
 * @return 持っているならばTRUE
 */
bool has_melee_weapon(player_type *creature_ptr, int i)
{
    return ((creature_ptr->inventory_list[i].k_idx && object_is_melee_weapon(&creature_ptr->inventory_list[i])) ? TRUE : FALSE);
}

/*!
 * @brief プレイヤーの現在開いている手の状態を返す
 * @param riding_control 乗馬中により片手を必要としている状態ならばTRUEを返す。
 * @return 開いている手のビットフラグ
 */
BIT_FLAGS16 empty_hands(player_type *creature_ptr, bool riding_control)
{
    BIT_FLAGS16 status = EMPTY_HAND_NONE;
    if (!creature_ptr->inventory_list[INVEN_RARM].k_idx)
        status |= EMPTY_HAND_RARM;
    if (!creature_ptr->inventory_list[INVEN_LARM].k_idx)
        status |= EMPTY_HAND_LARM;

    if (riding_control && (status != EMPTY_HAND_NONE) && creature_ptr->riding && !(creature_ptr->pet_extra_flags & PF_TWO_HANDS)) {
        if (status & EMPTY_HAND_LARM)
            status &= ~(EMPTY_HAND_LARM);
        else if (status & EMPTY_HAND_RARM)
            status &= ~(EMPTY_HAND_RARM);
    }

    return status;
}

/*!
 * @brief プレイヤーが防具重量制限のある職業時にペナルティを受ける状態にあるかどうかを返す。
 * @return ペナルティが適用されるならばTRUE。
 */
bool heavy_armor(player_type *creature_ptr)
{
    if ((creature_ptr->pclass != CLASS_MONK) && (creature_ptr->pclass != CLASS_FORCETRAINER) && (creature_ptr->pclass != CLASS_NINJA))
        return FALSE;

    WEIGHT monk_arm_wgt = 0;
    if (creature_ptr->inventory_list[INVEN_RARM].tval > TV_SWORD)
        monk_arm_wgt += creature_ptr->inventory_list[INVEN_RARM].weight;
    if (creature_ptr->inventory_list[INVEN_LARM].tval > TV_SWORD)
        monk_arm_wgt += creature_ptr->inventory_list[INVEN_LARM].weight;
    monk_arm_wgt += creature_ptr->inventory_list[INVEN_BODY].weight;
    monk_arm_wgt += creature_ptr->inventory_list[INVEN_HEAD].weight;
    monk_arm_wgt += creature_ptr->inventory_list[INVEN_OUTER].weight;
    monk_arm_wgt += creature_ptr->inventory_list[INVEN_HANDS].weight;
    monk_arm_wgt += creature_ptr->inventory_list[INVEN_FEET].weight;

    return (monk_arm_wgt > (100 + (creature_ptr->lev * 4)));
}

/*!
 * @brief update のフラグに応じた更新をまとめて行う / Handle "update"
 * @return なし
 * @details 更新処理の対象はプレイヤーの能力修正/光源寿命/HP/MP/魔法の学習状態、他多数の外界の状態判定。
 */
void update_creature(player_type *creature_ptr)
{
    if (!creature_ptr->update)
        return;

    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (creature_ptr->update & (PU_AUTODESTROY)) {
        creature_ptr->update &= ~(PU_AUTODESTROY);
        autopick_delayed_alter(creature_ptr);
    }

    if (creature_ptr->update & (PU_COMBINE)) {
        creature_ptr->update &= ~(PU_COMBINE);
        combine_pack(creature_ptr);
    }

    if (creature_ptr->update & (PU_REORDER)) {
        creature_ptr->update &= ~(PU_REORDER);
        reorder_pack(creature_ptr);
    }

    if (creature_ptr->update & (PU_BONUS)) {
        creature_ptr->update &= ~(PU_BONUS);
        calc_alignment(creature_ptr);
        calc_bonuses(creature_ptr);
    }

    if (creature_ptr->update & (PU_TORCH)) {
        creature_ptr->update &= ~(PU_TORCH);
        calc_torch(creature_ptr);
    }

    if (creature_ptr->update & (PU_HP)) {
        creature_ptr->update &= ~(PU_HP);
        calc_hitpoints(creature_ptr);
    }

    if (creature_ptr->update & (PU_MANA)) {
        creature_ptr->update &= ~(PU_MANA);
        calc_mana(creature_ptr);
    }

    if (creature_ptr->update & (PU_SPELLS)) {
        creature_ptr->update &= ~(PU_SPELLS);
        calc_spells(creature_ptr);
    }

    if (!current_world_ptr->character_generated)
        return;
    if (current_world_ptr->character_icky)
        return;
    if (creature_ptr->update & (PU_UN_LITE)) {
        creature_ptr->update &= ~(PU_UN_LITE);
        forget_lite(floor_ptr);
    }

    if (creature_ptr->update & (PU_UN_VIEW)) {
        creature_ptr->update &= ~(PU_UN_VIEW);
        forget_view(floor_ptr);
    }

    if (creature_ptr->update & (PU_VIEW)) {
        creature_ptr->update &= ~(PU_VIEW);
        update_view(creature_ptr);
    }

    if (creature_ptr->update & (PU_LITE)) {
        creature_ptr->update &= ~(PU_LITE);
        update_lite(creature_ptr);
    }

    if (creature_ptr->update & (PU_FLOW)) {
        creature_ptr->update &= ~(PU_FLOW);
        update_flow(creature_ptr);
    }

    if (creature_ptr->update & (PU_DISTANCE)) {
        creature_ptr->update &= ~(PU_DISTANCE);

        update_monsters(creature_ptr, TRUE);
    }

    if (creature_ptr->update & (PU_MON_LITE)) {
        creature_ptr->update &= ~(PU_MON_LITE);
        update_mon_lite(creature_ptr);
    }

    if (creature_ptr->update & (PU_DELAY_VIS)) {
        creature_ptr->update &= ~(PU_DELAY_VIS);
        delayed_visual_update(creature_ptr);
    }

    if (creature_ptr->update & (PU_MONSTERS)) {
        creature_ptr->update &= ~(PU_MONSTERS);
        update_monsters(creature_ptr, FALSE);
    }
}

/*!
 * @brief プレイヤーが魔道書を一冊も持っていないかを判定する
 * @return 魔道書を一冊も持っていないならTRUEを返す
 */
bool player_has_no_spellbooks(player_type *creature_ptr)
{
    object_type *o_ptr;
    for (int i = 0; i < INVEN_PACK; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (o_ptr->k_idx && check_book_realm(creature_ptr, o_ptr->tval, o_ptr->sval))
            return FALSE;
    }

    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    for (int i = floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].o_idx; i; i = o_ptr->next_o_idx) {
        o_ptr = &floor_ptr->o_list[i];
        if (o_ptr->k_idx && (o_ptr->marked & OM_FOUND) && check_book_realm(creature_ptr, o_ptr->tval, o_ptr->sval))
            return FALSE;
    }

    return TRUE;
}

void take_turn(player_type *creature_ptr, PERCENTAGE need_cost) { creature_ptr->energy_use = (ENERGY)need_cost; }

void free_turn(player_type *creature_ptr) { creature_ptr->energy_use = 0; }

/*!
 * @brief プレイヤーを指定座標に配置する / Place the player in the dungeon XXX XXX
 * @param x 配置先X座標
 * @param y 配置先Y座標
 * @return 配置に成功したらTRUE
 */
bool player_place(player_type *creature_ptr, POSITION y, POSITION x)
{
    if (creature_ptr->current_floor_ptr->grid_array[y][x].m_idx != 0)
        return FALSE;

    /* Save player location */
    creature_ptr->y = y;
    creature_ptr->x = x;
    return TRUE;
}

/*!
 * @brief 種族アンバライトが出血時パターンの上に乗った際のペナルティ処理
 * @return なし
 */
void wreck_the_pattern(player_type *creature_ptr)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    int pattern_type = f_info[floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].feat].subtype;
    if (pattern_type == PATTERN_TILE_WRECKED)
        return;

    msg_print(_("パターンを血で汚してしまった！", "You bleed on the Pattern!"));
    msg_print(_("何か恐ろしい事が起こった！", "Something terrible happens!"));

    if (!IS_INVULN(creature_ptr))
        take_hit(creature_ptr, DAMAGE_NOESCAPE, damroll(10, 8), _("パターン損壊", "corrupting the Pattern"), -1);

    int to_ruin = randint1(45) + 35;
    while (to_ruin--) {
        POSITION r_y, r_x;
        scatter(creature_ptr, &r_y, &r_x, creature_ptr->y, creature_ptr->x, 4, 0);

        if (pattern_tile(floor_ptr, r_y, r_x) && (f_info[floor_ptr->grid_array[r_y][r_x].feat].subtype != PATTERN_TILE_WRECKED)) {
            cave_set_feat(creature_ptr, r_y, r_x, feat_pattern_corrupted);
        }
    }

    cave_set_feat(creature_ptr, creature_ptr->y, creature_ptr->x, feat_pattern_corrupted);
}

/*!
 * @brief プレイヤーの経験値について整合性のためのチェックと調整を行う /
 * Advance experience levels and print experience
 * @return なし
 */
void check_experience(player_type *creature_ptr)
{
    if (creature_ptr->exp < 0)
        creature_ptr->exp = 0;
    if (creature_ptr->max_exp < 0)
        creature_ptr->max_exp = 0;
    if (creature_ptr->max_max_exp < 0)
        creature_ptr->max_max_exp = 0;

    if (creature_ptr->exp > PY_MAX_EXP)
        creature_ptr->exp = PY_MAX_EXP;
    if (creature_ptr->max_exp > PY_MAX_EXP)
        creature_ptr->max_exp = PY_MAX_EXP;
    if (creature_ptr->max_max_exp > PY_MAX_EXP)
        creature_ptr->max_max_exp = PY_MAX_EXP;

    if (creature_ptr->exp > creature_ptr->max_exp)
        creature_ptr->max_exp = creature_ptr->exp;
    if (creature_ptr->max_exp > creature_ptr->max_max_exp)
        creature_ptr->max_max_exp = creature_ptr->max_exp;

    creature_ptr->redraw |= (PR_EXP);
    handle_stuff(creature_ptr);

    bool android = (creature_ptr->prace == RACE_ANDROID ? TRUE : FALSE);
    PLAYER_LEVEL old_lev = creature_ptr->lev;
    while ((creature_ptr->lev > 1) && (creature_ptr->exp < ((android ? player_exp_a : player_exp)[creature_ptr->lev - 2] * creature_ptr->expfact / 100L))) {
        creature_ptr->lev--;
        creature_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
        creature_ptr->redraw |= (PR_LEV | PR_TITLE);
        creature_ptr->window |= (PW_PLAYER);
        handle_stuff(creature_ptr);
    }

    bool level_reward = FALSE;
    bool level_mutation = FALSE;
    bool level_inc_stat = FALSE;
    while ((creature_ptr->lev < PY_MAX_LEVEL)
        && (creature_ptr->exp >= ((android ? player_exp_a : player_exp)[creature_ptr->lev - 1] * creature_ptr->expfact / 100L))) {
        creature_ptr->lev++;
        if (creature_ptr->lev > creature_ptr->max_plv) {
            creature_ptr->max_plv = creature_ptr->lev;

            if ((creature_ptr->pclass == CLASS_CHAOS_WARRIOR) || (creature_ptr->muta2 & MUT2_CHAOS_GIFT)) {
                level_reward = TRUE;
            }
            if (creature_ptr->prace == RACE_BEASTMAN) {
                if (one_in_(5))
                    level_mutation = TRUE;
            }
            level_inc_stat = TRUE;

            exe_write_diary(creature_ptr, DIARY_LEVELUP, creature_ptr->lev, NULL);
        }

        sound(SOUND_LEVEL);
        msg_format(_("レベル %d にようこそ。", "Welcome to level %d."), creature_ptr->lev);
        creature_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
        creature_ptr->redraw |= (PR_LEV | PR_TITLE | PR_EXP);
        creature_ptr->window |= (PW_PLAYER | PW_SPELL | PW_INVEN);
        creature_ptr->level_up_message = TRUE;
        handle_stuff(creature_ptr);

        creature_ptr->level_up_message = FALSE;
        if (level_inc_stat) {
            if (!(creature_ptr->max_plv % 10)) {
                int choice;
                screen_save();
                while (TRUE) {
                    int n;
                    char tmp[32];

                    cnv_stat(creature_ptr->stat_max[0], tmp);
                    prt(format(_("        a) 腕力 (現在値 %s)", "        a) Str (cur %s)"), tmp), 2, 14);
                    cnv_stat(creature_ptr->stat_max[1], tmp);
                    prt(format(_("        b) 知能 (現在値 %s)", "        b) Int (cur %s)"), tmp), 3, 14);
                    cnv_stat(creature_ptr->stat_max[2], tmp);
                    prt(format(_("        c) 賢さ (現在値 %s)", "        c) Wis (cur %s)"), tmp), 4, 14);
                    cnv_stat(creature_ptr->stat_max[3], tmp);
                    prt(format(_("        d) 器用 (現在値 %s)", "        d) Dex (cur %s)"), tmp), 5, 14);
                    cnv_stat(creature_ptr->stat_max[4], tmp);
                    prt(format(_("        e) 耐久 (現在値 %s)", "        e) Con (cur %s)"), tmp), 6, 14);
                    cnv_stat(creature_ptr->stat_max[5], tmp);
                    prt(format(_("        f) 魅力 (現在値 %s)", "        f) Chr (cur %s)"), tmp), 7, 14);

                    prt("", 8, 14);
                    prt(_("        どの能力値を上げますか？", "        Which stat do you want to raise?"), 1, 14);

                    while (TRUE) {
                        choice = inkey();
                        if ((choice >= 'a') && (choice <= 'f'))
                            break;
                    }
                    for (n = 0; n < A_MAX; n++)
                        if (n != choice - 'a')
                            prt("", n + 2, 14);
                    if (get_check(_("よろしいですか？", "Are you sure? ")))
                        break;
                }
                do_inc_stat(creature_ptr, choice - 'a');
                screen_load();
            } else if (!(creature_ptr->max_plv % 2))
                do_inc_stat(creature_ptr, randint0(6));
        }

        if (level_mutation) {
            msg_print(_("あなたは変わった気がする...", "You feel different..."));
            (void)gain_mutation(creature_ptr, 0);
            level_mutation = FALSE;
        }

        /*
         * 報酬でレベルが上ると再帰的に check_experience(creature_ptr) が
         * 呼ばれるので順番を最後にする。
         */
        if (level_reward) {
            gain_level_reward(creature_ptr, 0);
            level_reward = FALSE;
        }

        creature_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
        creature_ptr->redraw |= (PR_LEV | PR_TITLE);
        creature_ptr->window |= (PW_PLAYER | PW_SPELL);
        handle_stuff(creature_ptr);
    }

    if (old_lev != creature_ptr->lev)
        autopick_load_pref(creature_ptr, FALSE);
}

/*!
 * @brief 現在の修正後能力値を3～17及び18/xxx形式に変換する / Converts stat num into a six-char (right justified) string
 * @param val 能力値
 * @param out_val 出力先文字列ポインタ
 * @return なし
 */
void cnv_stat(int val, char *out_val)
{
    if (val <= 18) {
        sprintf(out_val, "    %2d", val);
        return;
    }

    int bonus = (val - 18);
    if (bonus >= 220) {
        sprintf(out_val, "18/%3s", "***");
    } else if (bonus >= 100) {
        sprintf(out_val, "18/%03d", bonus);
    } else {
        sprintf(out_val, " 18/%02d", bonus);
    }
}

/*!
 * @brief 能力値現在値から3～17及び18/xxx様式に基づく加減算を行う。
 * Modify a stat value by a "modifier", return new value
 * @param value 現在値
 * @param amount 加減算値
 * @return 加減算後の値
 * @details
 * <pre>
 * Stats go up: 3,4,...,17,18,18/10,18/20,...,18/220
 * Or even: 18/13, 18/23, 18/33, ..., 18/220
 * Stats go down: 18/220, 18/210,..., 18/10, 18, 17, ..., 3
 * Or even: 18/13, 18/03, 18, 17, ..., 3
 * </pre>
 */
s16b modify_stat_value(int value, int amount)
{
    if (amount > 0) {
        for (int i = 0; i < amount; i++) {
            if (value < 18)
                value++;
            else
                value += 10;
        }
    } else if (amount < 0) {
        for (int i = 0; i < (0 - amount); i++) {
            if (value >= 18 + 10)
                value -= 10;
            else if (value > 18)
                value = 18;
            else if (value > 3)
                value--;
        }
    }

    return (s16b)value;
}

/*!
 * @brief スコアを計算する /
 * Hack -- Calculates the total number of points earned		-JWT-
 * @return なし
 * @details
 */
long calc_score(player_type *creature_ptr)
{
    int arena_win = MIN(creature_ptr->arena_number, MAX_ARENA_MONS);

    int mult = 100;
    if (!preserve_mode)
        mult += 10;
    if (!autoroller)
        mult += 10;
    if (!smart_learn)
        mult -= 20;
    if (smart_cheat)
        mult += 30;
    if (ironman_shops)
        mult += 50;
    if (ironman_small_levels)
        mult += 10;
    if (ironman_empty_levels)
        mult += 20;
    if (!powerup_home)
        mult += 50;
    if (ironman_rooms)
        mult += 100;
    if (ironman_nightmare)
        mult += 100;

    if (mult < 5)
        mult = 5;

    DEPTH max_dl = 0;
    for (int i = 0; i < current_world_ptr->max_d_idx; i++)
        if (max_dlv[i] > max_dl)
            max_dl = max_dlv[i];

    u32b point_l = (creature_ptr->max_max_exp + (100 * max_dl));
    u32b point_h = point_l / 0x10000L;
    point_l = point_l % 0x10000L;
    point_h *= mult;
    point_l *= mult;
    point_h += point_l / 0x10000L;
    point_l %= 0x10000L;

    point_l += ((point_h % 100) << 16);
    point_h /= 100;
    point_l /= 100;

    u32b point = (point_h << 16) + (point_l);
    if (creature_ptr->arena_number >= 0)
        point += (arena_win * arena_win * (arena_win > 29 ? 1000 : 100));

    if (ironman_downward)
        point *= 2;
    if (creature_ptr->pclass == CLASS_BERSERKER) {
        if (creature_ptr->prace == RACE_SPECTRE)
            point = point / 5;
    }

    if ((creature_ptr->pseikaku == PERSONALITY_MUNCHKIN) && point) {
        point = 1;
        if (current_world_ptr->total_winner)
            point = 2;
    }

    if (easy_band)
        point = (0 - point);

    return point;
}

void cheat_death(player_type *creature_ptr)
{
    if (creature_ptr->sc)
        creature_ptr->sc = creature_ptr->age = 0;
    creature_ptr->age++;

    current_world_ptr->noscore |= 0x0001;
    msg_print(_("ウィザードモードに念を送り、死を欺いた。", "You invoke wizard mode and cheat death."));
    msg_print(NULL);

    (void)life_stream(creature_ptr, FALSE, FALSE);
    if (creature_ptr->pclass == CLASS_MAGIC_EATER) {
        int magic_idx;
        for (magic_idx = 0; magic_idx < EATER_EXT * 2; magic_idx++) {
            creature_ptr->magic_num1[magic_idx] = creature_ptr->magic_num2[magic_idx] * EATER_CHARGE;
        }
        for (; magic_idx < EATER_EXT * 3; magic_idx++) {
            creature_ptr->magic_num1[magic_idx] = 0;
        }
    }

    creature_ptr->csp = creature_ptr->msp;
    creature_ptr->csp_frac = 0;
    if (creature_ptr->word_recall) {
        msg_print(_("張りつめた大気が流れ去った...", "A tension leaves the air around you..."));
        msg_print(NULL);
        creature_ptr->word_recall = 0;
        creature_ptr->redraw |= (PR_STATUS);
    }

    if (creature_ptr->alter_reality) {
        creature_ptr->alter_reality = 0;
        creature_ptr->redraw |= (PR_STATUS);
    }

    (void)strcpy(creature_ptr->died_from, _("死の欺き", "Cheating death"));
    creature_ptr->is_dead = FALSE;
    (void)set_food(creature_ptr, PY_FOOD_MAX - 1);

    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    floor_ptr->dun_level = 0;
    floor_ptr->inside_arena = FALSE;
    creature_ptr->phase_out = FALSE;
    leaving_quest = 0;
    floor_ptr->inside_quest = 0;
    if (creature_ptr->dungeon_idx)
        creature_ptr->recall_dungeon = creature_ptr->dungeon_idx;
    creature_ptr->dungeon_idx = 0;
    if (lite_town || vanilla_town) {
        creature_ptr->wilderness_y = 1;
        creature_ptr->wilderness_x = 1;
        if (vanilla_town) {
            creature_ptr->oldpy = 10;
            creature_ptr->oldpx = 34;
        } else {
            creature_ptr->oldpy = 33;
            creature_ptr->oldpx = 131;
        }
    } else {
        creature_ptr->wilderness_y = 48;
        creature_ptr->wilderness_x = 5;
        creature_ptr->oldpy = 33;
        creature_ptr->oldpx = 131;
    }

    creature_ptr->wild_mode = FALSE;
    creature_ptr->leaving = TRUE;

    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, _("                            しかし、生き返った。", "                            but revived."));

    leave_floor(creature_ptr);
    wipe_monsters_list(creature_ptr);
}

/*!
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 祝福状態ならばTRUE
 */
bool is_blessed(player_type *creature_ptr)
{
    return creature_ptr->blessed || music_singing(creature_ptr, MUSIC_BLESS) || hex_spelling(creature_ptr, HEX_BLESS);
}

bool is_oppose_acid(player_type *creature_ptr)
{
    return creature_ptr->oppose_acid || music_singing(creature_ptr, MUSIC_RESIST) || (creature_ptr->special_defense & KATA_MUSOU);
}

bool is_oppose_elec(player_type *creature_ptr)
{
    return creature_ptr->oppose_elec || music_singing(creature_ptr, MUSIC_RESIST) || (creature_ptr->special_defense & KATA_MUSOU);
}

bool is_oppose_fire(player_type *creature_ptr)
{
    return creature_ptr->oppose_fire || music_singing(creature_ptr, MUSIC_RESIST) || (creature_ptr->special_defense & KATA_MUSOU);
}

bool is_oppose_cold(player_type *creature_ptr)
{
    return creature_ptr->oppose_cold || music_singing(creature_ptr, MUSIC_RESIST) || (creature_ptr->special_defense & KATA_MUSOU);
}

bool is_oppose_pois(player_type *creature_ptr)
{
    return creature_ptr->oppose_pois || music_singing(creature_ptr, MUSIC_RESIST) || (creature_ptr->special_defense & KATA_MUSOU);
}

bool is_tim_esp(player_type *creature_ptr)
{
    return creature_ptr->tim_esp || music_singing(creature_ptr, MUSIC_MIND) || (creature_ptr->concent >= CONCENT_TELE_THRESHOLD);
}

bool is_tim_stealth(player_type *creature_ptr) { return creature_ptr->tim_stealth || music_singing(creature_ptr, MUSIC_STEALTH); }

bool is_time_limit_esp(player_type *creature_ptr)
{
    return creature_ptr->tim_esp || music_singing(creature_ptr, MUSIC_MIND) || (creature_ptr->concent >= CONCENT_TELE_THRESHOLD);
}

bool is_time_limit_stealth(player_type *creature_ptr) { return creature_ptr->tim_stealth || music_singing(creature_ptr, MUSIC_STEALTH); }

bool can_two_hands_wielding(player_type *creature_ptr) { return !creature_ptr->riding || (creature_ptr->pet_extra_flags & PF_TWO_HANDS); }

/*!
 * @brief 歌の停止を処理する / Stop singing if the player is a Bard
 * @return なし
 */
void stop_singing(player_type *creature_ptr)
{
    if (creature_ptr->pclass != CLASS_BARD)
        return;

    /* Are there interupted song? */
    if (INTERUPTING_SONG_EFFECT(creature_ptr)) {
        /* Forget interupted song */
        INTERUPTING_SONG_EFFECT(creature_ptr) = MUSIC_NONE;
        return;
    }

    /* The player is singing? */
    if (!SINGING_SONG_EFFECT(creature_ptr))
        return;

    /* Hack -- if called from set_action(), avoid recursive loop */
    if (creature_ptr->action == ACTION_SING)
        set_action(creature_ptr, ACTION_NONE);

    /* Message text of each song or etc. */
    exe_spell(creature_ptr, REALM_MUSIC, SINGING_SONG_ID(creature_ptr), SPELL_STOP);

    SINGING_SONG_EFFECT(creature_ptr) = MUSIC_NONE;
    SINGING_SONG_ID(creature_ptr) = 0;
    creature_ptr->update |= (PU_BONUS);
    creature_ptr->redraw |= (PR_STATUS);
}

/*!
 * @brief 口を使う継続的な処理を中断する
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void stop_mouth(player_type *caster_ptr)
{
    if (music_singing_any(caster_ptr))
        stop_singing(caster_ptr);
    if (hex_spelling_any(caster_ptr))
        stop_hex_spell_all(caster_ptr);
}

/*!
 * @brief ペットの維持コスト計算
 * @return 維持コスト(%)
 */
PERCENTAGE calculate_upkeep(player_type *creature_ptr)
{
    MONSTER_IDX m_idx;
    bool have_a_unique = FALSE;
    DEPTH total_friend_levels = 0;

    total_friends = 0;

    for (m_idx = creature_ptr->current_floor_ptr->m_max - 1; m_idx >= 1; m_idx--) {
        monster_type *m_ptr;
        monster_race *r_ptr;

        m_ptr = &creature_ptr->current_floor_ptr->m_list[m_idx];
        if (!monster_is_valid(m_ptr))
            continue;
        r_ptr = &r_info[m_ptr->r_idx];

        if (is_pet(m_ptr)) {
            total_friends++;
            if (r_ptr->flags1 & RF1_UNIQUE) {
                if (creature_ptr->pclass == CLASS_CAVALRY) {
                    if (creature_ptr->riding == m_idx)
                        total_friend_levels += (r_ptr->level + 5) * 2;
                    else if (!have_a_unique && (r_info[m_ptr->r_idx].flags7 & RF7_RIDING))
                        total_friend_levels += (r_ptr->level + 5) * 7 / 2;
                    else
                        total_friend_levels += (r_ptr->level + 5) * 10;
                    have_a_unique = TRUE;
                } else
                    total_friend_levels += (r_ptr->level + 5) * 10;
            } else
                total_friend_levels += r_ptr->level;
        }
    }

    if (total_friends) {
        int upkeep_factor;
        upkeep_factor = (total_friend_levels - (creature_ptr->lev * 80 / (cp_ptr->pet_upkeep_div)));
        if (upkeep_factor < 0)
            upkeep_factor = 0;
        if (upkeep_factor > 1000)
            upkeep_factor = 1000;
        return upkeep_factor;
    } else
        return 0;
}

bool music_singing(player_type *caster_ptr, int music_songs) { return (caster_ptr->pclass == CLASS_BARD) && (caster_ptr->magic_num1[0] == music_songs); }
