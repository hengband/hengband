#pragma once

#include "system/angband.h"

enum class WishResult { FAIL = -1, NOTHING = 0, NORMAL = 1, EGO = 2, ARTIFACT = 3, MAX };

typedef struct player_type player_type;
void wiz_modify_item(player_type *creature_ptr);
WishResult do_cmd_wishing(player_type *caster_ptr, int prob, bool art_ok, bool ego_ok, bool confirm);
void wiz_identify_full_inventory(player_type *caster_ptr);
