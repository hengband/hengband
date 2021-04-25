#pragma once

#include "system/angband.h"

typedef struct monster_type monster_type;
typedef struct player_type player_type;
bool common_saving_throw_control(player_type *operator_ptr, HIT_POINT pow, monster_type *m_ptr);
bool common_saving_throw_charm(player_type *operator_ptr, HIT_POINT pow, monster_type *m_ptr);
PERCENTAGE beam_chance(player_type *caster_ptr);
