#pragma once

#include "system/angband.h"

struct object_type;
struct player_type;
void identify_pack(player_type *player_ptr);
bool identify_item(player_type *player_ptr, object_type *o_ptr);
bool ident_spell(player_type *player_ptr, bool only_equip);
bool identify_fully(player_type *player_ptr, bool only_equip);
