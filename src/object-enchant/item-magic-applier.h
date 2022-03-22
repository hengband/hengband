#pragma once

#include "system/angband.h"

class ObjectType;
class PlayerType;
void apply_magic_to_object(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH lev, BIT_FLAGS mode);
