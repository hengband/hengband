#pragma once

typedef enum gf_switch_result
{
	GF_SWITCH_FALSE = 0,
	GF_SWITCH_TRUE = 1,
	GF_SWITCH_CONTINUE = 2,
} gf_switch_result;

gf_switch_result switch_effects_monster(player_type *caster_ptr, effect_monster_type *em_ptr);
