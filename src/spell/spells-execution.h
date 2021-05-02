#pragma once

#include "system/angband.h"
#include "spell/spells-util.h"

typedef struct player_type player_type;
concptr exe_spell(player_type *caster_ptr, REALM_IDX realm, SPELL_IDX spell, spell_type mode);
