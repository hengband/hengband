#pragma once

#include "system/angband.h"

class ObjectType;
class PlayerType;
bool object_sort_comp(PlayerType *player_ptr, ObjectType *o_ptr, int32_t o_value, ObjectType *j_ptr);
