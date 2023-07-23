#pragma once

#include "spell/spells-util.h"
#include "system/angband.h"
#include <optional>
#include <string>

class PlayerType;
std::optional<std::string> exe_spell(PlayerType *player_ptr, int16_t realm, SPELL_IDX spell, SpellProcessType mode);
