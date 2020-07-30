#pragma once

#include "system/angband.h"

MAGIC_NUM1 get_current_ki(player_type *caster_ptr);
void set_current_ki(player_type *caster_ptr, bool is_reset, MAGIC_NUM1 ki);
bool clear_mind(player_type *creature_ptr);
void set_lightspeed(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_sh_force(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool shock_power(player_type *caster_ptr);

typedef enum mind_force_trainer_type mind_force_trainer_type;
bool cast_force_spell(player_type *caster_ptr, mind_force_trainer_type spell);
