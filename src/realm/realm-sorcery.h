#pragma once

#include "system/angband.h"
#include "spell/spells-util.h"

class PlayerType;
concptr do_sorcery_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode);
