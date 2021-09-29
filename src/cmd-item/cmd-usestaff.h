#pragma once

#include "system/angband.h"

struct player_type;
int staff_effect(player_type *player_ptr, OBJECT_SUBTYPE_VALUE sval, bool *use_charge, bool powerful, bool magic, bool known);
void do_cmd_use_staff(player_type *player_ptr);
