#pragma once

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

	CLASS_IDX member_class[MAX_CLASS];   /* which classes are part of guild */
	RACE_IDX member_race[MAX_RACES];    /* which classes are part of guild */
	REALM_IDX member_realm[MAX_MAGIC + 1]; /* which realms are part of guild */
};

extern building_type building[MAX_BLDG];

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
#define BACT_KUBI                   39
#define BACT_KANKIN                 40
#define BACT_HEIKOUKA               41
#define BACT_TELE_TOWN              42
#define BACT_POKER                  43
#define BACT_IDENT_ONE              44
#define BACT_RECHARGE_ALL           45
#define BACT_EVAL_AC		        46
#define BACT_BROKEN_WEAPON          47
#define MAX_BACT                    48

extern const arena_type arena_info[MAX_ARENA_MONS + 2];
extern void update_gambling_monsters(void);
extern void do_cmd_bldg(void);

extern void clear_bldg(int min_row, int max_row);
