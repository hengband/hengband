#pragma once

#include "system/angband.h"

enum class RF_ABILITY;

PLAYER_LEVEL get_pseudo_monstetr_level(player_type *caster_ptr);
void learnt_info(player_type *learner_ptr, char *p, RF_ABILITY power);
