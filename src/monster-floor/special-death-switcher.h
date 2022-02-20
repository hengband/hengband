#pragma once
#include "system/angband.h"
#include "effect/attribute-types.h"

struct monster_death_type;
class PlayerType;
void switch_special_death(PlayerType *player_ptr, monster_death_type *md_ptr, AttributeFlags attribute_flags);
