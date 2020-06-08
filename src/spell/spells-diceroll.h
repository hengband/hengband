#pragma once

#include "system/angband.h"
#include "system/monster-type-definition.h"

bool common_saving_throw_control(player_type *operator_ptr, HIT_POINT pow, monster_type *m_ptr);
bool common_saving_throw_charm(player_type *operator_ptr, HIT_POINT pow, monster_type *m_ptr);
PERCENTAGE beam_chance(player_type *caster_ptr);
