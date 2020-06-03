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
#include "cmd-io/cmd-dump.h"
#include "dungeon/dungeon.h"
#include "floor/floor.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "monster/monster-race-hook.h"
#include "monster/monster.h"
#include "object-enchant/apply-magic-accessory.h"
#include "object-enchant/apply-magic-armor.h"
#include "object-enchant/apply-magic-others.h"
#include "object-enchant/apply-magic-weapon.h"
#include "object-enchant/artifact.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-feeling.h"
#include "object/object-appraiser.h" // 暫定、相互参照している.
#include "object-enchant/object-boost.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/object-ego.h"
#include "object/object-flavor.h"
#include "object/object-generator.h"
#include "object/object-hook.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "object/object-mark-types.h"
#include "object/object-value.h" // 暫定、相互参照している.
#include "object-enchant/special-object-flags.h"
#include "sv-definition/sv-amulet-types.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-ring-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "object-enchant/trc-types.h"
#include "player/player-class.h"
#include "player/player-effects.h"
#include "player/player-move.h"
#include "player/player-personalities-table.h"
#include "player/player-status.h"
#include "util/util.h"
#include "view/display-main-window.h"
#include "world/world-object.h"
#include "world/world.h"

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
* @brief 重度擬似鑑定の判断処理 / Return a "feeling" (or NULL) about an item.  Method 1 (Heavy).
* @param o_ptr 擬似鑑定を行うオブジェクトの参照ポインタ。
* @return 擬似鑑定結果のIDを返す。
*/
byte value_check_aux1(object_type *o_ptr)
{
	if (object_is_artifact(o_ptr))
	{
		if (object_is_cursed(o_ptr) || object_is_broken(o_ptr)) return FEEL_TERRIBLE;

		return FEEL_SPECIAL;
	}

	if (object_is_ego(o_ptr))
	{
		if (object_is_cursed(o_ptr) || object_is_broken(o_ptr)) return FEEL_WORTHLESS;

		return FEEL_EXCELLENT;
	}

	if (object_is_cursed(o_ptr)) return FEEL_CURSED;
	if (object_is_broken(o_ptr)) return FEEL_BROKEN;
	if ((o_ptr->tval == TV_RING) || (o_ptr->tval == TV_AMULET)) return FEEL_AVERAGE;
	if (o_ptr->to_a > 0) return FEEL_GOOD;
	if (o_ptr->to_h + o_ptr->to_d > 0) return FEEL_GOOD;

	return FEEL_AVERAGE;
}


/*!
* @brief 軽度擬似鑑定の判断処理 / Return a "feeling" (or NULL) about an item.  Method 2 (Light).
* @param o_ptr 擬似鑑定を行うオブジェクトの参照ポインタ。
* @return 擬似鑑定結果のIDを返す。
*/
byte value_check_aux2(object_type *o_ptr)
{
	if (object_is_cursed(o_ptr)) return FEEL_CURSED;
	if (object_is_broken(o_ptr)) return FEEL_BROKEN;
	if (object_is_artifact(o_ptr)) return FEEL_UNCURSED;
	if (object_is_ego(o_ptr)) return FEEL_UNCURSED;
	if (o_ptr->to_a > 0) return FEEL_UNCURSED;
	if (o_ptr->to_h + o_ptr->to_d > 0) return FEEL_UNCURSED;

	return FEEL_NONE;
}


/*!
 * @brief オブジェクトのフラグ類から価格を算出する /
 * Return the value of the flags the object has...
 * @param o_ptr フラグ価格を確認したいオブジェクトの構造体参照ポインタ
 * @param plusses フラグに与える価格の基本重み
 * @return オブジェクトのフラグ価格
 */
PRICE flag_cost(object_type *o_ptr, int plusses)
{
	PRICE total = 0;
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	object_kind *k_ptr = &k_info[o_ptr->k_idx];
	object_flags(o_ptr, flgs);

	/*
	 * Exclude fixed flags of the base item.
	 * pval bonuses of base item will be treated later.
	 */
	for (int i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] &= ~(k_ptr->flags[i]);

	if (object_is_fixed_artifact(o_ptr))
	{
		artifact_type *a_ptr = &a_info[o_ptr->name1];

		for (int i = 0; i < TR_FLAG_SIZE; i++)
			flgs[i] &= ~(a_ptr->flags[i]);
	}
	else if (object_is_ego(o_ptr))
	{
		ego_item_type *e_ptr = &e_info[o_ptr->name2];

		for (int i = 0; i < TR_FLAG_SIZE; i++)
			flgs[i] &= ~(e_ptr->flags[i]);
	}

	/*
	 * Calucurate values of remaining flags
	 */
	if (have_flag(flgs, TR_STR)) total += (1500 * plusses);
	if (have_flag(flgs, TR_INT)) total += (1500 * plusses);
	if (have_flag(flgs, TR_WIS)) total += (1500 * plusses);
	if (have_flag(flgs, TR_DEX)) total += (1500 * plusses);
	if (have_flag(flgs, TR_CON)) total += (1500 * plusses);
	if (have_flag(flgs, TR_CHR)) total += (750 * plusses);
	if (have_flag(flgs, TR_MAGIC_MASTERY)) total += (600 * plusses);
	if (have_flag(flgs, TR_STEALTH)) total += (250 * plusses);
	if (have_flag(flgs, TR_SEARCH)) total += (100 * plusses);
	if (have_flag(flgs, TR_INFRA)) total += (150 * plusses);
	if (have_flag(flgs, TR_TUNNEL)) total += (175 * plusses);
	if ((have_flag(flgs, TR_SPEED)) && (plusses > 0))
		total += (10000 + (2500 * plusses));
	if ((have_flag(flgs, TR_BLOWS)) && (plusses > 0))
		total += (10000 + (2500 * plusses));

	PRICE tmp_cost = 0;
	int count = 0;
	if (have_flag(flgs, TR_CHAOTIC)) { total += 5000; count++; }
	if (have_flag(flgs, TR_VAMPIRIC)) { total += 6500; count++; }
	if (have_flag(flgs, TR_FORCE_WEAPON)) { tmp_cost += 2500; count++; }
	if (have_flag(flgs, TR_KILL_ANIMAL)) { tmp_cost += 2800; count++; }
	else if (have_flag(flgs, TR_SLAY_ANIMAL)) { tmp_cost += 1800; count++; }
	if (have_flag(flgs, TR_KILL_EVIL)) { tmp_cost += 3300; count++; }
	else if (have_flag(flgs, TR_SLAY_EVIL)) { tmp_cost += 2300; count++; }
	if (have_flag(flgs, TR_KILL_HUMAN)) { tmp_cost += 2800; count++; }
	else if (have_flag(flgs, TR_SLAY_HUMAN)) { tmp_cost += 1800; count++; }
	if (have_flag(flgs, TR_KILL_UNDEAD)) { tmp_cost += 2800; count++; }
	else if (have_flag(flgs, TR_SLAY_UNDEAD)) { tmp_cost += 1800; count++; }
	if (have_flag(flgs, TR_KILL_DEMON)) { tmp_cost += 2800; count++; }
	else if (have_flag(flgs, TR_SLAY_DEMON)) { tmp_cost += 1800; count++; }
	if (have_flag(flgs, TR_KILL_ORC)) { tmp_cost += 2500; count++; }
	else if (have_flag(flgs, TR_SLAY_ORC)) { tmp_cost += 1500; count++; }
	if (have_flag(flgs, TR_KILL_TROLL)) { tmp_cost += 2800; count++; }
	else if (have_flag(flgs, TR_SLAY_TROLL)) { tmp_cost += 1800; count++; }
	if (have_flag(flgs, TR_KILL_GIANT)) { tmp_cost += 2800; count++; }
	else if (have_flag(flgs, TR_SLAY_GIANT)) { tmp_cost += 1800; count++; }
	if (have_flag(flgs, TR_KILL_DRAGON)) { tmp_cost += 2800; count++; }
	else if (have_flag(flgs, TR_SLAY_DRAGON)) { tmp_cost += 1800; count++; }

	if (have_flag(flgs, TR_VORPAL)) { tmp_cost += 2500; count++; }
	if (have_flag(flgs, TR_IMPACT)) { tmp_cost += 2500; count++; }
	if (have_flag(flgs, TR_BRAND_POIS)) { tmp_cost += 3800; count++; }
	if (have_flag(flgs, TR_BRAND_ACID)) { tmp_cost += 3800; count++; }
	if (have_flag(flgs, TR_BRAND_ELEC)) { tmp_cost += 3800; count++; }
	if (have_flag(flgs, TR_BRAND_FIRE)) { tmp_cost += 2500; count++; }
	if (have_flag(flgs, TR_BRAND_COLD)) { tmp_cost += 2500; count++; }
	total += (tmp_cost * count);

	if (have_flag(flgs, TR_SUST_STR)) total += 850;
	if (have_flag(flgs, TR_SUST_INT)) total += 850;
	if (have_flag(flgs, TR_SUST_WIS)) total += 850;
	if (have_flag(flgs, TR_SUST_DEX)) total += 850;
	if (have_flag(flgs, TR_SUST_CON)) total += 850;
	if (have_flag(flgs, TR_SUST_CHR)) total += 250;
	if (have_flag(flgs, TR_RIDING)) total += 0;
	if (have_flag(flgs, TR_EASY_SPELL)) total += 1500;
	if (have_flag(flgs, TR_THROW)) total += 5000;
	if (have_flag(flgs, TR_FREE_ACT)) total += 4500;
	if (have_flag(flgs, TR_HOLD_EXP)) total += 8500;

	tmp_cost = 0;
	count = 0;
	if (have_flag(flgs, TR_IM_ACID)) { tmp_cost += 15000; count += 2; }
	if (have_flag(flgs, TR_IM_ELEC)) { tmp_cost += 15000; count += 2; }
	if (have_flag(flgs, TR_IM_FIRE)) { tmp_cost += 15000; count += 2; }
	if (have_flag(flgs, TR_IM_COLD)) { tmp_cost += 15000; count += 2; }
	if (have_flag(flgs, TR_REFLECT)) { tmp_cost += 5000; count += 2; }
	if (have_flag(flgs, TR_RES_ACID)) { tmp_cost += 500; count++; }
	if (have_flag(flgs, TR_RES_ELEC)) { tmp_cost += 500; count++; }
	if (have_flag(flgs, TR_RES_FIRE)) { tmp_cost += 500; count++; }
	if (have_flag(flgs, TR_RES_COLD)) { tmp_cost += 500; count++; }
	if (have_flag(flgs, TR_RES_POIS)) { tmp_cost += 1000; count += 2; }
	if (have_flag(flgs, TR_RES_FEAR)) { tmp_cost += 1000; count += 2; }
	if (have_flag(flgs, TR_RES_LITE)) { tmp_cost += 800; count += 2; }
	if (have_flag(flgs, TR_RES_DARK)) { tmp_cost += 800; count += 2; }
	if (have_flag(flgs, TR_RES_BLIND)) { tmp_cost += 900; count += 2; }
	if (have_flag(flgs, TR_RES_CONF)) { tmp_cost += 900; count += 2; }
	if (have_flag(flgs, TR_RES_SOUND)) { tmp_cost += 900; count += 2; }
	if (have_flag(flgs, TR_RES_SHARDS)) { tmp_cost += 900; count += 2; }
	if (have_flag(flgs, TR_RES_NETHER)) { tmp_cost += 900; count += 2; }
	if (have_flag(flgs, TR_RES_NEXUS)) { tmp_cost += 900; count += 2; }
	if (have_flag(flgs, TR_RES_CHAOS)) { tmp_cost += 1000; count += 2; }
	if (have_flag(flgs, TR_RES_DISEN)) { tmp_cost += 2000; count += 2; }
	total += (tmp_cost * count);

	if (have_flag(flgs, TR_SH_FIRE)) total += 5000;
	if (have_flag(flgs, TR_SH_ELEC)) total += 5000;
	if (have_flag(flgs, TR_SH_COLD)) total += 5000;
	if (have_flag(flgs, TR_NO_TELE)) total -= 10000;
	if (have_flag(flgs, TR_NO_MAGIC)) total += 2500;
	if (have_flag(flgs, TR_TY_CURSE)) total -= 15000;
	if (have_flag(flgs, TR_HIDE_TYPE)) total += 0;
	if (have_flag(flgs, TR_SHOW_MODS)) total += 0;
	if (have_flag(flgs, TR_LEVITATION)) total += 1250;
	if (have_flag(flgs, TR_LITE_1)) total += 1500;
	if (have_flag(flgs, TR_LITE_2)) total += 2500;
	if (have_flag(flgs, TR_LITE_3)) total += 4000;
	if (have_flag(flgs, TR_LITE_M1)) total -= 1500;
	if (have_flag(flgs, TR_LITE_M2)) total -= 2500;
	if (have_flag(flgs, TR_LITE_M3)) total -= 4000;
	if (have_flag(flgs, TR_SEE_INVIS)) total += 2000;
	if (have_flag(flgs, TR_TELEPATHY)) total += 20000;
	if (have_flag(flgs, TR_ESP_ANIMAL)) total += 1000;
	if (have_flag(flgs, TR_ESP_UNDEAD)) total += 1000;
	if (have_flag(flgs, TR_ESP_DEMON)) total += 1000;
	if (have_flag(flgs, TR_ESP_ORC)) total += 1000;
	if (have_flag(flgs, TR_ESP_TROLL)) total += 1000;
	if (have_flag(flgs, TR_ESP_GIANT)) total += 1000;
	if (have_flag(flgs, TR_ESP_DRAGON)) total += 1000;
	if (have_flag(flgs, TR_ESP_HUMAN)) total += 1000;
	if (have_flag(flgs, TR_ESP_EVIL)) total += 15000;
	if (have_flag(flgs, TR_ESP_GOOD)) total += 2000;
	if (have_flag(flgs, TR_ESP_NONLIVING)) total += 2000;
	if (have_flag(flgs, TR_ESP_UNIQUE)) total += 10000;
	if (have_flag(flgs, TR_SLOW_DIGEST)) total += 750;
	if (have_flag(flgs, TR_REGEN)) total += 2500;
	if (have_flag(flgs, TR_WARNING)) total += 2000;
	if (have_flag(flgs, TR_DEC_MANA)) total += 10000;
	if (have_flag(flgs, TR_XTRA_MIGHT)) total += 2250;
	if (have_flag(flgs, TR_XTRA_SHOTS)) total += 10000;
	if (have_flag(flgs, TR_IGNORE_ACID)) total += 100;
	if (have_flag(flgs, TR_IGNORE_ELEC)) total += 100;
	if (have_flag(flgs, TR_IGNORE_FIRE)) total += 100;
	if (have_flag(flgs, TR_IGNORE_COLD)) total += 100;
	if (have_flag(flgs, TR_ACTIVATE)) total += 100;
	if (have_flag(flgs, TR_DRAIN_EXP)) total -= 12500;
	if (have_flag(flgs, TR_DRAIN_HP)) total -= 12500;
	if (have_flag(flgs, TR_DRAIN_MANA)) total -= 12500;
	if (have_flag(flgs, TR_CALL_ANIMAL)) total -= 12500;
	if (have_flag(flgs, TR_CALL_DEMON)) total -= 10000;
	if (have_flag(flgs, TR_CALL_DRAGON)) total -= 10000;
	if (have_flag(flgs, TR_CALL_UNDEAD)) total -= 10000;
	if (have_flag(flgs, TR_COWARDICE)) total -= 5000;
	if (have_flag(flgs, TR_LOW_MELEE)) total -= 5000;
	if (have_flag(flgs, TR_LOW_AC)) total -= 5000;
	if (have_flag(flgs, TR_LOW_MAGIC)) total -= 15000;
	if (have_flag(flgs, TR_FAST_DIGEST)) total -= 10000;
	if (have_flag(flgs, TR_SLOW_REGEN)) total -= 10000;
	if (have_flag(flgs, TR_TELEPORT))
	{
		if (object_is_cursed(o_ptr))
			total -= 7500;
		else
			total += 250;
	}

	if (have_flag(flgs, TR_AGGRAVATE)) total -= 10000;
	if (have_flag(flgs, TR_BLESSED)) total += 750;
	if (o_ptr->curse_flags & TR_ADD_L_CURSE) total -= 5000;
	if (o_ptr->curse_flags & TR_ADD_H_CURSE) total -= 12500;
	if (o_ptr->curse_flags & TRC_CURSED) total -= 5000;
	if (o_ptr->curse_flags & TRC_HEAVY_CURSE) total -= 12500;
	if (o_ptr->curse_flags & TRC_PERMA_CURSE) total -= 15000;

	/* Also, give some extra for activatable powers... */
	if (o_ptr->art_name && (have_flag(o_ptr->art_flags, TR_ACTIVATE)))
	{
		const activation_type* const act_ptr = find_activation_info(o_ptr);
		if (act_ptr) {
			total += act_ptr->value;
		}
	}

	return total;
}


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
 * @brief 生成されたベースアイテムに魔法的な強化を与えるメインルーチン
 * Complete the "creation" of an object by applying "magic" to the item
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param lev 生成基準階
 * @param mode 生成オプション
 * @return なし
 * @details
 * This includes not only rolling for random bonuses, but also putting the\n
 * finishing touches on ego-items and artifacts, giving charges to wands and\n
 * staffs, giving fuel to lites, and placing traps on chests.\n
 *\n
 * In particular, note that "Instant Artifacts", if "created" by an external\n
 * routine, must pass through this function to complete the actual creation.\n
 *\n
 * The base "chance" of the item being "good" increases with the "level"\n
 * parameter, which is usually derived from the dungeon level, being equal\n
 * to the level plus 10, up to a maximum of 75.  If "good" is true, then\n
 * the object is guaranteed to be "good".  If an object is "good", then\n
 * the chance that the object will be "great" (ego-item or artifact), also\n
 * increases with the "level", being equal to half the level, plus 5, up to\n
 * a maximum of 20.  If "great" is true, then the object is guaranteed to be\n
 * "great".  At dungeon level 65 and below, 15/100 objects are "great".\n
 *\n
 * If the object is not "good", there is a chance it will be "cursed", and\n
 * if it is "cursed", there is a chance it will be "broken".  These chances\n
 * are related to the "good" / "great" chances above.\n
 *\n
 * Otherwise "normal" rings and amulets will be "good" half the time and\n
 * "cursed" half the time, unless the ring/amulet is always good or cursed.\n
 *\n
 * If "okay" is true, and the object is going to be "great", then there is\n
 * a chance that an artifact will be created.  This is true even if both the\n
 * "good" and "great" arguments are false.  As a total hack, if "great" is\n
 * true, then the item gets 3 extra "attempts" to become an artifact.\n
 */
void apply_magic(player_type *owner_ptr, object_type *o_ptr, DEPTH lev, BIT_FLAGS mode)
{
	if (owner_ptr->pseikaku == PERSONALITY_MUNCHKIN) lev += randint0(owner_ptr->lev / 2 + 10);
	if (lev > MAX_DEPTH - 1) lev = MAX_DEPTH - 1;

	int f1 = lev + 10;
	if (f1 > d_info[owner_ptr->dungeon_idx].obj_good) f1 = d_info[owner_ptr->dungeon_idx].obj_good;

	int f2 = f1 * 2 / 3;
	if ((owner_ptr->pseikaku != PERSONALITY_MUNCHKIN) && (f2 > d_info[owner_ptr->dungeon_idx].obj_great))
		f2 = d_info[owner_ptr->dungeon_idx].obj_great;

	if (owner_ptr->muta3 & MUT3_GOOD_LUCK)
	{
		f1 += 5;
		f2 += 2;
	}
	else if (owner_ptr->muta3 & MUT3_BAD_LUCK)
	{
		f1 -= 5;
		f2 -= 2;
	}

	int power = 0;
	if ((mode & AM_GOOD) || magik(f1))
	{
		power = 1;
		if ((mode & AM_GREAT) || magik(f2))
		{
			power = 2;
			if (mode & AM_SPECIAL) power = 3;
		}
	}
	else if (magik(f1))
	{
		power = -1;
		if (magik(f2)) power = -2;
	}
	if (mode & AM_CURSED)
	{
		if (power > 0)
		{
			power = 0 - power;
		}
		else
		{
			power--;
		}
	}

	int rolls = 0;
	if (power >= 2) rolls = 1;

	if (mode & (AM_GREAT | AM_SPECIAL)) rolls = 4;
	if ((mode & AM_NO_FIXED_ART) || o_ptr->name1) rolls = 0;

	for (int i = 0; i < rolls; i++)
	{
		if (make_artifact(owner_ptr, o_ptr)) break;
		if ((owner_ptr->muta3 & MUT3_GOOD_LUCK) && one_in_(77))
		{
			if (make_artifact(owner_ptr, o_ptr)) break;
		}
	}

	if (object_is_fixed_artifact(o_ptr))
	{
		artifact_type *a_ptr = &a_info[o_ptr->name1];
		a_ptr->cur_num = 1;
		if (current_world_ptr->character_dungeon)
			a_ptr->floor_id = owner_ptr->floor_id;

		o_ptr->pval = a_ptr->pval;
		o_ptr->ac = a_ptr->ac;
		o_ptr->dd = a_ptr->dd;
		o_ptr->ds = a_ptr->ds;
		o_ptr->to_a = a_ptr->to_a;
		o_ptr->to_h = a_ptr->to_h;
		o_ptr->to_d = a_ptr->to_d;
		o_ptr->weight = a_ptr->weight;
		o_ptr->xtra2 = a_ptr->act_idx;

		if (o_ptr->name1 == ART_MILIM)
		{
			if (owner_ptr->pseikaku == PERSONALITY_SEXY)
			{
				o_ptr->pval = 3;
			}
		}

		if (!a_ptr->cost) o_ptr->ident |= (IDENT_BROKEN);
		if (a_ptr->gen_flags & TRG_CURSED) o_ptr->curse_flags |= (TRC_CURSED);
		if (a_ptr->gen_flags & TRG_HEAVY_CURSE) o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
		if (a_ptr->gen_flags & TRG_PERMA_CURSE) o_ptr->curse_flags |= (TRC_PERMA_CURSE);
		if (a_ptr->gen_flags & (TRG_RANDOM_CURSE0)) o_ptr->curse_flags |= get_curse(0, o_ptr);
		if (a_ptr->gen_flags & (TRG_RANDOM_CURSE1)) o_ptr->curse_flags |= get_curse(1, o_ptr);
		if (a_ptr->gen_flags & (TRG_RANDOM_CURSE2)) o_ptr->curse_flags |= get_curse(2, o_ptr);

		return;
	}

	switch (o_ptr->tval)
	{
	case TV_DIGGING:
	case TV_HAFTED:
	case TV_BOW:
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	{
		if (power) apply_magic_weapon(owner_ptr, o_ptr, lev, power);
		break;
	}
	case TV_POLEARM:
	{
		if (power && !(o_ptr->sval == SV_DEATH_SCYTHE)) apply_magic_weapon(owner_ptr, o_ptr, lev, power);
		break;
	}
	case TV_SWORD:
	{
		if (power && !(o_ptr->sval == SV_POISON_NEEDLE)) apply_magic_weapon(owner_ptr, o_ptr, lev, power);
		break;
	}
	case TV_DRAG_ARMOR:
	case TV_HARD_ARMOR:
	case TV_SOFT_ARMOR:
	case TV_SHIELD:
	case TV_HELM:
	case TV_CROWN:
	case TV_CLOAK:
	case TV_GLOVES:
	case TV_BOOTS:
	{
		if (((o_ptr->tval == TV_CLOAK) && (o_ptr->sval == SV_ELVEN_CLOAK)) ||
			((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_KUROSHOUZOKU)))
			o_ptr->pval = randint1(4);

		if (power ||
			((o_ptr->tval == TV_HELM) && (o_ptr->sval == SV_DRAGON_HELM)) ||
			((o_ptr->tval == TV_SHIELD) && (o_ptr->sval == SV_DRAGON_SHIELD)) ||
			((o_ptr->tval == TV_GLOVES) && (o_ptr->sval == SV_SET_OF_DRAGON_GLOVES)) ||
			((o_ptr->tval == TV_BOOTS) && (o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE)))
			apply_magic_armor(owner_ptr, o_ptr, lev, power);

		break;
	}
	case TV_RING:
	case TV_AMULET:
	{
		if (!power && (randint0(100) < 50)) power = -1;
		apply_magic_accessary(owner_ptr, o_ptr, lev, power);
		break;
	}
	default:
	{
		apply_magic_others(owner_ptr, o_ptr, power);
		break;
	}
	}

	if ((o_ptr->tval == TV_SOFT_ARMOR) &&
		(o_ptr->sval == SV_ABUNAI_MIZUGI) &&
		(owner_ptr->pseikaku == PERSONALITY_SEXY))
	{
		o_ptr->pval = 3;
		add_flag(o_ptr->art_flags, TR_STR);
		add_flag(o_ptr->art_flags, TR_INT);
		add_flag(o_ptr->art_flags, TR_WIS);
		add_flag(o_ptr->art_flags, TR_DEX);
		add_flag(o_ptr->art_flags, TR_CON);
		add_flag(o_ptr->art_flags, TR_CHR);
	}

	if (object_is_ego(o_ptr))
	{
		ego_item_type *e_ptr = &e_info[o_ptr->name2];
		if (!e_ptr->cost) o_ptr->ident |= (IDENT_BROKEN);

		if (e_ptr->gen_flags & TRG_CURSED) o_ptr->curse_flags |= (TRC_CURSED);
		if (e_ptr->gen_flags & TRG_HEAVY_CURSE) o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
		if (e_ptr->gen_flags & TRG_PERMA_CURSE) o_ptr->curse_flags |= (TRC_PERMA_CURSE);
		if (e_ptr->gen_flags & (TRG_RANDOM_CURSE0)) o_ptr->curse_flags |= get_curse(0, o_ptr);
		if (e_ptr->gen_flags & (TRG_RANDOM_CURSE1)) o_ptr->curse_flags |= get_curse(1, o_ptr);
		if (e_ptr->gen_flags & (TRG_RANDOM_CURSE2)) o_ptr->curse_flags |= get_curse(2, o_ptr);

		if (e_ptr->gen_flags & (TRG_ONE_SUSTAIN)) one_sustain(o_ptr);
		if (e_ptr->gen_flags & (TRG_XTRA_POWER)) one_ability(o_ptr);
		if (e_ptr->gen_flags & (TRG_XTRA_H_RES)) one_high_resistance(o_ptr);
		if (e_ptr->gen_flags & (TRG_XTRA_E_RES)) one_ele_resistance(o_ptr);
		if (e_ptr->gen_flags & (TRG_XTRA_D_RES)) one_dragon_ele_resistance(o_ptr);
		if (e_ptr->gen_flags & (TRG_XTRA_L_RES)) one_lordly_high_resistance(o_ptr);
		if (e_ptr->gen_flags & (TRG_XTRA_RES)) one_resistance(o_ptr);
		if (e_ptr->gen_flags & (TRG_XTRA_DICE))
		{
			do
			{
				o_ptr->dd++;
			} while (one_in_(o_ptr->dd));

			if (o_ptr->dd > 9) o_ptr->dd = 9;
		}

		if (e_ptr->act_idx) o_ptr->xtra2 = (XTRA8)e_ptr->act_idx;

		if ((object_is_cursed(o_ptr) || object_is_broken(o_ptr)) && !(e_ptr->gen_flags & (TRG_POWERFUL)))
		{
			if (e_ptr->max_to_h) o_ptr->to_h -= randint1(e_ptr->max_to_h);
			if (e_ptr->max_to_d) o_ptr->to_d -= randint1(e_ptr->max_to_d);
			if (e_ptr->max_to_a) o_ptr->to_a -= randint1(e_ptr->max_to_a);
			if (e_ptr->max_pval) o_ptr->pval -= randint1(e_ptr->max_pval);
		}
		else
		{
			if (e_ptr->max_to_h)
			{
				if (e_ptr->max_to_h > 127)
					o_ptr->to_h -= randint1(256 - e_ptr->max_to_h);
				else o_ptr->to_h += randint1(e_ptr->max_to_h);
			}

			if (e_ptr->max_to_d)
			{
				if (e_ptr->max_to_d > 127)
					o_ptr->to_d -= randint1(256 - e_ptr->max_to_d);
				else o_ptr->to_d += randint1(e_ptr->max_to_d);
			}

			if (e_ptr->max_to_a)
			{
				if (e_ptr->max_to_a > 127)
					o_ptr->to_a -= randint1(256 - e_ptr->max_to_a);
				else o_ptr->to_a += randint1(e_ptr->max_to_a);
			}

			if (o_ptr->name2 == EGO_ACCURACY)
			{
				while (o_ptr->to_h < o_ptr->to_d + 10)
				{
					o_ptr->to_h += 5;
					o_ptr->to_d -= 5;
				}
				o_ptr->to_h = MAX(o_ptr->to_h, 15);
			}

			if (o_ptr->name2 == EGO_VELOCITY)
			{
				while (o_ptr->to_d < o_ptr->to_h + 10)
				{
					o_ptr->to_d += 5;
					o_ptr->to_h -= 5;
				}
				o_ptr->to_d = MAX(o_ptr->to_d, 15);
			}

			if ((o_ptr->name2 == EGO_PROTECTION) || (o_ptr->name2 == EGO_S_PROTECTION) || (o_ptr->name2 == EGO_H_PROTECTION))
			{
				o_ptr->to_a = MAX(o_ptr->to_a, 15);
			}

			if (e_ptr->max_pval)
			{
				if ((o_ptr->name2 == EGO_HA) && (have_flag(o_ptr->art_flags, TR_BLOWS)))
				{
					o_ptr->pval++;
					if ((lev > 60) && one_in_(3) && ((o_ptr->dd*(o_ptr->ds + 1)) < 15)) o_ptr->pval++;
				}
				else if (o_ptr->name2 == EGO_DEMON)
				{
					if (have_flag(o_ptr->art_flags, TR_BLOWS))
					{
						o_ptr->pval += randint1(2);
					}
					else
					{
						o_ptr->pval += randint1(e_ptr->max_pval);
					}
				}
				else if (o_ptr->name2 == EGO_ATTACKS)
				{
					o_ptr->pval = randint1(e_ptr->max_pval*lev / 100 + 1);
					if (o_ptr->pval > 3) o_ptr->pval = 3;
					if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_HAYABUSA))
						o_ptr->pval += randint1(2);
				}
				else if (o_ptr->name2 == EGO_BAT)
				{
					o_ptr->pval = randint1(e_ptr->max_pval);
					if (o_ptr->sval == SV_ELVEN_CLOAK) o_ptr->pval += randint1(2);
				}
				else if (o_ptr->name2 == EGO_A_DEMON || o_ptr->name2 == EGO_DRUID || o_ptr->name2 == EGO_OLOG)
				{
					o_ptr->pval = randint1(e_ptr->max_pval);
				}
				else
				{
					o_ptr->pval += randint1(e_ptr->max_pval);
				}
			}

			if ((o_ptr->name2 == EGO_SPEED) && (lev < 50))
			{
				o_ptr->pval = randint1(o_ptr->pval);
			}

			if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_HAYABUSA) && (o_ptr->pval > 2) && (o_ptr->name2 != EGO_ATTACKS))
				o_ptr->pval = 2;
		}

		return;
	}

	if (o_ptr->k_idx)
	{
		object_kind *k_ptr = &k_info[o_ptr->k_idx];
		if (!k_info[o_ptr->k_idx].cost) o_ptr->ident |= (IDENT_BROKEN);

		if (k_ptr->gen_flags & (TRG_CURSED)) o_ptr->curse_flags |= (TRC_CURSED);
		if (k_ptr->gen_flags & (TRG_HEAVY_CURSE)) o_ptr->curse_flags |= TRC_HEAVY_CURSE;
		if (k_ptr->gen_flags & (TRG_PERMA_CURSE)) o_ptr->curse_flags |= TRC_PERMA_CURSE;
		if (k_ptr->gen_flags & (TRG_RANDOM_CURSE0)) o_ptr->curse_flags |= get_curse(0, o_ptr);
		if (k_ptr->gen_flags & (TRG_RANDOM_CURSE1)) o_ptr->curse_flags |= get_curse(1, o_ptr);
		if (k_ptr->gen_flags & (TRG_RANDOM_CURSE2)) o_ptr->curse_flags |= get_curse(2, o_ptr);
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
