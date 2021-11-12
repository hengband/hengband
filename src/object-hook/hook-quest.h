#pragma once

#include "system/angband.h"

struct object_type;
class PlayerType;
bool object_is_bounty(PlayerType *player_ptr, object_type *o_ptr);
bool object_is_quest_target(QUEST_IDX quest_idx, object_type *o_ptr);
