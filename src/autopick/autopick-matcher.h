#pragma once

#include "system/angband.h"
#include <string_view>

struct autopick_type;
class ItemEntity;
class PlayerType;
bool is_autopick_match(PlayerType *player_ptr, ItemEntity *o_ptr, const autopick_type &entry, std::string_view item_name);
