#pragma once

#include "system/angband.h"

struct player_type;
bool check_summon_specific(player_type *player_ptr, MONRACE_IDX summoner_idx, MONRACE_IDX r_idx);
