#pragma once

#include "spell/spells-util.h"
#include "system/angband.h"
#include <optional>
#include <string>

enum class RealmType;
class PlayerType;
std::optional<std::string> exe_spell(PlayerType *player_ptr, RealmType realm, SPELL_IDX spell, SpellProcessType mode);
