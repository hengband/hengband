#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
int32_t get_current_ki(player_type *caster_ptr);
void set_current_ki(player_type *caster_ptr, bool is_reset, int32_t ki);
bool clear_mind(player_type *creature_ptr);
void set_lightspeed(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_sh_force(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool shock_power(player_type *caster_ptr);

enum mind_force_trainer_type : int;
bool cast_force_spell(player_type *caster_ptr, mind_force_trainer_type spell);
