#pragma once

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
extern void mindcraft_info(char *p, int use_mind, int power);
extern void do_cmd_mind(void);
extern void do_cmd_mind_browse(void);

