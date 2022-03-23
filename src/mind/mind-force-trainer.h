#pragma once

#include "system/angband.h"

class PlayerType;
int32_t get_current_ki(PlayerType *player_ptr);
void set_current_ki(PlayerType *player_ptr, bool is_reset, int32_t ki);
bool clear_mind(PlayerType *player_ptr);
void set_lightspeed(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_sh_force(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool shock_power(PlayerType *player_ptr);

enum class MindForceTrainerType : int;
bool cast_force_spell(PlayerType *player_ptr, MindForceTrainerType spell);
