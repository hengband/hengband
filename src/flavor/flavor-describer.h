#pragma once

#include "system/angband.h"

class ItemEntity;
class PlayerType;
void describe_flavor(PlayerType *player_ptr, char *buf, const ItemEntity *o_ptr, const BIT_FLAGS mode);
