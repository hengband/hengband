#pragma once

#include "system/angband.h"

#include "combat/combat-options-type.h"
#include "effect/attribute-types.h"
#include "object-enchant/tr-flags.h"

class MonsterEntity;
class ItemEntity;
class PlayerType;
MULTIPLY mult_slaying(PlayerType *player_ptr, MULTIPLY mult, const TrFlags &flags, const MonsterEntity &monster);
MULTIPLY mult_brand(PlayerType *player_ptr, MULTIPLY mult, const TrFlags &flags, const MonsterEntity &monster);
int calc_attack_damage_with_slay(PlayerType *player_ptr, ItemEntity *o_ptr, int tdam, const MonsterEntity &monster, combat_options mode, bool thrown);
AttributeFlags melee_attribute(PlayerType *player_ptr, ItemEntity *o_ptr, combat_options mode);
