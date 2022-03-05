#pragma once

#include "system/angband.h"
#include <optional>

struct effect_monster_type;
class CapturedMonsterType;
class PlayerType;
ProcessResult switch_effects_monster(PlayerType *player_ptr, effect_monster_type *em_ptr, std::optional<CapturedMonsterType *> cap_mon_ptr = std::nullopt);
