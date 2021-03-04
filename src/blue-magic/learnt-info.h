#pragma once

#include "system/angband.h"

enum monster_spell_type : int;

PLAYER_LEVEL get_pseudo_monstetr_level(player_type *caster_ptr);
void learnt_info(player_type *learner_ptr, char *p, monster_spell_type power);
