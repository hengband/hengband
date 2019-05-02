#pragma once

/*
 * Information about "ego-items".
 */

typedef struct ego_item_type ego_item_type;

struct ego_item_type
{
	STR_OFFSET name;			/* Name (offset) */
	STR_OFFSET text;			/* Text (offset) */

	INVENTORY_IDX slot;		/*!< 装備部位 / Standard slot value */
	PRICE rating;		/*!< ベースアイテムからの価値加速 / Rating boost */

	DEPTH level;			/* Minimum level */
	RARITY rarity;		/* Object rarity */

	HIT_PROB max_to_h;		/* Maximum to-hit bonus */
	HIT_POINT max_to_d;		/* Maximum to-dam bonus */
	ARMOUR_CLASS max_to_a;		/* Maximum to-ac bonus */

	PARAMETER_VALUE max_pval;		/* Maximum pval */

	PRICE cost;			/* Ego-item "cost" */

	BIT_FLAGS flags[TR_FLAG_SIZE];	/* Ego-Item Flags */
	BIT_FLAGS gen_flags;		/* flags for generate */

	IDX act_idx;		/* Activative ability index */
};

extern ego_item_type *e_info;
extern char *e_name;
extern char *e_text;

