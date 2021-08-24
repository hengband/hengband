#pragma once

#include "object/tval-types.h"

#include <functional>

typedef struct object_type object_type;
typedef struct player_type player_type;

using ItemTester = std::function<bool(player_type *, object_type *)>;
extern ItemTester item_tester_hook;

bool item_tester_okay(player_type *player_ptr, object_type *o_ptr, tval_type tval);

ItemTester make_item_tester(std::function<bool(object_type *)> pred);
