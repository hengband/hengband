#pragma once

#include "system/angband.h"

class ItemEntity;
class PlayerType;
bool object_sort_comp(PlayerType *player_ptr, const ItemEntity &item1, const ItemEntity &item2);
