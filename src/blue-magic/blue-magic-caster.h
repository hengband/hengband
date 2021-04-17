#pragma once

#include "system/angband.h"

enum class RF_ABILITY;

bool cast_learned_spell(player_type *caster_ptr, RF_ABILITY spell, const bool success);
