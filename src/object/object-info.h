#pragma once

#include "object/tval-types.h"
#include "system/angband.h"
#include <string>

#define OBJ_GOLD_LIST 480 /* First "gold" entry */

class BaseitemKey;
class ItemEntity;
class PlayerType;
std::string activation_explanation(const ItemEntity *o_ptr);
char index_to_label(int i);
int16_t wield_slot(PlayerType *player_ptr, const ItemEntity *o_ptr);
bool check_book_realm(PlayerType *player_ptr, const BaseitemKey &bi_key);
ItemEntity *ref_item(PlayerType *player_ptr, INVENTORY_IDX item);
