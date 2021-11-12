#pragma once

#include "system/angband.h"

struct object_type;
class PlayerType;
PRICE object_value(const object_type *o_ptr);
PRICE object_value_real(const object_type *o_ptr);
