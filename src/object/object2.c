/*!
 * @brief オブジェクトの実装 / Object code, part 2
 * @date 2014/01/11
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * 
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "object/object2.h"
#include "object-enchant/artifact.h"
#include "perception/object-perception.h"
#include "object-enchant/object-ego.h"
#include "object/object-hook.h"
#include "object/object-kind.h"
#include "object/object-value.h"
#include "object-enchant/special-object-flags.h"
#include "sv-definition/sv-other-types.h"
#include "object-enchant/trc-types.h"

/*
 * todo この説明長すぎ。何とかしたい
 * Determine if an item can "absorb" a second item
 *
 * See "object_absorb()" for the actual "absorption" code.
 *
 * If permitted, we allow staffs (if they are known to have equal charges
 * and both are either known or confirmed empty) and wands (if both are
 * either known or confirmed empty) and rods (in all cases) to combine.
 * Staffs will unstack (if necessary) when they are used, but wands and
 * rods will only unstack if one is dropped. -LM-
 *
 * If permitted, we allow weapons/armor to stack, if fully "known".
 *
 * Missiles will combine if both stacks have the same "known" status.
 * This is done to make unidentified stacks of missiles useful.
 *
 * Food, potions, scrolls, and "easy know" items always stack.
 *
 * Chests, and activatable items, never stack (for various reasons).
 */

 /*
  * A "stack" of items is limited to less than or equal to 99 items (hard-coded).
  */
#define MAX_STACK_SIZE 99

/*!
 * Function hook to restrict "get_obj_num_prep()" function
 */
bool(*get_obj_num_hook)(KIND_OBJECT_IDX k_idx);

OBJECT_SUBTYPE_VALUE coin_type;	/* Hack -- force coin type */

/*!
 * @brief 魔法棒やロッドのスロット分割時に使用回数を分配する /
 * Distribute charges of rods or wands.
 * @param o_ptr 分割元オブジェクトの構造体参照ポインタ source item
 * @param q_ptr 分割先オブジェクトの構造体参照ポインタ target item, must be of the same type as o_ptr
 * @param amt 分割したい回数量 number of items that are transfered
 * @return なし
 * @details
 * Hack -- If rods or wands are dropped, the total maximum timeout or\n
 * charges need to be allocated between the two stacks.  If all the items\n
 * are being dropped, it makes for a neater message to leave the original\n
 * stack's pval alone. -LM-\n
 */
void distribute_charges(object_type *o_ptr, object_type *q_ptr, int amt)
{
	if ((o_ptr->tval != TV_WAND) && (o_ptr->tval != TV_ROD)) return;

	q_ptr->pval = o_ptr->pval * amt / o_ptr->number;
	if (amt < o_ptr->number) o_ptr->pval -= q_ptr->pval;

	if ((o_ptr->tval != TV_ROD) || !o_ptr->timeout) return;

	if (q_ptr->pval > o_ptr->timeout)
		q_ptr->timeout = o_ptr->timeout;
	else
		q_ptr->timeout = q_ptr->pval;

	if (amt < o_ptr->number) o_ptr->timeout -= q_ptr->timeout;
}


/*!
 * @brief 魔法棒やロッドの使用回数を減らす /
 * @param o_ptr オブジェクトの構造体参照ポインタ source item
 * @param amt 減らしたい回数量 number of items that are transfered
 * @return なし
 * @details
 * Hack -- If rods or wand are destroyed, the total maximum timeout or\n
 * charges of the stack needs to be reduced, unless all the items are\n
 * being destroyed. -LM-\n
 */
void reduce_charges(object_type *o_ptr, int amt)
{
	if (((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_ROD)) &&
		(amt < o_ptr->number))
	{
		o_ptr->pval -= o_ptr->pval * amt / o_ptr->number;
	}
}


/*!
 * @brief 両オブジェクトをスロットに重ね合わせ可能な最大数を返す。
 * Determine if an item can partly absorb a second item. Return maximum number of stack.
 * @param o_ptr 検証したいオブジェクトの構造体参照ポインタ1
 * @param j_ptr 検証したいオブジェクトの構造体参照ポインタ2
 * @return 重ね合わせ可能なアイテム数
 */
int object_similar_part(object_type *o_ptr, object_type *j_ptr)
{
	int max_num = MAX_STACK_SIZE;
	if (o_ptr->k_idx != j_ptr->k_idx) return 0;

	switch (o_ptr->tval)
	{
	case TV_CHEST:
	case TV_CARD:
	case TV_CAPTURE:
	{
		return 0;
	}
	case TV_STATUE:
	{
		if ((o_ptr->sval != SV_PHOTO) || (j_ptr->sval != SV_PHOTO)) return 0;
		if (o_ptr->pval != j_ptr->pval) return 0;
		break;
	}
	case TV_FIGURINE:
	case TV_CORPSE:
	{
		if (o_ptr->pval != j_ptr->pval) return 0;

		break;
	}
	case TV_FOOD:
	case TV_POTION:
	case TV_SCROLL:
	{
		break;
	}
	case TV_STAFF:
	{
		if ((!(o_ptr->ident & (IDENT_EMPTY)) &&
			!object_is_known(o_ptr)) ||
			(!(j_ptr->ident & (IDENT_EMPTY)) &&
				!object_is_known(j_ptr))) return 0;

		if (o_ptr->pval != j_ptr->pval) return 0;

		break;
	}
	case TV_WAND:
	{
		if ((!(o_ptr->ident & (IDENT_EMPTY)) &&
			!object_is_known(o_ptr)) ||
			(!(j_ptr->ident & (IDENT_EMPTY)) &&
				!object_is_known(j_ptr))) return 0;

		break;
	}
	case TV_ROD:
	{
		max_num = MIN(max_num, MAX_SHORT / k_info[o_ptr->k_idx].pval);
		break;
	}
	case TV_BOW:
	case TV_DIGGING:
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
	case TV_CROWN:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_DRAG_ARMOR:
	case TV_RING:
	case TV_AMULET:
	case TV_LITE:
	case TV_WHISTLE:
	{
		if (!object_is_known(o_ptr) || !object_is_known(j_ptr)) return 0;
	}
		/* Fall through */
	case TV_BOLT:
	case TV_ARROW:
	case TV_SHOT:
	{
		if (object_is_known(o_ptr) != object_is_known(j_ptr)) return 0;
		if (o_ptr->feeling != j_ptr->feeling) return 0;
		if (o_ptr->to_h != j_ptr->to_h) return 0;
		if (o_ptr->to_d != j_ptr->to_d) return 0;
		if (o_ptr->to_a != j_ptr->to_a) return 0;
		if (o_ptr->pval != j_ptr->pval) return 0;
		if (object_is_artifact(o_ptr) || object_is_artifact(j_ptr)) return 0;
		if (o_ptr->name2 != j_ptr->name2) return 0;
		if (o_ptr->xtra3 != j_ptr->xtra3) return 0;
		if (o_ptr->xtra4 != j_ptr->xtra4) return 0;
		if (o_ptr->xtra1 || j_ptr->xtra1) return 0;
		if (o_ptr->timeout || j_ptr->timeout) return 0;
		if (o_ptr->ac != j_ptr->ac) return 0;
		if (o_ptr->dd != j_ptr->dd) return 0;
		if (o_ptr->ds != j_ptr->ds) return 0;
		break;
	}
	default:
	{
		if (!object_is_known(o_ptr) || !object_is_known(j_ptr)) return 0;

		break;
	}
	}

	for (int i = 0; i < TR_FLAG_SIZE; i++)
		if (o_ptr->art_flags[i] != j_ptr->art_flags[i]) return 0;

	if (o_ptr->curse_flags != j_ptr->curse_flags) return 0;
	if ((o_ptr->ident & (IDENT_BROKEN)) != (j_ptr->ident & (IDENT_BROKEN))) return 0;

	if (o_ptr->inscription && j_ptr->inscription &&
		(o_ptr->inscription != j_ptr->inscription))
		return 0;

	if (!stack_force_notes && (o_ptr->inscription != j_ptr->inscription)) return 0;
	if (!stack_force_costs && (o_ptr->discount != j_ptr->discount)) return 0;

	return max_num;
}


/*!
 * @brief 両オブジェクトをスロットに重ねることができるかどうかを返す。
 * Determine if an item can absorb a second item.
 * @param o_ptr 検証したいオブジェクトの構造体参照ポインタ1
 * @param j_ptr 検証したいオブジェクトの構造体参照ポインタ2
 * @return 重ね合わせ可能ならばTRUEを返す。
 */
bool object_similar(object_type *o_ptr, object_type *j_ptr)
{
	int total = o_ptr->number + j_ptr->number;
	int max_num = object_similar_part(o_ptr, j_ptr);
	if (!max_num) return FALSE;
	if (total > max_num) return 0;

	return TRUE;
}


/*!
 * @brief 両オブジェクトをスロットに重ね合わせる。
 * Allow one item to "absorb" another, assuming they are similar
 * @param o_ptr 重ね合わせ先のオブジェクトの構造体参照ポインタ
 * @param j_ptr 重ね合わせ元のオブジェクトの構造体参照ポインタ
 * @return なし
 */
void object_absorb(object_type *o_ptr, object_type *j_ptr)
{
	int max_num = object_similar_part(o_ptr, j_ptr);
	int total = o_ptr->number + j_ptr->number;
	int diff = (total > max_num) ? total - max_num : 0;

	o_ptr->number = (total > max_num) ? max_num : total;
	if (object_is_known(j_ptr)) object_known(o_ptr);

	if (((o_ptr->ident & IDENT_STORE) || (j_ptr->ident & IDENT_STORE)) &&
		(!((o_ptr->ident & IDENT_STORE) && (j_ptr->ident & IDENT_STORE))))
	{
		if (j_ptr->ident & IDENT_STORE) j_ptr->ident &= 0xEF;
		if (o_ptr->ident & IDENT_STORE) o_ptr->ident &= 0xEF;
	}

	if (object_is_fully_known(j_ptr)) o_ptr->ident |= (IDENT_FULL_KNOWN);
	if (j_ptr->inscription) o_ptr->inscription = j_ptr->inscription;
	if (j_ptr->feeling) o_ptr->feeling = j_ptr->feeling;
	if (o_ptr->discount < j_ptr->discount) o_ptr->discount = j_ptr->discount;
	if (o_ptr->tval == TV_ROD)
	{
		o_ptr->pval += j_ptr->pval * (j_ptr->number - diff) / j_ptr->number;
		o_ptr->timeout += j_ptr->timeout * (j_ptr->number - diff) / j_ptr->number;
	}

	if (o_ptr->tval == TV_WAND)
	{
		o_ptr->pval += j_ptr->pval * (j_ptr->number - diff) / j_ptr->number;
	}
}


/*!
 * @brief オブジェクトを定義された基準に従いソートするための関数 /
 * Check if we have space for an item in the pack without overflow
 * @param o_ptr 比較対象オブジェクトの構造体参照ポインタ1
 * @param o_value o_ptrのアイテム価値（手動であらかじめ代入する必要がある？）
 * @param j_ptr 比較対象オブジェクトの構造体参照ポインタ2
 * @return o_ptrの方が上位ならばTRUEを返す。
 */
bool object_sort_comp(object_type *o_ptr, s32b o_value, object_type *j_ptr)
{
	int o_type, j_type;
	if (!j_ptr->k_idx) return TRUE;

	if ((o_ptr->tval == REALM1_BOOK) &&
		(j_ptr->tval != REALM1_BOOK)) return TRUE;
	if ((j_ptr->tval == REALM1_BOOK) &&
		(o_ptr->tval != REALM1_BOOK)) return FALSE;

	if ((o_ptr->tval == REALM2_BOOK) &&
		(j_ptr->tval != REALM2_BOOK)) return TRUE;
	if ((j_ptr->tval == REALM2_BOOK) &&
		(o_ptr->tval != REALM2_BOOK)) return FALSE;

	if (o_ptr->tval > j_ptr->tval) return TRUE;
	if (o_ptr->tval < j_ptr->tval) return FALSE;

	if (!object_is_aware(o_ptr)) return FALSE;
	if (!object_is_aware(j_ptr)) return TRUE;

	if (o_ptr->sval < j_ptr->sval) return TRUE;
	if (o_ptr->sval > j_ptr->sval) return FALSE;

	if (!object_is_known(o_ptr)) return FALSE;
	if (!object_is_known(j_ptr)) return TRUE;

	if (object_is_fixed_artifact(o_ptr)) o_type = 3;
	else if (o_ptr->art_name) o_type = 2;
	else if (object_is_ego(o_ptr)) o_type = 1;
	else o_type = 0;

	if (object_is_fixed_artifact(j_ptr)) j_type = 3;
	else if (j_ptr->art_name) j_type = 2;
	else if (object_is_ego(j_ptr)) j_type = 1;
	else j_type = 0;

	if (o_type < j_type) return TRUE;
	if (o_type > j_type) return FALSE;

	switch (o_ptr->tval)
	{
	case TV_FIGURINE:
	case TV_STATUE:
	case TV_CORPSE:
	case TV_CAPTURE:
		if (r_info[o_ptr->pval].level < r_info[j_ptr->pval].level) return TRUE;
		if ((r_info[o_ptr->pval].level == r_info[j_ptr->pval].level) && (o_ptr->pval < j_ptr->pval)) return TRUE;
		return FALSE;

	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
		if (o_ptr->to_h + o_ptr->to_d < j_ptr->to_h + j_ptr->to_d) return TRUE;
		if (o_ptr->to_h + o_ptr->to_d > j_ptr->to_h + j_ptr->to_d) return FALSE;
		break;

	case TV_ROD:
		if (o_ptr->pval < j_ptr->pval) return TRUE;
		if (o_ptr->pval > j_ptr->pval) return FALSE;
		break;
	}

	return o_value > object_value(j_ptr);
}
