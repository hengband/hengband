#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
bool check_summon_specific(player_type *player_ptr, MONRACE_IDX summoner_idx, MONRACE_IDX r_idx);
