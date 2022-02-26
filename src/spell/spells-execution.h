#pragma once

#include "spell/spells-util.h"
#include "system/angband.h"

class PlayerType;
concptr exe_spell(PlayerType *player_ptr, int16_t realm, SPELL_IDX spell, SpellProcessType mode);
