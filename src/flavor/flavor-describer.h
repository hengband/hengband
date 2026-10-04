#pragma once

#include "system/angband.h"
#include <string_view>
#include <tl/optional.hpp>

class ItemEntity;
class PlayerType;
int calc_displayed_charging_count(const ItemEntity &item);
tl::optional<int> calc_displayed_lamp_turns(const ItemEntity &item);
std::string describe_flavor(PlayerType *player_ptr, const ItemEntity &item, const BIT_FLAGS mode, const size_t max_length = std::string_view::npos);
