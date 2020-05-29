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

#include "object/object-util.h"

/* object1.c */
extern void reset_visuals(player_type *owner_ptr, void(*process_autopick_file_command)(char*));
extern void object_flags(object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE]);
extern void object_flags_known(object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE]);
extern concptr item_activation(object_type *o_ptr);

#define SCROBJ_FAKE_OBJECT  0x00000001
#define SCROBJ_FORCE_DETAIL 0x00000002
extern bool screen_object(player_type *player_ptr, object_type *o_ptr, BIT_FLAGS mode);

extern char index_to_label(int i);
extern s16b wield_slot(player_type *owner_ptr, object_type *o_ptr);

extern bool check_book_realm(player_type *owner_ptr, const tval_type book_tval, const OBJECT_SUBTYPE_VALUE book_sval);
extern object_type *ref_item(player_type *owner_ptr, INVENTORY_IDX item);
extern TERM_COLOR object_attr(object_type *o_ptr);

/*!
* todo ここに置くとコンパイルは通る (このファイルの冒頭やobject2.cでincludeするとコンパイルエラー)、しかし圧倒的にダメなので要調整
*/
#include "floor/floor.h"

/* object2.c */

extern OBJECT_SUBTYPE_VALUE coin_type;
extern s32b flag_cost(object_type *o_ptr, int plusses);

extern bool (*get_obj_num_hook)(KIND_OBJECT_IDX k_idx);

extern int bow_tval_ammo(object_type *o_ptr);

extern void excise_object_idx(floor_type *floor_ptr, OBJECT_IDX o_idx);
extern void delete_object_idx(player_type *owner_ptr, OBJECT_IDX o_idx);
extern void delete_object(player_type *owner_ptr, POSITION y, POSITION x);

extern OBJECT_IDX o_pop(floor_type *floor_ptr);
extern OBJECT_IDX get_obj_num(player_type *o_ptr, DEPTH level, BIT_FLAGS mode);
extern void object_known(object_type *o_ptr);
extern void object_aware(player_type *owner_ptr, object_type *o_ptr);
extern void object_tried(object_type *o_ptr);

extern byte value_check_aux1(object_type *o_ptr);
extern byte value_check_aux2(object_type *o_ptr);

extern PRICE object_value(object_type *o_ptr);
extern PRICE object_value_real(object_type *o_ptr);
extern void distribute_charges(object_type *o_ptr, object_type *q_ptr, int amt);
extern void reduce_charges(object_type *o_ptr, int amt);
extern int object_similar_part(object_type *o_ptr, object_type *j_ptr);
extern bool object_similar(object_type *o_ptr, object_type *j_ptr);
extern void object_absorb(object_type *o_ptr, object_type *j_ptr);
extern IDX lookup_kind(tval_type tval, OBJECT_SUBTYPE_VALUE sval);
extern void object_wipe(object_type *o_ptr);
extern void object_copy(object_type *o_ptr, object_type *j_ptr);
extern void object_prep(object_type *o_ptr, KIND_OBJECT_IDX k_idx);

extern void apply_magic(player_type *owner_type, object_type *o_ptr, DEPTH lev, BIT_FLAGS mode);

extern bool make_object(player_type *owner_ptr, object_type *j_ptr, BIT_FLAGS mode);
extern bool make_gold(floor_type *floor_ptr, object_type *j_ptr);
extern OBJECT_IDX drop_near(player_type *owner_type, object_type *o_ptr, PERCENTAGE chance, POSITION y, POSITION x);
extern void vary_item(player_type *owner_ptr, INVENTORY_IDX item, ITEM_NUMBER num);
extern void inven_item_charges(player_type *owner_ptr, INVENTORY_IDX item);
extern void inven_item_describe(player_type *owner_ptr, INVENTORY_IDX item);
extern void inven_item_increase(player_type *owner_ptr, INVENTORY_IDX item, ITEM_NUMBER num);
extern void inven_item_optimize(player_type *owner_ptr, INVENTORY_IDX item);
extern void floor_item_charges(floor_type *owner_ptr, INVENTORY_IDX item);
extern void floor_item_increase(floor_type *floor_ptr, INVENTORY_IDX item, ITEM_NUMBER num);
extern void floor_item_optimize(player_type *owner_ptr, INVENTORY_IDX item);
extern bool inven_carry_okay(object_type *o_ptr);
extern bool object_sort_comp(object_type *o_ptr, s32b o_value, object_type *j_ptr);
extern s16b inven_carry(player_type *owner_ptr, object_type *o_ptr);
extern INVENTORY_IDX inven_takeoff(player_type *owner_ptr, INVENTORY_IDX item, ITEM_NUMBER amt);
extern void drop_from_inventory(player_type *owner_type, INVENTORY_IDX item, ITEM_NUMBER amt);
extern void combine_pack(player_type *owner_ptr);
extern void reorder_pack(player_type *owner_ptr);
extern void display_koff(player_type *owner_ptr, KIND_OBJECT_IDX k_idx);
extern void torch_flags(object_type *o_ptr, BIT_FLAGS *flgs);
extern void torch_dice(object_type *o_ptr, DICE_NUMBER *dd, DICE_SID *ds);
extern void torch_lost_fuel(object_type *o_ptr);
