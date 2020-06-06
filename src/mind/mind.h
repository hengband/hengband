#pragma once

#include "system/angband.h"

#define MIND_MINDCRAFTER    0 /*!< 特殊能力: 超能力 */
#define MIND_KI             1 /*!< 特殊能力: 練気 */
#define MIND_BERSERKER      2 /*!< 特殊能力: 怒り */
#define MIND_MIRROR_MASTER  3 /*!< 特殊能力: 鏡魔法 */
#define MIND_NINJUTSU       4 /*!< 特殊能力: 忍術 */

#define MAX_MIND_POWERS  21 /*!< 超能力の数 / Mindcraft */

/* Mindcrafters */
typedef struct mind_type mind_type;
struct mind_type
{
	PLAYER_LEVEL min_lev;
	MANA_POINT mana_cost;
	PERCENTAGE fail;
	concptr name;
};

typedef struct mind_power mind_power;
struct mind_power
{
	mind_type info[MAX_MIND_POWERS];
};

/* mind.c */
extern mind_power const mind_powers[5];
extern void mindcraft_info(player_type *caster_ptr, char *p, int use_mind, int power);
extern void do_cmd_mind(player_type *caster_ptr);
extern void do_cmd_mind_browse(player_type *caster_ptr);
extern void do_cmd_mind_browse(player_type *caster_ptr);

