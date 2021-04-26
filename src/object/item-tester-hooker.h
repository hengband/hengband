#pragma once

#include "object/tval-types.h"

typedef struct object_type object_type;
typedef struct player_type player_type;
extern bool (*item_tester_hook)(player_type *, object_type *o_ptr);

bool item_tester_okay(player_type *player_ptr, object_type *o_ptr, tval_type tval);
