#pragma once

#include "system/angband.h"
#include "spell/spells-util.h"

typedef struct player_type player_type;
concptr do_crusade_spell(player_type *caster_ptr, SPELL_IDX spell, spell_type mode);
