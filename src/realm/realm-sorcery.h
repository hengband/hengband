#pragma once

#include "spell/spells-util.h"
#include "system/angband.h"

class PlayerType;
concptr do_sorcery_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode);
