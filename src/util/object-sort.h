#pragma once

#include "system/angband.h"

class ItemEntity;
class PlayerType;
bool object_sort_comp(PlayerType *player_ptr, ItemEntity *o_ptr, int32_t o_value, ItemEntity *j_ptr);
