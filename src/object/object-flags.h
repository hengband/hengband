#pragma once

#include "system/angband.h"
#include "system/system-variables.h"

struct object_type;;
struct player_type;
void object_flags(const object_type *o_ptr, TrFlags &flgs);
void object_flags_known(const object_type *o_ptr, TrFlags &flgs);
