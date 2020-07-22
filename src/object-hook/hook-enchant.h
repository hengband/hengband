#pragma once

#include "system/angband.h"

bool object_is_fixed_artifact(object_type *o_ptr);
bool object_is_ego(object_type *o_ptr);
bool object_is_rare(object_type *o_ptr);
bool object_is_artifact(object_type *o_ptr);
bool object_is_random_artifact(object_type *o_ptr);
bool object_is_nameless(player_type *player_ptr, object_type *o_ptr);
