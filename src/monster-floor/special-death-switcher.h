#pragma once
#include "effect/attribute-types.h"
#include "system/angband.h"

class MonsterDeath;
class PlayerType;
void switch_special_death(PlayerType *player_ptr, MonsterDeath *md_ptr, AttributeFlags attribute_flags);
