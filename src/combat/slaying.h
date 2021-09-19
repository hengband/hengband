#pragma once

#include "system/angband.h"

#include "combat/combat-options-type.h"
#include "object-enchant/tr-flags.h"

struct monster_type;
struct object_type;;
struct player_type;
MULTIPLY mult_slaying(player_type *player_ptr, MULTIPLY mult, const TrFlags &flgs, monster_type *m_ptr);
MULTIPLY mult_brand(player_type *player_ptr, MULTIPLY mult, const TrFlags &flgs, monster_type *m_ptr);
HIT_POINT calc_attack_damage_with_slay(player_type *player_ptr, object_type *o_ptr, HIT_POINT tdam, monster_type *m_ptr, combat_options mode, bool thrown);
