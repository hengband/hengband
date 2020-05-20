#pragma once

#include "realm/realm.h"
#include "player/player-race.h"
#include "player/race-info-table.h"
#include "player/player-class.h"
#include "player/player-races-table.h"
#include "player/player-classes-table.h"

#define MAX_BLDG 32 /*!< 施設の種類最大数 / Number of buildings */

#define BUILDING_NON_MEMBER 0 /*!< 不明(現在未使用) */
#define BUILDING_MEMBER     1 /*!< 不明(現在未使用) */
#define BUILDING_OWNER      2 /*!< 施設の種族/職業条件が一致している状態値 */

/*
 * Arena constants
 */
#define ARENA_DEFEATED_OLD_VER (-(MAX_SHORT)) /*<! 旧バージョンの闘技場敗北定義 */

/*
 * A structure to describe a building.
 * From Kamband
 */
typedef struct building_type building_type;

struct building_type
{
	GAME_TEXT name[20];                  /* proprietor name */
	GAME_TEXT owner_name[20];            /* proprietor name */
	GAME_TEXT owner_race[20];            /* proprietor race */

	GAME_TEXT act_names[8][30];          /* action names */
	PRICE member_costs[8];           /* Costs for class members of building */
	PRICE other_costs[8];		    /* Costs for nonguild members */
	char letters[8];                /* action letters */
	BACT_IDX actions[8];                /* action codes */
	BACT_RESTRICT_IDX action_restr[8];           /* action restrictions */

	player_class_type member_class[MAX_CLASS];   /* which classes are part of guild */
	player_race_type member_race[MAX_RACES];    /* which classes are part of guild */
	REALM_IDX member_realm[MAX_MAGIC + 1]; /* which realms are part of guild */
};

extern building_type building[MAX_BLDG];
extern bool reinit_wilderness;
extern MONRACE_IDX today_mon;

extern MONRACE_IDX battle_mon[4];
extern u32b mon_odds[4];
extern int battle_odds;
extern PRICE kakekin;
extern int sel_monster;

/*!
 * @struct arena_type
 * @brief 闘技場のモンスターエントリー構造体 / A structure type for arena entry
 */
typedef struct
{
	MONRACE_IDX r_idx; /*!< 闘技場のモンスター種族ID(0ならば表彰式) / Monster (0 means victory prizing) */
	OBJECT_TYPE_VALUE tval;  /*!< モンスター打倒後に得られるアイテムの大カテゴリID / tval of prize (0 means no prize) */
	OBJECT_SUBTYPE_VALUE sval;  /*!< モンスター打倒後に得られるアイテムの小カテゴリID / sval of prize */
} arena_type;

extern void do_cmd_building(player_type *player_ptr);
