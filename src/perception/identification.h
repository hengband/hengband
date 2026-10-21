#pragma once

#include "system/angband.h"

constexpr auto SCROBJ_FAKE_OBJECT = 0x00000001;
constexpr auto SCROBJ_FORCE_DETAIL = 0x00000002;
class ItemEntity;
class PlayerType;
bool screen_object(PlayerType *player_ptr, const ItemEntity &item, BIT_FLAGS mode);
