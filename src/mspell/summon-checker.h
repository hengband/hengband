#pragma once

#include "system/angband.h"

class PlayerType;
bool check_summon_specific(PlayerType *player_ptr, MONRACE_IDX summoner_idx, MONRACE_IDX r_idx);
