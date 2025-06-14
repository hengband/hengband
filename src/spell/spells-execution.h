#pragma once

#include "spell/spells-util.h"
#include "system/angband.h"
#include <string>
#include <tl/optional.hpp>

enum class RealmType;
class PlayerType;
tl::optional<std::string> exe_spell(PlayerType *player_ptr, RealmType realm, SPELL_IDX spell, SpellProcessType mode);
