#pragma once

#include "system/angband.h"

bool object_is_valid(object_type *o_ptr);
bool object_is_held_monster(object_type *o_ptr);
bool object_is_broken(object_type *o_ptr);
bool object_is_cursed(object_type *o_ptr);
