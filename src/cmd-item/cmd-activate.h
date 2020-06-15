#pragma once

#include "system/angband.h"
#include "system/object-type-definition.h"

void do_cmd_activate(player_type *user_ptr);
void exe_activate(player_type *user_ptr, INVENTORY_IDX item);
bool activate_artifact(player_type *user_ptr, object_type *o_ptr);
