#pragma once

#include "system/angband.h"
#include "effect/attribute-types.h"
#include <optional>

class CapturedMonsterType;
class PlayerType;
bool affect_monster(PlayerType *player_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, int dam, AttributeType typ, BIT_FLAGS flag, bool see_s_msg, std::optional<CapturedMonsterType *> cap_mon_ptr = std::nullopt);
