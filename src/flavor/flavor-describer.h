#pragma once

#include "system/angband.h"

class ItemEntity;
class PlayerType;
void describe_flavor(PlayerType *player_ptr, char *buf, ItemEntity *o_ptr, BIT_FLAGS mode);
