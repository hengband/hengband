#pragma once

#include "system/angband.h"

#define MAX_VIRTUE 18 /*!< 徳定義の最大数 */

enum virtue_idx {
    V_NONE = 0,
    V_COMPASSION = 1,
	V_HONOUR = 2,
	V_JUSTICE = 3,
	V_SACRIFICE = 4,
	V_KNOWLEDGE = 5,
	V_FAITH = 6,
	V_ENLIGHTEN = 7,
	V_ENCHANT = 8,
	V_CHANCE = 9,
	V_NATURE = 10,
	V_HARMONY = 11,
	V_VITALITY = 12,
	V_UNLIFE = 13,
	V_PATIENCE = 14,
	V_TEMPERANCE = 15,
	V_DILIGENCE = 16,
	V_VALOUR = 17,
	V_INDIVIDUALISM = 18,
};

#define VIRTUE_LARGE 1
#define VIRTUE_SMALL 2

struct player_type;
bool compare_virtue(player_type *player_ptr, int type, int num, int tekitou);
int virtue_number(player_type *player_ptr, int type);
extern concptr virtue[MAX_VIRTUE];
void initialize_virtues(player_type *player_ptr);
void chg_virtue(player_type *player_ptr, int virtue, int amount);
void set_virtue(player_type *player_ptr, int virtue, int amount);
void dump_virtues(player_type *player_ptr, FILE *OutFile);
