#pragma once

#include "player-ability/player-ability-types.h"
#include "system/angband.h"
#include "system/system-variables.h"

extern const byte adj_mag_study[];
extern const byte adj_mag_mana[];
extern const byte adj_mag_fail[];
extern const byte adj_mag_stat[];
extern const byte adj_chr_gold[];
extern const byte adj_int_dev[];
extern const byte adj_wis_sav[];
extern const byte adj_dex_dis[];
extern const byte adj_int_dis[];
extern const byte adj_dex_ta[];
extern const byte adj_str_td[];
extern const byte adj_dex_th[];
extern const byte adj_str_th[];
extern const byte adj_str_wgt[];
extern const byte adj_str_hold[];
extern const byte adj_str_dig[];
extern const byte adj_dex_safe[];
extern const byte adj_con_fix[];
extern const byte adj_con_mhp[];
extern const byte adj_chr_chm[];

extern const concptr stat_names[A_MAX];
extern const concptr stat_names_reduced[A_MAX];

extern const int32_t player_exp[PY_MAX_LEVEL];
extern const int32_t player_exp_a[PY_MAX_LEVEL];
