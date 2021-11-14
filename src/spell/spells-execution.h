#pragma once

#include "system/angband.h"
#include "spell/spells-util.h"

class PlayerType;
concptr exe_spell(PlayerType *player_ptr, int16_t realm, SPELL_IDX spell, SpellProcessType mode);
