#pragma once

#include "system/angband.h"

class ObjectType;
class PlayerType;
bool object_is_bounty(PlayerType *player_ptr, ObjectType *o_ptr);
bool object_is_quest_target(QUEST_IDX quest_idx, ObjectType *o_ptr);
