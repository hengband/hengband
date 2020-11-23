#pragma once

#include "system/angband.h"

/*! オートローラの年齢、身長、体重、社会的地位の要求水準 */
typedef struct {
    s16b agemin, agemax;
    s16b htmin, htmax;
    s16b wtmin, wtmax;
    s16b scmin, scmax;
} chara_limit_type;

extern s16b stat_limit[6];
extern s32b stat_match[6];
extern s32b auto_round;
extern s32b auto_upper_round;

/*! オートローラの要求値実現確率 */
static s32b autoroll_chance;

/* emulate 5 + 1d3 + 1d4 + 1d5 by randint0(60) */
static BASE_STATUS rand3_4_5[60] = {
    8, 9, 9, 9, 10, 10, 10, 10, 10, 10, /*00-09*/
    11, 11, 11, 11, 11, 11, 11, 11, 11, 12, /*10-19*/
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, /*20-29*/
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, /*30-49*/
    13, 14, 14, 14, 14, 14, 14, 14, 14, 14, /*40-49*/
    15, 15, 15, 15, 15, 15, 16, 16, 16, 17 /*50-59*/
};

bool get_stat_limits(player_type *creature_ptr);
void initialize_chara_limit(chara_limit_type *chara_limit_ptr);
bool get_chara_limits(player_type *creature_ptr, chara_limit_type *chara_limit_ptr);
