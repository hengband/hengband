#pragma once

/*
 * Object information, for a specific object.
 *
 * Note that a "discount" on an item is permanent and never goes away.
 *
 * Note that inscriptions are now handled via the "quark_str()" function
 * applied to the "note" field, which will return NULL if "note" is zero.
 *
 * Note that "object" records are "copied" on a fairly regular basis,
 * and care must be taken when handling such objects.
 *
 * Note that "object flags" must now be derived from the object kind,
 * the artifact and ego-item indexes, and the two "xtra" fields.
 *
 * Each grid points to one (or zero) objects via the "o_idx"
 * field (above).  Each object then points to one (or zero) objects
 * via the "next_o_idx" field, forming a singly linked list, which
 * in game terms, represents a "stack" of objects in the same grid.
 *
 * Each monster points to one (or zero) objects via the "hold_o_idx"
 * field (below).  Each object then points to one (or zero) objects
 * via the "next_o_idx" field, forming a singly linked list, which
 * in game terms, represents a pile of objects held by the monster.
 *
 * The "held_m_idx" field is used to indicate which monster, if any,
 * is holding the object.  Objects being held have "ix=0" and "iy=0".
 */

#define OBJ_GOLD_LIST   480     /* First "gold" entry */

#include "system/angband.h"
#include "object/object-util.h"

void reset_visuals(player_type *owner_ptr, void(*process_autopick_file_command)(char*));
void object_flags(object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE]);
void object_flags_known(object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE]);
concptr item_activation(object_type *o_ptr);

char index_to_label(int i);
s16b wield_slot(player_type *owner_ptr, object_type *o_ptr);

bool check_book_realm(player_type *owner_ptr, const tval_type book_tval, const OBJECT_SUBTYPE_VALUE book_sval);
object_type *ref_item(player_type *owner_ptr, INVENTORY_IDX item);
TERM_COLOR object_attr(object_type *o_ptr);
