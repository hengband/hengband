#pragma once

/*
 * A structure describing a town with
 * stores and buildings
 */
typedef struct town_type town_type;
struct town_type
{
	GAME_TEXT name[32];
	u32b seed;      /* Seed for RNG */
	store_type *store;    /* The stores [MAX_STORES] */
	byte numstores;
};

extern TOWN_IDX max_towns;
extern town_type *town_info;
