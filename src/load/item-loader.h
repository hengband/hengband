#pragma once

#include "system/angband.h"

struct object_type;;
struct player_type;
void rd_item(object_type *o_ptr);
errr load_item(void);
errr load_artifact(void);
