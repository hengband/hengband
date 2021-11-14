#pragma once

#include "system/angband.h"
#include "spell/spells-util.h"

class PlayerType;
concptr do_chaos_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode);
