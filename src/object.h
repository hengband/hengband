#pragma once
#include "defines.h"

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

typedef struct object_type object_type;

struct object_type
{
	KIND_OBJECT_IDX k_idx;			/* Kind index (zero if "dead") */

	POSITION iy;			/* Y-position on map, or zero */
	POSITION ix;			/* X-position on map, or zero */

	OBJECT_TYPE_VALUE tval;			/* Item type (from kind) */
	OBJECT_SUBTYPE_VALUE sval;			/* Item sub-type (from kind) */

	PARAMETER_VALUE pval;			/* Item extra-parameter */

	DISCOUNT_RATE discount;		/* Discount (if any) */

	ITEM_NUMBER number;	/* Number of items */

	WEIGHT weight;		/* Item weight */

	ARTIFACT_IDX name1;		/* Artifact type, if any */
	EGO_IDX name2;			/* Ego-Item type, if any */

	XTRA8 xtra1;			/* Extra info type (now unused) */
	XTRA8 xtra2;			/* Extra info activation index */
	XTRA8 xtra3;			/* Extra info for weaponsmith */
	XTRA16 xtra4;			/*!< 光源の残り寿命、あるいは捕らえたモンスターの現HP / Extra info fuel or captured monster's current HP */
	XTRA16 xtra5;			/*!< 捕らえたモンスターの最大HP / Extra info captured monster's max HP */

	HIT_PROB to_h;			/* Plusses to hit */
	HIT_POINT to_d;			/* Plusses to damage */
	ARMOUR_CLASS to_a;			/* Plusses to AC */

	ARMOUR_CLASS ac;			/* Normal AC */

	DICE_NUMBER dd;
	DICE_SID ds;		/* Damage dice/sides */

	TIME_EFFECT timeout;	/* Timeout Counter */

	byte ident;			/* Special flags  */
	byte marked;		/* Object is marked */

	u16b inscription;	/* Inscription index */
	u16b art_name;      /* Artifact name (random artifacts) */

	byte feeling;          /* Game generated inscription number (eg, pseudo-id) */

	BIT_FLAGS art_flags[TR_FLAG_SIZE];        /* Extra Flags for ego and artifacts */
	BIT_FLAGS curse_flags;        /* Flags for curse */

	OBJECT_IDX next_o_idx;	/* Next object in stack (if any) */
	MONSTER_IDX held_m_idx;	/* Monster holding us (if any) */

	ARTIFACT_BIAS_IDX artifact_bias; /*!< ランダムアーティファクト生成時のバイアスID */
};

/* object1.c */
extern ITEM_NUMBER scan_floor(OBJECT_IDX *items, POSITION y, POSITION x, BIT_FLAGS mode);
extern COMMAND_CODE show_floor(int target_item, POSITION y, POSITION x, TERM_LEN *min_width);
extern void reset_visuals(void);
extern void object_flags(object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE]);
extern void object_flags_known(object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE]);
extern concptr item_activation(object_type *o_ptr);
extern bool screen_object(object_type *o_ptr, BIT_FLAGS mode);
extern char index_to_label(int i);
extern INVENTORY_IDX label_to_inven(int c);
extern INVENTORY_IDX label_to_equip(int c);
extern s16b wield_slot(object_type *o_ptr);
extern concptr mention_use(int i);
extern concptr describe_use(int i);
extern bool check_book_realm(const OBJECT_TYPE_VALUE book_tval, const OBJECT_SUBTYPE_VALUE book_sval);
extern bool item_tester_okay(object_type *o_ptr);
extern void display_inven(void);
extern void display_equip(void);
extern COMMAND_CODE show_inven(int target_item, BIT_FLAGS mode);
extern COMMAND_CODE show_equip(int target_item, BIT_FLAGS mode);
extern void toggle_inven_equip(void);
extern bool can_get_item(void);
extern bool get_item(OBJECT_IDX *cp, concptr pmt, concptr str, BIT_FLAGS mode);
extern object_type *choose_object(OBJECT_IDX *idx, concptr q, concptr s, BIT_FLAGS option);
PERCENTAGE breakage_chance(object_type *o_ptr, SPELL_IDX snipe_type);

extern int bow_tval_ammo(object_type *o_ptr);

/* object2.c */
extern void excise_object_idx(OBJECT_IDX o_idx);
extern void delete_object_idx(OBJECT_IDX o_idx);
extern void delete_object(POSITION y, POSITION x);
extern void compact_objects(int size);
extern void wipe_o_list(void);
extern OBJECT_IDX o_pop(void);
extern OBJECT_IDX get_obj_num(DEPTH level, BIT_FLAGS mode);
extern void object_known(object_type *o_ptr);
extern void object_aware(object_type *o_ptr);
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
extern IDX lookup_kind(OBJECT_TYPE_VALUE tval, OBJECT_SUBTYPE_VALUE sval);
extern void object_wipe(object_type *o_ptr);
extern void object_prep(object_type *o_ptr, KIND_OBJECT_IDX k_idx);
extern void object_copy(object_type *o_ptr, object_type *j_ptr);

/*
 * Bit flags for apply_magic() (etc)
 */
#define AM_NO_FIXED_ART 0x00000001 /*!< Don't allow roll for fixed artifacts */
#define AM_GOOD         0x00000002 /*!< Generate good items */
#define AM_GREAT        0x00000004 /*!< Generate great items */
#define AM_SPECIAL      0x00000008 /*!< Generate artifacts (for debug mode only) */
#define AM_CURSED       0x00000010 /*!< Generate cursed/worthless items */
#define AM_FORBID_CHEST 0x00000020 /*!< 箱からさらに箱が出現することを抑止する */
extern void apply_magic(object_type *o_ptr, DEPTH lev, BIT_FLAGS mode);

extern bool make_object(object_type *j_ptr, BIT_FLAGS mode);
extern void place_object(POSITION y, POSITION x, BIT_FLAGS mode);
extern bool make_gold(object_type *j_ptr);
extern void place_gold(POSITION y, POSITION x);
extern OBJECT_IDX drop_near(object_type *o_ptr, PERCENTAGE chance, POSITION y, POSITION x);
extern void inven_item_charges(INVENTORY_IDX item);
extern void inven_item_describe(INVENTORY_IDX item);
extern void inven_item_increase(INVENTORY_IDX item, ITEM_NUMBER num);
extern void inven_item_optimize(INVENTORY_IDX item);
extern void floor_item_charges(INVENTORY_IDX item);
extern void floor_item_describe(INVENTORY_IDX item);
extern void floor_item_increase(INVENTORY_IDX item, ITEM_NUMBER num);
extern void floor_item_optimize(INVENTORY_IDX item);
extern bool inven_carry_okay(object_type *o_ptr);
extern bool object_sort_comp(object_type *o_ptr, s32b o_value, object_type *j_ptr);
extern s16b inven_carry(object_type *o_ptr);
extern INVENTORY_IDX inven_takeoff(INVENTORY_IDX item, ITEM_NUMBER amt);
extern void inven_drop(INVENTORY_IDX item, ITEM_NUMBER amt);
extern void combine_pack(void);
extern void reorder_pack(void);
extern void display_koff(KIND_OBJECT_IDX k_idx);
extern void torch_flags(object_type *o_ptr, BIT_FLAGS *flgs);
extern void torch_dice(object_type *o_ptr, DICE_NUMBER *dd, DICE_SID *ds);
extern void torch_lost_fuel(object_type *o_ptr);
extern concptr essence_name[];

extern s32b flag_cost(object_type *o_ptr, int plusses);
