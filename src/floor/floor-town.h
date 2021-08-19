﻿#pragma once

#include "system/angband.h"

typedef struct store_type store_type;

/*
 * A structure describing a town with
 * stores and buildings
 */
typedef struct town_type {
	GAME_TEXT name[32];
	uint seed;      /* Seed for RNG */
	store_type *store;    /* The stores [MAX_STORES] */
	byte numstores;
} town_type;

extern short max_towns;
extern town_type *town_info;
