#pragma once

#include "system/angband.h"

class ObjectType;
class PlayerType;
ObjectType *choose_warning_item(PlayerType *player_ptr);
bool process_warning(PlayerType *player_ptr, POSITION xx, POSITION yy);
