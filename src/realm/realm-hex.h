#pragma once

#include "realm/realm-hex-numbers.h"
#include "system/angband.h"
#include "spell/spells-util.h"

struct player_type;
concptr do_hex_spell(player_type *player_ptr, spell_hex_type spell, spell_type mode);
