#pragma once

#include "system/angband.h"

typedef struct object_type object_type;
typedef struct player_type player_type;
void rd_item(object_type *o_ptr);
errr load_item(void);
errr load_artifact(void);
