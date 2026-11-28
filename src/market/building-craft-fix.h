#pragma once

#include "object-enchant/tr-types.h"
#include "system/angband.h"
#include <vector>

class ItemEntity;
class PlayerType;
std::vector<tr_type> collect_transplantable_abilities(const ItemEntity &to_item, const ItemEntity &from_item);
int repair_broken_weapon(PlayerType *player_ptr, PRICE bcost);
