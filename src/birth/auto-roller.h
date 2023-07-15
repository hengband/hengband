#pragma once

#include "system/angband.h"

/*! オートローラの年齢、身長、体重、社会的地位の要求水準 */
struct chara_limit_type {
    int16_t agemin, agemax;
    int16_t htmin, htmax;
    int16_t wtmin, wtmax;
    int16_t scmin, scmax;
};

extern int16_t stat_limit[6];
extern int32_t auto_round;
extern int32_t auto_upper_round;
extern int32_t autoroll_chance;

class PlayerType;
bool get_stat_limits(PlayerType *player_ptr);
void initialize_chara_limit(chara_limit_type *chara_limit_ptr);
bool get_chara_limits(PlayerType *player_ptr, chara_limit_type *chara_limit_ptr);
