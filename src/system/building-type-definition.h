#pragma once

/* 人畜無害なenumヘッダを先に読み込む */
#include "object/tval-types.h"
#include "player-info/class-types.h"
#include "player-info/race-types.h"
#include "realm/realm-types.h"
#include "system/angband.h"

#define MAX_BLDG 32 /*!< 施設の種類最大数 / Number of buildings */

/*!
 * @brief 闘技場のモンスターエントリー構造体 / A structure type for on_defeat_arena_monster entry
 */
typedef struct arena_type {
    MONRACE_IDX r_idx; /*!< 闘技場のモンスター種族ID(0ならば表彰式) / Monster (0 means victory prizing) */
    tval_type tval; /*!< モンスター打倒後に得られるアイテムの大カテゴリID / tval of prize (0 means no prize) */
    OBJECT_SUBTYPE_VALUE sval; /*!< モンスター打倒後に得られるアイテムの小カテゴリID / sval of prize */
} arena_type;

typedef struct building_type {
    GAME_TEXT name[20]; /* proprietor name */
    GAME_TEXT owner_name[20]; /* proprietor name */
    GAME_TEXT owner_race[20]; /* proprietor race */

    GAME_TEXT act_names[8][30]; /* action names */
    PRICE member_costs[8]; /* Costs for class members of building */
    PRICE other_costs[8]; /* Costs for nonguild members */
    char letters[8]; /* action letters */
    int16_t actions[8]; /*!< 町の施設処理における行動ID */
    int16_t action_restr[8]; /*!< 町の施設処理の規制処理ID */

    player_class_type member_class[MAX_CLASS]; /* which classes are part of guild */
    PlayerRaceType member_race[MAX_RACES]; /* which classes are part of guild */
    int16_t member_realm[MAX_MAGIC + 1]; /* ギルド (店主？)ごとの魔法領域ID / which realms are part of guild */
} building_type;

extern building_type building[MAX_BLDG];
extern MONRACE_IDX battle_mon[4];
