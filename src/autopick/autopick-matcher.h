#pragma once

#include "system/angband.h"

typedef struct autopick_type autopick_type;
struct object_type;
class PlayerType;
bool is_autopick_match(PlayerType *player_ptr, object_type *o_ptr, autopick_type *entry, concptr o_name);
