#pragma once

#include "system/angband.h"

#include "combat/combat-options-type.h"
#include "object-enchant/tr-flags.h"
#include "effect/attribute-types.h"

struct monster_type;
struct object_type;
class PlayerType;
MULTIPLY mult_slaying(PlayerType *player_ptr, MULTIPLY mult, const TrFlags &flgs, monster_type *m_ptr);
MULTIPLY mult_brand(PlayerType *player_ptr, MULTIPLY mult, const TrFlags &flgs, monster_type *m_ptr);
HIT_POINT calc_attack_damage_with_slay(PlayerType *player_ptr, object_type *o_ptr, HIT_POINT tdam, monster_type *m_ptr, combat_options mode, bool thrown);
AttributeFlags melee_attribute(PlayerType *player_ptr, object_type *o_ptr, combat_options mode);
