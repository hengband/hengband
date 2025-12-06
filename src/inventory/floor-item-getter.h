#pragma once

#include "object/tval-types.h"
#include "system/angband.h"
#include <tl/optional.hpp>

class PlayerType;
class ItemTester;
tl::optional<short> get_item_floor(PlayerType *player_ptr, std::string_view pmt, std::string_view str, BIT_FLAGS mode, const ItemTester &item_tester);
