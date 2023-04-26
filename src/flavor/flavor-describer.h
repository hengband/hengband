#pragma once

#include "system/angband.h"
#include <string>

class ItemEntity;
class PlayerType;
std::string describe_flavor(PlayerType *player_ptr, const ItemEntity *o_ptr, const BIT_FLAGS mode);
