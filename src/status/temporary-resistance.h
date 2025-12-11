#pragma once

#include "system/angband.h"

class PlayerType;
bool set_tim_levitation(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_ultimate_res(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_nether(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_lite(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_dark(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_fear(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_time(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_imm_dark(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
