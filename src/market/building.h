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
	player_race_table member_race[MAX_RACES];    /* which classes are part of guild */
	REALM_IDX member_realm[MAX_MAGIC + 1]; /* which realms are part of guild */
};

extern building_type building[MAX_BLDG];
extern bool reinit_wilderness;
extern MONRACE_IDX today_mon;

/*
 * Buildings actions
 */
#define BACT_NOTHING                 0
#define BACT_RESEARCH_ITEM           1
#define BACT_TOWN_HISTORY            2
#define BACT_RACE_LEGENDS            3
#define BACT_GREET_KING              4
#define BACT_KING_LEGENDS            5
#define BACT_QUEST                   6
#define BACT_XXX_UNUSED              7
#define BACT_POSTER                  8
#define BACT_ARENA_RULES             9
#define BACT_ARENA                  10
#define BACT_ARENA_LEGENDS          11
#define BACT_IN_BETWEEN             12
#define BACT_GAMBLE_RULES           13
#define BACT_CRAPS                  14
#define BACT_SPIN_WHEEL             15
#define BACT_DICE_SLOTS             16
#define BACT_REST                   17
#define BACT_FOOD                   18
#define BACT_RUMORS                 19
#define BACT_RESEARCH_MONSTER       20
#define BACT_COMPARE_WEAPONS        21
#define BACT_LEGENDS                22
#define BACT_ENCHANT_WEAPON         23
#define BACT_ENCHANT_ARMOR          24
#define BACT_RECHARGE               25
#define BACT_IDENTS                 26
#define BACT_LEARN                  27
#define BACT_HEALING                28
#define BACT_RESTORE                29
#define BACT_ENCHANT_ARROWS         30
#define BACT_ENCHANT_BOW            31
#define BACT_GREET                  32
#define BACT_RECALL                 33
#define BACT_TELEPORT_LEVEL         34
#define BACT_LOSE_MUTATION          35
#define BACT_BATTLE                 36
#define BACT_TSUCHINOKO             37
#define BACT_TARGET                 38
#define BACT_BOUNTY                 39
#define BACT_KANKIN                 40
#define BACT_HEIKOUKA               41
#define BACT_TELE_TOWN              42
#define BACT_POKER                  43
#define BACT_IDENT_ONE              44
#define BACT_RECHARGE_ALL           45
#define BACT_EVAL_AC		        46
#define BACT_BROKEN_WEAPON          47
#define MAX_BACT                    48

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

extern void update_gambling_monsters(player_type *player_ptr);
extern void do_cmd_bldg(player_type *player_ptr);

extern void determine_daily_bounty(player_type *player_ptr, bool conv_old);
extern void determine_bounty_uniques(player_type *player_ptr);
