#pragma once

#include "system/angband.h"

#define MAX_RUN_CYCLES 17
#define MAX_RUN_CHOME 10

extern bool ignore_avoid_run;
extern byte cycle[MAX_RUN_CYCLES];
extern byte chome[MAX_RUN_CHOME];

void run_step(player_type *creature_ptr, DIRECTION dir);
