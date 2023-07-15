#pragma once

#include "system/angband.h"
#include "system/system-variables.h"

#define MAX_ADJ_STR 38
#define MAX_ADJ_DEX 39

extern const int monk_ave_damage[PY_MAX_LEVEL + 1][3];
extern const byte adj_str_blow[MAX_ADJ_STR];
extern const byte adj_dex_blow[MAX_ADJ_DEX];
extern const byte blows_table[12][12];
