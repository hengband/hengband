#pragma once

#include "system/angband.h"

/*! オートローラの年齢、身長、体重、社会的地位の要求水準 */
typedef struct {
    int16_t agemin, agemax;
    int16_t htmin, htmax;
    int16_t wtmin, wtmax;
    int16_t scmin, scmax;
} chara_limit_type;

extern int16_t stat_limit[6];
extern int32_t auto_round;
extern int32_t auto_upper_round;
extern int32_t autoroll_chance;

typedef struct player_type player_type;
bool get_stat_limits(player_type *creature_ptr);
void initialize_chara_limit(chara_limit_type *chara_limit_ptr);
bool get_chara_limits(player_type *creature_ptr, chara_limit_type *chara_limit_ptr);
