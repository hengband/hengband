#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

#define OBJ_GOLD_LIST 480 /* First "gold" entry */

class ItemEntity;
class PlayerType;
concptr activation_explanation(ItemEntity *o_ptr);
char index_to_label(int i);
int16_t wield_slot(PlayerType *player_ptr, const ItemEntity *o_ptr);
bool check_book_realm(PlayerType *player_ptr, const ItemKindType book_tval, const OBJECT_SUBTYPE_VALUE book_sval);
ItemEntity *ref_item(PlayerType *player_ptr, INVENTORY_IDX item);
