#pragma once

#include "system/angband.h"
#include "spell/spells-util.h"

struct player_type;
concptr do_hissatsu_spell(player_type *caster_ptr, SPELL_IDX spell, spell_type mode);
