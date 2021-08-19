﻿#pragma once

#include "system/angband.h"

/*! オートローラの年齢、身長、体重、社会的地位の要求水準 */
typedef struct {
    short agemin, agemax;
    short htmin, htmax;
    short wtmin, wtmax;
    short scmin, scmax;
} chara_limit_type;

extern short stat_limit[6];
extern s32b auto_round;
extern s32b auto_upper_round;
extern s32b autoroll_chance;

typedef struct player_type player_type;
bool get_stat_limits(player_type *creature_ptr);
void initialize_chara_limit(chara_limit_type *chara_limit_ptr);
bool get_chara_limits(player_type *creature_ptr, chara_limit_type *chara_limit_ptr);
