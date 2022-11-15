#pragma once

#include "system/angband.h"

struct autopick_type;
class ItemEntity;
class PlayerType;
bool is_autopick_match(PlayerType *player_ptr, ItemEntity *o_ptr, autopick_type *entry, concptr o_name);
