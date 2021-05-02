#pragma once

#include "system/angband.h"

typedef struct object_type object_type;
void wr_item(object_type *o_ptr);
void wr_perception(KIND_OBJECT_IDX k_idx);
