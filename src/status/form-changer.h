#pragma once

#include "system/angband.h"

bool set_mimic(player_type *creature_ptr, TIME_EFFECT v, MIMIC_RACE_IDX p, bool do_dec);
bool set_shero(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_wraith_form(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tsuyoshi(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
