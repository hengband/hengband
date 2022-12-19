#pragma once

#include "spell/spells-util.h"
#include "system/angband.h"

class PlayerType;
std::optional<std::string> do_music_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode);
