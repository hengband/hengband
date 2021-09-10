#pragma once

#include "system/angband.h"
#include "spell/spells-util.h"

struct player_type;
concptr do_daemon_spell(player_type *player_ptr, SPELL_IDX spell, spell_type mode);

