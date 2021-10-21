#pragma once

#include "system/angband.h"

typedef struct autopick_type autopick_type;
struct object_type;;
class player_type;
bool is_autopick_match(player_type *player_ptr, object_type *o_ptr, autopick_type *entry, concptr o_name);
