#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"
#include <tl/optional.hpp>

class CapturedMonsterType;
class FallOffHorseEffect;
class PlayerType;
bool affect_monster(PlayerType *player_ptr, MONSTER_IDX src_idx, POSITION r, POSITION y, POSITION x, int dam, AttributeType typ, BIT_FLAGS flag, bool see_s_msg, tl::optional<CapturedMonsterType *> cap_mon_ptr = tl::nullopt, FallOffHorseEffect *fall_off_horse_effect = nullptr);
