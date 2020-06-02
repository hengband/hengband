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
#include "inventory/inventory-object.h" // 暫定、相互参照している.
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "monster/monster-race-hook.h"
#include "monster/monster.h"
#include "object/artifact.h"
#include "object/item-apply-magic.h"
#include "object/item-feeling.h"
#include "object/object-appraiser.h" // 暫定、相互参照している.
#include "object/object-boost.h"
#include "object/object-curse.h"
#include "object/object-ego.h"
#include "object/object-flavor.h"
#include "object/object-hook.h"
#include "object/object-kind.h"
#include "object/object-mark-types.h"
#include "object/object-value.h" // 暫定、相互参照している.
#include "object/special-object-flags.h"
#include "object/sv-amulet-types.h"
#include "object/sv-armor-types.h"
#include "object/sv-lite-types.h"
#include "object/sv-other-types.h"
#include "object/sv-protector-types.h"
#include "object/sv-ring-types.h"
#include "object/sv-weapon-types.h"
#include "object/trc-types.h"
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
 * todo この関数ポインタは何とかならんのか？
 * Hack -- function hook to restrict "get_obj_num_prep()" function
 */
bool(*get_obj_num_hook)(KIND_OBJECT_IDX k_idx);

OBJECT_SUBTYPE_VALUE coin_type;	/* Hack -- force coin type */

/*!
 * @brief オブジェクト生成テーブルからアイテムを取得する /
 * Choose an object kind that seems "appropriate" to the given level
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param level 生成階
 * @return 選ばれたオブジェクトベースID
 * @details
 * This function uses the "prob2" field of the "object allocation table",\n
 * and various local information, to calculate the "prob3" field of the\n
 * same table, which is then used to choose an "appropriate" object, in\n
 * a relatively efficient manner.\n
 *\n
 * It is (slightly) more likely to acquire an object of the given level\n
 * than one of a lower level.  This is done by choosing several objects\n
 * appropriate to the given level and keeping the "hardest" one.\n
 *\n
 * Note that if no objects are "appropriate", then this function will\n
 * fail, and return zero, but this should *almost* never happen.\n
 */
OBJECT_IDX get_obj_num(player_type *owner_ptr, DEPTH level, BIT_FLAGS mode)
{
	int i, j, p;
	KIND_OBJECT_IDX k_idx;
	long value, total;
	object_kind     *k_ptr;
	alloc_entry     *table = alloc_kind_table;

	if (level > MAX_DEPTH - 1) level = MAX_DEPTH - 1;

	if ((level > 0) && !(d_info[owner_ptr->dungeon_idx].flags1 & DF1_BEGINNER))
	{
		if (one_in_(GREAT_OBJ))
		{
			level = 1 + (level * MAX_DEPTH / randint1(MAX_DEPTH));
		}
	}

	total = 0L;
	for (i = 0; i < alloc_kind_size; i++)
	{
		if (table[i].level > level) break;

		table[i].prob3 = 0;
		k_idx = table[i].index;
		k_ptr = &k_info[k_idx];

		if ((mode & AM_FORBID_CHEST) && (k_ptr->tval == TV_CHEST)) continue;

		table[i].prob3 = table[i].prob2;
		total += table[i].prob3;
	}

	if (total <= 0) return 0;

	value = randint0(total);
	for (i = 0; i < alloc_kind_size; i++)
	{
		if (value < table[i].prob3) break;

		value = value - table[i].prob3;
	}

	p = randint0(100);
	if (p < 60)
	{
		j = i;
		value = randint0(total);
		for (i = 0; i < alloc_kind_size; i++)
		{
			if (value < table[i].prob3) break;

			value = value - table[i].prob3;
		}

		if (table[i].level < table[j].level) i = j;
	}

	if (p >= 10) return (table[i].index);

	j = i;
	value = randint0(total);
	for (i = 0; i < alloc_kind_size; i++)
	{
		if (value < table[i].prob3) break;

		value = value - table[i].prob3;
	}

	if (table[i].level < table[j].level) i = j;
	return (table[i].index);
}


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
 * @brief tvalとsvalに対応するベースアイテムのIDを返す。
 * Find the index of the object_kind with the given tval and sval
 * @param tval 検索したいベースアイテムのtval
 * @param sval 検索したいベースアイテムのsval
 * @return なし
 */
KIND_OBJECT_IDX lookup_kind(tval_type tval, OBJECT_SUBTYPE_VALUE sval)
{
	int num = 0;
	KIND_OBJECT_IDX bk = 0;

	for (KIND_OBJECT_IDX k = 1; k < max_k_idx; k++)
	{
		object_kind *k_ptr = &k_info[k];
		if (k_ptr->tval != tval) continue;
		if (k_ptr->sval == sval) return (k);
		if (sval != SV_ANY) continue;
		if (!one_in_(++num)) continue;

		bk = k;
	}

	if (sval == SV_ANY)
	{
		return bk;
	}

	return 0;
}


/*!
 * @brief オブジェクトを初期化する
 * Wipe an object clean.
 * @param o_ptr 初期化したいオブジェクトの構造体参照ポインタ
 * @return なし
 */
void object_wipe(object_type *o_ptr)
{
	(void)WIPE(o_ptr, object_type);
}


/*!
 * @brief オブジェクトを複製する
 * Wipe an object clean.
 * @param o_ptr 複製元のオブジェクトの構造体参照ポインタ
 * @param j_ptr 複製先のオブジェクトの構造体参照ポインタ
 * @return なし
 */
void object_copy(object_type *o_ptr, object_type *j_ptr)
{
	(void)COPY(o_ptr, j_ptr, object_type);
}


/*!
 * @brief オブジェクト構造体にベースアイテムを作成する
 * Prepare an object based on an object kind.
 * @param o_ptr 代入したいオブジェクトの構造体参照ポインタ
 * @param k_idx 新たに作成したいベースアイテム情報のID
 * @return なし
 */
void object_prep(object_type *o_ptr, KIND_OBJECT_IDX k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];
	object_wipe(o_ptr);
	o_ptr->k_idx = k_idx;
	o_ptr->tval = k_ptr->tval;
	o_ptr->sval = k_ptr->sval;
	o_ptr->pval = k_ptr->pval;
	o_ptr->number = 1;
	o_ptr->weight = k_ptr->weight;
	o_ptr->to_h = k_ptr->to_h;
	o_ptr->to_d = k_ptr->to_d;
	o_ptr->to_a = k_ptr->to_a;
	o_ptr->ac = k_ptr->ac;
	o_ptr->dd = k_ptr->dd;
	o_ptr->ds = k_ptr->ds;

	if (k_ptr->act_idx > 0) o_ptr->xtra2 = (XTRA8)k_ptr->act_idx;
	if (k_info[o_ptr->k_idx].cost <= 0) o_ptr->ident |= (IDENT_BROKEN);

	if (k_ptr->gen_flags & (TRG_CURSED)) o_ptr->curse_flags |= (TRC_CURSED);
	if (k_ptr->gen_flags & (TRG_HEAVY_CURSE)) o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
	if (k_ptr->gen_flags & (TRG_PERMA_CURSE)) o_ptr->curse_flags |= (TRC_PERMA_CURSE);
	if (k_ptr->gen_flags & (TRG_RANDOM_CURSE0)) o_ptr->curse_flags |= get_curse(0, o_ptr);
	if (k_ptr->gen_flags & (TRG_RANDOM_CURSE1)) o_ptr->curse_flags |= get_curse(1, o_ptr);
	if (k_ptr->gen_flags & (TRG_RANDOM_CURSE2)) o_ptr->curse_flags |= get_curse(2, o_ptr);
}


/*!
 * @brief アイテムのエゴをレア度の重みに合わせてランダムに選択する
 * Choose random ego type
 * @param slot 取得したいエゴの装備部位
 * @param good TRUEならば通常のエゴ、FALSEならば呪いのエゴが選択対象となる。
 * @return 選択されたエゴ情報のID、万一選択できなかった場合はmax_e_idxが返る。
 */
static byte get_random_ego(byte slot, bool good)
{
	long total = 0L;
	for (int i = 1; i < max_e_idx; i++)
	{
		ego_item_type *e_ptr;
		e_ptr = &e_info[i];
		if (e_ptr->slot == slot
			&& ((good && e_ptr->rating) || (!good && !e_ptr->rating)))
		{
			if (e_ptr->rarity)
				total += (255 / e_ptr->rarity);
		}
	}

	int value = randint1(total);
	int j;
	for (j = 1; j < max_e_idx; j++)
	{
		ego_item_type *e_ptr;
		e_ptr = &e_info[j];
		if (e_ptr->slot == slot
			&& ((good && e_ptr->rating) || (!good && !e_ptr->rating)))
		{
			if (e_ptr->rarity)
				value -= (255 / e_ptr->rarity);
			if (value <= 0L) break;
		}
	}

	return (byte)j;
}


/*!
 * @brief 武器系オブジェクトに生成ランクごとの強化を与えるサブルーチン
 * Apply magic to an item known to be a "weapon"
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 * @return なし
 * @details
 * Hack -- note special base damage dice boosting\n
 * Hack -- note special processing for weapon/digger\n
 */
void apply_magic_weapon(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power)
{
	HIT_PROB tohit1 = randint1(5) + (HIT_PROB)m_bonus(5, level);
	HIT_POINT todam1 = randint1(5) + (HIT_POINT)m_bonus(5, level);

	HIT_PROB tohit2 = (HIT_PROB)m_bonus(10, level);
	HIT_POINT todam2 = (HIT_POINT)m_bonus(10, level);

	if ((o_ptr->tval == TV_BOLT) || (o_ptr->tval == TV_ARROW) || (o_ptr->tval == TV_SHOT))
	{
		tohit2 = (tohit2 + 1) / 2;
		todam2 = (todam2 + 1) / 2;
	}

	if (power > 0)
	{
		o_ptr->to_h += tohit1;
		o_ptr->to_d += todam1;
		if (power > 1)
		{
			o_ptr->to_h += tohit2;
			o_ptr->to_d += todam2;
		}
	}
	else if (power < 0)
	{
		o_ptr->to_h -= tohit1;
		o_ptr->to_d -= todam1;
		if (power < -1)
		{
			o_ptr->to_h -= tohit2;
			o_ptr->to_d -= todam2;
		}

		if (o_ptr->to_h + o_ptr->to_d < 0)
			o_ptr->curse_flags |= TRC_CURSED;
	}

	if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DIAMOND_EDGE)) return;

	switch (o_ptr->tval)
	{
	case TV_DIGGING:
	{
		if (power > 1)
		{
			/* power > 2 is debug only */
			if (one_in_(30) || (power > 2))
				become_random_artifact(owner_ptr, o_ptr, FALSE);
			else
				o_ptr->name2 = EGO_DIGGING;
		}
		else if (power < -1)
		{
			o_ptr->pval = 0 - (5 + randint1(5));
		}
		else if (power < 0)
		{
			o_ptr->pval = 0 - (o_ptr->pval);
		}

		break;
	}
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_SWORD:
	{
		if (power > 1)
		{
			/* power > 2 is debug only */
			if (one_in_(40) || (power > 2))
			{
				become_random_artifact(owner_ptr, o_ptr, FALSE);
				break;
			}
			while (TRUE)
			{
				o_ptr->name2 = get_random_ego(INVEN_RARM, TRUE);
				if (o_ptr->name2 == EGO_SHARPNESS && o_ptr->tval != TV_SWORD)
					continue;
				if (o_ptr->name2 == EGO_EARTHQUAKES && o_ptr->tval != TV_HAFTED)
					continue;
				if (o_ptr->name2 == EGO_WEIRD && o_ptr->tval != TV_SWORD)
					continue;
				break;
			}

			switch (o_ptr->name2)
			{
			case EGO_HA:
				if (one_in_(4) && (level > 40))
					add_flag(o_ptr->art_flags, TR_BLOWS);
				break;
			case EGO_DF:
				if (one_in_(3))
					add_flag(o_ptr->art_flags, TR_RES_POIS);
				if (one_in_(3))
					add_flag(o_ptr->art_flags, TR_WARNING);
				break;
			case EGO_KILL_DRAGON:
				if (one_in_(3))
					add_flag(o_ptr->art_flags, TR_RES_POIS);
				break;
			case EGO_WEST:
				if (one_in_(3))
					add_flag(o_ptr->art_flags, TR_RES_FEAR);
				break;
			case EGO_SLAYING_WEAPON:
				if (one_in_(3))
					o_ptr->dd *= 2;
				else
				{
					do
					{
						o_ptr->dd++;
					} while (one_in_(o_ptr->dd));

					do
					{
						o_ptr->ds++;
					} while (one_in_(o_ptr->ds));
				}

				if (one_in_(5))
				{
					add_flag(o_ptr->art_flags, TR_BRAND_POIS);
				}
				if (o_ptr->tval == TV_SWORD && one_in_(3))
				{
					add_flag(o_ptr->art_flags, TR_VORPAL);
				}
				break;
			case EGO_TRUMP:
				if (one_in_(5))
					add_flag(o_ptr->art_flags, TR_SLAY_DEMON);
				if (one_in_(7))
					one_ability(o_ptr);
				break;
			case EGO_PATTERN:
				if (one_in_(3))
					add_flag(o_ptr->art_flags, TR_HOLD_EXP);
				if (one_in_(3))
					add_flag(o_ptr->art_flags, TR_DEX);
				if (one_in_(5))
					add_flag(o_ptr->art_flags, TR_RES_FEAR);
				break;
			case EGO_SHARPNESS:
				o_ptr->pval = (PARAMETER_VALUE)m_bonus(5, level) + 1;
				break;
			case EGO_EARTHQUAKES:
				if (one_in_(3) && (level > 60))
					add_flag(o_ptr->art_flags, TR_BLOWS);
				else
					o_ptr->pval = (PARAMETER_VALUE)m_bonus(3, level);
				break;
			case EGO_VAMPIRIC:
				if (one_in_(5))
					add_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
				break;
			case EGO_DEMON:

				if (one_in_(3)) o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
				one_in_(3) ?
					add_flag(o_ptr->art_flags, TR_DRAIN_EXP) :
					one_in_(2) ?
					add_flag(o_ptr->art_flags, TR_DRAIN_HP) :
					add_flag(o_ptr->art_flags, TR_DRAIN_MANA);


				if (one_in_(3)) add_flag(o_ptr->art_flags, TR_CHAOTIC);
				if (one_in_(4)) add_flag(o_ptr->art_flags, TR_BLOWS);
				if (one_in_(5)) add_flag(o_ptr->art_flags, TR_ADD_H_CURSE);
				if (one_in_(5)) add_flag(o_ptr->art_flags, TR_CALL_DEMON);
				break;
			}

			if (!o_ptr->art_name)
			{
				while (one_in_(10L * o_ptr->dd * o_ptr->ds))
					o_ptr->dd++;

				if (o_ptr->dd > 9) o_ptr->dd = 9;
			}
		}
		else if (power < -1)
		{
			if (randint0(MAX_DEPTH) < level)
			{
				while (TRUE)
				{
					o_ptr->name2 = get_random_ego(INVEN_RARM, FALSE);
					if (o_ptr->name2 == EGO_WEIRD && o_ptr->tval != TV_SWORD)
					{
						continue;
					}

					break;
				}

				switch (o_ptr->name2)
				{
				case EGO_MORGUL:
					if (one_in_(6)) add_flag(o_ptr->art_flags, TR_TY_CURSE);
					if (one_in_(3)) o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
					break;
				case EGO_WEIRD:
					if (one_in_(4)) add_flag(o_ptr->art_flags, TR_BRAND_POIS);
					if (one_in_(4)) add_flag(o_ptr->art_flags, TR_RES_NETHER);
					if (one_in_(3)) add_flag(o_ptr->art_flags, TR_NO_MAGIC);
					if (one_in_(6)) add_flag(o_ptr->art_flags, TR_NO_TELE);
					if (one_in_(6)) add_flag(o_ptr->art_flags, TR_TY_CURSE);
					if (one_in_(6)) add_flag(o_ptr->art_flags, TR_ADD_H_CURSE);
					break;
				}
			}
		}

		break;
	}
	case TV_BOW:
	{
		if (power > 1)
		{
			/* power > 2 is debug only */
			if (one_in_(20) || (power > 2))
			{
				become_random_artifact(owner_ptr, o_ptr, FALSE);
				break;
			}

			o_ptr->name2 = get_random_ego(INVEN_BOW, TRUE);
		}

		break;
	}
	case TV_BOLT:
	case TV_ARROW:
	case TV_SHOT:
	{
		if (power > 1)
		{
			/* power > 2 is debug only */
			if (power > 2)
			{
				become_random_artifact(owner_ptr, o_ptr, FALSE);
				break;
			}

			o_ptr->name2 = get_random_ego(INVEN_AMMO, TRUE);
			switch (o_ptr->name2)
			{
			case EGO_SLAYING_BOLT:
				o_ptr->dd++;
				break;
			}

			while (one_in_(10L * o_ptr->dd * o_ptr->ds))
				o_ptr->dd++;

			if (o_ptr->dd > 9) o_ptr->dd = 9;
		}
		else if (power < -1)
		{
			if (randint0(MAX_DEPTH) < level)
			{
				o_ptr->name2 = get_random_ego(INVEN_AMMO, FALSE);
			}
		}

		break;
	}
	}
}


/*!
 * @brief 防具系オブジェクトに生成ランクごとの強化を与えるサブルーチン
 * Apply magic to an item known to be "armor"
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 * @return なし
 * @details
 * Hack -- note special processing for crown/helm\n
 * Hack -- note special processing for robe of permanence\n
 */
static void a_m_aux_2(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power)
{
	ARMOUR_CLASS toac1 = (ARMOUR_CLASS)randint1(5) + m_bonus(5, level);
	ARMOUR_CLASS toac2 = (ARMOUR_CLASS)m_bonus(10, level);
	if (power > 0)
	{
		o_ptr->to_a += toac1;
		if (power > 1)
		{
			o_ptr->to_a += toac2;
		}
	}
	else if (power < 0)
	{
		o_ptr->to_a -= toac1;
		if (power < -1)
		{
			o_ptr->to_a -= toac2;
		}

		if (o_ptr->to_a < 0) o_ptr->curse_flags |= TRC_CURSED;
	}

	switch (o_ptr->tval)
	{
	case TV_DRAG_ARMOR:
	{
		/* power > 2 is debug only */
		if (one_in_(50) || (power > 2))
			become_random_artifact(owner_ptr, o_ptr, FALSE);
		break;
	}
	case TV_HARD_ARMOR:
	case TV_SOFT_ARMOR:
	{
		if (power > 1)
		{
			if ((o_ptr->tval == TV_SOFT_ARMOR) &&
				(o_ptr->sval == SV_ROBE) &&
				(randint0(100) < 15))
			{
				if (one_in_(5))
				{
					o_ptr->name2 = EGO_YOIYAMI;
					o_ptr->k_idx = lookup_kind(TV_SOFT_ARMOR, SV_YOIYAMI_ROBE);
					o_ptr->sval = SV_YOIYAMI_ROBE;
					o_ptr->ac = 0;
					o_ptr->to_a = 0;
				}
				else
				{
					o_ptr->name2 = EGO_PERMANENCE;
				}

				break;
			}

			/* power > 2 is debug only */
			if (one_in_(20) || (power > 2))
			{
				become_random_artifact(owner_ptr, o_ptr, FALSE);
				break;
			}

			while (TRUE)
			{
				bool okay_flag = TRUE;
				o_ptr->name2 = get_random_ego(INVEN_BODY, TRUE);
				switch (o_ptr->name2)
				{
				case EGO_DWARVEN:
					if (o_ptr->tval != TV_HARD_ARMOR)
					{
						okay_flag = FALSE;
					}

					break;
				case EGO_DRUID:
					if (o_ptr->tval != TV_SOFT_ARMOR)
					{
						okay_flag = FALSE;
					}

					break;
				default:
					break;
				}

				if (okay_flag) break;
			}

			switch (o_ptr->name2)
			{
			case EGO_RESISTANCE:
				if (one_in_(4))
					add_flag(o_ptr->art_flags, TR_RES_POIS);
				break;
			case EGO_DWARVEN:
				o_ptr->weight = (2 * k_info[o_ptr->k_idx].weight / 3);
				o_ptr->ac = k_info[o_ptr->k_idx].ac + 5;
				break;

			case EGO_A_DEMON:
				if (one_in_(3)) o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
				one_in_(3) ?
					add_flag(o_ptr->art_flags, TR_DRAIN_EXP) :
					one_in_(2) ?
					add_flag(o_ptr->art_flags, TR_DRAIN_HP) :
					add_flag(o_ptr->art_flags, TR_DRAIN_MANA);

				if (one_in_(3)) add_flag(o_ptr->art_flags, TR_AGGRAVATE);
				if (one_in_(3)) add_flag(o_ptr->art_flags, TR_ADD_L_CURSE);
				if (one_in_(5)) add_flag(o_ptr->art_flags, TR_ADD_H_CURSE);
				if (one_in_(5)) add_flag(o_ptr->art_flags, TR_DRAIN_HP);
				if (one_in_(5)) add_flag(o_ptr->art_flags, TR_DRAIN_MANA);
				if (one_in_(5)) add_flag(o_ptr->art_flags, TR_DRAIN_EXP);
				if (one_in_(5)) add_flag(o_ptr->art_flags, TR_TY_CURSE);
				if (one_in_(5)) add_flag(o_ptr->art_flags, TR_CALL_DEMON);
				break;
			case EGO_A_MORGUL:
				if (one_in_(3)) o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
				if (one_in_(9)) add_flag(o_ptr->art_flags, TR_TY_CURSE);
				if (one_in_(4)) add_flag(o_ptr->art_flags, TR_ADD_H_CURSE);
				if (one_in_(6)) add_flag(o_ptr->art_flags, TR_AGGRAVATE);
				if (one_in_(9)) add_flag(o_ptr->art_flags, TR_NO_MAGIC);
				if (one_in_(9)) add_flag(o_ptr->art_flags, TR_NO_TELE);
				break;
			default:
				break;
			}
		}

		break;
	}
	case TV_SHIELD:
	{
		if (o_ptr->sval == SV_DRAGON_SHIELD)
		{
			dragon_resist(o_ptr);
			if (!one_in_(3)) break;
		}

		if (power > 1)
		{
			/* power > 2 is debug only */
			if (one_in_(20) || (power > 2))
			{
				become_random_artifact(owner_ptr, o_ptr, FALSE);
				break;
			}

			while (TRUE)
			{
				o_ptr->name2 = get_random_ego(INVEN_LARM, TRUE);
				if (o_ptr->sval != SV_SMALL_METAL_SHIELD && o_ptr->sval != SV_LARGE_METAL_SHIELD
					&& o_ptr->name2 == EGO_S_DWARVEN)
				{
					continue;
				}

				break;
			}

			switch (o_ptr->name2)
			{
			case EGO_ENDURANCE:
				if (!one_in_(3)) one_high_resistance(o_ptr);
				if (one_in_(4)) add_flag(o_ptr->art_flags, TR_RES_POIS);
				break;
			case EGO_REFLECTION:
				if (o_ptr->sval == SV_MIRROR_SHIELD)
					o_ptr->name2 = 0;
				break;

			case EGO_S_DWARVEN:
				o_ptr->weight = (2 * k_info[o_ptr->k_idx].weight / 3);
				o_ptr->ac = k_info[o_ptr->k_idx].ac + 3;
				break;
			}
		}

		break;
	}
	case TV_GLOVES:
	{
		if (o_ptr->sval == SV_SET_OF_DRAGON_GLOVES)
		{
			dragon_resist(o_ptr);
			if (!one_in_(3)) break;
		}

		if (power > 1)
		{
			/* power > 2 is debug only */
			if (one_in_(20) || (power > 2))
			{
				become_random_artifact(owner_ptr, o_ptr, FALSE);
				break;
			}
			o_ptr->name2 = get_random_ego(INVEN_HANDS, TRUE);
		}
		else if (power < -1)
		{
			o_ptr->name2 = get_random_ego(INVEN_HANDS, FALSE);
		}

		break;
	}

	case TV_BOOTS:
	{
		if (o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE)
		{
			dragon_resist(o_ptr);
			if (!one_in_(3)) break;
		}

		if (power > 1)
		{
			/* power > 2 is debug only */
			if (one_in_(20) || (power > 2))
			{
				become_random_artifact(owner_ptr, o_ptr, FALSE);
				break;
			}

			o_ptr->name2 = get_random_ego(INVEN_FEET, TRUE);
			switch (o_ptr->name2)
			{
			case EGO_SLOW_DESCENT:
				if (one_in_(2))
				{
					one_high_resistance(o_ptr);
				}

				break;
			}
		}
		else if (power < -1)
		{
			o_ptr->name2 = get_random_ego(INVEN_FEET, FALSE);
		}

		break;
	}
	case TV_CROWN:
	{
		if (power > 1)
		{
			/* power > 2 is debug only */
			if (one_in_(20) || (power > 2))
			{
				become_random_artifact(owner_ptr, o_ptr, FALSE);
				break;
			}

			while (TRUE)
			{
				bool ok_flag = TRUE;
				o_ptr->name2 = get_random_ego(INVEN_HEAD, TRUE);

				switch (o_ptr->name2)
				{
				case EGO_TELEPATHY:
					if (add_esp_strong(o_ptr)) add_esp_weak(o_ptr, TRUE);
					else add_esp_weak(o_ptr, FALSE);
					break;
				case EGO_MAGI:
				case EGO_MIGHT:
				case EGO_REGENERATION:
				case EGO_LORDLINESS:
				case EGO_BASILISK:
					break;
				case EGO_SEEING:
					if (one_in_(3))
					{
						if (one_in_(2)) add_esp_strong(o_ptr);
						else add_esp_weak(o_ptr, FALSE);
					}
					break;
				default:
					/* not existing crown (wisdom,lite, etc...) */
					ok_flag = FALSE;
				}

				if (ok_flag)
					break;
			}

			break;
		}
		else if (power < -1)
		{
			while (TRUE)
			{
				bool ok_flag = TRUE;
				o_ptr->name2 = get_random_ego(INVEN_HEAD, FALSE);

				switch (o_ptr->name2)
				{
				case EGO_ANCIENT_CURSE:
					if (one_in_(3)) add_flag(o_ptr->art_flags, TR_NO_MAGIC);
					if (one_in_(3)) add_flag(o_ptr->art_flags, TR_NO_TELE);
					if (one_in_(3)) add_flag(o_ptr->art_flags, TR_TY_CURSE);
					if (one_in_(3)) add_flag(o_ptr->art_flags, TR_DRAIN_EXP);
					if (one_in_(3)) add_flag(o_ptr->art_flags, TR_DRAIN_HP);
					if (one_in_(3)) add_flag(o_ptr->art_flags, TR_DRAIN_MANA);
					break;
				}

				if (ok_flag)
					break;
			}
		}

		break;
	}
	case TV_HELM:
	{
		if (o_ptr->sval == SV_DRAGON_HELM)
		{
			dragon_resist(o_ptr);
			if (!one_in_(3)) break;
		}

		if (power > 1)
		{
			/* power > 2 is debug only */
			if (one_in_(20) || (power > 2))
			{
				become_random_artifact(owner_ptr, o_ptr, FALSE);
				break;
			}

			while (TRUE)
			{
				bool ok_flag = TRUE;
				o_ptr->name2 = get_random_ego(INVEN_HEAD, TRUE);
				switch (o_ptr->name2)
				{
				case EGO_BRILLIANCE:
				case EGO_DARK:
				case EGO_INFRAVISION:
				case EGO_H_PROTECTION:
					break;
				case EGO_SEEING:
					if (one_in_(7))
					{
						if (one_in_(2)) add_esp_strong(o_ptr);
						else add_esp_weak(o_ptr, FALSE);
					}

					break;
				case EGO_LITE:
					if (one_in_(3)) add_flag(o_ptr->art_flags, TR_LITE_1);
					if (one_in_(3)) add_flag(o_ptr->art_flags, TR_LITE_2);
					break;
				case EGO_H_DEMON:
					if (one_in_(3)) o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
					one_in_(3) ?
						add_flag(o_ptr->art_flags, TR_DRAIN_EXP) :
						one_in_(2) ?
						add_flag(o_ptr->art_flags, TR_DRAIN_HP) :
						add_flag(o_ptr->art_flags, TR_DRAIN_MANA);

					if (one_in_(3)) add_flag(o_ptr->art_flags, TR_AGGRAVATE);
					if (one_in_(3)) add_flag(o_ptr->art_flags, TR_ADD_L_CURSE);
					if (one_in_(5)) add_flag(o_ptr->art_flags, TR_ADD_H_CURSE);
					if (one_in_(5)) add_flag(o_ptr->art_flags, TR_DRAIN_HP);
					if (one_in_(5)) add_flag(o_ptr->art_flags, TR_DRAIN_MANA);
					if (one_in_(5)) add_flag(o_ptr->art_flags, TR_DRAIN_EXP);
					if (one_in_(5)) add_flag(o_ptr->art_flags, TR_TY_CURSE);
					if (one_in_(5)) add_flag(o_ptr->art_flags, TR_CALL_DEMON);
					break;
				default:
					/* not existing helm (Magi, Might, etc...)*/
					ok_flag = FALSE;
				}
				if (ok_flag)
					break;
			}

			break;
		}
		else if (power < -1)
		{
			while (TRUE)
			{
				bool ok_flag = TRUE;
				o_ptr->name2 = get_random_ego(INVEN_HEAD, FALSE);

				switch (o_ptr->name2)
				{
				case EGO_ANCIENT_CURSE:
					ok_flag = FALSE;
				}

				if (ok_flag)
					break;
			}
		}

		break;
	}
	case TV_CLOAK:
	{
		if (power > 1)
		{
			/* power > 2 is debug only */
			if (one_in_(20) || (power > 2))
			{
				become_random_artifact(owner_ptr, o_ptr, FALSE);
				break;
			}
			o_ptr->name2 = get_random_ego(INVEN_OUTER, TRUE);

			switch (o_ptr->name2)
			{
			case EGO_BAT:
				o_ptr->to_d -= 6;
				o_ptr->to_h -= 6;
				break;
			case EGO_NAZGUL:
				o_ptr->to_d -= 3;
				o_ptr->to_h -= 3;
				if (one_in_(3)) add_flag(o_ptr->art_flags, TR_COWARDICE);
				if (one_in_(3)) add_flag(o_ptr->art_flags, TR_CALL_UNDEAD);
				if (one_in_(3)) add_flag(o_ptr->art_flags, TR_SLOW_REGEN);
				if (one_in_(3)) add_flag(o_ptr->art_flags, TR_DRAIN_EXP);
				break;
			}

		}
		else if (power < -1)
		{
			o_ptr->name2 = get_random_ego(INVEN_OUTER, FALSE);
		}

		break;
	}
	}
}


/*!
 * @brief 装飾品系オブジェクトに生成ランクごとの強化を与えるサブルーチン
 * Apply magic to an item known to be a "ring" or "amulet"
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 * @return なし
 * @details
 * Hack -- note special "pval boost" code for ring of speed\n
 * Hack -- note that some items must be cursed (or blessed)\n
 */
static void a_m_aux_3(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power)
{
	switch (o_ptr->tval)
	{
	case TV_RING:
	{
		switch (o_ptr->sval)
		{
		case SV_RING_ATTACKS:
		{
			o_ptr->pval = (PARAMETER_VALUE)m_bonus(2, level);
			if (one_in_(15)) o_ptr->pval++;
			if (o_ptr->pval < 1) o_ptr->pval = 1;

			if (power < 0)
			{
				o_ptr->ident |= (IDENT_BROKEN);
				o_ptr->curse_flags |= TRC_CURSED;
				o_ptr->pval = 0 - (o_ptr->pval);
			}

			break;
		}
		case SV_RING_SHOTS:
		{
			break;
		}
		case SV_RING_STR:
		case SV_RING_CON:
		case SV_RING_DEX:
		{
			o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(5, level);
			if (power < 0)
			{
				o_ptr->ident |= (IDENT_BROKEN);
				o_ptr->curse_flags |= TRC_CURSED;
				o_ptr->pval = 0 - (o_ptr->pval);
			}

			break;
		}
		case SV_RING_SPEED:
		{
			o_ptr->pval = randint1(5) + (PARAMETER_VALUE)m_bonus(5, level);
			while (randint0(100) < 50) o_ptr->pval++;

			if (power < 0)
			{
				o_ptr->ident |= (IDENT_BROKEN);
				o_ptr->curse_flags |= TRC_CURSED;
				o_ptr->pval = 0 - (o_ptr->pval);
				break;
			}

			break;
		}
		case SV_RING_LORDLY:
		{
			do
			{
				one_lordly_high_resistance(o_ptr);
			} while (one_in_(4));

			o_ptr->to_a = 10 + randint1(5) + (ARMOUR_CLASS)m_bonus(10, level);
			break;
		}
		case SV_RING_WARNING:
		{
			if (one_in_(3)) one_low_esp(o_ptr);
			break;
		}
		case SV_RING_SEARCHING:
		{
			o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(5, level);
			if (power < 0)
			{
				o_ptr->ident |= (IDENT_BROKEN);
				o_ptr->curse_flags |= TRC_CURSED;
				o_ptr->pval = 0 - (o_ptr->pval);
			}

			break;
		}
		case SV_RING_FLAMES:
		case SV_RING_ACID:
		case SV_RING_ICE:
		case SV_RING_ELEC:
		{
			o_ptr->to_a = 5 + randint1(5) + (ARMOUR_CLASS)m_bonus(10, level);
			break;
		}
		case SV_RING_WEAKNESS:
		case SV_RING_STUPIDITY:
		{
			o_ptr->ident |= (IDENT_BROKEN);
			o_ptr->curse_flags |= TRC_CURSED;
			o_ptr->pval = 0 - (1 + (PARAMETER_VALUE)m_bonus(5, level));
			if (power > 0) power = 0 - power;

			break;
		}
		case SV_RING_WOE:
		{
			o_ptr->ident |= (IDENT_BROKEN);
			o_ptr->curse_flags |= TRC_CURSED;
			o_ptr->to_a = 0 - (5 + (ARMOUR_CLASS)m_bonus(10, level));
			o_ptr->pval = 0 - (1 + (PARAMETER_VALUE)m_bonus(5, level));
			if (power > 0) power = 0 - power;

			break;
		}
		case SV_RING_DAMAGE:
		{
			o_ptr->to_d = 1 + randint1(5) + (HIT_POINT)m_bonus(16, level);
			if (power < 0)
			{
				o_ptr->ident |= (IDENT_BROKEN);
				o_ptr->curse_flags |= TRC_CURSED;
				o_ptr->to_d = 0 - o_ptr->to_d;
			}

			break;
		}
		case SV_RING_ACCURACY:
		{
			o_ptr->to_h = 1 + randint1(5) + (HIT_PROB)m_bonus(16, level);
			if (power < 0)
			{
				o_ptr->ident |= (IDENT_BROKEN);
				o_ptr->curse_flags |= TRC_CURSED;
				o_ptr->to_h = 0 - o_ptr->to_h;
			}

			break;
		}
		case SV_RING_PROTECTION:
		{
			o_ptr->to_a = 5 + randint1(8) + (ARMOUR_CLASS)m_bonus(10, level);
			if (power < 0)
			{
				o_ptr->ident |= (IDENT_BROKEN);
				o_ptr->curse_flags |= TRC_CURSED;
				o_ptr->to_a = 0 - o_ptr->to_a;
			}

			break;
		}
		case SV_RING_SLAYING:
		{
			o_ptr->to_d = randint1(5) + (HIT_POINT)m_bonus(12, level);
			o_ptr->to_h = randint1(5) + (HIT_PROB)m_bonus(12, level);

			if (power < 0)
			{
				o_ptr->ident |= (IDENT_BROKEN);
				o_ptr->curse_flags |= TRC_CURSED;
				o_ptr->to_h = 0 - o_ptr->to_h;
				o_ptr->to_d = 0 - o_ptr->to_d;
			}

			break;
		}
		case SV_RING_MUSCLE:
		{
			o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(3, level);
			if (one_in_(4)) o_ptr->pval++;

			if (power < 0)
			{
				o_ptr->ident |= (IDENT_BROKEN);
				o_ptr->curse_flags |= TRC_CURSED;
				o_ptr->pval = 0 - o_ptr->pval;
			}

			break;
		}
		case SV_RING_AGGRAVATION:
		{
			o_ptr->ident |= (IDENT_BROKEN);
			o_ptr->curse_flags |= TRC_CURSED;
			if (power > 0) power = 0 - power;
			break;
		}
		}

		/* power > 2 is debug only */
		if ((one_in_(400) && (power > 0) && !object_is_cursed(o_ptr) && (level > 79)) || (power > 2))
		{
			o_ptr->pval = MIN(o_ptr->pval, 4);
			become_random_artifact(owner_ptr, o_ptr, FALSE);
		}
		else if ((power == 2) && one_in_(2))
		{
			while (!o_ptr->name2)
			{
				int tmp = m_bonus(10, level);
				object_kind *k_ptr = &k_info[o_ptr->k_idx];
				switch (randint1(28))
				{
				case 1: case 2:
					o_ptr->name2 = EGO_RING_THROW;
					break;
				case 3: case 4:
					if (have_flag(k_ptr->flags, TR_REGEN)) break;
					o_ptr->name2 = EGO_RING_REGEN;
					break;
				case 5: case 6:
					if (have_flag(k_ptr->flags, TR_LITE_1)) break;
					o_ptr->name2 = EGO_RING_LITE;
					break;
				case 7: case 8:
					if (have_flag(k_ptr->flags, TR_TELEPORT)) break;
					o_ptr->name2 = EGO_RING_TELEPORT;
					break;
				case 9: case 10:
					if (o_ptr->to_h) break;
					o_ptr->name2 = EGO_RING_TO_H;
					break;
				case 11: case 12:
					if (o_ptr->to_d) break;
					o_ptr->name2 = EGO_RING_TO_D;
					break;
				case 13:
					if ((o_ptr->to_h) || (o_ptr->to_d)) break;
					o_ptr->name2 = EGO_RING_SLAY;
					break;
				case 14:
					if ((have_flag(k_ptr->flags, TR_STR)) || o_ptr->to_h || o_ptr->to_d) break;
					o_ptr->name2 = EGO_RING_WIZARD;
					break;
				case 15:
					if (have_flag(k_ptr->flags, TR_ACTIVATE)) break;
					o_ptr->name2 = EGO_RING_HERO;
					break;
				case 16:
					if (have_flag(k_ptr->flags, TR_ACTIVATE)) break;
					if (tmp > 8) o_ptr->name2 = EGO_RING_MANA_BALL;
					else if (tmp > 4) o_ptr->name2 = EGO_RING_MANA_BOLT;
					else o_ptr->name2 = EGO_RING_MAGIC_MIS;
					break;
				case 17:
					if (have_flag(k_ptr->flags, TR_ACTIVATE)) break;
					if (!(have_flag(k_ptr->flags, TR_RES_FIRE)) && (have_flag(k_ptr->flags, TR_RES_COLD) || have_flag(k_ptr->flags, TR_RES_ELEC) || have_flag(k_ptr->flags, TR_RES_ACID))) break;
					if (tmp > 7) o_ptr->name2 = EGO_RING_DRAGON_F;
					else if (tmp > 3) o_ptr->name2 = EGO_RING_FIRE_BALL;
					else o_ptr->name2 = EGO_RING_FIRE_BOLT;
					break;
				case 18:
					if (have_flag(k_ptr->flags, TR_ACTIVATE)) break;
					if (!(have_flag(k_ptr->flags, TR_RES_COLD)) && (have_flag(k_ptr->flags, TR_RES_FIRE) || have_flag(k_ptr->flags, TR_RES_ELEC) || have_flag(k_ptr->flags, TR_RES_ACID))) break;
					if (tmp > 7) o_ptr->name2 = EGO_RING_DRAGON_C;
					else if (tmp > 3) o_ptr->name2 = EGO_RING_COLD_BALL;
					else o_ptr->name2 = EGO_RING_COLD_BOLT;
					break;
				case 19:
					if (have_flag(k_ptr->flags, TR_ACTIVATE)) break;
					if (!(have_flag(k_ptr->flags, TR_RES_ELEC)) && (have_flag(k_ptr->flags, TR_RES_COLD) || have_flag(k_ptr->flags, TR_RES_FIRE) || have_flag(k_ptr->flags, TR_RES_ACID))) break;
					if (tmp > 4) o_ptr->name2 = EGO_RING_ELEC_BALL;
					else o_ptr->name2 = EGO_RING_ELEC_BOLT;
					break;
				case 20:
					if (have_flag(k_ptr->flags, TR_ACTIVATE)) break;
					if (!(have_flag(k_ptr->flags, TR_RES_ACID)) && (have_flag(k_ptr->flags, TR_RES_COLD) || have_flag(k_ptr->flags, TR_RES_ELEC) || have_flag(k_ptr->flags, TR_RES_FIRE))) break;
					if (tmp > 4) o_ptr->name2 = EGO_RING_ACID_BALL;
					else o_ptr->name2 = EGO_RING_ACID_BOLT;
					break;
				case 21: case 22: case 23: case 24: case 25: case 26:
					switch (o_ptr->sval)
					{
					case SV_RING_SPEED:
						if (!one_in_(3)) break;
						o_ptr->name2 = EGO_RING_D_SPEED;
						break;
					case SV_RING_DAMAGE:
					case SV_RING_ACCURACY:
					case SV_RING_SLAYING:
						if (one_in_(2)) break;
						if (one_in_(2)) o_ptr->name2 = EGO_RING_HERO;
						else
						{
							o_ptr->name2 = EGO_RING_BERSERKER;
							o_ptr->to_h -= 2 + randint1(4);
							o_ptr->to_d += 2 + randint1(4);
						}

						break;
					case SV_RING_PROTECTION:
						o_ptr->name2 = EGO_RING_SUPER_AC;
						o_ptr->to_a += 7 + m_bonus(5, level);
						break;
					case SV_RING_RES_FEAR:
						o_ptr->name2 = EGO_RING_HERO;
						break;
					case SV_RING_SHOTS:
						if (one_in_(2)) break;
						o_ptr->name2 = EGO_RING_HUNTER;
						break;
					case SV_RING_SEARCHING:
						o_ptr->name2 = EGO_RING_STEALTH;
						break;
					case SV_RING_TELEPORTATION:
						o_ptr->name2 = EGO_RING_TELE_AWAY;
						break;
					case SV_RING_RES_BLINDNESS:
						if (one_in_(2))
							o_ptr->name2 = EGO_RING_RES_LITE;
						else
							o_ptr->name2 = EGO_RING_RES_DARK;
						break;
					case SV_RING_LORDLY:
						if (!one_in_(20)) break;
						one_lordly_high_resistance(o_ptr);
						one_lordly_high_resistance(o_ptr);
						o_ptr->name2 = EGO_RING_TRUE;
						break;
					case SV_RING_SUSTAIN:
						if (!one_in_(4)) break;
						o_ptr->name2 = EGO_RING_RES_TIME;
						break;
					case SV_RING_FLAMES:
						if (!one_in_(2)) break;
						o_ptr->name2 = EGO_RING_DRAGON_F;
						break;
					case SV_RING_ICE:
						if (!one_in_(2)) break;
						o_ptr->name2 = EGO_RING_DRAGON_C;
						break;
					case SV_RING_WARNING:
						if (!one_in_(2)) break;
						o_ptr->name2 = EGO_RING_M_DETECT;
						break;
					default:
						break;
					}

					break;
				}
			}

			o_ptr->curse_flags = 0L;
		}
		else if ((power == -2) && one_in_(2))
		{
			if (o_ptr->to_h > 0) o_ptr->to_h = 0 - o_ptr->to_h;
			if (o_ptr->to_d > 0) o_ptr->to_d = 0 - o_ptr->to_d;
			if (o_ptr->to_a > 0) o_ptr->to_a = 0 - o_ptr->to_a;
			if (o_ptr->pval > 0) o_ptr->pval = 0 - o_ptr->pval;
			o_ptr->art_flags[0] = 0;
			o_ptr->art_flags[1] = 0;
			while (!o_ptr->name2)
			{
				object_kind *k_ptr = &k_info[o_ptr->k_idx];
				switch (randint1(5))
				{
				case 1:
					if (have_flag(k_ptr->flags, TR_DRAIN_EXP)) break;
					o_ptr->name2 = EGO_RING_DRAIN_EXP;
					break;
				case 2:
					o_ptr->name2 = EGO_RING_NO_MELEE;
					break;
				case 3:
					if (have_flag(k_ptr->flags, TR_AGGRAVATE)) break;
					o_ptr->name2 = EGO_RING_AGGRAVATE;
					break;
				case 4:
					if (have_flag(k_ptr->flags, TR_TY_CURSE)) break;
					o_ptr->name2 = EGO_RING_TY_CURSE;
					break;
				case 5:
					o_ptr->name2 = EGO_RING_ALBINO;
					break;
				}
			}

			o_ptr->ident |= (IDENT_BROKEN);
			o_ptr->curse_flags |= (TRC_CURSED | TRC_HEAVY_CURSE);
		}

		break;
	}
	case TV_AMULET:
	{
		switch (o_ptr->sval)
		{
		case SV_AMULET_INTELLIGENCE:
		case SV_AMULET_WISDOM:
		case SV_AMULET_CHARISMA:
		{
			o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(5, level);
			if (power < 0)
			{
				o_ptr->ident |= (IDENT_BROKEN);
				o_ptr->curse_flags |= (TRC_CURSED);
				o_ptr->pval = 0 - o_ptr->pval;
			}

			break;
		}
		case SV_AMULET_BRILLIANCE:
		{
			o_ptr->pval = 1 + m_bonus(3, level);
			if (one_in_(4)) o_ptr->pval++;

			if (power < 0)
			{
				o_ptr->ident |= (IDENT_BROKEN);
				o_ptr->curse_flags |= (TRC_CURSED);
				o_ptr->pval = 0 - o_ptr->pval;
			}

			break;
		}
		case SV_AMULET_NO_MAGIC: case SV_AMULET_NO_TELE:
		{
			if (power < 0)
			{
				o_ptr->curse_flags |= (TRC_CURSED);
			}

			break;
		}
		case SV_AMULET_RESISTANCE:
		{
			if (one_in_(5)) one_high_resistance(o_ptr);
			if (one_in_(5)) add_flag(o_ptr->art_flags, TR_RES_POIS);
			break;
		}
		case SV_AMULET_SEARCHING:
		{
			o_ptr->pval = randint1(2) + (PARAMETER_VALUE)m_bonus(4, level);
			if (power < 0)
			{
				o_ptr->ident |= (IDENT_BROKEN);
				o_ptr->curse_flags |= (TRC_CURSED);
				o_ptr->pval = 0 - (o_ptr->pval);
			}

			break;
		}
		case SV_AMULET_THE_MAGI:
		{
			o_ptr->pval = randint1(5) + (PARAMETER_VALUE)m_bonus(5, level);
			o_ptr->to_a = randint1(5) + (ARMOUR_CLASS)m_bonus(5, level);
			add_esp_weak(o_ptr, FALSE);
			break;
		}
		case SV_AMULET_DOOM:
		{
			o_ptr->ident |= (IDENT_BROKEN);
			o_ptr->curse_flags |= (TRC_CURSED);
			o_ptr->pval = 0 - (randint1(5) + (PARAMETER_VALUE)m_bonus(5, level));
			o_ptr->to_a = 0 - (randint1(5) + (ARMOUR_CLASS)m_bonus(5, level));
			if (power > 0) power = 0 - power;

			break;
		}
		case SV_AMULET_MAGIC_MASTERY:
		{
			o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(4, level);
			if (power < 0)
			{
				o_ptr->ident |= (IDENT_BROKEN);
				o_ptr->curse_flags |= (TRC_CURSED);
				o_ptr->pval = 0 - o_ptr->pval;
			}

			break;
		}
		}

		/* power > 2 is debug only */
		if ((one_in_(150) && (power > 0) && !object_is_cursed(o_ptr) && (level > 79)) || (power > 2))
		{
			o_ptr->pval = MIN(o_ptr->pval, 4);
			become_random_artifact(owner_ptr, o_ptr, FALSE);
		}
		else if ((power == 2) && one_in_(2))
		{
			while (!o_ptr->name2)
			{
				object_kind *k_ptr = &k_info[o_ptr->k_idx];
				switch (randint1(21))
				{
				case 1: case 2:
					if (have_flag(k_ptr->flags, TR_SLOW_DIGEST)) break;
					o_ptr->name2 = EGO_AMU_SLOW_D;
					break;
				case 3: case 4:
					if (o_ptr->pval) break;
					o_ptr->name2 = EGO_AMU_INFRA;
					break;
				case 5: case 6:
					if (have_flag(k_ptr->flags, TR_SEE_INVIS)) break;
					o_ptr->name2 = EGO_AMU_SEE_INVIS;
					break;
				case 7: case 8:
					if (have_flag(k_ptr->flags, TR_HOLD_EXP)) break;
					o_ptr->name2 = EGO_AMU_HOLD_EXP;
					break;
				case 9:
					if (have_flag(k_ptr->flags, TR_LEVITATION)) break;
					o_ptr->name2 = EGO_AMU_LEVITATION;
					break;
				case 10: case 11: case 21:
					o_ptr->name2 = EGO_AMU_AC;
					break;
				case 12:
					if (have_flag(k_ptr->flags, TR_RES_FIRE)) break;
					if (m_bonus(10, level) > 8)
						o_ptr->name2 = EGO_AMU_RES_FIRE_;
					else
						o_ptr->name2 = EGO_AMU_RES_FIRE;
					break;
				case 13:
					if (have_flag(k_ptr->flags, TR_RES_COLD)) break;
					if (m_bonus(10, level) > 8)
						o_ptr->name2 = EGO_AMU_RES_COLD_;
					else
						o_ptr->name2 = EGO_AMU_RES_COLD;
					break;
				case 14:
					if (have_flag(k_ptr->flags, TR_RES_ELEC)) break;
					if (m_bonus(10, level) > 8)
						o_ptr->name2 = EGO_AMU_RES_ELEC_;
					else
						o_ptr->name2 = EGO_AMU_RES_ELEC;
					break;
				case 15:
					if (have_flag(k_ptr->flags, TR_RES_ACID)) break;
					if (m_bonus(10, level) > 8)
						o_ptr->name2 = EGO_AMU_RES_ACID_;
					else
						o_ptr->name2 = EGO_AMU_RES_ACID;
					break;
				case 16: case 17: case 18: case 19: case 20:
					switch (o_ptr->sval)
					{
					case SV_AMULET_TELEPORT:
						if (m_bonus(10, level) > 9) o_ptr->name2 = EGO_AMU_D_DOOR;
						else if (one_in_(2)) o_ptr->name2 = EGO_AMU_JUMP;
						else o_ptr->name2 = EGO_AMU_TELEPORT;
						break;
					case SV_AMULET_RESIST_ACID:
						if ((m_bonus(10, level) > 6) && one_in_(2)) o_ptr->name2 = EGO_AMU_RES_ACID_;
						break;
					case SV_AMULET_SEARCHING:
						o_ptr->name2 = EGO_AMU_STEALTH;
						break;
					case SV_AMULET_BRILLIANCE:
						if (!one_in_(3)) break;
						o_ptr->name2 = EGO_AMU_IDENT;
						break;
					case SV_AMULET_CHARISMA:
						if (!one_in_(3)) break;
						o_ptr->name2 = EGO_AMU_CHARM;
						break;
					case SV_AMULET_THE_MAGI:
						if (one_in_(2)) break;
						o_ptr->name2 = EGO_AMU_GREAT;
						break;
					case SV_AMULET_RESISTANCE:
						if (!one_in_(5)) break;
						o_ptr->name2 = EGO_AMU_DEFENDER;
						break;
					case SV_AMULET_TELEPATHY:
						if (!one_in_(3)) break;
						o_ptr->name2 = EGO_AMU_DETECTION;
						break;
					}
				}
			}
			o_ptr->curse_flags = 0L;
		}
		else if ((power == -2) && one_in_(2))
		{
			if (o_ptr->to_h > 0) o_ptr->to_h = 0 - o_ptr->to_h;
			if (o_ptr->to_d > 0) o_ptr->to_d = 0 - o_ptr->to_d;
			if (o_ptr->to_a > 0) o_ptr->to_a = 0 - o_ptr->to_a;
			if (o_ptr->pval > 0) o_ptr->pval = 0 - o_ptr->pval;
			o_ptr->art_flags[0] = 0;
			o_ptr->art_flags[1] = 0;
			while (!o_ptr->name2)
			{
				object_kind *k_ptr = &k_info[o_ptr->k_idx];
				switch (randint1(5))
				{
				case 1:
					if (have_flag(k_ptr->flags, TR_DRAIN_EXP)) break;
					o_ptr->name2 = EGO_AMU_DRAIN_EXP;
					break;
				case 2:
					o_ptr->name2 = EGO_AMU_FOOL;
					break;
				case 3:
					if (have_flag(k_ptr->flags, TR_AGGRAVATE)) break;
					o_ptr->name2 = EGO_AMU_AGGRAVATE;
					break;
				case 4:
					if (have_flag(k_ptr->flags, TR_TY_CURSE)) break;
					o_ptr->name2 = EGO_AMU_TY_CURSE;
					break;
				case 5:
					o_ptr->name2 = EGO_AMU_NAIVETY;
					break;
				}
			}

			o_ptr->ident |= (IDENT_BROKEN);
			o_ptr->curse_flags |= (TRC_CURSED | TRC_HEAVY_CURSE);
		}

		break;
	}
	}
}


/*!
 * @brief その他雑多のオブジェクトに生成ランクごとの強化を与えるサブルーチン
 * Apply magic to an item known to be "boring"
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param power 生成ランク
 * @return なし
 * @details
 * Hack -- note the special code for various items
 */
static void a_m_aux_4(player_type *owner_ptr, object_type *o_ptr, int power)
{
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	floor_type *floor_ptr = owner_ptr->current_floor_ptr;
	switch (o_ptr->tval)
	{
	case TV_WHISTLE:
	{
		break;
	}
	case TV_FLASK:
	{
		o_ptr->xtra4 = o_ptr->pval;
		o_ptr->pval = 0;
		break;
	}
	case TV_LITE:
	{
		if (o_ptr->sval == SV_LITE_TORCH)
		{
			if (o_ptr->pval > 0) o_ptr->xtra4 = randint1(o_ptr->pval);
			o_ptr->pval = 0;
		}

		if (o_ptr->sval == SV_LITE_LANTERN)
		{
			if (o_ptr->pval > 0) o_ptr->xtra4 = randint1(o_ptr->pval);
			o_ptr->pval = 0;
		}

		/* power > 2 is debug only */
		if (power > 2)
		{
			become_random_artifact(owner_ptr, o_ptr, FALSE);
		}
		else if ((power == 2) || ((power == 1) && one_in_(3)))
		{
			while (!o_ptr->name2)
			{
				while (TRUE)
				{
					bool okay_flag = TRUE;

					o_ptr->name2 = get_random_ego(INVEN_LITE, TRUE);

					switch (o_ptr->name2)
					{
					case EGO_LITE_LONG:
						if (o_ptr->sval == SV_LITE_FEANOR)
							okay_flag = FALSE;
					}

					if (okay_flag)
						break;
				}
			}
		}
		else if (power == -2)
		{
			o_ptr->name2 = get_random_ego(INVEN_LITE, FALSE);
			switch (o_ptr->name2)
			{
			case EGO_LITE_DARKNESS:
				o_ptr->xtra4 = 0;

				if (o_ptr->sval == SV_LITE_TORCH)
				{
					add_flag(o_ptr->art_flags, TR_LITE_M1);
				}
				else if (o_ptr->sval == SV_LITE_LANTERN)
				{
					add_flag(o_ptr->art_flags, TR_LITE_M2);
				}
				else if (o_ptr->sval == SV_LITE_FEANOR)
				{
					add_flag(o_ptr->art_flags, TR_LITE_M3);
				}
				break;
			}
		}

		break;
	}
	case TV_WAND:
	case TV_STAFF:
	{
		/* The wand or staff gets a number of initial charges equal
		 * to between 1/2 (+1) and the full object kind's pval. -LM-
		 */
		o_ptr->pval = k_ptr->pval / 2 + randint1((k_ptr->pval + 1) / 2);
		break;
	}
	case TV_ROD:
	{
		o_ptr->pval = k_ptr->pval;
		break;
	}
	case TV_CAPTURE:
	{
		o_ptr->pval = 0;
		object_aware(owner_ptr, o_ptr);
		object_known(o_ptr);
		break;
	}
	case TV_FIGURINE:
	{
		PARAMETER_VALUE i = 1;
		int check;
		monster_race *r_ptr;
		while (TRUE)
		{
			i = randint1(max_r_idx - 1);

			if (!item_monster_okay(i)) continue;
			if (i == MON_TSUCHINOKO) continue;

			r_ptr = &r_info[i];
			check = (floor_ptr->dun_level < r_ptr->level) ? (r_ptr->level - floor_ptr->dun_level) : 0;
			if (!r_ptr->rarity) continue;
			if (r_ptr->rarity > 100) continue;
			if (randint0(check)) continue;

			break;
		}

		o_ptr->pval = i;
		if (one_in_(6)) o_ptr->curse_flags |= TRC_CURSED;

		break;
	}
	case TV_CORPSE:
	{
		PARAMETER_VALUE i = 1;
		int check;
		u32b match = 0;
		monster_race *r_ptr;
		if (o_ptr->sval == SV_SKELETON)
		{
			match = RF9_DROP_SKELETON;
		}
		else if (o_ptr->sval == SV_CORPSE)
		{
			match = RF9_DROP_CORPSE;
		}

		get_mon_num_prep(owner_ptr, item_monster_okay, NULL);
		while (TRUE)
		{
			i = get_mon_num(owner_ptr, floor_ptr->dun_level, 0);
			r_ptr = &r_info[i];
			check = (floor_ptr->dun_level < r_ptr->level) ? (r_ptr->level - floor_ptr->dun_level) : 0;
			if (!r_ptr->rarity) continue;
			if (!(r_ptr->flags9 & match)) continue;
			if (randint0(check)) continue;

			break;
		}

		o_ptr->pval = i;
		object_aware(owner_ptr, o_ptr);
		object_known(o_ptr);
		break;
	}
	case TV_STATUE:
	{
		PARAMETER_VALUE i = 1;
		monster_race *r_ptr;
		while (TRUE)
		{
			i = randint1(max_r_idx - 1);
			r_ptr = &r_info[i];
			if (!r_ptr->rarity) continue;

			break;
		}

		o_ptr->pval = i;
		if (cheat_peek)
		{
			msg_format(_("%sの像", "Statue of %s"), r_name + r_ptr->name);
		}

		object_aware(owner_ptr, o_ptr);
		object_known(o_ptr);
		break;
	}
	case TV_CHEST:
	{
		DEPTH obj_level = k_info[o_ptr->k_idx].level;
		if (obj_level <= 0) break;

		o_ptr->pval = randint1(obj_level);
		if (o_ptr->sval == SV_CHEST_KANDUME) o_ptr->pval = 6;

		o_ptr->xtra3 = floor_ptr->dun_level + 5;
		if (o_ptr->pval > 55) o_ptr->pval = 55 + (byte)randint0(5);

		break;
	}
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
			a_m_aux_2(owner_ptr, o_ptr, lev, power);

		break;
	}
	case TV_RING:
	case TV_AMULET:
	{
		if (!power && (randint0(100) < 50)) power = -1;
		a_m_aux_3(owner_ptr, o_ptr, lev, power);
		break;
	}
	default:
	{
		a_m_aux_4(owner_ptr, o_ptr, power);
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
 * @brief 生成済のオブジェクトをフロアの所定の位置に落とす。
 * Let an object fall to the ground at or near a location.
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param j_ptr 落としたいオブジェクト構造体の参照ポインタ
 * @param chance ドロップの消滅率(%)
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 * @return 生成に成功したらオブジェクトのIDを返す。
 * @details
 * The initial location is assumed to be "in_bounds(floor_ptr, )".\n
 *\n
 * This function takes a parameter "chance".  This is the percentage\n
 * chance that the item will "disappear" instead of drop.  If the object\n
 * has been thrown, then this is the chance of disappearance on contact.\n
 *\n
 * Hack -- this function uses "chance" to determine if it should produce\n
 * some form of "description" of the drop event (under the player).\n
 *\n
 * We check several locations to see if we can find a location at which\n
 * the object can combine, stack, or be placed.  Artifacts will try very\n
 * hard to be placed, including "teleporting" to a useful grid if needed.\n
 */
OBJECT_IDX drop_near(player_type *owner_ptr, object_type *j_ptr, PERCENTAGE chance, POSITION y, POSITION x)
{
	int i, k, d, s;
	POSITION dy, dx;
	POSITION ty, tx = 0;
	OBJECT_IDX o_idx = 0;
	OBJECT_IDX this_o_idx, next_o_idx = 0;
	grid_type *g_ptr;
	GAME_TEXT o_name[MAX_NLEN];
	bool flag = FALSE;
	bool done = FALSE;
#ifdef JP
#else
	bool plural = (j_ptr->number != 1);
#endif
	object_desc(owner_ptr, o_name, j_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
	if (!object_is_artifact(j_ptr) && (randint0(100) < chance))
	{
#ifdef JP
		msg_format("%sは消えた。", o_name);
#else
		msg_format("The %s disappear%s.", o_name, (plural ? "" : "s"));
#endif
		if (current_world_ptr->wizard) msg_print(_("(破損)", "(breakage)"));

		return 0;
	}

	int bs = -1;
	int bn = 0;

	POSITION by = y;
	POSITION bx = x;
	floor_type *floor_ptr = owner_ptr->current_floor_ptr;
	for (dy = -3; dy <= 3; dy++)
	{
		for (dx = -3; dx <= 3; dx++)
		{
			bool comb = FALSE;
			d = (dy * dy) + (dx * dx);
			if (d > 10) continue;

			ty = y + dy;
			tx = x + dx;
			if (!in_bounds(floor_ptr, ty, tx)) continue;
			if (!projectable(owner_ptr, y, x, ty, tx)) continue;

			g_ptr = &floor_ptr->grid_array[ty][tx];
			if (!cave_drop_bold(floor_ptr, ty, tx)) continue;

			k = 0;
			for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
			{
				object_type *o_ptr;
				o_ptr = &floor_ptr->o_list[this_o_idx];
				next_o_idx = o_ptr->next_o_idx;
				if (object_similar(o_ptr, j_ptr)) comb = TRUE;

				k++;
			}

			if (!comb) k++;
			if (k > 99) continue;

			s = 1000 - (d + k * 5);
			if (s < bs) continue;

			if (s > bs) bn = 0;

			if ((++bn >= 2) && !one_in_(bn)) continue;

			bs = s;
			by = ty;
			bx = tx;

			flag = TRUE;
		}
	}

	if (!flag && !object_is_artifact(j_ptr))
	{
#ifdef JP
		msg_format("%sは消えた。", o_name);
#else
		msg_format("The %s disappear%s.", o_name, (plural ? "" : "s"));
#endif
		if (current_world_ptr->wizard) msg_print(_("(床スペースがない)", "(no floor space)"));

		return 0;
	}

	for (i = 0; !flag && (i < 1000); i++)
	{
		ty = rand_spread(by, 1);
		tx = rand_spread(bx, 1);

		if (!in_bounds(floor_ptr, ty, tx)) continue;

		by = ty;
		bx = tx;

		if (!cave_drop_bold(floor_ptr, by, bx)) continue;

		flag = TRUE;
	}

	if (!flag)
	{
		int candidates = 0, pick;
		for (ty = 1; ty < floor_ptr->height - 1; ty++)
		{
			for (tx = 1; tx < floor_ptr->width - 1; tx++)
			{
				if (cave_drop_bold(floor_ptr, ty, tx)) candidates++;
			}
		}

		if (!candidates)
		{
#ifdef JP
			msg_format("%sは消えた。", o_name);
#else
			msg_format("The %s disappear%s.", o_name, (plural ? "" : "s"));
#endif

			if (current_world_ptr->wizard) msg_print(_("(床スペースがない)", "(no floor space)"));

			if (preserve_mode)
			{
				if (object_is_fixed_artifact(j_ptr) && !object_is_known(j_ptr))
				{
					a_info[j_ptr->name1].cur_num = 0;
				}
			}

			return 0;
		}

		pick = randint1(candidates);
		for (ty = 1; ty < floor_ptr->height - 1; ty++)
		{
			for (tx = 1; tx < floor_ptr->width - 1; tx++)
			{
				if (cave_drop_bold(floor_ptr, ty, tx))
				{
					pick--;
					if (!pick) break;
				}
			}

			if (!pick) break;
		}

		by = ty;
		bx = tx;
	}


	g_ptr = &floor_ptr->grid_array[by][bx];
	for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;
		o_ptr = &floor_ptr->o_list[this_o_idx];
		next_o_idx = o_ptr->next_o_idx;
		if (object_similar(o_ptr, j_ptr))
		{
			object_absorb(o_ptr, j_ptr);
			done = TRUE;
			break;
		}
	}

	if (!done) o_idx = o_pop(floor_ptr);

	if (!done && !o_idx)
	{
#ifdef JP
		msg_format("%sは消えた。", o_name);
#else
		msg_format("The %s disappear%s.", o_name, (plural ? "" : "s"));
#endif
		if (current_world_ptr->wizard) msg_print(_("(アイテムが多過ぎる)", "(too many objects)"));

		if (object_is_fixed_artifact(j_ptr))
		{
			a_info[j_ptr->name1].cur_num = 0;
		}

		return 0;
	}

	if (!done)
	{
		object_copy(&floor_ptr->o_list[o_idx], j_ptr);
		j_ptr = &floor_ptr->o_list[o_idx];
		j_ptr->iy = by;
		j_ptr->ix = bx;
		j_ptr->held_m_idx = 0;
		j_ptr->next_o_idx = g_ptr->o_idx;

		g_ptr->o_idx = o_idx;
		done = TRUE;
	}

	note_spot(owner_ptr, by, bx);
	lite_spot(owner_ptr, by, bx);
	sound(SOUND_DROP);

	if (chance && player_bold(owner_ptr, by, bx))
	{
		msg_print(_("何かが足下に転がってきた。", "You feel something roll beneath your feet."));
	}

	return o_idx;
}


/*!
 * @brief 床上の魔道具の残り残量メッセージを表示する /
 * Describe the charges on an item on the floor.
 * @param floo_ptr 現在フロアへの参照ポインタ
 * @param item メッセージの対象にしたいアイテム所持スロット
 * @return なし
 */
void floor_item_charges(floor_type *floor_ptr, INVENTORY_IDX item)
{
	object_type *o_ptr = &floor_ptr->o_list[item];
	if ((o_ptr->tval != TV_STAFF) && (o_ptr->tval != TV_WAND)) return;
	if (!object_is_known(o_ptr)) return;

#ifdef JP
	if (o_ptr->pval <= 0)
	{
		msg_print("この床上のアイテムは、もう魔力が残っていない。");
	}
	else
	{
		msg_format("この床上のアイテムは、あと %d 回分の魔力が残っている。", o_ptr->pval);
	}
#else
	if (o_ptr->pval != 1)
	{
		msg_format("There are %d charges remaining.", o_ptr->pval);
	}
	else
	{
		msg_format("There is %d charge remaining.", o_ptr->pval);
	}
#endif
}


/*!
 * @brief 床上のアイテムの残り数メッセージを表示する /
 * Describe the charges on an item on the floor.
 * @param floo_ptr 現在フロアへの参照ポインタ
 * @param item メッセージの対象にしたいアイテム所持スロット
 * @return なし
 */
void floor_item_describe(player_type *owner_ptr, INVENTORY_IDX item)
{
	object_type *o_ptr = &owner_ptr->current_floor_ptr->o_list[item];
	GAME_TEXT o_name[MAX_NLEN];
	object_desc(owner_ptr, o_name, o_ptr, 0);
#ifdef JP
	if (o_ptr->number <= 0)
	{
		msg_format("床上には、もう%sはない。", o_name);
	}
	else
	{
		msg_format("床上には、まだ %sがある。", o_name);
	}
#else
	msg_format("You see %s.", o_name);
#endif
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
