#pragma once

#include "system/angband.h"
#include "system/system-variables.h"

struct object_type;;
TrFlags object_flags(const object_type *o_ptr);
TrFlags object_flags_known(const object_type *o_ptr);
