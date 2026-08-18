#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"

class PlayerType;
class AbstractAttribute;
bool affect_feature(PlayerType *player_ptr, MONSTER_IDX src_idx, POSITION r, POSITION y, POSITION x, int dam, AttributeType typ, const std::shared_ptr<AbstractAttribute> &attribute_ptr = nullptr);
