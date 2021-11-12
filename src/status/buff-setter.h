#pragma once

#include "system/angband.h"

class PlayerType;
void reset_tim_flags(PlayerType *player_ptr);
bool set_fast(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_shield(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_magicdef(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_blessed(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_hero(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_mimic(PlayerType *player_ptr, TIME_EFFECT v, int16_t mimic_race_idx, bool do_dec);
bool set_shero(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_wraith_form(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tsuyoshi(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
