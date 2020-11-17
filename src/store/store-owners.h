#pragma once

#include "system/angband.h"

#define MAX_STORES      10 /*!< 店舗の種類最大数 / Total number of stores (see "store.c", etc) */
#define MAX_OWNERS      32 /*!< 各店舗毎の店主定義最大数 / Total number of owners per store (see "store.c", etc) */

typedef struct owner_type {
	concptr owner_name;	/* Name */
	PRICE max_cost;		/* Purse limit */
	byte max_inflate;	/* Inflation (max) */
	byte min_inflate;	/* Inflation (min) */
	byte haggle_per;	/* Haggle unit */
	byte insult_max;	/* Insult limit */
	byte owner_race;	/* Owner race */
} owner_type;

extern const owner_type owners[MAX_STORES][MAX_OWNERS];
