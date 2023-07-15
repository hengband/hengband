#pragma once

#include "system/angband.h"

class ItemEntity;
class PlayerType;
ItemEntity *choose_warning_item(PlayerType *player_ptr);
bool process_warning(PlayerType *player_ptr, POSITION xx, POSITION yy);
