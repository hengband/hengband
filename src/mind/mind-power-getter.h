#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
bool get_mind_power(player_type *caster_ptr, SPELL_IDX *sn, bool only_browse);
