/*!
 * @brief 店舗処理関係のユーティリティ
 * @date 2020/03/20
 * @author Hourier
 */

#include "store/store-util.h"
#include "object/item-apply-magic.h"
#include "object/item-feeling.h"
#include "object/object-hook.h"
#include "object/object-kind.h"
#include "object/object2.h"
#include "object/special-object-flags.h"
#include "object/sv-lite-types.h"
#include "object/sv-potion-types.h"
#include "object/sv-scroll-types.h"
#include "object/sv-weapon-types.h"
#include "object/tr-types.h"

int cur_store_num = 0;
store_type *st_ptr = NULL;

/*!
 * @brief 店舗のオブジェクト数を増やす /
 * Add the item "o_ptr" to a real stores inventory.
 * @param item 増やしたいアイテムのID
 * @param num 増やしたい数
 * @return なし
 * @details
 * <pre>
 * Increase, by a given amount, the number of a certain item
 * in a certain store.	This can result in zero items.
 * </pre>
 * @todo numは本来ITEM_NUMBER型にしたい。
 */
void store_item_increase(INVENTORY_IDX item, int num)
{
	object_type *o_ptr;
	o_ptr = &st_ptr->stock[item];
	int cnt = o_ptr->number + num;
	if (cnt > 255) cnt = 255;
	else if (cnt < 0) cnt = 0;

	num = cnt - o_ptr->number;
	o_ptr->number += (ITEM_NUMBER)num;
}


/*!
 * @brief 店舗のオブジェクト数を削除する /
 * Remove a slot if it is empty
 * @param item 削除したいアイテムのID
 * @return なし
 */
void store_item_optimize(INVENTORY_IDX item)
{
	object_type *o_ptr;
	o_ptr = &st_ptr->stock[item];
	if (!o_ptr->k_idx) return;
	if (o_ptr->number) return;

	st_ptr->stock_num--;
	for (int j = item; j < st_ptr->stock_num; j++)
	{
		st_ptr->stock[j] = st_ptr->stock[j + 1];
	}

	object_wipe(&st_ptr->stock[st_ptr->stock_num]);
}


/*!
 * @brief 店舗の品揃え変化のためにアイテムを削除する /
 * Attempt to delete (some of) a random item from the store
 * @return なし
 * @details
 * <pre>
 * Hack -- we attempt to "maintain" piles of items when possible.
 * </pre>
 */
void store_delete(void)
{
	INVENTORY_IDX what = (INVENTORY_IDX)randint0(st_ptr->stock_num);
	int num = st_ptr->stock[what].number;
	if (randint0(100) < 50) num = (num + 1) / 2;
	if (randint0(100) < 50) num = 1;
	if ((st_ptr->stock[what].tval == TV_ROD) || (st_ptr->stock[what].tval == TV_WAND))
	{
		st_ptr->stock[what].pval -= num * st_ptr->stock[what].pval / st_ptr->stock[what].number;
	}

	store_item_increase(what, -num);
	store_item_optimize(what);
}


/*!
 * @brief 安価な消耗品の販売数を増やし、低確率で割引にする /
 * Certain "cheap" objects should be created in "piles"
 * @param o_ptr 店舗に並べるオブジェクト構造体の参照ポインタ
 * @return なし
 * @details
 * <pre>
 * Some objects can be sold at a "discount" (in small piles)
 * </pre>
 */
static void mass_produce(object_type *o_ptr)
{
	int size = 1;
	PRICE cost = object_value(o_ptr);
	switch (o_ptr->tval)
	{
	case TV_FOOD:
	case TV_FLASK:
	case TV_LITE:
	{
		if (cost <= 5L) size += damroll(3, 5);
		if (cost <= 20L) size += damroll(3, 5);
		if (cost <= 50L) size += damroll(2, 2);
		break;
	}
	case TV_POTION:
	case TV_SCROLL:
	{
		if (cost <= 60L) size += damroll(3, 5);
		if (cost <= 240L) size += damroll(1, 5);
		if (o_ptr->sval == SV_SCROLL_STAR_IDENTIFY) size += damroll(3, 5);
		if (o_ptr->sval == SV_SCROLL_STAR_REMOVE_CURSE) size += damroll(1, 4);
		break;
	}
	case TV_LIFE_BOOK:
	case TV_SORCERY_BOOK:
	case TV_NATURE_BOOK:
	case TV_CHAOS_BOOK:
	case TV_DEATH_BOOK:
	case TV_TRUMP_BOOK:
	case TV_ARCANE_BOOK:
	case TV_CRAFT_BOOK:
	case TV_DAEMON_BOOK:
	case TV_CRUSADE_BOOK:
	case TV_MUSIC_BOOK:
	case TV_HISSATSU_BOOK:
	case TV_HEX_BOOK:
	{
		if (cost <= 50L) size += damroll(2, 3);
		if (cost <= 500L) size += damroll(1, 3);
		break;
	}
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_SHIELD:
	case TV_GLOVES:
	case TV_BOOTS:
	case TV_CLOAK:
	case TV_HELM:
	case TV_CROWN:
	case TV_SWORD:
	case TV_POLEARM:
	case TV_HAFTED:
	case TV_DIGGING:
	case TV_BOW:
	{
		if (object_is_artifact(o_ptr)) break;
		if (object_is_ego(o_ptr)) break;
		if (cost <= 10L) size += damroll(3, 5);
		if (cost <= 100L) size += damroll(3, 5);
		break;
	}
	case TV_SPIKE:
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	{
		if (cost <= 5L) size += damroll(5, 5);
		if (cost <= 50L) size += damroll(5, 5);
		if (cost <= 500L) size += damroll(5, 5);
		break;
	}
	case TV_FIGURINE:
	{
		if (cost <= 100L) size += damroll(2, 2);
		if (cost <= 1000L) size += damroll(2, 2);
		break;
	}
	case TV_CAPTURE:
	case TV_STATUE:
	case TV_CARD:
	{
		size = 1;
		break;
	}

	/*
	 * Because many rods (and a few wands and staffs) are useful mainly
	 * in quantity, the Black Market will occasionally have a bunch of
	 * one kind. -LM-
	 */
	case TV_ROD:
	case TV_WAND:
	case TV_STAFF:
	{
		if ((cur_store_num == STORE_BLACK) && one_in_(3))
		{
			if (cost < 1601L) size += damroll(1, 5);
			else if (cost < 3201L) size += damroll(1, 3);
		}
		break;
	}
	}

	DISCOUNT_RATE discount = 0;
	if (cost < 5)
	{
		discount = 0;
	}
	else if (one_in_(25))
	{
		discount = 25;
	}
	else if (one_in_(150))
	{
		discount = 50;
	}
	else if (one_in_(300))
	{
		discount = 75;
	}
	else if (one_in_(500))
	{
		discount = 90;
	}

	if (o_ptr->art_name)
	{
		discount = 0;
	}

	o_ptr->discount = discount;
	o_ptr->number = size - (size * discount / 100);
	if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND))
	{
		o_ptr->pval *= (PARAMETER_VALUE)o_ptr->number;
	}
}


/*!
 * @brief 店舗の品揃え変化のためにアイテムを追加する /
 * Creates a random item and gives it to a store
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * <pre>
 * This algorithm needs to be rethought.  A lot.
 * Currently, "normal" stores use a pre-built array.
 * Note -- the "level" given to "obj_get_num()" is a "favored"
 * level, that is, there is a much higher chance of getting
 * items with a level approaching that of the given level...
 * Should we check for "permission" to have the given item?
 * </pre>
 */
void store_create(player_type *player_ptr, bool (*black_market_crap)(player_type*, object_type*))
{
	if (st_ptr->stock_num >= st_ptr->stock_size) return;

	for (int tries = 0; tries < 4; tries++)
	{
		OBJECT_IDX i;
		DEPTH level;
		if (cur_store_num == STORE_BLACK)
		{
			/* Pick a level for object/magic */
			level = 25 + randint0(25);

			/* Random item (usually of given level) */
			i = get_obj_num(player_ptr, level, 0x00000000);

			/* Handle failure */
			if (i == 0) continue;
		}
		else
		{
			i = st_ptr->table[randint0(st_ptr->table_num)];
			level = rand_range(1, STORE_OBJ_LEVEL);
		}

		object_type forge;
		object_type *q_ptr;
		q_ptr = &forge;
		object_prep(q_ptr, i);
		apply_magic(player_ptr, q_ptr, level, AM_NO_FIXED_ART);
		if (!store_will_buy(q_ptr)) continue;

		if (q_ptr->tval == TV_LITE)
		{
			if (q_ptr->sval == SV_LITE_TORCH) q_ptr->xtra4 = FUEL_TORCH / 2;
			if (q_ptr->sval == SV_LITE_LANTERN) q_ptr->xtra4 = FUEL_LAMP / 2;
		}

		object_known(q_ptr);
		q_ptr->ident |= IDENT_STORE;
		if (q_ptr->tval == TV_CHEST) continue;

		if (cur_store_num == STORE_BLACK)
		{
			if (black_market_crap(player_ptr, q_ptr)) continue;
			if (object_value(q_ptr) < 10) continue;
		}
		else
		{
			if (object_value(q_ptr) <= 0) continue;
		}

		mass_produce(q_ptr);
		(void)store_carry(q_ptr);
		break;
	}
}


/*!
 * @brief オブジェクトが祝福されているかの判定を返す /
 * @param o_ptr 判定したいオブジェクト構造体の参照ポインタ
 * @return アイテムが祝福されたアイテムならばTRUEを返す
 */
static bool is_blessed_item(object_type *o_ptr)
{
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	object_flags(o_ptr, flgs);
	if (have_flag(flgs, TR_BLESSED)) return TRUE;
	else return FALSE;
}


/*!
 * @brief オブジェクトが所定の店舗で引き取れるかどうかを返す /
 * Determine if the current store will purchase the given item
 * @param o_ptr 判定したいオブジェクト構造体の参照ポインタ
 * @return アイテムが買い取れるならばTRUEを返す
 * @note
 * Note that a shop-keeper must refuse to buy "worthless" items
 */
bool store_will_buy(object_type *o_ptr)
{
	if ((cur_store_num == STORE_HOME) || (cur_store_num == STORE_MUSEUM)) return TRUE;
	switch (cur_store_num)
	{
	case STORE_GENERAL:
	{
		switch (o_ptr->tval)
		{
		case TV_POTION:
			if (o_ptr->sval != SV_POTION_WATER) return FALSE;

		case TV_WHISTLE:
		case TV_FOOD:
		case TV_LITE:
		case TV_FLASK:
		case TV_SPIKE:
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_DIGGING:
		case TV_CLOAK:
		case TV_BOTTLE:
		case TV_FIGURINE:
		case TV_STATUE:
		case TV_CAPTURE:
		case TV_CARD:
			break;
		default:
			return FALSE;
		}

		break;
	}
	case STORE_ARMOURY:
	{
		switch (o_ptr->tval)
		{
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CROWN:
		case TV_HELM:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
			break;
		default:
			return FALSE;
		}

		break;
	}
	case STORE_WEAPON:
	{
		switch (o_ptr->tval)
		{
		case TV_SHOT:
		case TV_BOLT:
		case TV_ARROW:
		case TV_BOW:
		case TV_DIGGING:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_HISSATSU_BOOK:
			break;
		case TV_HAFTED:
		{
			if (o_ptr->sval == SV_WIZSTAFF) return FALSE;
		}
		break;
		default:
			return FALSE;
		}

		break;
	}
	case STORE_TEMPLE:
	{
		switch (o_ptr->tval)
		{
		case TV_LIFE_BOOK:
		case TV_CRUSADE_BOOK:
		case TV_SCROLL:
		case TV_POTION:
		case TV_HAFTED:
		{
			break;
		}
		case TV_FIGURINE:
		case TV_STATUE:
		{
			monster_race *r_ptr = &r_info[o_ptr->pval];
			if (!(r_ptr->flags3 & RF3_EVIL))
			{
				if (r_ptr->flags3 & RF3_GOOD) break;
				if (r_ptr->flags3 & RF3_ANIMAL) break;
				if (my_strchr("?!", r_ptr->d_char)) break;
			}
		}
			/* Fall through */
		case TV_POLEARM:
		case TV_SWORD:
		{
			if (is_blessed_item(o_ptr)) break;
		}
			/* Fall through */
		default:
			return FALSE;
		}

		break;
	}
	case STORE_ALCHEMIST:
	{
		switch (o_ptr->tval)
		{
		case TV_SCROLL:
		case TV_POTION:
			break;
		default:
			return FALSE;
		}

		break;
	}
	case STORE_MAGIC:
	{
		switch (o_ptr->tval)
		{
		case TV_SORCERY_BOOK:
		case TV_NATURE_BOOK:
		case TV_CHAOS_BOOK:
		case TV_DEATH_BOOK:
		case TV_TRUMP_BOOK:
		case TV_ARCANE_BOOK:
		case TV_CRAFT_BOOK:
		case TV_DAEMON_BOOK:
		case TV_MUSIC_BOOK:
		case TV_HEX_BOOK:
		case TV_AMULET:
		case TV_RING:
		case TV_STAFF:
		case TV_WAND:
		case TV_ROD:
		case TV_SCROLL:
		case TV_POTION:
		case TV_FIGURINE:
			break;
		case TV_HAFTED:
		{
			if (o_ptr->sval == SV_WIZSTAFF) break;
			else return FALSE;
		}
		default:
			return FALSE;
		}

		break;
	}
	case STORE_BOOK:
	{
		switch (o_ptr->tval)
		{
		case TV_SORCERY_BOOK:
		case TV_NATURE_BOOK:
		case TV_CHAOS_BOOK:
		case TV_DEATH_BOOK:
		case TV_LIFE_BOOK:
		case TV_TRUMP_BOOK:
		case TV_ARCANE_BOOK:
		case TV_CRAFT_BOOK:
		case TV_DAEMON_BOOK:
		case TV_CRUSADE_BOOK:
		case TV_MUSIC_BOOK:
		case TV_HEX_BOOK:
			break;
		default:
			return FALSE;
		}

		break;
	}
	}

	if (object_value(o_ptr) <= 0) return FALSE;
	return TRUE;
}


/*!
 * @brief 店舗に並べた品を同一品であるかどうか判定する /
 * Determine if a store item can "absorb" another item
 * @param o_ptr 判定するオブジェクト構造体の参照ポインタ1
 * @param j_ptr 判定するオブジェクト構造体の参照ポインタ2
 * @return 同一扱いできるならTRUEを返す
 * @details
 * <pre>
 * See "object_similar()" for the same function for the "player"
 * </pre>
 */
bool store_object_similar(object_type *o_ptr, object_type *j_ptr)
{
	if (o_ptr == j_ptr) return 0;
	if (o_ptr->k_idx != j_ptr->k_idx) return 0;
	if ((o_ptr->pval != j_ptr->pval) && (o_ptr->tval != TV_WAND) && (o_ptr->tval != TV_ROD)) return 0;
	if (o_ptr->to_h != j_ptr->to_h) return 0;
	if (o_ptr->to_d != j_ptr->to_d) return 0;
	if (o_ptr->to_a != j_ptr->to_a) return 0;
	if (o_ptr->name2 != j_ptr->name2) return 0;
	if (object_is_artifact(o_ptr) || object_is_artifact(j_ptr)) return 0;
	for (int i = 0; i < TR_FLAG_SIZE; i++)
		if (o_ptr->art_flags[i] != j_ptr->art_flags[i]) return 0;
	if (o_ptr->xtra1 || j_ptr->xtra1) return 0;
	if (o_ptr->timeout || j_ptr->timeout) return 0;
	if (o_ptr->ac != j_ptr->ac)   return 0;
	if (o_ptr->dd != j_ptr->dd)   return 0;
	if (o_ptr->ds != j_ptr->ds)   return 0;
	if (o_ptr->tval == TV_CHEST) return 0;
	if (o_ptr->tval == TV_STATUE) return 0;
	if (o_ptr->tval == TV_CAPTURE) return 0;
	if (o_ptr->discount != j_ptr->discount) return 0;
	return TRUE;
}


/*!
 * @brief 店舗に並べた品を重ね合わせできるかどうか判定する /
 * Allow a store item to absorb another item
 * @param o_ptr 判定するオブジェクト構造体の参照ポインタ1
 * @param j_ptr 判定するオブジェクト構造体の参照ポインタ2
 * @return 重ね合わせできるならTRUEを返す
 * @details
 * <pre>
 * See "object_similar()" for the same function for the "player"
 * </pre>
 */
static void store_object_absorb(object_type *o_ptr, object_type *j_ptr)
{
	int max_num = (o_ptr->tval == TV_ROD) ?
		MIN(99, MAX_SHORT / k_info[o_ptr->k_idx].pval) : 99;
	int total = o_ptr->number + j_ptr->number;
	int diff = (total > max_num) ? total - max_num : 0;
	o_ptr->number = (total > max_num) ? max_num : total;
	if (o_ptr->tval == TV_ROD)
	{
		o_ptr->pval += j_ptr->pval * (j_ptr->number - diff) / j_ptr->number;
	}

	if (o_ptr->tval == TV_WAND)
	{
		o_ptr->pval += j_ptr->pval * (j_ptr->number - diff) / j_ptr->number;
	}
}


/*!
 * @brief 店舗にオブジェクトを加える /
 * Add the item "o_ptr" to a real stores inventory.
 * @param o_ptr 加えたいオブジェクトの構造体参照ポインタ
 * @return 収めた先のID
 * @details
 * <pre>
 * In all cases, return the slot (or -1) where the object was placed
 * Note that this is a hacked up version of "inven_carry()".
 * Also note that it may not correctly "adapt" to "knowledge" bacoming
 * known, the player may have to pick stuff up and drop it again.
 * </pre>
 */
int store_carry(object_type *o_ptr)
{
	PRICE value = object_value(o_ptr);
	if (value <= 0) return -1;
	o_ptr->ident |= IDENT_FULL_KNOWN;
	o_ptr->inscription = 0;
	o_ptr->feeling = FEEL_NONE;
	int slot;
	for (slot = 0; slot < st_ptr->stock_num; slot++)
	{
		object_type *j_ptr;
		j_ptr = &st_ptr->stock[slot];
		if (store_object_similar(j_ptr, o_ptr))
		{
			store_object_absorb(j_ptr, o_ptr);
			return slot;
		}
	}

	if (st_ptr->stock_num >= st_ptr->stock_size) return -1;

	for (slot = 0; slot < st_ptr->stock_num; slot++)
	{
		object_type *j_ptr;
		j_ptr = &st_ptr->stock[slot];
		if (o_ptr->tval > j_ptr->tval) break;
		if (o_ptr->tval < j_ptr->tval) continue;
		if (o_ptr->sval < j_ptr->sval) break;
		if (o_ptr->sval > j_ptr->sval) continue;
		if (o_ptr->tval == TV_ROD)
		{
			if (o_ptr->pval < j_ptr->pval) break;
			if (o_ptr->pval > j_ptr->pval) continue;
		}

		PRICE j_value = object_value(j_ptr);
		if (value > j_value) break;
		if (value < j_value) continue;
	}

	for (int i = st_ptr->stock_num; i > slot; i--)
	{
		st_ptr->stock[i] = st_ptr->stock[i - 1];
	}

	st_ptr->stock_num++;
	st_ptr->stock[slot] = *o_ptr;
	return slot;
}
