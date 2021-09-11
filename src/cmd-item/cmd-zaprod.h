#pragma once

#include "system/angband.h"

struct player_type;
int rod_effect(player_type *player_ptr, OBJECT_SUBTYPE_VALUE sval, DIRECTION dir, bool *use_charge, bool powerful, bool magic);
void exe_zap_rod(player_type *player_ptr, INVENTORY_IDX item);
void do_cmd_zap_rod(player_type *player_ptr);
