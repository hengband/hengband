#pragma once

#include "system/angband.h"

struct artifact_type;
bool create_named_art(player_type *player_ptr, ARTIFACT_IDX a_idx, POSITION y, POSITION x);
bool make_artifact(player_type *player_ptr, object_type *o_ptr);
artifact_type *apply_artifact(player_type *player_ptr, object_type *o_ptr);
bool make_artifact_special(player_type *player_ptr, object_type *o_ptr);
