#pragma once

#include "system/angband.h"

enum mind_berserker_type : int;
bool cast_berserk_spell(player_type *caster_ptr, mind_berserker_type spell);
