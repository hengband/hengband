#pragma once

#include "system/angband.h"

class PlayerType;
bool set_tim_levitation(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_ultimate_res(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_nether(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_time(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_lite(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_dark(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_shard(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_blind(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_conf(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_sound(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_nexus(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_chaos(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_disenchant(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_water(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_fear(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_curse(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
