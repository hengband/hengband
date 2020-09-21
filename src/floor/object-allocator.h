#pragma once

#include "system/angband.h"

typedef enum dap_type dap_type;
bool alloc_stairs(player_type *owner_ptr, FEAT_IDX feat, int num, int walls);
void alloc_object(player_type *owner_ptr, dap_type set, spell_type typ, int num);
