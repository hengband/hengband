#pragma once

#include "system/angband.h"

#define MAX_VIRTUE 18 /*!< 徳定義の最大数 */

#define V_COMPASSION    1
#define V_HONOUR	    2
#define V_JUSTICE	    3
#define V_SACRIFICE	    4
#define V_KNOWLEDGE	    5
#define V_FAITH 	    6
#define V_ENLIGHTEN	    7
#define V_ENCHANT	    8
#define V_CHANCE	    9
#define V_NATURE	   10
#define V_HARMONY	   11
#define V_VITALITY	   12
#define V_UNLIFE	   13
#define V_PATIENCE	   14
#define V_TEMPERANCE	   15
#define V_DILIGENCE	   16
#define V_VALOUR	   17
#define V_INDIVIDUALISM    18

#define VIRTUE_LARGE 1
#define VIRTUE_SMALL 2

typedef struct player_type player_type;
bool compare_virtue(player_type *creature_ptr, int type, int num, int tekitou);
int virtue_number(player_type *creature_ptr, int type);
extern concptr virtue[MAX_VIRTUE];
void get_virtues(player_type *creature_ptr);
void chg_virtue(player_type *creature_ptr, int virtue, int amount);
void set_virtue(player_type *creature_ptr, int virtue, int amount);
void dump_virtues(player_type *creature_ptr, FILE *OutFile);
