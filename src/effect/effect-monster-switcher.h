#pragma once

#include "system/angband.h"
#include <tl/optional.hpp>

class EffectMonster;
class CapturedMonsterType;
class PlayerType;
ProcessResult switch_effects_monster(PlayerType *player_ptr, EffectMonster *em_ptr, tl::optional<CapturedMonsterType *> cap_mon_ptr = tl::nullopt);
