#pragma once

#include "system/angband.h"

#define MAX_ARENA_MONS 41 /*<! 闘技場のイベント件数 -KMW- */

typedef struct arena_type arena_type;
extern const arena_type arena_info[MAX_ARENA_MONS + 2];
