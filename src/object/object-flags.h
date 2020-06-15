#pragma once

#include "system/angband.h"
#include "system/object-type-definition.h"

void object_flags(object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE]);
void object_flags_known(object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE]);
