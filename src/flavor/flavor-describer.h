#pragma once

#include "system/angband.h"

class ObjectType;
class PlayerType;
void describe_flavor(PlayerType *player_ptr, char *buf, ObjectType *o_ptr, BIT_FLAGS mode);
