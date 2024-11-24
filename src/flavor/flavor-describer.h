#pragma once

#include "system/angband.h"
#include <string_view>

class ItemEntity;
class PlayerType;
std::string describe_flavor(PlayerType *player_ptr, const ItemEntity &item, const BIT_FLAGS mode, const size_t max_length = std::string_view::npos);
