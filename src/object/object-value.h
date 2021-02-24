#pragma once

#include "system/angband.h"
#include "system/object-type-definition.h"

PRICE object_value(const player_type *player_ptr, object_type *o_ptr);
PRICE object_value_real(const player_type *player_ptr, object_type *o_ptr);
