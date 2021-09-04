#pragma once

#include "system/angband.h"

struct player_type;
void reset_tim_flags(player_type *creature_ptr);
bool set_fast(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_shield(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_magicdef(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_blessed(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_hero(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_mimic(player_type *creature_ptr, TIME_EFFECT v, int16_t mimic_race_idx, bool do_dec);
bool set_shero(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_wraith_form(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tsuyoshi(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
