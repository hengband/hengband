#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

ITEM_NUMBER scan_floor_items(player_type *owner_ptr, OBJECT_IDX *items, POSITION y, POSITION x, BIT_FLAGS mode, tval_type item_tester_tval);
COMMAND_CODE show_floor_items(player_type *owner_ptr, int target_item, POSITION y, POSITION x, TERM_LEN *min_width, tval_type item_tester_tval);
