#pragma once

#include "system/angband.h"

#include "combat/combat-options-type.h"
#include "effect/attribute-types.h"
#include "object-enchant/tr-flags.h"

struct monster_type;
class ObjectType;
class PlayerType;
MULTIPLY mult_slaying(PlayerType *player_ptr, MULTIPLY mult, const TrFlags &flgs, monster_type *m_ptr);
MULTIPLY mult_brand(PlayerType *player_ptr, MULTIPLY mult, const TrFlags &flgs, monster_type *m_ptr);
int calc_attack_damage_with_slay(PlayerType *player_ptr, ObjectType *o_ptr, int tdam, monster_type *m_ptr, combat_options mode, bool thrown);
AttributeFlags melee_attribute(PlayerType *player_ptr, ObjectType *o_ptr, combat_options mode);
