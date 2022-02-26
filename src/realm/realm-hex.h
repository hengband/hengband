#pragma once

#include "realm/realm-hex-numbers.h"
#include "spell/spells-util.h"
#include "system/angband.h"

class PlayerType;
concptr do_hex_spell(PlayerType *player_ptr, spell_hex_type spell, SpellProcessType mode);
