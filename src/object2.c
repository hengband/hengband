/*!
 * @file object2.c
 * @brief オブジェクトの実装 / Object code, part 2
 * @date 2014/01/11
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 *\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 */

#include "angband.h"
#include "util.h"
#include "world.h"

#include "object.h"

#include "cmd-dump.h"
#include "cmd-spell.h"
#include "spells.h"
#include "dungeon.h"
#include "floor.h"
#include "grid.h"
#include "objectkind.h"
#include "object-boost.h"
#include "object-ego.h"
#include "object-flavor.h"
#include "object-hook.h"
#include "object-curse.h"
#include "objectkind-hook.h"
#include "artifact.h"
#include "feature.h"
#include "player-status.h"
#include "player-move.h"
#include "player-effects.h"
#include "monster.h"
#include "monsterrace-hook.h"
#include "object-ego.h"

/*!
 * @brief 床上、モンスター所持でスタックされたアイテムを削除しスタックを補完する / Excise a dungeon object from any stacks
 * @param o_idx 削除対象のオブジェクト構造体ポインタ
 * @return なし
 */
void excise_object_idx(OBJECT_IDX o_idx)
{
	object_type *j_ptr;

	OBJECT_IDX this_o_idx, next_o_idx = 0;
	OBJECT_IDX prev_o_idx = 0;

	/* Object */
	j_ptr = &current_floor_ptr->o_list[o_idx];

	if (j_ptr->held_m_idx)
	{
		monster_type *m_ptr;
		m_ptr = &current_floor_ptr->m_list[j_ptr->held_m_idx];

		/* Scan all objects in the grid */
		for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
		{
			object_type *o_ptr;
			o_ptr = &current_floor_ptr->o_list[this_o_idx];
			next_o_idx = o_ptr->next_o_idx;

			if (this_o_idx == o_idx)
			{
				/* No previous */
				if (prev_o_idx == 0)
				{
					/* Remove from list */
					m_ptr->hold_o_idx = next_o_idx;
				}

				/* Real previous */
				else
				{
					object_type *k_ptr;

					/* Previous object */
					k_ptr = &current_floor_ptr->o_list[prev_o_idx];

					/* Remove from list */
					k_ptr->next_o_idx = next_o_idx;
				}

				/* Forget next pointer */
				o_ptr->next_o_idx = 0;

				break;
			}

			/* Save prev_o_idx */
			prev_o_idx = this_o_idx;
		}
	}

	/* Dungeon */
	else
	{
		grid_type *g_ptr;

		POSITION y = j_ptr->iy;
		POSITION x = j_ptr->ix;

		g_ptr = &current_floor_ptr->grid_array[y][x];

		/* Scan all objects in the grid */
		for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
		{
			object_type *o_ptr;
			o_ptr = &current_floor_ptr->o_list[this_o_idx];
			next_o_idx = o_ptr->next_o_idx;

			if (this_o_idx == o_idx)
			{
				/* No previous */
				if (prev_o_idx == 0)
				{
					/* Remove from list */
					g_ptr->o_idx = next_o_idx;
				}

				/* Real previous */
				else
				{
					object_type *k_ptr;

					/* Previous object */
					k_ptr = &current_floor_ptr->o_list[prev_o_idx];

					/* Remove from list */
					k_ptr->next_o_idx = next_o_idx;
				}

				/* Forget next pointer */
				o_ptr->next_o_idx = 0;

				break;
			}

			/* Save prev_o_idx */
			prev_o_idx = this_o_idx;
		}
	}
}

/*!
 * @brief オブジェクトを削除する /
 * Delete a dungeon object
 * @param o_idx 削除対象のオブジェクト構造体ポインタ
 * @return なし
 * @details
 * Handle "stacks" of objects correctly.
 */
void delete_object_idx(OBJECT_IDX o_idx)
{
	object_type *j_ptr;

	/* Excise */
	excise_object_idx(o_idx);

	/* Object */
	j_ptr = &current_floor_ptr->o_list[o_idx];

	/* Dungeon floor */
	if (!(j_ptr->held_m_idx))
	{
		POSITION y, x;
		y = j_ptr->iy;
		x = j_ptr->ix;
		lite_spot(y, x);
	}
	object_wipe(j_ptr);

	/* Count objects */
	current_floor_ptr->o_cnt--;
}


/*!
 * @brief フロアにマスに落ちているオブジェクトを全て削除する / Deletes all objects at given location
 * Delete a dungeon object
 * @param y 削除したフロアマスのY座標
 * @param x 削除したフロアマスのX座標
 * @return なし
 */
void delete_object(POSITION y, POSITION x)
{
	grid_type *g_ptr;
	OBJECT_IDX this_o_idx, next_o_idx = 0;

	/* Refuse "illegal" locations */
	if (!in_bounds(y, x)) return;

	g_ptr = &current_floor_ptr->grid_array[y][x];

	/* Scan all objects in the grid */
	for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;
		o_ptr = &current_floor_ptr->o_list[this_o_idx];
		next_o_idx = o_ptr->next_o_idx;
		object_wipe(o_ptr);

		/* Count objects */
		current_floor_ptr->o_cnt--;
	}

	/* Objects are gone */
	g_ptr->o_idx = 0;

	lite_spot(y, x);
}


/*!
 * @brief グローバルオブジェクト配列に対し指定範囲のオブジェクトを整理してIDの若い順に寄せる /
 * Move an object from index i1 to index i2 in the object list
 * @param i1 整理したい配列の始点
 * @param i2 整理したい配列の終点
 * @return なし
 */
static void compact_objects_aux(OBJECT_IDX i1, OBJECT_IDX i2)
{
	OBJECT_IDX i;
	grid_type *g_ptr;
	object_type *o_ptr;

	/* Do nothing */
	if (i1 == i2) return;

	/* Repair objects */
	for (i = 1; i < current_floor_ptr->o_max; i++)
	{
		o_ptr = &current_floor_ptr->o_list[i];

		/* Skip "dead" objects */
		if (!o_ptr->k_idx) continue;

		/* Repair "next" pointers */
		if (o_ptr->next_o_idx == i1)
		{
			/* Repair */
			o_ptr->next_o_idx = i2;
		}
	}
	o_ptr = &current_floor_ptr->o_list[i1];

	if (o_ptr->held_m_idx)
	{
		monster_type *m_ptr;

		/* Acquire monster */
		m_ptr = &current_floor_ptr->m_list[o_ptr->held_m_idx];

		/* Repair monster */
		if (m_ptr->hold_o_idx == i1)
		{
			/* Repair */
			m_ptr->hold_o_idx = i2;
		}
	}

	/* Dungeon */
	else
	{
		POSITION y, x;

		/* Acquire location */
		y = o_ptr->iy;
		x = o_ptr->ix;

		/* Acquire grid */
		g_ptr = &current_floor_ptr->grid_array[y][x];

		/* Repair grid */
		if (g_ptr->o_idx == i1)
		{
			/* Repair */
			g_ptr->o_idx = i2;
		}
	}

	/* Structure copy */
	current_floor_ptr->o_list[i2] = current_floor_ptr->o_list[i1];

	/* Wipe the hole */
	object_wipe(o_ptr);
}


/*!
 * @brief グローバルオブジェクト配列から優先度の低いものを削除し、データを圧縮する。 /
 * Compact and Reorder the object list.
 * @param size 最低でも減らしたいオブジェクト数の水準
 * @return なし
 * @details
 * （危険なので使用には注意すること）
 * This function can be very dangerous, use with caution!\n
 *\n
 * When actually "compacting" objects, we base the saving throw on a\n
 * combination of object level, distance from player, and current\n
 * "desperation".\n
 *\n
 * After "compacting" (if needed), we "reorder" the objects into a more\n
 * compact order, and we reset the allocation info, and the "live" array.\n
 */
void compact_objects(int size)
{
	OBJECT_IDX i;
	POSITION y, x;
	int num, cnt;
	int cur_lev, cur_dis, chance;
	object_type *o_ptr;

	/* Compact */
	if (size)
	{
		msg_print(_("アイテム情報を圧縮しています...", "Compacting objects..."));
		p_ptr->redraw |= (PR_MAP);
		p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
	}


	/* Compact at least 'size' objects */
	for (num = 0, cnt = 1; num < size; cnt++)
	{
		/* Get more vicious each iteration */
		cur_lev = 5 * cnt;

		/* Get closer each iteration */
		cur_dis = 5 * (20 - cnt);

		/* Examine the objects */
		for (i = 1; i < current_floor_ptr->o_max; i++)
		{
			o_ptr = &current_floor_ptr->o_list[i];

			/* Skip dead objects */
			if (!o_ptr->k_idx) continue;

			/* Hack -- High level objects start out "immune" */
			if (k_info[o_ptr->k_idx].level > cur_lev) continue;

			if (o_ptr->held_m_idx)
			{
				monster_type *m_ptr;

				/* Acquire monster */
				m_ptr = &current_floor_ptr->m_list[o_ptr->held_m_idx];

				y = m_ptr->fy;
				x = m_ptr->fx;

				/* Monsters protect their objects */
				if (randint0(100) < 90) continue;
			}

			/* Dungeon */
			else
			{
				y = o_ptr->iy;
				x = o_ptr->ix;
			}

			/* Nearby objects start out "immune" */
			if ((cur_dis > 0) && (distance(p_ptr->y, p_ptr->x, y, x) < cur_dis)) continue;

			/* Saving throw */
			chance = 90;

			/* Hack -- only compact artifacts in emergencies */
			if ((object_is_fixed_artifact(o_ptr) || o_ptr->art_name) &&
			    (cnt < 1000)) chance = 100;

			/* Apply the saving throw */
			if (randint0(100) < chance) continue;

			delete_object_idx(i);

			/* Count it */
			num++;
		}
	}


	/* Excise dead objects (backwards!) */
	for (i = current_floor_ptr->o_max - 1; i >= 1; i--)
	{
		o_ptr = &current_floor_ptr->o_list[i];

		/* Skip real objects */
		if (o_ptr->k_idx) continue;

		/* Move last object into open hole */
		compact_objects_aux(current_floor_ptr->o_max - 1, i);

		/* Compress "current_floor_ptr->o_max" */
		current_floor_ptr->o_max--;
	}
}


/*!
 * @brief グローバルオブジェクト配列を初期化する /
 * Delete all the items when player leaves the level
 * @note we do NOT visually reflect these (irrelevant) changes
 * @details
 * Hack -- we clear the "g_ptr->o_idx" field for every grid,
 * and the "m_ptr->next_o_idx" field for every monster, since
 * we know we are clearing every object.  Technically, we only
 * clear those fields for grids/monsters containing objects,
 * and we clear it once for every such object.
 * @return なし
 */
void wipe_o_list(void)
{
	int i;

	/* Delete the existing objects */
	for (i = 1; i < current_floor_ptr->o_max; i++)
	{
		object_type *o_ptr = &current_floor_ptr->o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Mega-Hack -- preserve artifacts */
		if (!current_world_ptr->character_dungeon || preserve_mode)
		{
			/* Hack -- Preserve unknown artifacts */
			if (object_is_fixed_artifact(o_ptr) && !object_is_known(o_ptr))
			{
				/* Mega-Hack -- Preserve the artifact */
				a_info[o_ptr->name1].cur_num = 0;
			}
		}

		if (o_ptr->held_m_idx)
		{
			monster_type *m_ptr;
			m_ptr = &current_floor_ptr->m_list[o_ptr->held_m_idx];

			/* Hack -- see above */
			m_ptr->hold_o_idx = 0;
		}

		/* Dungeon */
		else
		{
			grid_type *g_ptr;

			/* Access location */
			POSITION y = o_ptr->iy;
			POSITION x = o_ptr->ix;

			/* Access grid */
			g_ptr = &current_floor_ptr->grid_array[y][x];

			/* Hack -- see above */
			g_ptr->o_idx = 0;
		}
		object_wipe(o_ptr);
	}

	/* Reset "current_floor_ptr->o_max" */
	current_floor_ptr->o_max = 1;

	/* Reset "current_floor_ptr->o_cnt" */
	current_floor_ptr->o_cnt = 0;
}


/*!
 * @brief グローバルオブジェクト配列から空きを取得する /
 * Acquires and returns the index of a "free" object.
 * @return 開いているオブジェクト要素のID
 * @details
 * This routine should almost never fail, but in case it does,
 * we must be sure to handle "failure" of this routine.
 */
OBJECT_IDX o_pop(void)
{
	OBJECT_IDX i;

	/* Initial allocation */
	if (current_floor_ptr->o_max < current_floor_ptr->max_o_idx)
	{
		/* Get next space */
		i = current_floor_ptr->o_max;

		/* Expand object array */
		current_floor_ptr->o_max++;

		/* Count objects */
		current_floor_ptr->o_cnt++;

		/* Use this object */
		return (i);
	}


	/* Recycle dead objects */
	for (i = 1; i < current_floor_ptr->o_max; i++)
	{
		object_type *o_ptr;
		o_ptr = &current_floor_ptr->o_list[i];

		/* Skip live objects */
		if (o_ptr->k_idx) continue;

		/* Count objects */
		current_floor_ptr->o_cnt++;

		/* Use this object */
		return (i);
	}


	/* Warn the player (except during dungeon creation) */
	if (current_world_ptr->character_dungeon) msg_print(_("アイテムが多すぎる！", "Too many objects!"));

	return (0);
}

/*
 * Hack -- function hook to restrict "get_obj_num_prep()" function
 */
bool(*get_obj_num_hook)(KIND_OBJECT_IDX k_idx);

/*!
 * @brief オブジェクト生成テーブルに生成制約を加える /
 * Apply a "object restriction function" to the "object allocation table"
 * @return 常に0を返す。
 * @details 生成の制約はグローバルのget_obj_num_hook関数ポインタで加える
 */
static errr get_obj_num_prep(void)
{
	int i;

	/* Get the entry */
	alloc_entry *table = alloc_kind_table;

	/* Scan the allocation table */
	for (i = 0; i < alloc_kind_size; i++)
	{
		/* Accept objects which pass the restriction, if any */
		if (!get_obj_num_hook || (*get_obj_num_hook)(table[i].index))
		{
			/* Accept this object */
			table[i].prob2 = table[i].prob1;
		}

		/* Do not use this object */
		else
		{
			/* Decline this object */
			table[i].prob2 = 0;
		}
	}

	/* Success */
	return (0);
}


/*!
 * @brief オブジェクト生成テーブルからアイテムを取得する /
 * Choose an object kind that seems "appropriate" to the given level
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
OBJECT_IDX get_obj_num(DEPTH level, BIT_FLAGS mode)
{
	int i, j, p;
	KIND_OBJECT_IDX k_idx;
	long value, total;
	object_kind     *k_ptr;
	alloc_entry     *table = alloc_kind_table;

	if (level > MAX_DEPTH - 1) level = MAX_DEPTH - 1;

	/* Boost level */
	if ((level > 0) && !(d_info[p_ptr->dungeon_idx].flags1 & DF1_BEGINNER))
	{
		/* Occasional "boost" */
		if (one_in_(GREAT_OBJ))
		{
			/* What a bizarre calculation */
			level = 1 + (level * MAX_DEPTH / randint1(MAX_DEPTH));
		}
	}

	/* Reset total */
	total = 0L;

	/* Process probabilities */
	for (i = 0; i < alloc_kind_size; i++)
	{
		/* Objects are sorted by depth */
		if (table[i].level > level) break;

		/* Default */
		table[i].prob3 = 0;

		k_idx = table[i].index;

		/* Access the actual kind */
		k_ptr = &k_info[k_idx];

		if ((mode & AM_FORBID_CHEST) && (k_ptr->tval == TV_CHEST)) continue;

		/* Accept */
		table[i].prob3 = table[i].prob2;

		/* Total */
		total += table[i].prob3;
	}

	/* No legal objects */
	if (total <= 0) return (0);


	/* Pick an object */
	value = randint0(total);

	/* Find the object */
	for (i = 0; i < alloc_kind_size; i++)
	{
		/* Found the entry */
		if (value < table[i].prob3) break;

		/* Decrement */
		value = value - table[i].prob3;
	}


	/* Power boost */
	p = randint0(100);

	/* Try for a "better" object once (50%) or twice (10%) */
	if (p < 60)
	{
		/* Save old */
		j = i;

		/* Pick a object */
		value = randint0(total);

		/* Find the object */
		for (i = 0; i < alloc_kind_size; i++)
		{
			/* Found the entry */
			if (value < table[i].prob3) break;

			/* Decrement */
			value = value - table[i].prob3;
		}

		/* Keep the "best" one */
		if (table[i].level < table[j].level) i = j;
	}

	/* Try for a "better" object twice (10%) */
	if (p < 10)
	{
		/* Save old */
		j = i;

		/* Pick a object */
		value = randint0(total);

		/* Find the object */
		for (i = 0; i < alloc_kind_size; i++)
		{
			/* Found the entry */
			if (value < table[i].prob3) break;

			/* Decrement */
			value = value - table[i].prob3;
		}

		/* Keep the "best" one */
		if (table[i].level < table[j].level) i = j;
	}

	return (table[i].index);
}

/*!
 * @brief オブジェクトを鑑定済にする /
 * Known is true when the "attributes" of an object are "known".
 * @param o_ptr 鑑定済にするオブジェクトの構造体参照ポインタ
 * @return なし
 * These include tohit, todam, toac, cost, and pval (charges).\n
 *\n
 * Note that "knowing" an object gives you everything that an "awareness"\n
 * gives you, and much more.  In fact, the player is always "aware" of any\n
 * item of which he has full "knowledge".\n
 *\n
 * But having full knowledge of, say, one "wand of wonder", does not, by\n
 * itself, give you knowledge, or even awareness, of other "wands of wonder".\n
 * It happens that most "identify" routines (including "buying from a shop")\n
 * will make the player "aware" of the object as well as fully "know" it.\n
 *\n
 * This routine also removes any inscriptions generated by "feelings".\n
 */
void object_known(object_type *o_ptr)
{
	/* Remove "default inscriptions" */
	o_ptr->feeling = FEEL_NONE;

	/* Clear the "Felt" info */
	o_ptr->ident &= ~(IDENT_SENSE);

	/* Clear the "Empty" info */
	o_ptr->ident &= ~(IDENT_EMPTY);

	/* Now we know about the item */
	o_ptr->ident |= (IDENT_KNOWN);
}

/*!
 * @brief オブジェクトを＊鑑定＊済にする /
 * The player is now aware of the effects of the given object.
 * @param o_ptr ＊鑑定＊済にするオブジェクトの構造体参照ポインタ
 * @return なし
 */
void object_aware(object_type *o_ptr)
{
	bool mihanmei = !object_is_aware(o_ptr);

	/* Fully aware of the effects */
	k_info[o_ptr->k_idx].aware = TRUE;

	if(mihanmei && !(k_info[o_ptr->k_idx].gen_flags & TRG_INSTA_ART) && record_ident &&
	   !p_ptr->is_dead && ((o_ptr->tval >= TV_AMULET && o_ptr->tval <= TV_POTION) || (o_ptr->tval == TV_FOOD)))
	{
		object_type forge;
		object_type *q_ptr;
		GAME_TEXT o_name[MAX_NLEN];

		q_ptr = &forge;
		object_copy(q_ptr, o_ptr);

		q_ptr->number = 1;
		object_desc(o_name, q_ptr, OD_NAME_ONLY);
		
		do_cmd_write_nikki(NIKKI_HANMEI, 0, o_name);
	}
}

/*!
 * @brief オブジェクトを試行済にする /
 * Something has been "sampled"
 * @param o_ptr 試行済にするオブジェクトの構造体参照ポインタ
 * @return なし
 */
void object_tried(object_type *o_ptr)
{
	/* Mark it as tried (even if "aware") */
	k_info[o_ptr->k_idx].tried = TRUE;
}

/*!
* @brief 重度擬似鑑定の判断処理 / Return a "feeling" (or NULL) about an item.  Method 1 (Heavy).
* @param o_ptr 擬似鑑定を行うオブジェクトの参照ポインタ。
* @return 擬似鑑定結果のIDを返す。
*/
byte value_check_aux1(object_type *o_ptr)
{
	/* Artifacts */
	if (object_is_artifact(o_ptr))
	{
		/* Cursed/Broken */
		if (object_is_cursed(o_ptr) || object_is_broken(o_ptr)) return FEEL_TERRIBLE;

		/* Normal */
		return FEEL_SPECIAL;
	}

	/* Ego-Items */
	if (object_is_ego(o_ptr))
	{
		/* Cursed/Broken */
		if (object_is_cursed(o_ptr) || object_is_broken(o_ptr)) return FEEL_WORTHLESS;

		/* Normal */
		return FEEL_EXCELLENT;
	}

	/* Cursed items */
	if (object_is_cursed(o_ptr)) return FEEL_CURSED;

	/* Broken items */
	if (object_is_broken(o_ptr)) return FEEL_BROKEN;

	if ((o_ptr->tval == TV_RING) || (o_ptr->tval == TV_AMULET)) return FEEL_AVERAGE;

	/* Good "armor" bonus */
	if (o_ptr->to_a > 0) return FEEL_GOOD;

	/* Good "weapon" bonus */
	if (o_ptr->to_h + o_ptr->to_d > 0) return FEEL_GOOD;

	/* Default to "average" */
	return FEEL_AVERAGE;
}

/*!
* @brief 軽度擬似鑑定の判断処理 / Return a "feeling" (or NULL) about an item.  Method 2 (Light).
* @param o_ptr 擬似鑑定を行うオブジェクトの参照ポインタ。
* @return 擬似鑑定結果のIDを返す。
*/
byte value_check_aux2(object_type *o_ptr)
{
	/* Cursed items (all of them) */
	if (object_is_cursed(o_ptr)) return FEEL_CURSED;

	/* Broken items (all of them) */
	if (object_is_broken(o_ptr)) return FEEL_BROKEN;

	/* Artifacts -- except cursed/broken ones */
	if (object_is_artifact(o_ptr)) return FEEL_UNCURSED;

	/* Ego-Items -- except cursed/broken ones */
	if (object_is_ego(o_ptr)) return FEEL_UNCURSED;

	/* Good armor bonus */
	if (o_ptr->to_a > 0) return FEEL_UNCURSED;

	/* Good weapon bonuses */
	if (o_ptr->to_h + o_ptr->to_d > 0) return FEEL_UNCURSED;

	/* No feeling */
	return FEEL_NONE;
}

/*!
 * @brief 未鑑定なベースアイテムの基本価格を返す /
 * Return the "value" of an "unknown" item Make a guess at the value of non-aware items
 * @param o_ptr 未鑑定価格を確認したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトの未鑑定価格
 */
static PRICE object_value_base(object_type *o_ptr)
{
	/* Aware item -- use template cost */
	if (object_is_aware(o_ptr)) return (k_info[o_ptr->k_idx].cost);

	/* Analyze the type */
	switch (o_ptr->tval)
	{

		/* Un-aware Food */
		case TV_FOOD: return (5L);

		/* Un-aware Potions */
		case TV_POTION: return (20L);

		/* Un-aware Scrolls */
		case TV_SCROLL: return (20L);

		/* Un-aware Staffs */
		case TV_STAFF: return (70L);

		/* Un-aware Wands */
		case TV_WAND: return (50L);

		/* Un-aware Rods */
		case TV_ROD: return (90L);

		/* Un-aware Rings */
		case TV_RING: return (45L);

		/* Un-aware Amulets */
		case TV_AMULET: return (45L);

		/* Figurines, relative to monster level */
		case TV_FIGURINE:
		{
			DEPTH level = r_info[o_ptr->pval].level;
			if (level < 20) return level*50L;
			else if (level < 30) return 1000+(level-20)*150L;
			else if (level < 40) return 2500+(level-30)*350L;
			else if (level < 50) return 6000+(level-40)*800L;
			else return 14000+(level-50)*2000L;
		}

		case TV_CAPTURE:
			if (!o_ptr->pval) return 1000L;
			else return ((r_info[o_ptr->pval].level) * 50L + 1000);
	}

	/* Paranoia -- Oops */
	return (0L);
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
	PRICE tmp_cost;
	int count;
	int i;
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	object_flags(o_ptr, flgs);

	/*
	 * Exclude fixed flags of the base item.
	 * pval bonuses of base item will be treated later.
	 */
	for (i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] &= ~(k_ptr->flags[i]);

	/* Exclude fixed flags of the fixed artifact. */
	if (object_is_fixed_artifact(o_ptr))
	{
		artifact_type *a_ptr = &a_info[o_ptr->name1];

		for (i = 0; i < TR_FLAG_SIZE; i++)
			flgs[i] &= ~(a_ptr->flags[i]);
	}

	/* Exclude fixed flags of the ego-item. */
	else if (object_is_ego(o_ptr))
	{
		ego_item_type *e_ptr = &e_info[o_ptr->name2];

		for (i = 0; i < TR_FLAG_SIZE; i++)
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

	tmp_cost = 0;
	count = 0;
	if (have_flag(flgs, TR_CHAOTIC)) {total += 5000;count++;}
	if (have_flag(flgs, TR_VAMPIRIC)) {total += 6500;count++;}
	if (have_flag(flgs, TR_FORCE_WEAPON)) {tmp_cost += 2500;count++;}
	if (have_flag(flgs, TR_KILL_ANIMAL)) {tmp_cost += 2800;count++;}
	else if (have_flag(flgs, TR_SLAY_ANIMAL)) {tmp_cost += 1800;count++;}
	if (have_flag(flgs, TR_KILL_EVIL)) {tmp_cost += 3300;count++;}
	else if (have_flag(flgs, TR_SLAY_EVIL)) {tmp_cost += 2300;count++;}
	if (have_flag(flgs, TR_KILL_HUMAN)) {tmp_cost += 2800;count++;}
	else if (have_flag(flgs, TR_SLAY_HUMAN)) {tmp_cost += 1800;count++;}
	if (have_flag(flgs, TR_KILL_UNDEAD)) {tmp_cost += 2800;count++;}
	else if (have_flag(flgs, TR_SLAY_UNDEAD)) {tmp_cost += 1800;count++;}
	if (have_flag(flgs, TR_KILL_DEMON)) {tmp_cost += 2800;count++;}
	else if (have_flag(flgs, TR_SLAY_DEMON)) {tmp_cost += 1800;count++;}
	if (have_flag(flgs, TR_KILL_ORC)) {tmp_cost += 2500;count++;}
	else if (have_flag(flgs, TR_SLAY_ORC)) {tmp_cost += 1500;count++;}
	if (have_flag(flgs, TR_KILL_TROLL)) {tmp_cost += 2800;count++;}
	else if (have_flag(flgs, TR_SLAY_TROLL)) {tmp_cost += 1800;count++;}
	if (have_flag(flgs, TR_KILL_GIANT)) {tmp_cost += 2800;count++;}
	else if (have_flag(flgs, TR_SLAY_GIANT)) {tmp_cost += 1800;count++;}
	if (have_flag(flgs, TR_KILL_DRAGON)) {tmp_cost += 2800;count++;}
	else if (have_flag(flgs, TR_SLAY_DRAGON)) {tmp_cost += 1800;count++;}

	if (have_flag(flgs, TR_VORPAL)) {tmp_cost += 2500;count++;}
	if (have_flag(flgs, TR_IMPACT)) {tmp_cost += 2500;count++;}
	if (have_flag(flgs, TR_BRAND_POIS)) {tmp_cost += 3800;count++;}
	if (have_flag(flgs, TR_BRAND_ACID)) {tmp_cost += 3800;count++;}
	if (have_flag(flgs, TR_BRAND_ELEC)) {tmp_cost += 3800;count++;}
	if (have_flag(flgs, TR_BRAND_FIRE)) {tmp_cost += 2500;count++;}
	if (have_flag(flgs, TR_BRAND_COLD)) {tmp_cost += 2500;count++;}
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
	if (have_flag(flgs, TR_IM_ACID)) {tmp_cost += 15000;count += 2;}
	if (have_flag(flgs, TR_IM_ELEC)) {tmp_cost += 15000;count += 2;}
	if (have_flag(flgs, TR_IM_FIRE)) {tmp_cost += 15000;count += 2;}
	if (have_flag(flgs, TR_IM_COLD)) {tmp_cost += 15000;count += 2;}
	if (have_flag(flgs, TR_REFLECT)) {tmp_cost += 5000;count += 2;}
	if (have_flag(flgs, TR_RES_ACID)) {tmp_cost += 500;count++;}
	if (have_flag(flgs, TR_RES_ELEC)) {tmp_cost += 500;count++;}
	if (have_flag(flgs, TR_RES_FIRE)) {tmp_cost += 500;count++;}
	if (have_flag(flgs, TR_RES_COLD)) {tmp_cost += 500;count++;}
	if (have_flag(flgs, TR_RES_POIS)) {tmp_cost += 1000;count += 2;}
	if (have_flag(flgs, TR_RES_FEAR)) {tmp_cost += 1000;count += 2;}
	if (have_flag(flgs, TR_RES_LITE)) {tmp_cost += 800;count += 2;}
	if (have_flag(flgs, TR_RES_DARK)) {tmp_cost += 800;count += 2;}
	if (have_flag(flgs, TR_RES_BLIND)) {tmp_cost += 900;count += 2;}
	if (have_flag(flgs, TR_RES_CONF)) {tmp_cost += 900;count += 2;}
	if (have_flag(flgs, TR_RES_SOUND)) {tmp_cost += 900;count += 2;}
	if (have_flag(flgs, TR_RES_SHARDS)) {tmp_cost += 900;count += 2;}
	if (have_flag(flgs, TR_RES_NETHER)) {tmp_cost += 900;count += 2;}
	if (have_flag(flgs, TR_RES_NEXUS)) {tmp_cost += 900;count += 2;}
	if (have_flag(flgs, TR_RES_CHAOS)) {tmp_cost += 1000;count += 2;}
	if (have_flag(flgs, TR_RES_DISEN)) {tmp_cost += 2000;count += 2;}
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
 * @brief オブジェクトの真の価格を算出する /
 * Return the value of the flags the object has...
 * @param o_ptr 本価格を確認したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトの本価格
 * @details
 * Return the "real" price of a "known" item, not including discounts\n
 *\n
 * Wand and staffs get cost for each charge\n
 *\n
 * Armor is worth an extra 100 gold per bonus point to armor class.\n
 *\n
 * Weapons are worth an extra 100 gold per bonus point (AC,TH,TD).\n
 *\n
 * Missiles are only worth 5 gold per bonus point, since they\n
 * usually appear in groups of 20, and we want the player to get\n
 * the same amount of cash for any "equivalent" item.  Note that\n
 * missiles never have any of the "pval" flags, and in fact, they\n
 * only have a few of the available flags, primarily of the "slay"\n
 * and "brand" and "ignore" variety.\n
 *\n
 * Armor with a negative armor bonus is worthless.\n
 * Weapons with negative hit+damage bonuses are worthless.\n
 *\n
 * Every wearable item with a "pval" bonus is worth extra (see below).\n
 */
PRICE object_value_real(object_type *o_ptr)
{
	PRICE value;
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	object_kind *k_ptr = &k_info[o_ptr->k_idx];


	/* Hack -- "worthless" items */
	if (!k_info[o_ptr->k_idx].cost) return (0L);

	/* Base cost */
	value = k_info[o_ptr->k_idx].cost;

	/* Extract some flags */
	object_flags(o_ptr, flgs);

	/* Artifact */
	if (object_is_fixed_artifact(o_ptr))
	{
		artifact_type *a_ptr = &a_info[o_ptr->name1];

		/* Hack -- "worthless" artifacts */
		if (!a_ptr->cost) return (0L);

		/* Hack -- Use the artifact cost instead */
		value = a_ptr->cost;
		value += flag_cost(o_ptr, o_ptr->pval);

		/* Don't add pval bonuses etc. */
		return (value);
	}

	/* Ego-Item */
	else if (object_is_ego(o_ptr))
	{
		ego_item_type *e_ptr = &e_info[o_ptr->name2];

		/* Hack -- "worthless" ego-items */
		if (!e_ptr->cost) return (0L);

		/* Hack -- Reward the ego-item with a bonus */
		value += e_ptr->cost;
		value += flag_cost(o_ptr, o_ptr->pval);
	}

	else
	{
		int i;
		bool flag = FALSE;

		for (i = 0; i < TR_FLAG_SIZE; i++) 
			if (o_ptr->art_flags[i]) flag = TRUE;

		if (flag) value += flag_cost(o_ptr, o_ptr->pval);
	}

	/* Analyze pval bonus for normal object */
	switch (o_ptr->tval)
	{
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
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
	case TV_LITE:
	case TV_AMULET:
	case TV_RING:
		/* No pval */
		if (!o_ptr->pval) break;

		/* Hack -- Negative "pval" is always bad */
		if (o_ptr->pval < 0) return (0L);

		/* Give credit for stat bonuses */
		if (have_flag(flgs, TR_STR)) value += (o_ptr->pval * 200L);
		if (have_flag(flgs, TR_INT)) value += (o_ptr->pval * 200L);
		if (have_flag(flgs, TR_WIS)) value += (o_ptr->pval * 200L);
		if (have_flag(flgs, TR_DEX)) value += (o_ptr->pval * 200L);
		if (have_flag(flgs, TR_CON)) value += (o_ptr->pval * 200L);
		if (have_flag(flgs, TR_CHR)) value += (o_ptr->pval * 200L);

		/* Give credit for stealth and searching */
		if (have_flag(flgs, TR_MAGIC_MASTERY)) value += (o_ptr->pval * 100);
		if (have_flag(flgs, TR_STEALTH)) value += (o_ptr->pval * 100L);
		if (have_flag(flgs, TR_SEARCH)) value += (o_ptr->pval * 100L);

		/* Give credit for infra-vision and tunneling */
		if (have_flag(flgs, TR_INFRA)) value += (o_ptr->pval * 50L);
		if (have_flag(flgs, TR_TUNNEL)) value += (o_ptr->pval * 50L);

		/* Give credit for extra attacks */
		if (have_flag(flgs, TR_BLOWS)) value += (o_ptr->pval * 5000L);

		/* Give credit for speed bonus */
		if (have_flag(flgs, TR_SPEED)) value += (o_ptr->pval * 10000L);

		break;
	}


	/* Analyze the item */
	switch (o_ptr->tval)
	{
		/* Wands/Staffs */
		case TV_WAND:
		{
			/* Pay extra for charges, depending on standard number of
			 * charges.  Handle new-style wands correctly. -LM-
			 */
			value += (value * o_ptr->pval / o_ptr->number / (k_ptr->pval * 2));

			break;
		}
		case TV_STAFF:
		{
			/* Pay extra for charges, depending on standard number of
			 * charges.  -LM-
			 */
			value += (value * o_ptr->pval / (k_ptr->pval * 2));

			break;
		}

		/* Rings/Amulets */
		case TV_RING:
		case TV_AMULET:
		{
			/* Hack -- negative bonuses are bad */
			if (o_ptr->to_h + o_ptr->to_d + o_ptr->to_a < 0) return (0L);

			/* Give credit for bonuses */
			value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 200L);

			break;
		}

		/* Armor */
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CLOAK:
		case TV_CROWN:
		case TV_HELM:
		case TV_SHIELD:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		{
			/* Hack -- negative armor bonus */
			if (o_ptr->to_a < 0) return (0L);

			/* Give credit for bonuses */
			value += (((o_ptr->to_h - k_ptr->to_h) + (o_ptr->to_d - k_ptr->to_d)) * 200L + (o_ptr->to_a) * 100L);

			break;
		}

		/* Bows/Weapons */
		case TV_BOW:
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_SWORD:
		case TV_POLEARM:
		{
			/* Hack -- negative hit/damage bonuses */
			if (o_ptr->to_h + o_ptr->to_d < 0) return (0L);

			/* Factor in the bonuses */
			value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);

			/* Hack -- Factor in extra damage dice and sides */
			value += (o_ptr->dd - k_ptr->dd) * o_ptr->ds * 250L;
			value += (o_ptr->ds - k_ptr->ds) * o_ptr->dd * 250L;

			break;
		}

		/* Ammo */
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		{
			/* Hack -- negative hit/damage bonuses */
			if (o_ptr->to_h + o_ptr->to_d < 0) return (0L);

			/* Factor in the bonuses */
			value += ((o_ptr->to_h + o_ptr->to_d) * 5L);

			/* Hack -- Factor in extra damage dice and sides */
			value += (o_ptr->dd - k_ptr->dd) * o_ptr->ds * 5L;
			value += (o_ptr->ds - k_ptr->ds) * o_ptr->dd * 5L;

			break;
		}

		/* Figurines, relative to monster level */
		case TV_FIGURINE:
		{
			DEPTH level = r_info[o_ptr->pval].level;
			if (level < 20) value = level*50L;
			else if (level < 30) value = 1000+(level-20)*150L;
			else if (level < 40) value = 2500+(level-30)*350L;
			else if (level < 50) value = 6000+(level-40)*800L;
			else value = 14000+(level-50)*2000L;
			break;
		}

		case TV_CAPTURE:
		{
			if (!o_ptr->pval) value = 1000L;
			else value = ((r_info[o_ptr->pval].level) * 50L + 1000);
			break;
		}

		case TV_CHEST:
		{
			if (!o_ptr->pval) value = 0L;
			break;
		}
	}

	/* Worthless object */
	if (value < 0) return 0L;

	/* Return the value */
	return (value);
}


/*!
 * @brief オブジェクト価格算出のメインルーチン /
 * Return the price of an item including plusses (and charges)
 * @param o_ptr 判明している現価格を確認したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトの判明している現価格
 * @details
 * This function returns the "value" of the given item (qty one)\n
 *\n
 * Never notice "unknown" bonuses or properties, including "curses",\n
 * since that would give the player information he did not have.\n
 *\n
 * Note that discounted items stay discounted forever, even if\n
 * the discount is "forgotten" by the player via memory loss.\n
 */
PRICE object_value(object_type *o_ptr)
{
	PRICE value;

	/* Unknown items -- acquire a base value */
	if (object_is_known(o_ptr))
	{
		/* Broken items -- worthless */
		if (object_is_broken(o_ptr)) return (0L);

		/* Cursed items -- worthless */
		if (object_is_cursed(o_ptr)) return (0L);

		/* Real value (see above) */
		value = object_value_real(o_ptr);
	}

	/* Known items -- acquire the actual value */
	else
	{
		/* Hack -- Felt broken items */
		if ((o_ptr->ident & (IDENT_SENSE)) && object_is_broken(o_ptr)) return (0L);

		/* Hack -- Felt cursed items */
		if ((o_ptr->ident & (IDENT_SENSE)) && object_is_cursed(o_ptr)) return (0L);

		/* Base value (see above) */
		value = object_value_base(o_ptr);
	}

	/* Apply discount (if any) */
	if (o_ptr->discount) value -= (value * o_ptr->discount / 100L);

	/* Return the final value */
	return (value);
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
	if ((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_ROD))
	{
		q_ptr->pval = o_ptr->pval * amt / o_ptr->number;
		if (amt < o_ptr->number) o_ptr->pval -= q_ptr->pval;

		/* Hack -- Rods also need to have their timeouts distributed.  The
		 * dropped stack will accept all time remaining to charge up to its
		 * maximum.
		 */
		if ((o_ptr->tval == TV_ROD) && (o_ptr->timeout))
		{
			if (q_ptr->pval > o_ptr->timeout)
				q_ptr->timeout = o_ptr->timeout;
			else
				q_ptr->timeout = q_ptr->pval;

			if (amt < o_ptr->number) o_ptr->timeout -= q_ptr->timeout;
		}
	}
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

/*
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
 * @brief 両オブジェクトをスロットに重ね合わせ可能な最大数を返す。
 * Determine if an item can partly absorb a second item. Return maximum number of stack.
 * @param o_ptr 検証したいオブジェクトの構造体参照ポインタ1
 * @param j_ptr 検証したいオブジェクトの構造体参照ポインタ2
 * @return 重ね合わせ可能なアイテム数
 */
int object_similar_part(object_type *o_ptr, object_type *j_ptr)
{
	int i;

	/* Default maximum number of stack */
	int max_num = MAX_STACK_SIZE;

	/* Require identical object types */
	if (o_ptr->k_idx != j_ptr->k_idx) return 0;


	/* Analyze the items */
	switch (o_ptr->tval)
	{
		/* Chests and Statues*/
		case TV_CHEST:
		case TV_CARD:
		case TV_CAPTURE:
		{
			/* Never okay */
			return 0;
		}

		case TV_STATUE:
		{
			if ((o_ptr->sval != SV_PHOTO) || (j_ptr->sval != SV_PHOTO)) return 0;
			if (o_ptr->pval != j_ptr->pval) return 0;
			break;
		}

		/* Figurines and Corpses*/
		case TV_FIGURINE:
		case TV_CORPSE:
		{
			/* Same monster */
			if (o_ptr->pval != j_ptr->pval) return 0;

			/* Assume okay */
			break;
		}

		/* Food and Potions and Scrolls */
		case TV_FOOD:
		case TV_POTION:
		case TV_SCROLL:
		{
			/* Assume okay */
			break;
		}

		/* Staffs */
		case TV_STAFF:
		{
			/* Require either knowledge or known empty for both staffs. */
			if ((!(o_ptr->ident & (IDENT_EMPTY)) &&
				!object_is_known(o_ptr)) ||
				(!(j_ptr->ident & (IDENT_EMPTY)) &&
				!object_is_known(j_ptr))) return 0;

			/* Require identical charges, since staffs are bulky. */
			if (o_ptr->pval != j_ptr->pval) return 0;

			/* Assume okay */
			break;
		}

		/* Wands */
		case TV_WAND:
		{
			/* Require either knowledge or known empty for both wands. */
			if ((!(o_ptr->ident & (IDENT_EMPTY)) &&
				!object_is_known(o_ptr)) ||
				(!(j_ptr->ident & (IDENT_EMPTY)) &&
				!object_is_known(j_ptr))) return 0;

			/* Wand charges combine in O&ZAngband.  */

			/* Assume okay */
			break;
		}

		/* Staffs and Wands and Rods */
		case TV_ROD:
		{
			/* Prevent overflaw of timeout */
			max_num = MIN(max_num, MAX_SHORT / k_info[o_ptr->k_idx].pval);

			/* Assume okay */
			break;
		}

		/* Weapons and Armor */
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

		/* Rings, Amulets, Lites */
		case TV_RING:
		case TV_AMULET:
		case TV_LITE:
		case TV_WHISTLE:
		{
			/* Require full knowledge of both items */
			if (!object_is_known(o_ptr) || !object_is_known(j_ptr)) return 0;

			/* Fall through */
		}

		/* Missiles */
		case TV_BOLT:
		case TV_ARROW:
		case TV_SHOT:
		{
			/* Require identical knowledge of both items */
			if (object_is_known(o_ptr) != object_is_known(j_ptr)) return 0;
			if (o_ptr->feeling != j_ptr->feeling) return 0;

			/* Require identical "bonuses" */
			if (o_ptr->to_h != j_ptr->to_h) return 0;
			if (o_ptr->to_d != j_ptr->to_d) return 0;
			if (o_ptr->to_a != j_ptr->to_a) return 0;

			/* Require identical "pval" code */
			if (o_ptr->pval != j_ptr->pval) return 0;

			/* Artifacts never stack */
			if (object_is_artifact(o_ptr) || object_is_artifact(j_ptr)) return 0;

			/* Require identical "ego-item" names */
			if (o_ptr->name2 != j_ptr->name2) return 0;

			/* Require identical added essence  */
			if (o_ptr->xtra3 != j_ptr->xtra3) return 0;
			if (o_ptr->xtra4 != j_ptr->xtra4) return 0;

			/* Hack -- Never stack "powerful" items */
			if (o_ptr->xtra1 || j_ptr->xtra1) return 0;

			/* Hack -- Never stack recharging items */
			if (o_ptr->timeout || j_ptr->timeout) return 0;

			/* Require identical "values" */
			if (o_ptr->ac != j_ptr->ac) return 0;
			if (o_ptr->dd != j_ptr->dd) return 0;
			if (o_ptr->ds != j_ptr->ds) return 0;

			/* Probably okay */
			break;
		}

		/* Various */
		default:
		{
			/* Require knowledge */
			if (!object_is_known(o_ptr) || !object_is_known(j_ptr)) return 0;

			/* Probably okay */
			break;
		}
	}


	/* Hack -- Identical art_flags! */
	for (i = 0; i < TR_FLAG_SIZE; i++)
		if (o_ptr->art_flags[i] != j_ptr->art_flags[i]) return 0;

	/* Hack -- Require identical "cursed" status */
	if (o_ptr->curse_flags != j_ptr->curse_flags) return 0;

	/* Hack -- Require identical "broken" status */
	if ((o_ptr->ident & (IDENT_BROKEN)) != (j_ptr->ident & (IDENT_BROKEN))) return 0;


	/* Hack -- require semi-matching "inscriptions" */
	if (o_ptr->inscription && j_ptr->inscription &&
	    (o_ptr->inscription != j_ptr->inscription))
		return 0;

	/* Hack -- normally require matching "inscriptions" */
	if (!stack_force_notes && (o_ptr->inscription != j_ptr->inscription)) return 0;

	/* Hack -- normally require matching "discounts" */
	if (!stack_force_costs && (o_ptr->discount != j_ptr->discount)) return 0;


	/* They match, so they must be similar */
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
	int max_num;

	/* Are these objects similar? */
	max_num = object_similar_part(o_ptr, j_ptr);

	/* Return if not similar */
	if (!max_num) return FALSE;

	/* Maximal "stacking" limit */
	if (total > max_num) return (0);


	/* They match, so they must be similar */
	return (TRUE);
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

	/* Combine quantity, lose excess items */
	o_ptr->number = (total > max_num) ? max_num : total;

	/* Hack -- blend "known" status */
	if (object_is_known(j_ptr)) object_known(o_ptr);

	/* Hack -- clear "storebought" if only one has it */
	if (((o_ptr->ident & IDENT_STORE) || (j_ptr->ident & IDENT_STORE)) &&
	    (!((o_ptr->ident & IDENT_STORE) && (j_ptr->ident & IDENT_STORE))))
	{
		if (j_ptr->ident & IDENT_STORE) j_ptr->ident &= 0xEF;
		if (o_ptr->ident & IDENT_STORE) o_ptr->ident &= 0xEF;
	}

	/* Hack -- blend "mental" status */
	if (j_ptr->ident & (IDENT_MENTAL)) o_ptr->ident |= (IDENT_MENTAL);

	/* Hack -- blend "inscriptions" */
	if (j_ptr->inscription) o_ptr->inscription = j_ptr->inscription;

	/* Hack -- blend "feelings" */
	if (j_ptr->feeling) o_ptr->feeling = j_ptr->feeling;

	/* Hack -- could average discounts */
	/* Hack -- save largest discount */
	if (o_ptr->discount < j_ptr->discount) o_ptr->discount = j_ptr->discount;

	/* Hack -- if rods are stacking, add the pvals (maximum timeouts) and current timeouts together. -LM- */
	if (o_ptr->tval == TV_ROD)
	{
		o_ptr->pval += j_ptr->pval * (j_ptr->number - diff) / j_ptr->number;
		o_ptr->timeout += j_ptr->timeout * (j_ptr->number - diff) / j_ptr->number;
	}

	/* Hack -- if wands are stacking, combine the charges. -LM- */
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
KIND_OBJECT_IDX lookup_kind(OBJECT_TYPE_VALUE tval, OBJECT_SUBTYPE_VALUE sval)
{
	KIND_OBJECT_IDX k;
	int num = 0;
	KIND_OBJECT_IDX bk = 0;

	/* Look for it */
	for (k = 1; k < max_k_idx; k++)
	{
		object_kind *k_ptr = &k_info[k];

		/* Require correct tval */
		if (k_ptr->tval != tval) continue;

		/* Found a match */
		if (k_ptr->sval == sval) return (k);

		/* Ignore illegal items */
		if (sval != SV_ANY) continue;

		/* Apply the randomizer */
		if (!one_in_(++num)) continue;

		/* Use this value */
		bk = k;
	}

	/* Return this choice */
	if (sval == SV_ANY)
	{
		return bk;
	}

#if 0
	msg_format(_("アイテムがない (%d,%d)", "No object (%d,%d)"), tval, sval);
#endif


	return (0);
}


/*!
 * @brief オブジェクトを初期化する
 * Wipe an object clean.
 * @param o_ptr 初期化したいオブジェクトの構造体参照ポインタ
 * @return なし
 */
void object_wipe(object_type *o_ptr)
{
	/* Wipe the structure */
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
	/* Copy the structure */
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

	/* Clear the record */
	object_wipe(o_ptr);

	/* Save the kind index */
	o_ptr->k_idx = k_idx;

	/* Efficiency -- tval/sval */
	o_ptr->tval = k_ptr->tval;
	o_ptr->sval = k_ptr->sval;

	/* Default "pval" */
	o_ptr->pval = k_ptr->pval;

	/* Default number */
	o_ptr->number = 1;

	/* Default weight */
	o_ptr->weight = k_ptr->weight;

	/* Default magic */
	o_ptr->to_h = k_ptr->to_h;
	o_ptr->to_d = k_ptr->to_d;
	o_ptr->to_a = k_ptr->to_a;

	/* Default power */
	o_ptr->ac = k_ptr->ac;
	o_ptr->dd = k_ptr->dd;
	o_ptr->ds = k_ptr->ds;

	/* Default activation */
	if (k_ptr->act_idx > 0) o_ptr->xtra2 = (XTRA8)k_ptr->act_idx;

	/* Hack -- worthless items are always "broken" */
	if (k_info[o_ptr->k_idx].cost <= 0) o_ptr->ident |= (IDENT_BROKEN);

	/* Hack -- cursed items are always "cursed" */
	if (k_ptr->gen_flags & (TRG_CURSED)) o_ptr->curse_flags |= (TRC_CURSED);
	if (k_ptr->gen_flags & (TRG_HEAVY_CURSE)) o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
	if (k_ptr->gen_flags & (TRG_PERMA_CURSE)) o_ptr->curse_flags |= (TRC_PERMA_CURSE);
	if (k_ptr->gen_flags & (TRG_RANDOM_CURSE0)) o_ptr->curse_flags |= get_curse(0, o_ptr);
	if (k_ptr->gen_flags & (TRG_RANDOM_CURSE1)) o_ptr->curse_flags |= get_curse(1, o_ptr);
	if (k_ptr->gen_flags & (TRG_RANDOM_CURSE2)) o_ptr->curse_flags |= get_curse(2, o_ptr);
}


/*!
 * @brief デバッグ時にアイテム生成情報をメッセージに出力する / Cheat -- describe a created object for the user
 * @param o_ptr デバッグ出力するオブジェクトの構造体参照ポインタ
 * @return なし
 */
static void object_mention(object_type *o_ptr)
{
	GAME_TEXT o_name[MAX_NLEN];

	object_aware(o_ptr);
	object_known(o_ptr);

	/* Mark the item as fully known */
	o_ptr->ident |= (IDENT_MENTAL);
	object_desc(o_name, o_ptr, 0);
	msg_format_wizard(CHEAT_OBJECT, _("%sを生成しました。", "%s was generated."), o_name);
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
	int i, value;
	ego_item_type *e_ptr;

	long total = 0L;
	
	for (i = 1; i < max_e_idx; i++)
	{
		e_ptr = &e_info[i];
		
		if (e_ptr->slot == slot
		    && ((good && e_ptr->rating) || (!good && !e_ptr->rating)) )
		{
			if (e_ptr->rarity)
				total += (255 / e_ptr->rarity);
		}
	}

	value = randint1(total);

	for (i = 1; i < max_e_idx; i++)
	{
		e_ptr = &e_info[i];
		
		if (e_ptr->slot == slot
		    && ((good && e_ptr->rating) || (!good && !e_ptr->rating)) )
		{
			if (e_ptr->rarity)
				value -= (255 / e_ptr->rarity);
			if (value <= 0L) break;
		}
	}
	return (byte)i;
}


/*!
 * @brief 武器系オブジェクトに生成ランクごとの強化を与えるサブルーチン
 * Apply magic to an item known to be a "weapon"
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 * @return なし
 * @details
 * Hack -- note special base damage dice boosting\n
 * Hack -- note special processing for weapon/digger\n
 */
void apply_magic_weapon(object_type *o_ptr, DEPTH level, int power)
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

	/* Good */
	if (power > 0)
	{
		/* Enchant */
		o_ptr->to_h += tohit1;
		o_ptr->to_d += todam1;

		/* Very good */
		if (power > 1)
		{
			/* Enchant again */
			o_ptr->to_h += tohit2;
			o_ptr->to_d += todam2;
		}
	}

	/* Cursed */
	else if (power < 0)
	{
		/* Penalize */
		o_ptr->to_h -= tohit1;
		o_ptr->to_d -= todam1;

		/* Very cursed */
		if (power < -1)
		{
			/* Penalize again */
			o_ptr->to_h -= tohit2;
			o_ptr->to_d -= todam2;
		}

		/* Cursed (if "bad") */
		if (o_ptr->to_h + o_ptr->to_d < 0) o_ptr->curse_flags |= TRC_CURSED;
	}

	if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DIAMOND_EDGE)) return;

	switch (o_ptr->tval)
	{
		case TV_DIGGING:
		{
			/* Very good */
			if (power > 1)
			{
				if (one_in_(30) || (power > 2)) /* power > 2 is debug only */
					create_artifact(o_ptr, FALSE);
				else
					/* Special Ego-item */
					o_ptr->name2 = EGO_DIGGING;
			}

			/* Very bad */
			else if (power < -1)
			{
				/* Hack -- Horrible digging bonus */
				o_ptr->pval = 0 - (5 + randint1(5));
			}

			/* Bad */
			else if (power < 0)
			{
				/* Hack -- Reverse digging bonus */
				o_ptr->pval = 0 - (o_ptr->pval);
			}

			break;
		}

		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		{
			/* Very Good */
			if (power > 1)
			{
				if (one_in_(40) || (power > 2)) /* power > 2 is debug only */
				{
					create_artifact(o_ptr, FALSE);
					break;
				}
				while (1)
				{
					/* Roll for an ego-item */
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
					if (one_in_(3)) /* double damage */
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
					/* Hack -- Super-charge the damage dice */
					while (one_in_(10L * o_ptr->dd * o_ptr->ds)) o_ptr->dd++;

					/* Hack -- Lower the damage dice */
					if (o_ptr->dd > 9) o_ptr->dd = 9;
				}
			}

			/* Very cursed */
			else if (power < -1)
			{
				/* Roll for ego-item */
				if (randint0(MAX_DEPTH) < level)
				{
					while (1)
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
			/* Very good */
			if (power > 1)
			{
				if (one_in_(20) || (power > 2)) /* power > 2 is debug only */
				{
					create_artifact(o_ptr, FALSE);
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
			/* Very good */
			if (power > 1)
			{
				if (power > 2) /* power > 2 is debug only */
				{
					create_artifact(o_ptr, FALSE);
					break;
				}

				o_ptr->name2 = get_random_ego(INVEN_AMMO, TRUE);

				switch (o_ptr->name2)
				{
				case EGO_SLAYING_BOLT:
					o_ptr->dd++;
					break;
				}

				/* Hack -- super-charge the damage dice */
				while (one_in_(10L * o_ptr->dd * o_ptr->ds)) o_ptr->dd++;

				/* Hack -- restrict the damage dice */
				if (o_ptr->dd > 9) o_ptr->dd = 9;
			}

			/* Very cursed */
			else if (power < -1)
			{
				/* Roll for ego-item */
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
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 * @return なし
 * @details
 * Hack -- note special processing for crown/helm\n
 * Hack -- note special processing for robe of permanence\n
 */
static void a_m_aux_2(object_type *o_ptr, DEPTH level, int power)
{
	ARMOUR_CLASS toac1 = (ARMOUR_CLASS)randint1(5) + m_bonus(5, level);
	ARMOUR_CLASS toac2 = (ARMOUR_CLASS)m_bonus(10, level);

	/* Good */
	if (power > 0)
	{
		/* Enchant */
		o_ptr->to_a += toac1;

		/* Very good */
		if (power > 1)
		{
			/* Enchant again */
			o_ptr->to_a += toac2;
		}
	}

	/* Cursed */
	else if (power < 0)
	{
		/* Penalize */
		o_ptr->to_a -= toac1;

		/* Very cursed */
		if (power < -1)
		{
			/* Penalize again */
			o_ptr->to_a -= toac2;
		}

		/* Cursed (if "bad") */
		if (o_ptr->to_a < 0) o_ptr->curse_flags |= TRC_CURSED;
	}

	switch (o_ptr->tval)
	{
		case TV_DRAG_ARMOR:
		{
			if (one_in_(50) || (power > 2)) /* power > 2 is debug only */
				create_artifact(o_ptr, FALSE);
			break;
		}

		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		{
			/* Very good */
			if (power > 1)
			{
				/* Hack -- Try for "Robes of the Magi" */
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

				if (one_in_(20) || (power > 2)) /* power > 2 is debug only */
				{
					create_artifact(o_ptr, FALSE);
					break;
				}

				while (1)
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
					if(one_in_(3)) o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
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

			/* Very good */
			if (power > 1)
			{
				if (one_in_(20) || (power > 2)) /* power > 2 is debug only */
				{
					create_artifact(o_ptr, FALSE);
					break;
				}
				
				while(1)
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
				if (one_in_(20) || (power > 2)) /* power > 2 is debug only */
				{
					create_artifact(o_ptr, FALSE);
					break;
				}
				o_ptr->name2 = get_random_ego(INVEN_HANDS, TRUE);
			}
			
			/* Very cursed */
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
			/* Very good */
			if (power > 1)
			{
				if (one_in_(20) || (power > 2)) /* power > 2 is debug only */
				{
					create_artifact(o_ptr, FALSE);
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
			/* Very cursed */
			else if (power < -1)
			{
				o_ptr->name2 = get_random_ego(INVEN_FEET, FALSE);
			}

			break;
		}

		case TV_CROWN:
		{
			/* Very good */
			if (power > 1)
			{
				if (one_in_(20) || (power > 2)) /* power > 2 is debug only */
				{
					create_artifact(o_ptr, FALSE);
					break;
				}
				while (1)
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
					default:/* not existing crown (wisdom,lite, etc...) */
						ok_flag = FALSE;
					}
					if (ok_flag)
						break; /* while (1) */
				}
				break;
			}

			/* Very cursed */
			else if (power < -1)
			{	
				while (1)
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
						break; /* while (1) */
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

			/* Very good */
			if (power > 1)
			{
				if (one_in_(20) || (power > 2)) /* power > 2 is debug only */
				{
					create_artifact(o_ptr, FALSE);
					break;
				}
				while (1)
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
						if(one_in_(3)) o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
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
					default:/* not existing helm (Magi, Might, etc...)*/
						ok_flag = FALSE;
					}
					if (ok_flag)
						break; /* while (1) */
				}
				break;
			}
			/* Very cursed */
			else if (power < -1)
			{
				while (1)
				{
					bool ok_flag = TRUE;
					o_ptr->name2 = get_random_ego(INVEN_HEAD, FALSE);

					switch (o_ptr->name2)
					{
					  case EGO_ANCIENT_CURSE:
						ok_flag = FALSE;
					}
					if (ok_flag)
						break; /* while (1) */
				}
			}
			break;
		}

		case TV_CLOAK:
		{
			/* Very good */
			if (power > 1)
			{
				if (one_in_(20) || (power > 2)) /* power > 2 is debug only */
				{
					create_artifact(o_ptr, FALSE);
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

			/* Very cursed */
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
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 * @return なし
 * @details
 * Hack -- note special "pval boost" code for ring of speed\n
 * Hack -- note that some items must be cursed (or blessed)\n
 */
static void a_m_aux_3(object_type *o_ptr, DEPTH level, int power)
{
	/* Apply magic (good or bad) according to type */
	switch (o_ptr->tval)
	{
		case TV_RING:
		{
			/* Analyze */
			switch (o_ptr->sval)
			{
				case SV_RING_ATTACKS:
				{
					/* Stat bonus */
					o_ptr->pval = (PARAMETER_VALUE)m_bonus(2, level);
					if (one_in_(15)) o_ptr->pval++;
					if (o_ptr->pval < 1) o_ptr->pval = 1;

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= (IDENT_BROKEN);

						/* Cursed */
						o_ptr->curse_flags |= TRC_CURSED;

						/* Reverse pval */
						o_ptr->pval = 0 - (o_ptr->pval);
					}

					break;
				}

				case SV_RING_SHOTS:
				{
					break;
				}

				/* Strength, Constitution, Dexterity, Intelligence */
				case SV_RING_STR:
				case SV_RING_CON:
				case SV_RING_DEX:
				{
					/* Stat bonus */
					o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(5, level);

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= (IDENT_BROKEN);

						/* Cursed */
						o_ptr->curse_flags |= TRC_CURSED;

						/* Reverse pval */
						o_ptr->pval = 0 - (o_ptr->pval);
					}

					break;
				}

				/* Ring of Speed! */
				case SV_RING_SPEED:
				{
					/* Base speed (1 to 10) */
					o_ptr->pval = randint1(5) + (PARAMETER_VALUE)m_bonus(5, level);

					/* Super-charge the ring */
					while (randint0(100) < 50) o_ptr->pval++;

					/* Cursed Ring */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= (IDENT_BROKEN);

						/* Cursed */
						o_ptr->curse_flags |= TRC_CURSED;

						/* Reverse pval */
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
					}
					while (one_in_(4));

					/* Bonus to armor class */
					o_ptr->to_a = 10 + randint1(5) + (ARMOUR_CLASS)m_bonus(10, level);
				}
				break;

				case SV_RING_WARNING:
				{
					if (one_in_(3)) one_low_esp(o_ptr);
					break;
				}

				/* Searching */
				case SV_RING_SEARCHING:
				{
					/* Bonus to searching */
					o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(5, level);

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= (IDENT_BROKEN);

						/* Cursed */
						o_ptr->curse_flags |= TRC_CURSED;

						/* Reverse pval */
						o_ptr->pval = 0 - (o_ptr->pval);
					}

					break;
				}

				/* Flames, Acid, Ice */
				case SV_RING_FLAMES:
				case SV_RING_ACID:
				case SV_RING_ICE:
				case SV_RING_ELEC:
				{
					/* Bonus to armor class */
					o_ptr->to_a = 5 + randint1(5) + (ARMOUR_CLASS)m_bonus(10, level);
					break;
				}

				/* Weakness, Stupidity */
				case SV_RING_WEAKNESS:
				case SV_RING_STUPIDITY:
				{
					/* Broken */
					o_ptr->ident |= (IDENT_BROKEN);

					/* Cursed */
					o_ptr->curse_flags |= TRC_CURSED;

					/* Penalize */
					o_ptr->pval = 0 - (1 + (PARAMETER_VALUE)m_bonus(5, level));
					if (power > 0) power = 0 - power;

					break;
				}

				/* WOE, Stupidity */
				case SV_RING_WOE:
				{
					/* Broken */
					o_ptr->ident |= (IDENT_BROKEN);

					/* Cursed */
					o_ptr->curse_flags |= TRC_CURSED;

					/* Penalize */
					o_ptr->to_a = 0 - (5 + (ARMOUR_CLASS)m_bonus(10, level));
					o_ptr->pval = 0 - (1 + (PARAMETER_VALUE)m_bonus(5, level));
					if (power > 0) power = 0 - power;

					break;
				}

				/* Ring of damage */
				case SV_RING_DAMAGE:
				{
					/* Bonus to damage */
					o_ptr->to_d = 1 + randint1(5) + (HIT_POINT)m_bonus(16, level);

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= (IDENT_BROKEN);

						/* Cursed */
						o_ptr->curse_flags |= TRC_CURSED;

						/* Reverse bonus */
						o_ptr->to_d = 0 - o_ptr->to_d;
					}

					break;
				}

				/* Ring of Accuracy */
				case SV_RING_ACCURACY:
				{
					/* Bonus to hit */
					o_ptr->to_h = 1 + randint1(5) + (HIT_PROB)m_bonus(16, level);

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= (IDENT_BROKEN);

						/* Cursed */
						o_ptr->curse_flags |= TRC_CURSED;

						/* Reverse tohit */
						o_ptr->to_h = 0 - o_ptr->to_h;
					}

					break;
				}

				/* Ring of Protection */
				case SV_RING_PROTECTION:
				{
					/* Bonus to armor class */
					o_ptr->to_a = 5 + randint1(8) + (ARMOUR_CLASS)m_bonus(10, level);

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= (IDENT_BROKEN);

						/* Cursed */
						o_ptr->curse_flags |= TRC_CURSED;

						/* Reverse toac */
						o_ptr->to_a = 0 - o_ptr->to_a;
					}

					break;
				}

				/* Ring of Slaying */
				case SV_RING_SLAYING:
				{
					/* Bonus to damage and to hit */
					o_ptr->to_d = randint1(5) + (HIT_POINT)m_bonus(12, level);
					o_ptr->to_h = randint1(5) + (HIT_PROB)m_bonus(12, level);

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= (IDENT_BROKEN);

						/* Cursed */
						o_ptr->curse_flags |= TRC_CURSED;

						/* Reverse bonuses */
						o_ptr->to_h = 0 - o_ptr->to_h;
						o_ptr->to_d = 0 - o_ptr->to_d;
					}

					break;
				}

				case SV_RING_MUSCLE:
				{
					o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(3, level);
					if (one_in_(4)) o_ptr->pval++;

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= (IDENT_BROKEN);

						/* Cursed */
						o_ptr->curse_flags |= TRC_CURSED;

						/* Reverse bonuses */
						o_ptr->pval = 0 - o_ptr->pval;
					}

					break;
				}
				case SV_RING_AGGRAVATION:
				{
					/* Broken */
					o_ptr->ident |= (IDENT_BROKEN);

					/* Cursed */
					o_ptr->curse_flags |= TRC_CURSED;

					if (power > 0) power = 0 - power;
					break;
				}
			}
			if ((one_in_(400) && (power > 0) && !object_is_cursed(o_ptr) && (level > 79))
			    || (power > 2)) /* power > 2 is debug only */
			{
				o_ptr->pval = MIN(o_ptr->pval, 4);
				/* Randart amulet */
				create_artifact(o_ptr, FALSE);
			}
			else if ((power == 2) && one_in_(2))
			{
				while(!o_ptr->name2)
				{
					int tmp = m_bonus(10, level);
					object_kind *k_ptr = &k_info[o_ptr->k_idx];
					switch(randint1(28))
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
								o_ptr->to_h -= 2+randint1(4);
								o_ptr->to_d += 2+randint1(4);
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
				if (o_ptr->to_h > 0) o_ptr->to_h = 0-o_ptr->to_h;
				if (o_ptr->to_d > 0) o_ptr->to_d = 0-o_ptr->to_d;
				if (o_ptr->to_a > 0) o_ptr->to_a = 0-o_ptr->to_a;
				if (o_ptr->pval > 0) o_ptr->pval = 0-o_ptr->pval;
				o_ptr->art_flags[0] = 0;
				o_ptr->art_flags[1] = 0;
				while(!o_ptr->name2)
				{
					object_kind *k_ptr = &k_info[o_ptr->k_idx];
					switch(randint1(5))
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
				/* Broken */
				o_ptr->ident |= (IDENT_BROKEN);

				/* Cursed */
				o_ptr->curse_flags |= (TRC_CURSED | TRC_HEAVY_CURSE);
			}
			break;
		}

		case TV_AMULET:
		{
			/* Analyze */
			switch (o_ptr->sval)
			{
				/* Amulet of wisdom/charisma */
				case SV_AMULET_INTELLIGENCE:
				case SV_AMULET_WISDOM:
				case SV_AMULET_CHARISMA:
				{
					o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(5, level);

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= (IDENT_BROKEN);

						/* Cursed */
						o_ptr->curse_flags |= (TRC_CURSED);

						/* Reverse bonuses */
						o_ptr->pval = 0 - o_ptr->pval;
					}

					break;
				}

				/* Amulet of brilliance */
				case SV_AMULET_BRILLIANCE:
				{
					o_ptr->pval = 1 + m_bonus(3, level);
					if (one_in_(4)) o_ptr->pval++;

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= (IDENT_BROKEN);

						/* Cursed */
						o_ptr->curse_flags |= (TRC_CURSED);

						/* Reverse bonuses */
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
				}
				break;

				/* Amulet of searching */
				case SV_AMULET_SEARCHING:
				{
					o_ptr->pval = randint1(2) + (PARAMETER_VALUE)m_bonus(4, level);

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= (IDENT_BROKEN);

						/* Cursed */
						o_ptr->curse_flags |= (TRC_CURSED);

						/* Reverse bonuses */
						o_ptr->pval = 0 - (o_ptr->pval);
					}

					break;
				}

				/* Amulet of the Magi -- never cursed */
				case SV_AMULET_THE_MAGI:
				{
					o_ptr->pval = randint1(5) + (PARAMETER_VALUE)m_bonus(5, level);
					o_ptr->to_a = randint1(5) + (ARMOUR_CLASS)m_bonus(5, level);

					/* gain one low ESP */
					add_esp_weak(o_ptr, FALSE);

					break;
				}

				/* Amulet of Doom -- always cursed */
				case SV_AMULET_DOOM:
				{
					/* Broken */
					o_ptr->ident |= (IDENT_BROKEN);

					/* Cursed */
					o_ptr->curse_flags |= (TRC_CURSED);

					/* Penalize */
					o_ptr->pval = 0 - (randint1(5) + (PARAMETER_VALUE)m_bonus(5, level));
					o_ptr->to_a = 0 - (randint1(5) + (ARMOUR_CLASS)m_bonus(5, level));
					if (power > 0) power = 0 - power;

					break;
				}

				case SV_AMULET_MAGIC_MASTERY:
				{
					o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(4, level);

					/* Cursed */
					if (power < 0)
					{
						/* Broken */
						o_ptr->ident |= (IDENT_BROKEN);

						/* Cursed */
						o_ptr->curse_flags |= (TRC_CURSED);

						/* Reverse bonuses */
						o_ptr->pval = 0 - o_ptr->pval;
					}

					break;
				}
			}
			if ((one_in_(150) && (power > 0) && !object_is_cursed(o_ptr) && (level > 79))
			    || (power > 2)) /* power > 2 is debug only */
			{
				o_ptr->pval = MIN(o_ptr->pval, 4);
				/* Randart amulet */
				create_artifact(o_ptr, FALSE);
			}
			else if ((power == 2) && one_in_(2))
			{
				while(!o_ptr->name2)
				{
					object_kind *k_ptr = &k_info[o_ptr->k_idx];
					switch(randint1(21))
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
				if (o_ptr->to_h > 0) o_ptr->to_h = 0-o_ptr->to_h;
				if (o_ptr->to_d > 0) o_ptr->to_d = 0-o_ptr->to_d;
				if (o_ptr->to_a > 0) o_ptr->to_a = 0-o_ptr->to_a;
				if (o_ptr->pval > 0) o_ptr->pval = 0-o_ptr->pval;
				o_ptr->art_flags[0] = 0;
				o_ptr->art_flags[1] = 0;
				while(!o_ptr->name2)
				{
					object_kind *k_ptr = &k_info[o_ptr->k_idx];
					switch(randint1(5))
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
				/* Broken */
				o_ptr->ident |= (IDENT_BROKEN);

				/* Cursed */
				o_ptr->curse_flags |= (TRC_CURSED | TRC_HEAVY_CURSE);
			}
			break;
		}
	}
}


/*!
 * @brief その他雑多のオブジェクトに生成ランクごとの強化を与えるサブルーチン
 * Apply magic to an item known to be "boring"
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 * @return なし
 * @details
 * Hack -- note the special code for various items
 */
static void a_m_aux_4(object_type *o_ptr, DEPTH level, int power)
{
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Unused */
	(void)level;

	/* Apply magic (good or bad) according to type */
	switch (o_ptr->tval)
	{
		case TV_WHISTLE:
		{
#if 0
			/* Cursed */
			if (power < 0)
			{
				/* Broken */
				o_ptr->ident |= (IDENT_BROKEN);

				/* Cursed */
				o_ptr->curse_flags |= (TRC_CURSED);
			}
#endif
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
			/* Hack -- Torches -- random fuel */
			if (o_ptr->sval == SV_LITE_TORCH)
			{
				if (o_ptr->pval > 0) o_ptr->xtra4 = randint1(o_ptr->pval);
				o_ptr->pval = 0;
			}

			/* Hack -- Lanterns -- random fuel */
			if (o_ptr->sval == SV_LITE_LANTERN)
			{
				if (o_ptr->pval > 0) o_ptr->xtra4 = randint1(o_ptr->pval);
				o_ptr->pval = 0;
			}

			if (power > 2) /* power > 2 is debug only */
			{
				create_artifact(o_ptr, FALSE);
			}
			else if ((power == 2) || ((power == 1) && one_in_(3)))
			{
				while (!o_ptr->name2)
				{
					while (1)
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
			/* Transfer the pval. -LM- */
			o_ptr->pval = k_ptr->pval;
			break;
		}

		case TV_CAPTURE:
		{
			o_ptr->pval = 0;
			object_aware(o_ptr);
			object_known(o_ptr);
			break;
		}

		case TV_FIGURINE:
		{
			PARAMETER_VALUE i = 1;
			int check;

			monster_race *r_ptr;

			/* Pick a random non-unique monster race */
			while (1)
			{
				i = randint1(max_r_idx - 1);

				if (!item_monster_okay(i)) continue;
				if (i == MON_TSUCHINOKO) continue;

				r_ptr = &r_info[i];

				check = (current_floor_ptr->dun_level < r_ptr->level) ? (r_ptr->level - current_floor_ptr->dun_level) : 0;

				/* Ignore dead monsters */
				if (!r_ptr->rarity) continue;

				/* Ignore uncommon monsters */
				if (r_ptr->rarity > 100) continue;

				/* Prefer less out-of-depth monsters */
				if (randint0(check)) continue;

				break;
			}

			o_ptr->pval = i;

			/* Some figurines are cursed */
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

			/* Hack -- Remove the monster restriction */
			get_mon_num_prep(item_monster_okay, NULL);

			/* Pick a random non-unique monster race */
			while (1)
			{
				i = get_mon_num(current_floor_ptr->dun_level);

				r_ptr = &r_info[i];

				check = (current_floor_ptr->dun_level < r_ptr->level) ? (r_ptr->level - current_floor_ptr->dun_level) : 0;

				/* Ignore dead monsters */
				if (!r_ptr->rarity) continue;

				/* Ignore corpseless monsters */
				if (!(r_ptr->flags9 & match)) continue;

				/* Prefer less out-of-depth monsters */
				if (randint0(check)) continue;

				break;
			}

			o_ptr->pval = i;


			object_aware(o_ptr);
			object_known(o_ptr);
			break;
		}

		case TV_STATUE:
		{
			PARAMETER_VALUE i = 1;

			monster_race *r_ptr;

			/* Pick a random monster race */
			while (1)
			{
				i = randint1(max_r_idx - 1);

				r_ptr = &r_info[i];

				/* Ignore dead monsters */
				if (!r_ptr->rarity) continue;

				break;
			}

			o_ptr->pval = i;

			if (cheat_peek)
			{
				msg_format(_("%sの像", "Statue of %s"), r_name + r_ptr->name);
			}
			object_aware(o_ptr);
			object_known(o_ptr);

			break;
		}

		case TV_CHEST:
		{
			DEPTH obj_level = k_info[o_ptr->k_idx].level;

			/* Hack -- skip ruined chests */
			if (obj_level <= 0) break;

			/* Hack -- pick a "difficulty" */
			o_ptr->pval = randint1(obj_level);
			if (o_ptr->sval == SV_CHEST_KANDUME) o_ptr->pval = 6;

			o_ptr->xtra3 = current_floor_ptr->dun_level + 5;

			/* Never exceed "difficulty" of 55 to 59 */
			if (o_ptr->pval > 55) o_ptr->pval = 55 + (byte)randint0(5);

			break;
		}
	}
}

/*!
 * @brief 生成されたベースアイテムに魔法的な強化を与えるメインルーチン
 * Complete the "creation" of an object by applying "magic" to the item
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
void apply_magic(object_type *o_ptr, DEPTH lev, BIT_FLAGS mode)
{
	int i, rolls, f1, f2, power;

	if (p_ptr->pseikaku == SEIKAKU_MUNCHKIN) lev += randint0(p_ptr->lev/2+10);

	/* Maximum "level" for various things */
	if (lev > MAX_DEPTH - 1) lev = MAX_DEPTH - 1;

	/* Base chance of being "good" */
	f1 = lev + 10;

	/* Maximal chance of being "good" */
	if (f1 > d_info[p_ptr->dungeon_idx].obj_good) f1 = d_info[p_ptr->dungeon_idx].obj_good;

	/* Base chance of being "great" */
	f2 = f1 * 2 / 3;

	/* Maximal chance of being "great" */
	if ((p_ptr->pseikaku != SEIKAKU_MUNCHKIN) && (f2 > d_info[p_ptr->dungeon_idx].obj_great))
		f2 = d_info[p_ptr->dungeon_idx].obj_great;

	if (p_ptr->muta3 & MUT3_GOOD_LUCK)
	{
		f1 += 5;
		f2 += 2;
	}
	else if(p_ptr->muta3 & MUT3_BAD_LUCK)
	{
		f1 -= 5;
		f2 -= 2;
	}

	/* Assume normal */
	power = 0;

	/* Roll for "good" */
	if ((mode & AM_GOOD) || magik(f1))
	{
		/* Assume "good" */
		power = 1;

		/* Roll for "great" */
		if ((mode & AM_GREAT) || magik(f2))
		{
			power = 2;

			/* Roll for "special" */
			if (mode & AM_SPECIAL) power = 3;
		}
	}

	/* Roll for "cursed" */
	else if (magik(f1))
	{
		/* Assume "cursed" */
		power = -1;

		/* Roll for "broken" */
		if (magik(f2)) power = -2;
	}

	/* Apply curse */
	if (mode & AM_CURSED)
	{
		/* Assume 'cursed' */
		if (power > 0)
		{
			power = 0 - power;
		}
		/* Everything else gets more badly cursed */
		else
		{
			power--;
		}
	}

	/* Assume no rolls */
	rolls = 0;

	/* Get one roll if excellent */
	if (power >= 2) rolls = 1;

	/* Hack -- Get four rolls if forced great or special */
	if (mode & (AM_GREAT | AM_SPECIAL)) rolls = 4;

	/* Hack -- Get no rolls if not allowed */
	if ((mode & AM_NO_FIXED_ART) || o_ptr->name1) rolls = 0;

	/* Roll for artifacts if allowed */
	for (i = 0; i < rolls; i++)
	{
		/* Roll for an artifact */
		if (make_artifact(o_ptr)) break;
		if ((p_ptr->muta3 & MUT3_GOOD_LUCK) && one_in_(77))
		{
			if (make_artifact(o_ptr)) break;
		}
	}


	/* Hack -- analyze artifacts */
	if (object_is_fixed_artifact(o_ptr))
	{
		artifact_type *a_ptr = &a_info[o_ptr->name1];

		/* Hack -- Mark the artifact as "created" */
		a_ptr->cur_num = 1;

		/* Hack -- Memorize location of artifact in saved floors */
		if (current_world_ptr->character_dungeon)
			a_ptr->floor_id = p_ptr->floor_id;

		/* Extract the other fields */
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
		    if(p_ptr->pseikaku == SEIKAKU_SEXY)
		    {
			o_ptr->pval = 3;
		    }
		}

		/* Hack -- extract the "broken" flag */
		if (!a_ptr->cost) o_ptr->ident |= (IDENT_BROKEN);

		/* Hack -- extract the "cursed" flag */
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
			if (power) apply_magic_weapon(o_ptr, lev, power);
			break;
		}

		case TV_POLEARM:
		{
			if (power && !(o_ptr->sval == SV_DEATH_SCYTHE)) apply_magic_weapon(o_ptr, lev, power);
			break;
		}

		case TV_SWORD:
		{
			if (power && !(o_ptr->sval == SV_DOKUBARI)) apply_magic_weapon(o_ptr, lev, power);
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
			/* Elven Cloak and Black Clothes ... */
			if (((o_ptr->tval == TV_CLOAK) && (o_ptr->sval == SV_ELVEN_CLOAK)) ||
			    ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_KUROSHOUZOKU)))
				o_ptr->pval = randint1(4);

#if 1
			if (power ||
			     ((o_ptr->tval == TV_HELM) && (o_ptr->sval == SV_DRAGON_HELM)) ||
			     ((o_ptr->tval == TV_SHIELD) && (o_ptr->sval == SV_DRAGON_SHIELD)) ||
			     ((o_ptr->tval == TV_GLOVES) && (o_ptr->sval == SV_SET_OF_DRAGON_GLOVES)) ||
			     ((o_ptr->tval == TV_BOOTS) && (o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE)))
				a_m_aux_2(o_ptr, lev, power);
#else
			if (power) a_m_aux_2(o_ptr, lev, power);
#endif
			break;
		}

		case TV_RING:
		case TV_AMULET:
		{
			if (!power && (randint0(100) < 50)) power = -1;
			a_m_aux_3(o_ptr, lev, power);
			break;
		}

		default:
		{
			a_m_aux_4(o_ptr, lev, power);
			break;
		}
	}

	if ((o_ptr->tval == TV_SOFT_ARMOR) &&
	    (o_ptr->sval == SV_ABUNAI_MIZUGI) &&
	    (p_ptr->pseikaku == SEIKAKU_SEXY))
	{
		o_ptr->pval = 3;
		add_flag(o_ptr->art_flags, TR_STR);
		add_flag(o_ptr->art_flags, TR_INT);
		add_flag(o_ptr->art_flags, TR_WIS);
		add_flag(o_ptr->art_flags, TR_DEX);
		add_flag(o_ptr->art_flags, TR_CON);
		add_flag(o_ptr->art_flags, TR_CHR);
	}

	/* Hack -- analyze ego-items */
	if (object_is_ego(o_ptr))
	{
		ego_item_type *e_ptr = &e_info[o_ptr->name2];

		/* Hack -- acquire "broken" flag */
		if (!e_ptr->cost) o_ptr->ident |= (IDENT_BROKEN);

		/* Hack -- acquire "cursed" flag */
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
			}
			while (one_in_(o_ptr->dd));

			if (o_ptr->dd > 9) o_ptr->dd = 9;
		}

		/* Hack -- apply activatin index if needed */
		if (e_ptr->act_idx) o_ptr->xtra2 = (XTRA8)e_ptr->act_idx;

		/* Hack -- apply extra penalties if needed */
		if ((object_is_cursed(o_ptr) || object_is_broken(o_ptr)) && !(e_ptr->gen_flags & (TRG_POWERFUL)))
		{
			/* Hack -- obtain bonuses */
			if (e_ptr->max_to_h) o_ptr->to_h -= randint1(e_ptr->max_to_h);
			if (e_ptr->max_to_d) o_ptr->to_d -= randint1(e_ptr->max_to_d);
			if (e_ptr->max_to_a) o_ptr->to_a -= randint1(e_ptr->max_to_a);

			/* Hack -- obtain pval */
			if (e_ptr->max_pval) o_ptr->pval -= randint1(e_ptr->max_pval);
		}

		/* Hack -- apply extra bonuses if needed */
		else
		{
			/* Hack -- obtain bonuses */
			if (e_ptr->max_to_h)
			{
				if (e_ptr->max_to_h > 127)
					o_ptr->to_h -= randint1(256-e_ptr->max_to_h);
				else o_ptr->to_h += randint1(e_ptr->max_to_h);
			}
			if (e_ptr->max_to_d)
			{
				if (e_ptr->max_to_d > 127)
					o_ptr->to_d -= randint1(256-e_ptr->max_to_d);
				else o_ptr->to_d += randint1(e_ptr->max_to_d);
			}
			if (e_ptr->max_to_a)
			{
				if (e_ptr->max_to_a > 127)
					o_ptr->to_a -= randint1(256-e_ptr->max_to_a);
				else o_ptr->to_a += randint1(e_ptr->max_to_a);
			}
			
			/* Accuracy ego must have high to_h */
			if(o_ptr->name2 == EGO_ACCURACY)
			{
				while(o_ptr->to_h < o_ptr->to_d + 10)
				{
					o_ptr->to_h += 5;
					o_ptr->to_d -= 5;
				}
				o_ptr->to_h = MAX(o_ptr->to_h, 15);
			}
			
			/* Accuracy ego must have high to_h */
			if(o_ptr->name2 == EGO_VELOCITY)
			{
				while(o_ptr->to_d < o_ptr->to_h + 10)
				{
					o_ptr->to_d += 5;
					o_ptr->to_h -= 5;
				}
				o_ptr->to_d = MAX(o_ptr->to_d, 15);
			}
			
			/* Protection ego must have high to_a */
			if((o_ptr->name2 == EGO_PROTECTION) || (o_ptr->name2 == EGO_S_PROTECTION) || (o_ptr->name2 == EGO_H_PROTECTION))
			{
				o_ptr->to_a = MAX(o_ptr->to_a, 15);
			}

			/* Hack -- obtain pval */
			if (e_ptr->max_pval)
			{
				if ((o_ptr->name2 == EGO_HA) && (have_flag(o_ptr->art_flags, TR_BLOWS)))
				{
					o_ptr->pval++;
					if ((lev > 60) && one_in_(3) && ((o_ptr->dd*(o_ptr->ds+1)) < 15)) o_ptr->pval++;
				}
				else if (o_ptr->name2 == EGO_DEMON)
				{
					if(have_flag(o_ptr->art_flags, TR_BLOWS))
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
					o_ptr->pval = randint1(e_ptr->max_pval*lev/100+1);
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

	/* Examine real objects */
	if (o_ptr->k_idx)
	{
		object_kind *k_ptr = &k_info[o_ptr->k_idx];

		/* Hack -- acquire "broken" flag */
		if (!k_info[o_ptr->k_idx].cost) o_ptr->ident |= (IDENT_BROKEN);

		/* Hack -- acquire "cursed" flag */
		if (k_ptr->gen_flags & (TRG_CURSED)) o_ptr->curse_flags |= (TRC_CURSED);
		if (k_ptr->gen_flags & (TRG_HEAVY_CURSE)) o_ptr->curse_flags |= TRC_HEAVY_CURSE;
		if (k_ptr->gen_flags & (TRG_PERMA_CURSE)) o_ptr->curse_flags |= TRC_PERMA_CURSE;
		if (k_ptr->gen_flags & (TRG_RANDOM_CURSE0)) o_ptr->curse_flags |= get_curse(0, o_ptr);
		if (k_ptr->gen_flags & (TRG_RANDOM_CURSE1)) o_ptr->curse_flags |= get_curse(1, o_ptr);
		if (k_ptr->gen_flags & (TRG_RANDOM_CURSE2)) o_ptr->curse_flags |= get_curse(2, o_ptr);
	}

	
}


/*!
 * @brief 生成階に応じたベースアイテムの生成を行う。
 * Attempt to make an object (normal or good/great)
 * @param j_ptr 生成結果を収めたいオブジェクト構造体の参照ポインタ
 * @param mode オプションフラグ
 * @return 生成に成功したらTRUEを返す。
 * @details
 * This routine plays nasty games to generate the "special artifacts".\n
 * This routine uses "current_floor_ptr->object_level" for the "generation level".\n
 * We assume that the given object has been "wiped".\n
 */
bool make_object(object_type *j_ptr, BIT_FLAGS mode)
{
	PERCENTAGE prob;
	DEPTH base;


	/* Chance of "special object" */
	prob = ((mode & AM_GOOD) ? 10 : 1000);

	/* Base level for the object */
	base = ((mode & AM_GOOD) ? (current_floor_ptr->object_level + 10) : current_floor_ptr->object_level);


	/* Generate a special object, or a normal object */
	if (!one_in_(prob) || !make_artifact_special(j_ptr))
	{
		KIND_OBJECT_IDX k_idx;

		/* Good objects */
		if ((mode & AM_GOOD) && !get_obj_num_hook)
		{
			/* Activate restriction (if already specified, use that) */
			get_obj_num_hook = kind_is_good;
		}

		/* Restricted objects - prepare allocation table */
		if (get_obj_num_hook) get_obj_num_prep();

		/* Pick a random object */
		k_idx = get_obj_num(base, mode);

		/* Restricted objects */
		if (get_obj_num_hook)
		{
			/* Clear restriction */
			get_obj_num_hook = NULL;

			/* Reset allocation table to default */
			get_obj_num_prep();
		}

		/* Handle failure */
		if (!k_idx) return (FALSE);

		/* Prepare the object */
		object_prep(j_ptr, k_idx);
	}

	/* Apply magic (allow artifacts) */
	apply_magic(j_ptr, current_floor_ptr->object_level, mode);

	/* Hack -- generate multiple spikes/missiles */
	switch (j_ptr->tval)
	{
		case TV_SPIKE:
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		{
			if (!j_ptr->name1)
				j_ptr->number = (byte)damroll(6, 7);
		}
	}

	if (cheat_peek) object_mention(j_ptr);

	/* Success */
	return (TRUE);
}


/*!
 * @brief フロアの指定位置に生成階に応じたベースアイテムの生成を行う。
 * Attempt to place an object (normal or good/great) at the given location.
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 * @param mode オプションフラグ
 * @return 生成に成功したらTRUEを返す。
 * @details
 * This routine plays nasty games to generate the "special artifacts".\n
 * This routine uses "current_floor_ptr->object_level" for the "generation level".\n
 * This routine requires a clean floor grid destination.\n
 */
void place_object(POSITION y, POSITION x, BIT_FLAGS mode)
{
	OBJECT_IDX o_idx;

	/* Acquire grid */
	grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];

	object_type forge;
	object_type *q_ptr;


	/* Paranoia -- check bounds */
	if (!in_bounds(y, x)) return;

	/* Require floor space */
	if (!cave_drop_bold(y, x)) return;

	/* Avoid stacking on other objects */
	if (g_ptr->o_idx) return;

	q_ptr = &forge;
	object_wipe(q_ptr);

	/* Make an object (if possible) */
	if (!make_object(q_ptr, mode)) return;

	o_idx = o_pop();

	/* Success */
	if (o_idx)
	{
		object_type *o_ptr;
		o_ptr = &current_floor_ptr->o_list[o_idx];

		/* Structure Copy */
		object_copy(o_ptr, q_ptr);

		o_ptr->iy = y;
		o_ptr->ix = x;

		/* Build a stack */
		o_ptr->next_o_idx = g_ptr->o_idx;

		g_ptr->o_idx = o_idx;
		note_spot(y, x);
		lite_spot(y, x);
	}
	else
	{
		/* Hack -- Preserve artifacts */
		if (object_is_fixed_artifact(q_ptr))
		{
			a_info[q_ptr->name1].cur_num = 0;
		}
	}
}


OBJECT_SUBTYPE_VALUE coin_type;	/* Hack -- force coin type */

/*!
 * @brief 生成階に応じた財宝オブジェクトの生成を行う。
 * Make a treasure object
 * @param j_ptr 生成結果を収めたいオブジェクト構造体の参照ポインタ
 * @return 生成に成功したらTRUEを返す。
 * @details
 * The location must be a legal, clean, floor grid.
 */
bool make_gold(object_type *j_ptr)
{
	int i;
	s32b base;

	/* Hack -- Pick a Treasure variety */
	i = ((randint1(current_floor_ptr->object_level + 2) + 2) / 2) - 1;

	/* Apply "extra" magic */
	if (one_in_(GREAT_OBJ))
	{
		i += randint1(current_floor_ptr->object_level + 1);
	}

	/* Hack -- Creeping Coins only generate "themselves" */
	if (coin_type) i = coin_type;

	/* Do not create "illegal" Treasure Types */
	if (i >= MAX_GOLD) i = MAX_GOLD - 1;

	/* Prepare a gold object */
	object_prep(j_ptr, OBJ_GOLD_LIST + i);

	/* Hack -- Base coin cost */
	base = k_info[OBJ_GOLD_LIST + i].cost;

	/* Determine how much the treasure is "worth" */
	j_ptr->pval = (base + (8L * randint1(base)) + randint1(8));

	/* Success */
	return (TRUE);
}


/*!
 * @brief フロアの指定位置に生成階に応じた財宝オブジェクトの生成を行う。
 * Places a treasure (Gold or Gems) at given location
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 * @return 生成に成功したらTRUEを返す。
 * @details
 * The location must be a legal, clean, floor grid.
 */
void place_gold(POSITION y, POSITION x)
{
	OBJECT_IDX o_idx;

	/* Acquire grid */
	grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];

	object_type forge;
	object_type *q_ptr;


	/* Paranoia -- check bounds */
	if (!in_bounds(y, x)) return;

	/* Require floor space */
	if (!cave_drop_bold(y, x)) return;

	/* Avoid stacking on other objects */
	if (g_ptr->o_idx) return;

	q_ptr = &forge;
	object_wipe(q_ptr);

	/* Make some gold */
	if (!make_gold(q_ptr)) return;

	o_idx = o_pop();

	/* Success */
	if (o_idx)
	{
		object_type *o_ptr;
		o_ptr = &current_floor_ptr->o_list[o_idx];
		object_copy(o_ptr, q_ptr);

		/* Save location */
		o_ptr->iy = y;
		o_ptr->ix = x;

		/* Build a stack */
		o_ptr->next_o_idx = g_ptr->o_idx;

		g_ptr->o_idx = o_idx;
		note_spot(y, x);
		lite_spot(y, x);
	}
}


/*!
 * @brief 生成済のオブジェクトをフロアの所定の位置に落とす。
 * Let an object fall to the ground at or near a location.
 * @param j_ptr 落としたいオブジェクト構造体の参照ポインタ
 * @param chance ドロップの消滅率(%)
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 * @return 生成に成功したらオブジェクトのIDを返す。
 * @details
 * The initial location is assumed to be "in_bounds()".\n
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
OBJECT_IDX drop_near(object_type *j_ptr, PERCENTAGE chance, POSITION y, POSITION x)
{
	int i, k, d, s;

	int bs, bn;
	POSITION by, bx;
	POSITION dy, dx;
	POSITION ty, tx = 0;

	OBJECT_IDX o_idx = 0;
	OBJECT_IDX this_o_idx, next_o_idx = 0;

	grid_type *g_ptr;

	GAME_TEXT o_name[MAX_NLEN];

	bool flag = FALSE;
	bool done = FALSE;

#ifndef JP
	/* Extract plural */
	bool plural = (j_ptr->number != 1);
#endif

	/* Describe object */
	object_desc(o_name, j_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));


	/* Handle normal "breakage" */
	if (!object_is_artifact(j_ptr) && (randint0(100) < chance))
	{
#ifdef JP
		msg_format("%sは消えた。", o_name);
#else
		msg_format("The %s disappear%s.", o_name, (plural ? "" : "s"));
#endif
		if (p_ptr->wizard) msg_print(_("(破損)", "(breakage)"));

		/* Failure */
		return (0);
	}


	/* Score */
	bs = -1;

	/* Picker */
	bn = 0;

	/* Default */
	by = y;
	bx = x;

	/* Scan local grids */
	for (dy = -3; dy <= 3; dy++)
	{
		/* Scan local grids */
		for (dx = -3; dx <= 3; dx++)
		{
			bool comb = FALSE;

			/* Calculate actual distance */
			d = (dy * dy) + (dx * dx);

			/* Ignore distant grids */
			if (d > 10) continue;

			ty = y + dy;
			tx = x + dx;

			/* Skip illegal grids */
			if (!in_bounds(ty, tx)) continue;

			/* Require line of projection */
			if (!projectable(y, x, ty, tx)) continue;

			/* Obtain grid */
			g_ptr = &current_floor_ptr->grid_array[ty][tx];

			/* Require floor space */
			if (!cave_drop_bold(ty, tx)) continue;

			/* No objects */
			k = 0;

			/* Scan objects in that grid */
			for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
			{
				object_type *o_ptr;
				o_ptr = &current_floor_ptr->o_list[this_o_idx];
				next_o_idx = o_ptr->next_o_idx;

				/* Check for possible combination */
				if (object_similar(o_ptr, j_ptr)) comb = TRUE;

				/* Count objects */
				k++;
			}

			/* Add new object */
			if (!comb) k++;
			if (k > 99) continue;

			/* Calculate score */
			s = 1000 - (d + k * 5);

			/* Skip bad values */
			if (s < bs) continue;

			/* New best value */
			if (s > bs) bn = 0;

			/* Apply the randomizer to equivalent values */
			if ((++bn >= 2) && !one_in_(bn)) continue;

			/* Keep score */
			bs = s;

			/* Track it */
			by = ty;
			bx = tx;

			flag = TRUE;
		}
	}


	/* Handle lack of space */
	if (!flag && !object_is_artifact(j_ptr))
	{
#ifdef JP
		msg_format("%sは消えた。", o_name);
#else
		msg_format("The %s disappear%s.", o_name, (plural ? "" : "s"));
#endif

		if (p_ptr->wizard) msg_print(_("(床スペースがない)", "(no floor space)"));

		/* Failure */
		return (0);
	}


	/* Find a grid */
	for (i = 0; !flag && (i < 1000); i++)
	{
		/* Bounce around */
		ty = rand_spread(by, 1);
		tx = rand_spread(bx, 1);

		if (!in_bounds(ty, tx)) continue;

		/* Bounce to that location */
		by = ty;
		bx = tx;

		/* Require floor space */
		if (!cave_drop_bold(by, bx)) continue;

		flag = TRUE;
	}


	if (!flag)
	{
		int candidates = 0, pick;

		for (ty = 1; ty < current_floor_ptr->height - 1; ty++)
		{
			for (tx = 1; tx < current_floor_ptr->width - 1; tx++)
			{
				/* A valid space found */
				if (cave_drop_bold(ty, tx)) candidates++;
			}
		}

		/* No valid place! */
		if (!candidates)
		{
#ifdef JP
			msg_format("%sは消えた。", o_name);
#else
			msg_format("The %s disappear%s.", o_name, (plural ? "" : "s"));
#endif

			if (p_ptr->wizard) msg_print(_("(床スペースがない)", "(no floor space)"));

			/* Mega-Hack -- preserve artifacts */
			if (preserve_mode)
			{
				/* Hack -- Preserve unknown artifacts */
				if (object_is_fixed_artifact(j_ptr) && !object_is_known(j_ptr))
				{
					/* Mega-Hack -- Preserve the artifact */
					a_info[j_ptr->name1].cur_num = 0;
				}
			}

			/* Failure */
			return 0;
		}

		/* Choose a random one */
		pick = randint1(candidates);

		for (ty = 1; ty < current_floor_ptr->height - 1; ty++)
		{
			for (tx = 1; tx < current_floor_ptr->width - 1; tx++)
			{
				if (cave_drop_bold(ty, tx))
				{
					pick--;

					/* Is this a picked one? */
					if (!pick) break;
				}
			}

			if (!pick) break;
		}

		by = ty;
		bx = tx;
	}


	g_ptr = &current_floor_ptr->grid_array[by][bx];

	/* Scan objects in that grid for combination */
	for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;
		o_ptr = &current_floor_ptr->o_list[this_o_idx];
		next_o_idx = o_ptr->next_o_idx;

		/* Check for combination */
		if (object_similar(o_ptr, j_ptr))
		{
			object_absorb(o_ptr, j_ptr);

			/* Success */
			done = TRUE;

			break;
		}
	}

	if (!done) o_idx = o_pop();

	/* Failure */
	if (!done && !o_idx)
	{
#ifdef JP
		msg_format("%sは消えた。", o_name);
#else
		msg_format("The %s disappear%s.", o_name, (plural ? "" : "s"));
#endif

		if (p_ptr->wizard) msg_print(_("(アイテムが多過ぎる)", "(too many objects)"));

		/* Hack -- Preserve artifacts */
		if (object_is_fixed_artifact(j_ptr))
		{
			a_info[j_ptr->name1].cur_num = 0;
		}

		/* Failure */
		return (0);
	}

	/* Stack */
	if (!done)
	{
		/* Structure copy */
		object_copy(&current_floor_ptr->o_list[o_idx], j_ptr);

		/* Access new object */
		j_ptr = &current_floor_ptr->o_list[o_idx];

		/* Locate */
		j_ptr->iy = by;
		j_ptr->ix = bx;

		/* No monster */
		j_ptr->held_m_idx = 0;

		/* Build a stack */
		j_ptr->next_o_idx = g_ptr->o_idx;

		g_ptr->o_idx = o_idx;

		/* Success */
		done = TRUE;
	}

	note_spot(by, bx);
	lite_spot(by, bx);
	sound(SOUND_DROP);

	/* Mega-Hack -- no message if "dropped" by player */
	/* Message when an object falls under the player */
	if (chance && player_bold(by, bx))
	{
		msg_print(_("何かが足下に転がってきた。", "You feel something roll beneath your feet."));
	}

	return (o_idx);
}

/*!
 * @brief 魔道具の使用回数の残量を示すメッセージを表示する /
 * Describe the charges on an item in the p_ptr->inventory_list.
 * @param item 残量を表示したいプレイヤーのアイテム所持スロット
 * @return なし
 */
void inven_item_charges(INVENTORY_IDX item)
{
	object_type *o_ptr = &p_ptr->inventory_list[item];

	/* Require staff/wand */
	if ((o_ptr->tval != TV_STAFF) && (o_ptr->tval != TV_WAND)) return;

	/* Require known item */
	if (!object_is_known(o_ptr)) return;

#ifdef JP
	if (o_ptr->pval <= 0)
	{
		msg_print("もう魔力が残っていない。");
	}
	else
	{
		msg_format("あと %d 回分の魔力が残っている。", o_ptr->pval);
	}
#else
	/* Multiple charges */
	if (o_ptr->pval != 1)
	{
		msg_format("You have %d charges remaining.", o_ptr->pval);
	}

	/* Single charge */
	else
	{
		msg_format("You have %d charge remaining.", o_ptr->pval);
	}
#endif

}

/*!
 * @brief アイテムの残り所持数メッセージを表示する /
 * Describe an item in the p_ptr->inventory_list.
 * @param item 残量を表示したいプレイヤーのアイテム所持スロット
 * @return なし
 */
void inven_item_describe(INVENTORY_IDX item)
{
	object_type *o_ptr = &p_ptr->inventory_list[item];
	GAME_TEXT o_name[MAX_NLEN];

	object_desc(o_name, o_ptr, 0);

#ifdef JP
	/* "no more" の場合はこちらで表示する */
	if (o_ptr->number <= 0)
	{
		/*FIRST*//*ここはもう通らないかも */
		msg_format("もう%sを持っていない。", o_name);
	}
	else
	{
		/* アイテム名を英日切り替え機能対応 */
		msg_format("まだ %sを持っている。", o_name);
	}
#else
	msg_format("You have %s.", o_name);
#endif

}

/*!
 * @brief アイテムを増減させ残り所持数メッセージを表示する /
 * Increase the "number" of an item in the p_ptr->inventory_list
 * @param item 所持数を増やしたいプレイヤーのアイテム所持スロット
 * @param num 増やしたい量
 * @return なし
 */
void inven_item_increase(INVENTORY_IDX item, ITEM_NUMBER num)
{
	object_type *o_ptr = &p_ptr->inventory_list[item];

	/* Apply */
	num += o_ptr->number;

	/* Bounds check */
	if (num > 255) num = 255;
	else if (num < 0) num = 0;

	/* Un-apply */
	num -= o_ptr->number;

	/* Change the number and weight */
	if (num)
	{
		/* Add the number */
		o_ptr->number += num;

		/* Add the weight */
		p_ptr->total_weight += (num * o_ptr->weight);
		p_ptr->update |= (PU_BONUS);
		p_ptr->update |= (PU_MANA);
		p_ptr->update |= (PU_COMBINE);
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Hack -- Clear temporary elemental brands if player takes off weapons */
		if (!o_ptr->number && p_ptr->ele_attack)
		{
			if ((item == INVEN_RARM) || (item == INVEN_LARM))
			{
				if (!has_melee_weapon(INVEN_RARM + INVEN_LARM - item))
				{
					/* Clear all temporary elemental brands */
					set_ele_attack(0, 0);
				}
			}
		}
	}
}

/*!
 * @brief 所持アイテムスロットから所持数のなくなったアイテムを消去する /
 * Erase an p_ptr->inventory_list slot if it has no more items
 * @param item 消去したいプレイヤーのアイテム所持スロット
 * @return なし
 */
void inven_item_optimize(INVENTORY_IDX item)
{
	object_type *o_ptr = &p_ptr->inventory_list[item];

	/* Only optimize real items */
	if (!o_ptr->k_idx) return;

	/* Only optimize empty items */
	if (o_ptr->number) return;

	/* The item is in the pack */
	if (item < INVEN_RARM)
	{
		int i;

		/* One less item */
		p_ptr->inven_cnt--;

		/* Slide everything down */
		for (i = item; i < INVEN_PACK; i++)
		{
			/* Structure copy */
			p_ptr->inventory_list[i] = p_ptr->inventory_list[i+1];
		}

		/* Erase the "final" slot */
		object_wipe(&p_ptr->inventory_list[i]);

		p_ptr->window |= (PW_INVEN);
	}

	/* The item is being wielded */
	else
	{
		/* One less item */
		p_ptr->equip_cnt--;

		/* Erase the empty slot */
		object_wipe(&p_ptr->inventory_list[item]);
		p_ptr->update |= (PU_BONUS);
		p_ptr->update |= (PU_TORCH);
		p_ptr->update |= (PU_MANA);

		p_ptr->window |= (PW_EQUIP);
	}

	p_ptr->window |= (PW_SPELL);
}

/*!
 * @brief 床上の魔道具の残り残量メッセージを表示する /
 * Describe the charges on an item on the floor.
 * @param item メッセージの対象にしたいアイテム所持スロット
 * @return なし
 */
void floor_item_charges(INVENTORY_IDX item)
{
	object_type *o_ptr = &current_floor_ptr->o_list[item];

	/* Require staff/wand */
	if ((o_ptr->tval != TV_STAFF) && (o_ptr->tval != TV_WAND)) return;

	/* Require known item */
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
	/* Multiple charges */
	if (o_ptr->pval != 1)
	{
		msg_format("There are %d charges remaining.", o_ptr->pval);
	}

	/* Single charge */
	else
	{
		msg_format("There is %d charge remaining.", o_ptr->pval);
	}
#endif

}

/*!
 * @brief 床上のアイテムの残り数メッセージを表示する /
 * Describe the charges on an item on the floor.
 * @param item メッセージの対象にしたいアイテム所持スロット
 * @return なし
 */
void floor_item_describe(INVENTORY_IDX item)
{
	object_type *o_ptr = &current_floor_ptr->o_list[item];
	GAME_TEXT o_name[MAX_NLEN];

	object_desc(o_name, o_ptr, 0);

#ifdef JP
	/* "no more" の場合はこちらで表示を分ける */
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
 * @brief 床上のアイテムの数を増やす /
 * Increase the "number" of an item on the floor
 * @param item 増やしたいアイテムの所持スロット
 * @param num 増やしたいアイテムの数
 * @return なし
 */
void floor_item_increase(INVENTORY_IDX item, ITEM_NUMBER num)
{
	object_type *o_ptr = &current_floor_ptr->o_list[item];

	/* Apply */
	num += o_ptr->number;

	/* Bounds check */
	if (num > 255) num = 255;
	else if (num < 0) num = 0;

	/* Un-apply */
	num -=  o_ptr->number;

	/* Change the number */
	o_ptr->number += num;
}


/*!
 * @brief 床上の数の無くなったアイテムスロットを消去する /
 * Optimize an item on the floor (destroy "empty" items)
 * @param item 消去したいアイテムの所持スロット
 * @return なし
 */
void floor_item_optimize(INVENTORY_IDX item)
{
	object_type *o_ptr = &current_floor_ptr->o_list[item];

	/* Paranoia -- be sure it exists */
	if (!o_ptr->k_idx) return;

	/* Only optimize empty items */
	if (o_ptr->number) return;

	delete_object_idx(item);
}


/*!
 * @brief アイテムを拾う際にザックから溢れずに済むかを判定する /
 * Check if we have space for an item in the pack without overflow
 * @param o_ptr 拾いたいオブジェクトの構造体参照ポインタ
 * @return 溢れずに済むならTRUEを返す
 */
bool inven_carry_okay(object_type *o_ptr)
{
	int j;

	/* Empty slot? */
	if (p_ptr->inven_cnt < INVEN_PACK) return (TRUE);

	/* Similar slot? */
	for (j = 0; j < INVEN_PACK; j++)
	{
		object_type *j_ptr = &p_ptr->inventory_list[j];
		if (!j_ptr->k_idx) continue;

		/* Check if the two items can be combined */
		if (object_similar(j_ptr, o_ptr)) return (TRUE);
	}

	return (FALSE);
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

	/* Use empty slots */
	if (!j_ptr->k_idx) return TRUE;

	/* Hack -- readable books always come first */
	if ((o_ptr->tval == REALM1_BOOK) &&
	    (j_ptr->tval != REALM1_BOOK)) return TRUE;
	if ((j_ptr->tval == REALM1_BOOK) &&
	    (o_ptr->tval != REALM1_BOOK)) return FALSE;

	if ((o_ptr->tval == REALM2_BOOK) &&
	    (j_ptr->tval != REALM2_BOOK)) return TRUE;
	if ((j_ptr->tval == REALM2_BOOK) &&
	    (o_ptr->tval != REALM2_BOOK)) return FALSE;

	/* Objects sort by decreasing type */
	if (o_ptr->tval > j_ptr->tval) return TRUE;
	if (o_ptr->tval < j_ptr->tval) return FALSE;

	/* Non-aware (flavored) items always come last */
	/* Can happen in the home */
	if (!object_is_aware(o_ptr)) return FALSE;
	if (!object_is_aware(j_ptr)) return TRUE;

	/* Objects sort by increasing sval */
	if (o_ptr->sval < j_ptr->sval) return TRUE;
	if (o_ptr->sval > j_ptr->sval) return FALSE;

	/* Unidentified objects always come last */
	/* Objects in the home can be unknown */
	if (!object_is_known(o_ptr)) return FALSE;
	if (!object_is_known(j_ptr)) return TRUE;

	/* Fixed artifacts, random artifacts and ego items */
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
		/* Objects sort by increasing hit/damage bonuses */
		if (o_ptr->to_h + o_ptr->to_d < j_ptr->to_h + j_ptr->to_d) return TRUE;
		if (o_ptr->to_h + o_ptr->to_d > j_ptr->to_h + j_ptr->to_d) return FALSE;
		break;

	/* Hack:  otherwise identical rods sort by
	increasing recharge time --dsb */
	case TV_ROD:
		if (o_ptr->pval < j_ptr->pval) return TRUE;
		if (o_ptr->pval > j_ptr->pval) return FALSE;
		break;
	}

	/* Objects sort by decreasing value */
	return o_value > object_value(j_ptr);
}


/*!
 * @brief オブジェクトをプレイヤーが拾って所持スロットに納めるメインルーチン /
 * Add an item to the players p_ptr->inventory_list, and return the slot used.
 * @param o_ptr 拾うオブジェクトの構造体参照ポインタ
 * @return 収められた所持スロットのID、拾うことができなかった場合-1を返す。
 * @details
 * If the new item can combine with an existing item in the p_ptr->inventory_list,\n
 * it will do so, using "object_similar()" and "object_absorb()", else,\n
 * the item will be placed into the "proper" location in the p_ptr->inventory_list.\n
 *\n
 * This function can be used to "over-fill" the player's pack, but only\n
 * once, and such an action must trigger the "overflow" code immediately.\n
 * Note that when the pack is being "over-filled", the new item must be\n
 * placed into the "overflow" slot, and the "overflow" must take place\n
 * before the pack is reordered, but (optionally) after the pack is\n
 * combined.  This may be tricky.  See "dungeon.c" for info.\n
 *\n
 * Note that this code must remove any location/stack information\n
 * from the object once it is placed into the p_ptr->inventory_list.\n
 */
s16b inven_carry(object_type *o_ptr)
{
	INVENTORY_IDX i, j, k;
	INVENTORY_IDX n = -1;

	object_type *j_ptr;


	/* Check for combining */
	for (j = 0; j < INVEN_PACK; j++)
	{
		j_ptr = &p_ptr->inventory_list[j];
		if (!j_ptr->k_idx) continue;

		/* Hack -- track last item */
		n = j;

		/* Check if the two items can be combined */
		if (object_similar(j_ptr, o_ptr))
		{
			object_absorb(j_ptr, o_ptr);

			p_ptr->total_weight += (o_ptr->number * o_ptr->weight);
			p_ptr->update |= (PU_BONUS);
			p_ptr->window |= (PW_INVEN);

			/* Success */
			return (j);
		}
	}

	if (p_ptr->inven_cnt > INVEN_PACK) return (-1);

	/* Find an empty slot */
	for (j = 0; j <= INVEN_PACK; j++)
	{
		j_ptr = &p_ptr->inventory_list[j];

		/* Use it if found */
		if (!j_ptr->k_idx) break;
	}

	/* Use that slot */
	i = j;


	/* Reorder the pack */
	if (i < INVEN_PACK)
	{
		/* Get the "value" of the item */
		s32b o_value = object_value(o_ptr);

		/* Scan every occupied slot */
		for (j = 0; j < INVEN_PACK; j++)
		{
			if (object_sort_comp(o_ptr, o_value, &p_ptr->inventory_list[j])) break;
		}

		/* Use that slot */
		i = j;

		/* Slide objects */
		for (k = n; k >= i; k--)
		{
			/* Hack -- Slide the item */
			object_copy(&p_ptr->inventory_list[k+1], &p_ptr->inventory_list[k]);
		}

		/* Wipe the empty slot */
		object_wipe(&p_ptr->inventory_list[i]);
	}


	/* Copy the item */
	object_copy(&p_ptr->inventory_list[i], o_ptr);

	/* Access new object */
	j_ptr = &p_ptr->inventory_list[i];

	/* Forget stack */
	j_ptr->next_o_idx = 0;

	/* Forget monster */
	j_ptr->held_m_idx = 0;

	/* Forget location */
	j_ptr->iy = j_ptr->ix = 0;

	/* Player touches it, and no longer marked */
	j_ptr->marked = OM_TOUCHED;

	p_ptr->total_weight += (j_ptr->number * j_ptr->weight);

	/* Count the items */
	p_ptr->inven_cnt++;
	p_ptr->update |= (PU_BONUS | PU_COMBINE | PU_REORDER);
	p_ptr->window |= (PW_INVEN);

	/* Return the slot */
	return (i);
}


/*!
 * @brief 装備スロットからオブジェクトを外すメインルーチン /
 * Take off (some of) a non-cursed equipment item
 * @param item オブジェクトを外したい所持テーブルのID
 * @param amt 外したい個数
 * @return 収められた所持スロットのID、拾うことができなかった場合-1を返す。
 * @details
 * Note that only one item at a time can be wielded per slot.\n
 * Note that taking off an item when "full" may cause that item\n
 * to fall to the ground.\n
 * Return the p_ptr->inventory_list slot into which the item is placed.\n
 */
INVENTORY_IDX inven_takeoff(INVENTORY_IDX item, ITEM_NUMBER amt)
{
	INVENTORY_IDX slot;

	object_type forge;
	object_type *q_ptr;

	object_type *o_ptr;

	concptr act;

	GAME_TEXT o_name[MAX_NLEN];


	/* Get the item to take off */
	o_ptr = &p_ptr->inventory_list[item];
	if (amt <= 0) return (-1);

	/* Verify */
	if (amt > o_ptr->number) amt = o_ptr->number;
	q_ptr = &forge;

	/* Obtain a local object */
	object_copy(q_ptr, o_ptr);

	/* Modify quantity */
	q_ptr->number = amt;

	object_desc(o_name, q_ptr, 0);

	/* Took off weapon */
	if (((item == INVEN_RARM) || (item == INVEN_LARM)) &&
	    object_is_melee_weapon(o_ptr))
	{
		act = _("を装備からはずした", "You were wielding");
	}

	/* Took off bow */
	else if (item == INVEN_BOW)
	{
		act = _("を装備からはずした", "You were holding");
	}

	/* Took off light */
	else if (item == INVEN_LITE)
	{
		act = _("を光源からはずした", "You were holding");
	}

	/* Took off something */
	else
	{
		act = _("を装備からはずした", "You were wearing");
	}

	/* Modify, Optimize */
	inven_item_increase(item, -amt);
	inven_item_optimize(item);

	/* Carry the object */
	slot = inven_carry(q_ptr);

#ifdef JP
	msg_format("%s(%c)%s。", o_name, index_to_label(slot), act);
#else
	msg_format("%s %s (%c).", act, o_name, index_to_label(slot));
#endif


	/* Return slot */
	return (slot);
}


/*!
 * @brief 所持スロットから床下にオブジェクトを落とすメインルーチン /
 * Drop (some of) a non-cursed p_ptr->inventory_list/equipment item
 * @param item 所持テーブルのID
 * @param amt 落としたい個数
 * @return なし
 * @details
 * The object will be dropped "near" the current location
 */
void inven_drop(INVENTORY_IDX item, ITEM_NUMBER amt)
{
	object_type forge;
	object_type *q_ptr;
	object_type *o_ptr;

	GAME_TEXT o_name[MAX_NLEN];

	/* Access original object */
	o_ptr = &p_ptr->inventory_list[item];

	/* Error check */
	if (amt <= 0) return;

	/* Not too many */
	if (amt > o_ptr->number) amt = o_ptr->number;

	/* Take off equipment */
	if (item >= INVEN_RARM)
	{
		/* Take off first */
		item = inven_takeoff(item, amt);

		/* Access original object */
		o_ptr = &p_ptr->inventory_list[item];
	}

	q_ptr = &forge;

	/* Obtain local object */
	object_copy(q_ptr, o_ptr);

	/* Distribute charges of wands or rods */
	distribute_charges(o_ptr, q_ptr, amt);

	/* Modify quantity */
	q_ptr->number = amt;

	/* Describe local object */
	object_desc(o_name, q_ptr, 0);

	msg_format(_("%s(%c)を落とした。", "You drop %s (%c)."), o_name, index_to_label(item));

	/* Drop it near the player */
	(void)drop_near(q_ptr, 0, p_ptr->y, p_ptr->x);

	/* Modify, Describe, Optimize */
	inven_item_increase(item, -amt);
	inven_item_describe(item);
	inven_item_optimize(item);
}


/*!
 * @brief プレイヤーの所持スロットに存在するオブジェクトをまとめなおす /
 * Combine items in the pack
 * @return なし
 * @details
 * Note special handling of the "overflow" slot
 */
void combine_pack(void)
{
	int             i, j, k;
	object_type *o_ptr;
	object_type     *j_ptr;
	bool            flag = FALSE, combined;

	do
	{
		combined = FALSE;

		/* Combine the pack (backwards) */
		for (i = INVEN_PACK; i > 0; i--)
		{
			o_ptr = &p_ptr->inventory_list[i];

			/* Skip empty items */
			if (!o_ptr->k_idx) continue;

			/* Scan the items above that item */
			for (j = 0; j < i; j++)
			{
				int max_num;

				j_ptr = &p_ptr->inventory_list[j];

				/* Skip empty items */
				if (!j_ptr->k_idx) continue;

				/*
				 * Get maximum number of the stack if these
				 * are similar, get zero otherwise.
				 */
				max_num = object_similar_part(j_ptr, o_ptr);

				/* Can we (partialy) drop "o_ptr" onto "j_ptr"? */
				if (max_num && j_ptr->number < max_num)
				{
					if (o_ptr->number + j_ptr->number <= max_num)
					{
						/* Take note */
						flag = TRUE;

						/* Add together the item counts */
						object_absorb(j_ptr, o_ptr);

						/* One object is gone */
						p_ptr->inven_cnt--;

						/* Slide everything down */
						for (k = i; k < INVEN_PACK; k++)
						{
							/* Structure copy */
							p_ptr->inventory_list[k] = p_ptr->inventory_list[k+1];
						}

						/* Erase the "final" slot */
						object_wipe(&p_ptr->inventory_list[k]);
					}
					else
					{
						int old_num = o_ptr->number;
						int remain = j_ptr->number + o_ptr->number - max_num;
#if 0
						o_ptr->number -= remain;
#endif
						/* Add together the item counts */
						object_absorb(j_ptr, o_ptr);

						o_ptr->number = remain;

						/* Hack -- if rods are stacking, add the pvals (maximum timeouts) and current timeouts together. -LM- */
						if (o_ptr->tval == TV_ROD)
						{
							o_ptr->pval =  o_ptr->pval * remain / old_num;
							o_ptr->timeout = o_ptr->timeout * remain / old_num;
						}

						/* Hack -- if wands are stacking, combine the charges. -LM- */
						if (o_ptr->tval == TV_WAND)
						{
							o_ptr->pval = o_ptr->pval * remain / old_num;
						}
					}

					p_ptr->window |= (PW_INVEN);

					/* Take note */
					combined = TRUE;

					break;
				}
			}
		}
	}
	while (combined);

	if (flag) msg_print(_("ザックの中のアイテムをまとめ直した。", "You combine some items in your pack."));
}

/*!
 * @brief プレイヤーの所持スロットに存在するオブジェクトを並び替える /
 * Reorder items in the pack
 * @return なし
 * @details
 * Note special handling of the "overflow" slot
 */
void reorder_pack(void)
{
	int             i, j, k;
	s32b            o_value;
	object_type     forge;
	object_type     *q_ptr;
	object_type *o_ptr;
	bool            flag = FALSE;


	/* Re-order the pack (forwards) */
	for (i = 0; i < INVEN_PACK; i++)
	{
		/* Mega-Hack -- allow "proper" over-flow */
		if ((i == INVEN_PACK) && (p_ptr->inven_cnt == INVEN_PACK)) break;

		o_ptr = &p_ptr->inventory_list[i];

		/* Skip empty slots */
		if (!o_ptr->k_idx) continue;

		/* Get the "value" of the item */
		o_value = object_value(o_ptr);

		/* Scan every occupied slot */
		for (j = 0; j < INVEN_PACK; j++)
		{
			if (object_sort_comp(o_ptr, o_value, &p_ptr->inventory_list[j])) break;
		}

		/* Never move down */
		if (j >= i) continue;

		/* Take note */
		flag = TRUE;
		q_ptr = &forge;

		/* Save a copy of the moving item */
		object_copy(q_ptr, &p_ptr->inventory_list[i]);

		/* Slide the objects */
		for (k = i; k > j; k--)
		{
			/* Slide the item */
			object_copy(&p_ptr->inventory_list[k], &p_ptr->inventory_list[k-1]);
		}

		/* Insert the moving item */
		object_copy(&p_ptr->inventory_list[j], q_ptr);

		p_ptr->window |= (PW_INVEN);
	}

	if (flag) msg_print(_("ザックの中のアイテムを並べ直した。", "You reorder some items in your pack."));
}

/*!
 * @brief 現在アクティブになっているウィンドウにオブジェクトの詳細を表示する /
 * Hack -- display an object kind in the current window
 * @param k_idx ベースアイテムの参照ID
 * @return なし
 * @details
 * Include list of usable spells for readible books
 */
void display_koff(KIND_OBJECT_IDX k_idx)
{
	int y;

	object_type forge;
	object_type *q_ptr;
	int         sval;
	REALM_IDX   use_realm;

	GAME_TEXT o_name[MAX_NLEN];


	/* Erase the window */
	for (y = 0; y < Term->hgt; y++)
	{
		/* Erase the line */
		Term_erase(0, y, 255);
	}

	/* No info */
	if (!k_idx) return;
	q_ptr = &forge;

	/* Prepare the object */
	object_prep(q_ptr, k_idx);
	object_desc(o_name, q_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY | OD_STORE));

	/* Mention the object name */
	Term_putstr(0, 0, -1, TERM_WHITE, o_name);

	/* Access the item's sval */
	sval = q_ptr->sval;
	use_realm = tval2realm(q_ptr->tval);

	/* Warriors are illiterate */
	if (p_ptr->realm1 || p_ptr->realm2)
	{
		if ((use_realm != p_ptr->realm1) && (use_realm != p_ptr->realm2)) return;
	}
	else
	{
		if ((p_ptr->pclass != CLASS_SORCERER) && (p_ptr->pclass != CLASS_RED_MAGE)) return;
		if (!is_magic(use_realm)) return;
		if ((p_ptr->pclass == CLASS_RED_MAGE) && (use_realm != REALM_ARCANE) && (sval > 1)) return;
	}

	/* Display spells in readible books */
	{
		int     spell = -1;
		int     num = 0;
		SPELL_IDX    spells[64];

		/* Extract spells */
		for (spell = 0; spell < 32; spell++)
		{
			/* Check for this spell */
			if (fake_spell_flags[sval] & (1L << spell))
			{
				/* Collect this spell */
				spells[num++] = spell;
			}
		}

		/* Print spells */
		print_spells(0, spells, num, 2, 0, use_realm);
	}
}


/*!
 * @brief 投擲時たいまつに投げやすい/焼棄/アンデッドスレイの特別効果を返す。
 * Torches have special abilities when they are flaming.
 * @param o_ptr 投擲するオブジェクトの構造体参照ポインタ
 * @param flgs 特別に追加するフラグを返す参照ポインタ
 * @return なし
 */
void torch_flags(object_type *o_ptr, BIT_FLAGS *flgs)
{
	if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_TORCH))
	{
		if (o_ptr->xtra4 > 0)
		{
			add_flag(flgs, TR_BRAND_FIRE);
			add_flag(flgs, TR_KILL_UNDEAD);
			add_flag(flgs, TR_THROW);
		}
	}
}

/*!
 * @brief 投擲時たいまつにダイスを与える。
 * Torches have special abilities when they are flaming.
 * @param o_ptr 投擲するオブジェクトの構造体参照ポインタ
 * @param dd 特別なダイス数を返す参照ポインタ
 * @param ds 特別なダイス面数を返す参照ポインタ
 * @return なし
 */
void torch_dice(object_type *o_ptr, DICE_NUMBER *dd, DICE_SID *ds)
{
	if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_TORCH))
	{
		if (o_ptr->xtra4 > 0)
		{
			(*dd) = 1;
			(*ds) = 6;
		}
	}
}

/*!
 * @brief 投擲時命中したたいまつの寿命を縮める。
 * Torches have special abilities when they are flaming.
 * @param o_ptr 投擲するオブジェクトの構造体参照ポインタ
 * @return なし
 */
void torch_lost_fuel(object_type *o_ptr)
{
	if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_TORCH))
	{
		o_ptr->xtra4 -= (FUEL_TORCH / 25);
		if (o_ptr->xtra4 < 0) o_ptr->xtra4 = 0;
	}
}

/*!
 * @brief 射撃武器に対応する矢/弾薬のベースアイテムIDを返す /
 * @param o_ptr 判定する射撃武器のアイテム情報参照ポインタ
 * @return 対応する矢/弾薬のベースアイテムID
 */
int bow_tval_ammo(object_type *o_ptr)
{
	/* Analyze the launcher */
	switch (o_ptr->sval)
	{
	case SV_SLING:
	{
		return TV_SHOT;
	}

	case SV_SHORT_BOW:
	case SV_LONG_BOW:
	case SV_NAMAKE_BOW:
	{
		return TV_ARROW;
	}

	case SV_LIGHT_XBOW:
	case SV_HEAVY_XBOW:
	{
		return TV_BOLT;
	}
	case SV_CRIMSON:
	case SV_HARP:
	{
		return TV_NO_AMMO;
	}
	}

	return 0;
}
