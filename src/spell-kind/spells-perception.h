#pragma once

#include "system/angband.h"

struct object_type;;
struct player_type;
void identify_pack(player_type *target_ptr);
bool identify_item(player_type *owner_ptr, object_type *o_ptr);
bool ident_spell(player_type *caster_ptr, bool only_equip);
bool identify_fully(player_type *caster_ptr, bool only_equip);
