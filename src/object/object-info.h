#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

#define OBJ_GOLD_LIST 480 /* First "gold" entry */

class ObjectType;
class PlayerType;
concptr activation_explanation(ObjectType *o_ptr);
char index_to_label(int i);
int16_t wield_slot(PlayerType *player_ptr, const ObjectType *o_ptr);
bool check_book_realm(PlayerType *player_ptr, const ItemKindType book_tval, const OBJECT_SUBTYPE_VALUE book_sval);
ObjectType *ref_item(PlayerType *player_ptr, INVENTORY_IDX item);
