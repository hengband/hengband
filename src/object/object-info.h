#pragma once

#include "system/angband.h"
#include "system/object-type-definition.h"

#define OBJ_GOLD_LIST 480 /* First "gold" entry */

concptr activation_explanation(player_type *owner_ptr, object_type *o_ptr);
char index_to_label(int i);
s16b wield_slot(player_type *owner_ptr, object_type *o_ptr);
bool check_book_realm(player_type *owner_ptr, const tval_type book_tval, const OBJECT_SUBTYPE_VALUE book_sval);
object_type *ref_item(player_type *owner_ptr, INVENTORY_IDX item);
TERM_COLOR object_attr(object_type *o_ptr);
