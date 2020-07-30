#pragma once

#include "system/angband.h"

bool psychometry(player_type *caster_ptr);

typedef enum mind_mindcrafter_type mind_mindcrafter_type;
bool cast_mindcrafter_spell(player_type *caster_ptr, mind_mindcrafter_type spell);
