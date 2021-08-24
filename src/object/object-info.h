#pragma once

#include "system/angband.h"
#include "object/tval-types.h"

#define OBJ_GOLD_LIST 480 /* First "gold" entry */

typedef struct object_type object_type;
typedef struct player_type player_type;
concptr activation_explanation(object_type *o_ptr);
char index_to_label(int i);
int16_t wield_slot(player_type *owner_ptr, const object_type *o_ptr);
bool check_book_realm(player_type *owner_ptr, const tval_type book_tval, const OBJECT_SUBTYPE_VALUE book_sval);
object_type *ref_item(player_type *owner_ptr, INVENTORY_IDX item);
TERM_COLOR object_attr(object_type *o_ptr);
