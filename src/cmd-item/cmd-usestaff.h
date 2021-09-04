#pragma once

#include "system/angband.h"

struct player_type;
int staff_effect(player_type *creature_ptr, OBJECT_SUBTYPE_VALUE sval, bool *use_charge, bool powerful, bool magic, bool known);
void exe_use_staff(player_type *creature_ptr, INVENTORY_IDX item);
void do_cmd_use_staff(player_type *creature_ptr);
