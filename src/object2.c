/* File: object2.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

/* Purpose: Object code, part 2 */

#include "angband.h"

#include "kajitips.h"


/*
 * Excise a dungeon object from any stacks
 */
void excise_object_idx(int o_idx)
{
	object_type *j_ptr;

	s16b this_o_idx, next_o_idx = 0;

	s16b prev_o_idx = 0;


	/* Object */
	j_ptr = &o_list[o_idx];

	/* Monster */
	if (j_ptr->held_m_idx)
	{
		monster_type *m_ptr;

		/* Monster */
		m_ptr = &m_list[j_ptr->held_m_idx];

		/* Scan all objects in the grid */
		for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
		{
			object_type *o_ptr;

			/* Acquire object */
			o_ptr = &o_list[this_o_idx];

			/* Acquire next object */
			next_o_idx = o_ptr->next_o_idx;

			/* Done */
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
					k_ptr = &o_list[prev_o_idx];

					/* Remove from list */
					k_ptr->next_o_idx = next_o_idx;
				}

				/* Forget next pointer */
				o_ptr->next_o_idx = 0;

				/* Done */
				break;
			}

			/* Save prev_o_idx */
			prev_o_idx = this_o_idx;
		}
	}

	/* Dungeon */
	else
	{
		cave_type *c_ptr;

		int y = j_ptr->iy;
		int x = j_ptr->ix;

		/* Grid */
		c_ptr = &cave[y][x];

		/* Scan all objects in the grid */
		for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
		{
			object_type *o_ptr;

			/* Acquire object */
			o_ptr = &o_list[this_o_idx];

			/* Acquire next object */
			next_o_idx = o_ptr->next_o_idx;

			/* Done */
			if (this_o_idx == o_idx)
			{
				/* No previous */
				if (prev_o_idx == 0)
				{
					/* Remove from list */
					c_ptr->o_idx = next_o_idx;
				}

				/* Real previous */
				else
				{
					object_type *k_ptr;

					/* Previous object */
					k_ptr = &o_list[prev_o_idx];

					/* Remove from list */
					k_ptr->next_o_idx = next_o_idx;
				}

				/* Forget next pointer */
				o_ptr->next_o_idx = 0;

				/* Done */
				break;
			}

			/* Save prev_o_idx */
			prev_o_idx = this_o_idx;
		}
	}
}


/*
 * Delete a dungeon object
 *
 * Handle "stacks" of objects correctly.
 */
void delete_object_idx(int o_idx)
{
	object_type *j_ptr;

	/* Excise */
	excise_object_idx(o_idx);

	/* Object */
	j_ptr = &o_list[o_idx];

	/* Dungeon floor */
	if (!(j_ptr->held_m_idx))
	{
		int y, x;

		/* Location */
		y = j_ptr->iy;
		x = j_ptr->ix;

		/* Visual update */
		lite_spot(y, x);
	}

	/* Wipe the object */
	object_wipe(j_ptr);

	/* Count objects */
	o_cnt--;
}


/*
 * Deletes all objects at given location
 */
void delete_object(int y, int x)
{
	cave_type *c_ptr;

	s16b this_o_idx, next_o_idx = 0;


	/* Refuse "illegal" locations */
	if (!in_bounds(y, x)) return;


	/* Grid */
	c_ptr = &cave[y][x];

	/* Scan all objects in the grid */
	for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Acquire object */
		o_ptr = &o_list[this_o_idx];

		/* Acquire next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Wipe the object */
		object_wipe(o_ptr);

		/* Count objects */
		o_cnt--;
	}

	/* Objects are gone */
	c_ptr->o_idx = 0;

	/* Visual update */
	lite_spot(y, x);
}


/*
 * Move an object from index i1 to index i2 in the object list
 */
static void compact_objects_aux(int i1, int i2)
{
	int i;

	cave_type *c_ptr;

	object_type *o_ptr;


	/* Do nothing */
	if (i1 == i2) return;


	/* Repair objects */
	for (i = 1; i < o_max; i++)
	{
		/* Acquire object */
		o_ptr = &o_list[i];

		/* Skip "dead" objects */
		if (!o_ptr->k_idx) continue;

		/* Repair "next" pointers */
		if (o_ptr->next_o_idx == i1)
		{
			/* Repair */
			o_ptr->next_o_idx = i2;
		}
	}


	/* Acquire object */
	o_ptr = &o_list[i1];


	/* Monster */
	if (o_ptr->held_m_idx)
	{
		monster_type *m_ptr;

		/* Acquire monster */
		m_ptr = &m_list[o_ptr->held_m_idx];

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
		int y, x;

		/* Acquire location */
		y = o_ptr->iy;
		x = o_ptr->ix;

		/* Acquire grid */
		c_ptr = &cave[y][x];

		/* Repair grid */
		if (c_ptr->o_idx == i1)
		{
			/* Repair */
			c_ptr->o_idx = i2;
		}
	}


	/* Structure copy */
	o_list[i2] = o_list[i1];

	/* Wipe the hole */
	object_wipe(o_ptr);
}


/*
 * Compact and Reorder the object list
 *
 * This function can be very dangerous, use with caution!
 *
 * When actually "compacting" objects, we base the saving throw on a
 * combination of object level, distance from player, and current
 * "desperation".
 *
 * After "compacting" (if needed), we "reorder" the objects into a more
 * compact order, and we reset the allocation info, and the "live" array.
 */
void compact_objects(int size)
{
	int i, y, x, num, cnt;
	int cur_lev, cur_dis, chance;
	object_type *o_ptr;


	/* Compact */
	if (size)
	{
		/* Message */
#ifdef JP
		msg_print("アイテム情報を圧縮しています...");
#else
		msg_print("Compacting objects...");
#endif


		/* Redraw map */
		p_ptr->redraw |= (PR_MAP);

		/* Window stuff */
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
		for (i = 1; i < o_max; i++)
		{
			o_ptr = &o_list[i];

			/* Skip dead objects */
			if (!o_ptr->k_idx) continue;

			/* Hack -- High level objects start out "immune" */
			if (k_info[o_ptr->k_idx].level > cur_lev) continue;

			/* Monster */
			if (o_ptr->held_m_idx)
			{
				monster_type *m_ptr;

				/* Acquire monster */
				m_ptr = &m_list[o_ptr->held_m_idx];

				/* Get the location */
				y = m_ptr->fy;
				x = m_ptr->fx;

				/* Monsters protect their objects */
				if (randint0(100) < 90) continue;
			}

			/* Dungeon */
			else
			{
				/* Get the location */
				y = o_ptr->iy;
				x = o_ptr->ix;
			}

			/* Nearby objects start out "immune" */
			if ((cur_dis > 0) && (distance(py, px, y, x) < cur_dis)) continue;

			/* Saving throw */
			chance = 90;

			/* Hack -- only compact artifacts in emergencies */
			if ((object_is_fixed_artifact(o_ptr) || o_ptr->art_name) &&
			    (cnt < 1000)) chance = 100;

			/* Apply the saving throw */
			if (randint0(100) < chance) continue;

			/* Delete the object */
			delete_object_idx(i);

			/* Count it */
			num++;
		}
	}


	/* Excise dead objects (backwards!) */
	for (i = o_max - 1; i >= 1; i--)
	{
		o_ptr = &o_list[i];

		/* Skip real objects */
		if (o_ptr->k_idx) continue;

		/* Move last object into open hole */
		compact_objects_aux(o_max - 1, i);

		/* Compress "o_max" */
		o_max--;
	}
}


/*
 * Delete all the items when player leaves the level
 *
 * Note -- we do NOT visually reflect these (irrelevant) changes
 *
 * Hack -- we clear the "c_ptr->o_idx" field for every grid,
 * and the "m_ptr->next_o_idx" field for every monster, since
 * we know we are clearing every object.  Technically, we only
 * clear those fields for grids/monsters containing objects,
 * and we clear it once for every such object.
 */
void wipe_o_list(void)
{
	int i;

	/* Delete the existing objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Mega-Hack -- preserve artifacts */
		if (!character_dungeon || preserve_mode)
		{
			/* Hack -- Preserve unknown artifacts */
			if (object_is_fixed_artifact(o_ptr) && !object_is_known(o_ptr))
			{
				/* Mega-Hack -- Preserve the artifact */
				a_info[o_ptr->name1].cur_num = 0;
			}
		}

		/* Monster */
		if (o_ptr->held_m_idx)
		{
			monster_type *m_ptr;

			/* Monster */
			m_ptr = &m_list[o_ptr->held_m_idx];

			/* Hack -- see above */
			m_ptr->hold_o_idx = 0;
		}

		/* Dungeon */
		else
		{
			cave_type *c_ptr;

			/* Access location */
			int y = o_ptr->iy;
			int x = o_ptr->ix;

			/* Access grid */
			c_ptr = &cave[y][x];

			/* Hack -- see above */
			c_ptr->o_idx = 0;
		}

		/* Wipe the object */
		object_wipe(o_ptr);
	}

	/* Reset "o_max" */
	o_max = 1;

	/* Reset "o_cnt" */
	o_cnt = 0;
}


/*
 * Acquires and returns the index of a "free" object.
 *
 * This routine should almost never fail, but in case it does,
 * we must be sure to handle "failure" of this routine.
 */
s16b o_pop(void)
{
	int i;


	/* Initial allocation */
	if (o_max < max_o_idx)
	{
		/* Get next space */
		i = o_max;

		/* Expand object array */
		o_max++;

		/* Count objects */
		o_cnt++;

		/* Use this object */
		return (i);
	}


	/* Recycle dead objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr;

		/* Acquire object */
		o_ptr = &o_list[i];

		/* Skip live objects */
		if (o_ptr->k_idx) continue;

		/* Count objects */
		o_cnt++;

		/* Use this object */
		return (i);
	}


	/* Warn the player (except during dungeon creation) */
#ifdef JP
	if (character_dungeon) msg_print("アイテムが多すぎる！");
#else
	if (character_dungeon) msg_print("Too many objects!");
#endif


	/* Oops */
	return (0);
}


/*
 * Apply a "object restriction function" to the "object allocation table"
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


/*
 * Choose an object kind that seems "appropriate" to the given level
 *
 * This function uses the "prob2" field of the "object allocation table",
 * and various local information, to calculate the "prob3" field of the
 * same table, which is then used to choose an "appropriate" object, in
 * a relatively efficient manner.
 *
 * It is (slightly) more likely to acquire an object of the given level
 * than one of a lower level.  This is done by choosing several objects
 * appropriate to the given level and keeping the "hardest" one.
 *
 * Note that if no objects are "appropriate", then this function will
 * fail, and return zero, but this should *almost* never happen.
 */
s16b get_obj_num(int level)
{
	int             i, j, p;
	int             k_idx;
	long            value, total;
	object_kind     *k_ptr;
	alloc_entry     *table = alloc_kind_table;

	if (level > MAX_DEPTH - 1) level = MAX_DEPTH - 1;

	/* Boost level */
	if ((level > 0) && !(d_info[dungeon_type].flags1 & DF1_BEGINNER))
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

		/* Access the index */
		k_idx = table[i].index;

		/* Access the actual kind */
		k_ptr = &k_info[k_idx];

		/* Hack -- prevent embedded chests */
		if (opening_chest && (k_ptr->tval == TV_CHEST)) continue;

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


	/* Result */
	return (table[i].index);
}


/*
 * Known is true when the "attributes" of an object are "known".
 * These include tohit, todam, toac, cost, and pval (charges).
 *
 * Note that "knowing" an object gives you everything that an "awareness"
 * gives you, and much more.  In fact, the player is always "aware" of any
 * item of which he has full "knowledge".
 *
 * But having full knowledge of, say, one "wand of wonder", does not, by
 * itself, give you knowledge, or even awareness, of other "wands of wonder".
 * It happens that most "identify" routines (including "buying from a shop")
 * will make the player "aware" of the object as well as fully "know" it.
 *
 * This routine also removes any inscriptions generated by "feelings".
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


/*
 * The player is now aware of the effects of the given object.
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
		char o_name[MAX_NLEN];

		q_ptr = &forge;
		object_copy(q_ptr, o_ptr);

		q_ptr->number = 1;
		object_desc(o_name, q_ptr, OD_NAME_ONLY);
		
		do_cmd_write_nikki(NIKKI_HANMEI, 0, o_name);
	}
}


/*
 * Something has been "sampled"
 */
void object_tried(object_type *o_ptr)
{
	/* Mark it as tried (even if "aware") */
	k_info[o_ptr->k_idx].tried = TRUE;
}


/*
 * Return the "value" of an "unknown" item
 * Make a guess at the value of non-aware items
 */
static s32b object_value_base(object_type *o_ptr)
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
			int level = r_info[o_ptr->pval].level;
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


/* Return the value of the flags the object has... */
s32b flag_cost(object_type *o_ptr, int plusses)
{
	s32b total = 0;
	u32b flgs[TR_FLAG_SIZE];
	s32b tmp_cost;
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
	if (have_flag(flgs, TR_HOLD_LIFE)) total += 8500;

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
	if (have_flag(flgs, TR_LITE)) total += 1250;
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
	if (have_flag(flgs, TR_TELEPORT))
	{
		if (object_is_cursed(o_ptr))
			total -= 7500;
		else
			total += 250;
	}
	if (have_flag(flgs, TR_AGGRAVATE)) total -= 10000;
	if (have_flag(flgs, TR_BLESSED)) total += 750;
	if (o_ptr->curse_flags & TRC_CURSED) total -= 5000;
	if (o_ptr->curse_flags & TRC_HEAVY_CURSE) total -= 12500;
	if (o_ptr->curse_flags & TRC_PERMA_CURSE) total -= 15000;

	/* Also, give some extra for activatable powers... */
	if (o_ptr->art_name && (have_flag(o_ptr->art_flags, TR_ACTIVATE)))
	{
		int type = o_ptr->xtra2;

		if (type == ACT_SUNLIGHT) total += 250;
		else if (type == ACT_BO_MISS_1) total += 250;
		else if (type == ACT_BA_POIS_1) total += 300;
		else if (type == ACT_BO_ELEC_1) total += 250;
		else if (type == ACT_BO_ACID_1) total += 250;
		else if (type == ACT_BO_COLD_1) total += 250;
		else if (type == ACT_BO_FIRE_1) total += 250;
		else if (type == ACT_BA_COLD_1) total += 750;
		else if (type == ACT_BA_FIRE_1) total += 1000;
		else if (type == ACT_DRAIN_1) total += 500;
		else if (type == ACT_BA_COLD_2) total += 1250;
		else if (type == ACT_BA_ELEC_2) total += 1500;
		else if (type == ACT_DRAIN_2) total += 750;
		else if (type == ACT_VAMPIRE_1) total += 1000;
		else if (type == ACT_BO_MISS_2) total += 1000;
		else if (type == ACT_BA_FIRE_2) total += 1750;
		else if (type == ACT_BA_COLD_3) total += 2500;
		else if (type == ACT_BA_ELEC_3) total += 2500;
		else if (type == ACT_WHIRLWIND) total += 7500;
		else if (type == ACT_VAMPIRE_2) total += 2500;
		else if (type == ACT_CALL_CHAOS) total += 5000;
		else if (type == ACT_ROCKET) total += 5000;
		else if (type == ACT_DISP_EVIL) total += 4000;
		else if (type == ACT_DISP_GOOD) total += 3500;
		else if (type == ACT_BA_MISS_3) total += 5000;
		else if (type == ACT_CONFUSE) total += 500;
		else if (type == ACT_SLEEP) total += 750;
		else if (type == ACT_QUAKE) total += 600;
		else if (type == ACT_TERROR) total += 2500;
		else if (type == ACT_TELE_AWAY) total += 2000;
		else if (type == ACT_BANISH_EVIL) total += 2000;
		else if (type == ACT_GENOCIDE) total += 10000;
		else if (type == ACT_MASS_GENO) total += 10000;
		else if (type == ACT_CHARM_ANIMAL) total += 7500;
		else if (type == ACT_CHARM_UNDEAD) total += 10000;
		else if (type == ACT_CHARM_OTHER) total += 10000;
		else if (type == ACT_CHARM_ANIMALS) total += 12500;
		else if (type == ACT_CHARM_OTHERS) total += 17500;
		else if (type == ACT_SUMMON_ANIMAL) total += 10000;
		else if (type == ACT_SUMMON_PHANTOM) total += 12000;
		else if (type == ACT_SUMMON_ELEMENTAL) total += 15000;
		else if (type == ACT_SUMMON_DEMON) total += 20000;
		else if (type == ACT_SUMMON_UNDEAD) total += 20000;
		else if (type == ACT_CURE_LW) total += 500;
		else if (type == ACT_CURE_MW) total += 750;
		else if (type == ACT_CURE_POISON) total += 1000;
		else if (type == ACT_REST_LIFE) total += 7500;
		else if (type == ACT_REST_ALL) total += 15000;
		else if (type == ACT_CURE_700) total += 10000;
		else if (type == ACT_CURE_1000) total += 15000;
		else if (type == ACT_ESP) total += 1500;
		else if (type == ACT_BERSERK) total += 800;
		else if (type == ACT_PROT_EVIL) total += 5000;
		else if (type == ACT_RESIST_ALL) total += 5000;
		else if (type == ACT_SPEED) total += 15000;
		else if (type == ACT_XTRA_SPEED) total += 25000;
		else if (type == ACT_WRAITH) total += 25000;
		else if (type == ACT_INVULN) total += 25000;
		else if (type == ACT_LIGHT) total += 150;
		else if (type == ACT_MAP_LIGHT) total += 500;
		else if (type == ACT_DETECT_ALL) total += 1000;
		else if (type == ACT_DETECT_XTRA) total += 12500;
		else if (type == ACT_ID_FULL) total += 10000;
		else if (type == ACT_ID_PLAIN) total += 1250;
		else if (type == ACT_RUNE_EXPLO) total += 4000;
		else if (type == ACT_RUNE_PROT) total += 10000;
		else if (type == ACT_SATIATE) total += 2000;
		else if (type == ACT_DEST_DOOR) total += 100;
		else if (type == ACT_STONE_MUD) total += 1000;
		else if (type == ACT_RECHARGE) total += 1000;
		else if (type == ACT_ALCHEMY) total += 10000;
		else if (type == ACT_DIM_DOOR) total += 10000;
		else if (type == ACT_TELEPORT) total += 2000;
		else if (type == ACT_RECALL) total += 7500;
	}

	return total;
}


/*
 * Return the "real" price of a "known" item, not including discounts
 *
 * Wand and staffs get cost for each charge
 *
 * Armor is worth an extra 100 gold per bonus point to armor class.
 *
 * Weapons are worth an extra 100 gold per bonus point (AC,TH,TD).
 *
 * Missiles are only worth 5 gold per bonus point, since they
 * usually appear in groups of 20, and we want the player to get
 * the same amount of cash for any "equivalent" item.  Note that
 * missiles never have any of the "pval" flags, and in fact, they
 * only have a few of the available flags, primarily of the "slay"
 * and "brand" and "ignore" variety.
 *
 * Armor with a negative armor bonus is worthless.
 * Weapons with negative hit+damage bonuses are worthless.
 *
 * Every wearable item with a "pval" bonus is worth extra (see below).
 */
s32b object_value_real(object_type *o_ptr)
{
	s32b value;

	u32b flgs[TR_FLAG_SIZE];

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

			/* Done */
			break;
		}
		case TV_STAFF:
		{
			/* Pay extra for charges, depending on standard number of
			 * charges.  -LM-
			 */
			value += (value * o_ptr->pval / (k_ptr->pval * 2));

			/* Done */
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

			/* Done */
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

			/* Done */
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

			/* Done */
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

			/* Done */
			break;
		}

		/* Figurines, relative to monster level */
		case TV_FIGURINE:
		{
			int level = r_info[o_ptr->pval].level;
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


/*
 * Return the price of an item including plusses (and charges)
 *
 * This function returns the "value" of the given item (qty one)
 *
 * Never notice "unknown" bonuses or properties, including "curses",
 * since that would give the player information he did not have.
 *
 * Note that discounted items stay discounted forever, even if
 * the discount is "forgotten" by the player via memory loss.
 */
s32b object_value(object_type *o_ptr)
{
	s32b value;


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


/*
 * Determines whether an object can be destroyed, and makes fake inscription.
 */
bool can_player_destroy_object(object_type *o_ptr)
{
	/* Artifacts cannot be destroyed */
	if (!object_is_artifact(o_ptr)) return TRUE;

	/* If object is unidentified, makes fake inscription */
	if (!object_is_known(o_ptr))
	{
		byte feel = FEEL_SPECIAL;

		/* Hack -- Handle icky artifacts */
		if (object_is_cursed(o_ptr) || object_is_broken(o_ptr)) feel = FEEL_TERRIBLE;

		/* Hack -- inscribe the artifact */
		o_ptr->feeling = feel;

		/* We have "felt" it (again) */
		o_ptr->ident |= (IDENT_SENSE);

		/* Combine the pack */
		p_ptr->notice |= (PN_COMBINE);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Done */
		return FALSE;
	}

	/* Identified artifact -- Nothing to do */
	return FALSE;
}


/*
 * Distribute charges of rods or wands.
 *
 * o_ptr = source item
 * q_ptr = target item, must be of the same type as o_ptr
 * amt   = number of items that are transfered
 */
void distribute_charges(object_type *o_ptr, object_type *q_ptr, int amt)
{
	/*
	 * Hack -- If rods or wands are dropped, the total maximum timeout or
	 * charges need to be allocated between the two stacks.  If all the items
	 * are being dropped, it makes for a neater message to leave the original
	 * stack's pval alone. -LM-
	 */
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

void reduce_charges(object_type *o_ptr, int amt)
{
	/*
	 * Hack -- If rods or wand are destroyed, the total maximum timeout or
	 * charges of the stack needs to be reduced, unless all the items are
	 * being destroyed. -LM-
	 */
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


/*
 *  Determine if an item can partly absorb a second item.
 *  Return maximum number of stack.
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

/*
 *  Determine if an item can absorb a second item.
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



/*
 * Allow one item to "absorb" another, assuming they are similar
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

	/* Hack -- could average discounts XXX XXX XXX */
	/* Hack -- save largest discount XXX XXX XXX */
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


/*
 * Find the index of the object_kind with the given tval and sval
 */
s16b lookup_kind(int tval, int sval)
{
	int k;
	int num = 0;
	int bk = 0;

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
	/* Oops */
#ifdef JP
	msg_format("アイテムがない (%d,%d)", tval, sval);
#else
	msg_format("No object (%d,%d)", tval, sval);
#endif
#endif


	/* Oops */
	return (0);
}


/*
 * Wipe an object clean.
 */
void object_wipe(object_type *o_ptr)
{
	/* Wipe the structure */
	(void)WIPE(o_ptr, object_type);
}


/*
 * Prepare an object based on an existing object
 */
void object_copy(object_type *o_ptr, object_type *j_ptr)
{
	/* Copy the structure */
	COPY(o_ptr, j_ptr, object_type);
}


/*
 * Prepare an object based on an object kind.
 */
void object_prep(object_type *o_ptr, int k_idx)
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


/*
 * Help determine an "enchantment bonus" for an object.
 *
 * To avoid floating point but still provide a smooth distribution of bonuses,
 * we simply round the results of division in such a way as to "average" the
 * correct floating point value.
 *
 * This function has been changed.  It uses "randnor()" to choose values from
 * a normal distribution, whose mean moves from zero towards the max as the
 * level increases, and whose standard deviation is equal to 1/4 of the max,
 * and whose values are forced to lie between zero and the max, inclusive.
 *
 * Since the "level" rarely passes 100 before Morgoth is dead, it is very
 * rare to get the "full" enchantment on an object, even a deep levels.
 *
 * It is always possible (albeit unlikely) to get the "full" enchantment.
 *
 * A sample distribution of values from "m_bonus(10, N)" is shown below:
 *
 *   N       0     1     2     3     4     5     6     7     8     9    10
 * ---    ----  ----  ----  ----  ----  ----  ----  ----  ----  ----  ----
 *   0   66.37 13.01  9.73  5.47  2.89  1.31  0.72  0.26  0.12  0.09  0.03
 *   8   46.85 24.66 12.13  8.13  4.20  2.30  1.05  0.36  0.19  0.08  0.05
 *  16   30.12 27.62 18.52 10.52  6.34  3.52  1.95  0.90  0.31  0.15  0.05
 *  24   22.44 15.62 30.14 12.92  8.55  5.30  2.39  1.63  0.62  0.28  0.11
 *  32   16.23 11.43 23.01 22.31 11.19  7.18  4.46  2.13  1.20  0.45  0.41
 *  40   10.76  8.91 12.80 29.51 16.00  9.69  5.90  3.43  1.47  0.88  0.65
 *  48    7.28  6.81 10.51 18.27 27.57 11.76  7.85  4.99  2.80  1.22  0.94
 *  56    4.41  4.73  8.52 11.96 24.94 19.78 11.06  7.18  3.68  1.96  1.78
 *  64    2.81  3.07  5.65  9.17 13.01 31.57 13.70  9.30  6.04  3.04  2.64
 *  72    1.87  1.99  3.68  7.15 10.56 20.24 25.78 12.17  7.52  4.42  4.62
 *  80    1.02  1.23  2.78  4.75  8.37 12.04 27.61 18.07 10.28  6.52  7.33
 *  88    0.70  0.57  1.56  3.12  6.34 10.06 15.76 30.46 12.58  8.47 10.38
 *  96    0.27  0.60  1.25  2.28  4.30  7.60 10.77 22.52 22.51 11.37 16.53
 * 104    0.22  0.42  0.77  1.36  2.62  5.33  8.93 13.05 29.54 15.23 22.53
 * 112    0.15  0.20  0.56  0.87  2.00  3.83  6.86 10.06 17.89 27.31 30.27
 * 120    0.03  0.11  0.31  0.46  1.31  2.48  4.60  7.78 11.67 25.53 45.72
 * 128    0.02  0.01  0.13  0.33  0.83  1.41  3.24  6.17  9.57 14.22 64.07
 */
s16b m_bonus(int max, int level)
{
	int bonus, stand, extra, value;


	/* Paranoia -- enforce maximal "level" */
	if (level > MAX_DEPTH - 1) level = MAX_DEPTH - 1;


	/* The "bonus" moves towards the max */
	bonus = ((max * level) / MAX_DEPTH);

	/* Hack -- determine fraction of error */
	extra = ((max * level) % MAX_DEPTH);

	/* Hack -- simulate floating point computations */
	if (randint0(MAX_DEPTH) < extra) bonus++;


	/* The "stand" is equal to one quarter of the max */
	stand = (max / 4);

	/* Hack -- determine fraction of error */
	extra = (max % 4);

	/* Hack -- simulate floating point computations */
	if (randint0(4) < extra) stand++;


	/* Choose an "interesting" value */
	value = randnor(bonus, stand);

	/* Enforce the minimum value */
	if (value < 0) return (0);

	/* Enforce the maximum value */
	if (value > max) return (max);

	/* Result */
	return (value);
}


/*
 * Cheat -- describe a created object for the user
 */
static void object_mention(object_type *o_ptr)
{
	char o_name[MAX_NLEN];

	/* Describe */
	object_desc(o_name, o_ptr, (OD_NAME_ONLY | OD_STORE));

	/* Artifact */
	if (object_is_fixed_artifact(o_ptr))
	{
		/* Silly message */
#ifdef JP
		msg_format("伝説のアイテム (%s)", o_name);
#else
		msg_format("Artifact (%s)", o_name);
#endif

	}

	/* Random Artifact */
	else if (o_ptr->art_name)
	{
#ifdef JP
		msg_print("ランダム・アーティファクト");
#else
		msg_print("Random artifact");
#endif

	}

	/* Ego-item */
	else if (object_is_ego(o_ptr))
	{
		/* Silly message */
#ifdef JP
		msg_format("名のあるアイテム (%s)", o_name);
#else
		msg_format("Ego-item (%s)", o_name);
#endif

	}

	/* Normal item */
	else
	{
		/* Silly message */
#ifdef JP
		msg_format("アイテム (%s)", o_name);
#else
		msg_format("Object (%s)", o_name);
#endif

	}
}


/*
 * Mega-Hack -- Attempt to create one of the "Special Objects"
 *
 * We are only called from "make_object()", and we assume that
 * "apply_magic()" is called immediately after we return.
 *
 * Note -- see "make_artifact()" and "apply_magic()"
 */
static bool make_artifact_special(object_type *o_ptr)
{
	int i;
	int k_idx = 0;


	/* No artifacts in the town */
	if (!dun_level) return (FALSE);

	/* Themed object */
	if (get_obj_num_hook) return (FALSE);

	/* Check the artifact list (just the "specials") */
	for (i = 0; i < max_a_idx; i++)
	{
		artifact_type *a_ptr = &a_info[i];

		/* Skip "empty" artifacts */
		if (!a_ptr->name) continue;

		/* Cannot make an artifact twice */
		if (a_ptr->cur_num) continue;

		if (a_ptr->gen_flags & TRG_QUESTITEM) continue;
		if (!(a_ptr->gen_flags & TRG_INSTA_ART)) continue;

		/* XXX XXX Enforce minimum "depth" (loosely) */
		if (a_ptr->level > dun_level)
		{
			/* Acquire the "out-of-depth factor" */
			int d = (a_ptr->level - dun_level) * 2;

			/* Roll for out-of-depth creation */
			if (!one_in_(d)) continue;
		}

		/* Artifact "rarity roll" */
		if (!one_in_(a_ptr->rarity)) continue;

		/* Find the base object */
		k_idx = lookup_kind(a_ptr->tval, a_ptr->sval);

		/* XXX XXX Enforce minimum "object" level (loosely) */
		if (k_info[k_idx].level > object_level)
		{
			/* Acquire the "out-of-depth factor" */
			int d = (k_info[k_idx].level - object_level) * 5;

			/* Roll for out-of-depth creation */
			if (!one_in_(d)) continue;
		}

		/* Assign the template */
		object_prep(o_ptr, k_idx);

		/* Mega-Hack -- mark the item as an artifact */
		o_ptr->name1 = i;

		/* Hack: Some artifacts get random extra powers */
		random_artifact_resistance(o_ptr, a_ptr);

		/* Success */
		return (TRUE);
	}

	/* Failure */
	return (FALSE);
}


/*
 * Attempt to change an object into an artifact
 *
 * This routine should only be called by "apply_magic()"
 *
 * Note -- see "make_artifact_special()" and "apply_magic()"
 */
static bool make_artifact(object_type *o_ptr)
{
	int i;


	/* No artifacts in the town */
	if (!dun_level) return (FALSE);

	/* Paranoia -- no "plural" artifacts */
	if (o_ptr->number != 1) return (FALSE);

	/* Check the artifact list (skip the "specials") */
	for (i = 0; i < max_a_idx; i++)
	{
		artifact_type *a_ptr = &a_info[i];

		/* Skip "empty" items */
		if (!a_ptr->name) continue;

		/* Cannot make an artifact twice */
		if (a_ptr->cur_num) continue;

		if (a_ptr->gen_flags & TRG_QUESTITEM) continue;

		if (a_ptr->gen_flags & TRG_INSTA_ART) continue;

		/* Must have the correct fields */
		if (a_ptr->tval != o_ptr->tval) continue;
		if (a_ptr->sval != o_ptr->sval) continue;

		/* XXX XXX Enforce minimum "depth" (loosely) */
		if (a_ptr->level > dun_level)
		{
			/* Acquire the "out-of-depth factor" */
			int d = (a_ptr->level - dun_level) * 2;

			/* Roll for out-of-depth creation */
			if (!one_in_(d)) continue;
		}

		/* We must make the "rarity roll" */
		if (!one_in_(a_ptr->rarity)) continue;

		/* Hack -- mark the item as an artifact */
		o_ptr->name1 = i;

		/* Hack: Some artifacts get random extra powers */
		random_artifact_resistance(o_ptr, a_ptr);

		/* Success */
		return (TRUE);
	}

	/* Failure */
	return (FALSE);
}


/*
 *  Choose random ego type
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


/*
 * Apply magic to an item known to be a "weapon"
 *
 * Hack -- note special base damage dice boosting
 * Hack -- note special processing for weapon/digger
 */
static void a_m_aux_1(object_type *o_ptr, int level, int power)
{
	int tohit1 = randint1(5) + m_bonus(5, level);
	int todam1 = randint1(5) + m_bonus(5, level);

	int tohit2 = m_bonus(10, level);
	int todam2 = m_bonus(10, level);

	if ((o_ptr->tval == TV_BOLT) || (o_ptr->tval == TV_ARROW) || (o_ptr->tval == TV_SHOT))
	{
		tohit2 = (tohit2+1)/2;
		todam2 = (todam2+1)/2;
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

	/* Analyze type */
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
						}
						while (one_in_(o_ptr->dd));
						
						do
						{
							o_ptr->ds++;
						}
						while (one_in_(o_ptr->ds));
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
						add_flag(o_ptr->art_flags, TR_HOLD_LIFE);
					if (one_in_(3))
						add_flag(o_ptr->art_flags, TR_DEX);
					if (one_in_(5))
						add_flag(o_ptr->art_flags, TR_RES_FEAR);
					break;
				case EGO_SHARPNESS:
					o_ptr->pval = m_bonus(5, level) + 1;
					break;
				case EGO_EARTHQUAKES:
					if (one_in_(3) && (level > 60))
						add_flag(o_ptr->art_flags, TR_BLOWS);
					else
						o_ptr->pval = m_bonus(3, level);
					break;
				case EGO_VAMPIRIC:
					if (one_in_(5))
						add_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
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
					o_ptr->name2 = get_random_ego(INVEN_RARM, FALSE);
					switch (o_ptr->name2)
					{
					case EGO_MORGUL:
						if (one_in_(6)) add_flag(o_ptr->art_flags, TR_TY_CURSE);
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


static void dragon_resist(object_type * o_ptr)
{
	do
	{
		if (one_in_(4))
			one_dragon_ele_resistance(o_ptr);
		else
			one_high_resistance(o_ptr);
	}
	while (one_in_(2));
}


static bool add_esp_strong(object_type *o_ptr)
{
	bool nonliv = FALSE;

	switch (randint1(3))
	{
	case 1: add_flag(o_ptr->art_flags, TR_ESP_EVIL); break;
	case 2: add_flag(o_ptr->art_flags, TR_TELEPATHY); break;
	case 3:	add_flag(o_ptr->art_flags, TR_ESP_NONLIVING); nonliv = TRUE; break;
	}

	return nonliv;
}


#define MAX_ESP_WEAK 9
static void add_esp_weak(object_type *o_ptr, bool extra)
{
	int i = 0;
	int idx[MAX_ESP_WEAK];
	int flg[MAX_ESP_WEAK];
	int n = (extra) ? (3 + randint1(randint1(6))) : randint1(3);
	int left = MAX_ESP_WEAK;

	for (i = 0; i < MAX_ESP_WEAK; i++) flg[i] = i + 1;

	/* Shuffle esp flags */
	for (i = 0; i < n; i++)
	{
		int k = randint0(left--);

		idx[i] = flg[k];

		while (k < left)
		{
			flg[k] = flg[k + 1];
			k++;
		}
	}

	while (n--) switch (idx[n])
	{
	case 1: add_flag(o_ptr->art_flags, TR_ESP_ANIMAL); break;
	case 2: add_flag(o_ptr->art_flags, TR_ESP_UNDEAD); break;
	case 3: add_flag(o_ptr->art_flags, TR_ESP_DEMON); break;
	case 4: add_flag(o_ptr->art_flags, TR_ESP_ORC); break;
	case 5: add_flag(o_ptr->art_flags, TR_ESP_TROLL); break;
	case 6: add_flag(o_ptr->art_flags, TR_ESP_GIANT); break;
	case 7: add_flag(o_ptr->art_flags, TR_ESP_DRAGON);   break;
	case 8: add_flag(o_ptr->art_flags, TR_ESP_HUMAN); break;
	case 9: add_flag(o_ptr->art_flags, TR_ESP_GOOD); break;
	}
}


/*
 * Apply magic to an item known to be "armor"
 *
 * Hack -- note special processing for crown/helm
 * Hack -- note special processing for robe of permanence
 */
static void a_m_aux_2(object_type *o_ptr, int level, int power)
{
	int toac1 = randint1(5) + m_bonus(5, level);

	int toac2 = m_bonus(10, level);

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


	/* Analyze type */
	switch (o_ptr->tval)
	{
		case TV_DRAG_ARMOR:
		{
			if (one_in_(50) || (power > 2)) /* power > 2 is debug only */
				create_artifact(o_ptr, FALSE);

			/* Mention the item */
			if (cheat_peek) object_mention(o_ptr);

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
					case EGO_RESISTANCE:
						if (one_in_(4))
							add_flag(o_ptr->art_flags, TR_RES_POIS);
						break;
					case EGO_ELVENKIND:
						break;
					case EGO_DWARVEN:
						if (o_ptr->tval != TV_HARD_ARMOR)
						{
							okay_flag = FALSE;
							break;
						}
						else
						{
							o_ptr->weight = (2 * k_info[o_ptr->k_idx].weight / 3);
							o_ptr->ac = k_info[o_ptr->k_idx].ac + 5;
							if (one_in_(4))
								add_flag(o_ptr->art_flags, TR_CON);
							break;
						}
					}

					if (okay_flag)
						break;
				}
			}

			break;
		}

		case TV_SHIELD:
		{

			if (o_ptr->sval == SV_DRAGON_SHIELD)
			{
				/* Mention the item */
				if (cheat_peek) object_mention(o_ptr);
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
				o_ptr->name2 = get_random_ego(INVEN_LARM, TRUE);
				
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
				}
			}
			break;
		}

		case TV_GLOVES:
		{
			if (o_ptr->sval == SV_SET_OF_DRAGON_GLOVES)
			{
				/* Mention the item */
				if (cheat_peek) object_mention(o_ptr);
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
				/* Mention the item */
				if (cheat_peek) object_mention(o_ptr);
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
				o_ptr->name2 = get_random_ego(INVEN_HEAD, FALSE);
			}

			break;
		}

		case TV_HELM:
		{
			if (o_ptr->sval == SV_DRAGON_HELM)
			{
				/* Mention the item */
				if (cheat_peek) object_mention(o_ptr);
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
					case EGO_INTELLIGENCE:
					case EGO_WISDOM:
					case EGO_BEAUTY:
					case EGO_LITE:
					case EGO_DARK:
					case EGO_INFRAVISION:
						break;
					case EGO_SEEING:
						if (one_in_(7))
						{
							if (one_in_(2)) add_esp_strong(o_ptr);
							else add_esp_weak(o_ptr, FALSE);
						}
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
				o_ptr->name2 = get_random_ego(INVEN_HEAD, FALSE);
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


/*
 * Apply magic to an item known to be a "ring" or "amulet"
 *
 * Hack -- note special "pval boost" code for ring of speed
 * Hack -- note that some items must be cursed (or blessed)
 */
static void a_m_aux_3(object_type *o_ptr, int level, int power)
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
					o_ptr->pval = m_bonus(2, level);
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
					o_ptr->pval = 1 + m_bonus(5, level);

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
					o_ptr->pval = randint1(5) + m_bonus(5, level);

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

					/* Mention the item */
					if (cheat_peek) object_mention(o_ptr);

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
					o_ptr->to_a = 10 + randint1(5) + m_bonus(10, level);
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
					o_ptr->pval = 1 + m_bonus(5, level);

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
					o_ptr->to_a = 5 + randint1(5) + m_bonus(10, level);
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
					o_ptr->pval = 0 - (1 + m_bonus(5, level));
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
					o_ptr->to_a = 0 - (5 + m_bonus(10, level));
					o_ptr->pval = 0 - (1 + m_bonus(5, level));
					if (power > 0) power = 0 - power;

					break;
				}

				/* Ring of damage */
				case SV_RING_DAMAGE:
				{
					/* Bonus to damage */
					o_ptr->to_d = 1 + randint1(5) + m_bonus(16, level);

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
					o_ptr->to_h = 1 + randint1(5) + m_bonus(16, level);

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
					o_ptr->to_a = 5 + randint1(8) + m_bonus(10, level);

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
					o_ptr->to_d = randint1(5) + m_bonus(12, level);
					o_ptr->to_h = randint1(5) + m_bonus(12, level);

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
					o_ptr->pval = 1 + m_bonus(3, level);
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
						if (have_flag(k_ptr->flags, TR_LITE)) break;
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
				/* Uncurse it */
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
					o_ptr->pval = 1 + m_bonus(5, level);

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
					o_ptr->pval = randint1(2) + m_bonus(4, level);

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
					o_ptr->pval = randint1(5) + m_bonus(5, level);
					o_ptr->to_a = randint1(5) + m_bonus(5, level);

					/* gain one low ESP */
					add_esp_weak(o_ptr, FALSE);

					/* Mention the item */
					if (cheat_peek) object_mention(o_ptr);

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
					o_ptr->pval = 0 - (randint1(5) + m_bonus(5, level));
					o_ptr->to_a = 0 - (randint1(5) + m_bonus(5, level));
					if (power > 0) power = 0 - power;

					break;
				}

				case SV_AMULET_MAGIC_MASTERY:
				{
					o_ptr->pval = 1 + m_bonus(4, level);

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
						if (have_flag(k_ptr->flags, TR_HOLD_LIFE)) break;
						o_ptr->name2 = EGO_AMU_HOLD_LIFE;
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
				/* Uncurse it */
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


/*
 * Hack -- help pick an item type
 */
static bool item_monster_okay(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* No uniques */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);
	if (r_ptr->flags7 & RF7_KAGE) return (FALSE);
	if (r_ptr->flagsr & RFR_RES_ALL) return (FALSE);
	if (r_ptr->flags7 & RF7_NAZGUL) return (FALSE);
	if (r_ptr->flags1 & RF1_FORCE_DEPTH) return (FALSE);
	if (r_ptr->flags7 & RF7_UNIQUE2) return (FALSE);

	/* Okay */
	return (TRUE);
}


/*
 * Apply magic to an item known to be "boring"
 *
 * Hack -- note the special code for various items
 */
static void a_m_aux_4(object_type *o_ptr, int level, int power)
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
			int i = 1;
			int check;

			monster_race *r_ptr;

			/* Pick a random non-unique monster race */
			while (1)
			{
				i = randint1(max_r_idx - 1);

				if (!item_monster_okay(i)) continue;
				if (i == MON_TSUCHINOKO) continue;

				r_ptr = &r_info[i];

				check = (dun_level < r_ptr->level) ? (r_ptr->level - dun_level) : 0;

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

			if (cheat_peek)
			{
#ifdef JP
				msg_format("%sの人形, 深さ +%d%s",
#else
				msg_format("Figurine of %s, depth +%d%s",
#endif

							  r_name + r_ptr->name, check - 1,
							  !object_is_cursed(o_ptr) ? "" : " {cursed}");
			}

			break;
		}

		case TV_CORPSE:
		{
			int i = 1;
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
				i = get_mon_num(dun_level);

				r_ptr = &r_info[i];

				check = (dun_level < r_ptr->level) ? (r_ptr->level - dun_level) : 0;

				/* Ignore dead monsters */
				if (!r_ptr->rarity) continue;

				/* Ignore corpseless monsters */
				if (!(r_ptr->flags9 & match)) continue;

				/* Prefer less out-of-depth monsters */
				if (randint0(check)) continue;

				break;
			}

			o_ptr->pval = i;

			if (cheat_peek)
			{
#ifdef JP
				msg_format("%sの死体, 深さ +%d",
#else
				msg_format("Corpse of %s, depth +%d",
#endif

							  r_name + r_ptr->name, check - 1);
			}

			object_aware(o_ptr);
			object_known(o_ptr);
			break;
		}

		case TV_STATUE:
		{
			int i = 1;

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
#ifdef JP
				msg_format("%sの像", r_name + r_ptr->name);
#else
				msg_format("Statue of %s", r_name + r_ptr->name);
#endif

			}
			object_aware(o_ptr);
			object_known(o_ptr);

			break;
		}

		case TV_CHEST:
		{
			byte obj_level = k_info[o_ptr->k_idx].level;

			/* Hack -- skip ruined chests */
			if (obj_level <= 0) break;

			/* Hack -- pick a "difficulty" */
			o_ptr->pval = randint1(obj_level);
			if (o_ptr->sval == SV_CHEST_KANDUME) o_ptr->pval = 6;

			o_ptr->xtra3 = dun_level + 5;

			/* Never exceed "difficulty" of 55 to 59 */
			if (o_ptr->pval > 55) o_ptr->pval = 55 + (byte)randint0(5);

			break;
		}
	}
}


/*
 * Complete the "creation" of an object by applying "magic" to the item
 *
 * This includes not only rolling for random bonuses, but also putting the
 * finishing touches on ego-items and artifacts, giving charges to wands and
 * staffs, giving fuel to lites, and placing traps on chests.
 *
 * In particular, note that "Instant Artifacts", if "created" by an external
 * routine, must pass through this function to complete the actual creation.
 *
 * The base "chance" of the item being "good" increases with the "level"
 * parameter, which is usually derived from the dungeon level, being equal
 * to the level plus 10, up to a maximum of 75.  If "good" is true, then
 * the object is guaranteed to be "good".  If an object is "good", then
 * the chance that the object will be "great" (ego-item or artifact), also
 * increases with the "level", being equal to half the level, plus 5, up to
 * a maximum of 20.  If "great" is true, then the object is guaranteed to be
 * "great".  At dungeon level 65 and below, 15/100 objects are "great".
 *
 * If the object is not "good", there is a chance it will be "cursed", and
 * if it is "cursed", there is a chance it will be "broken".  These chances
 * are related to the "good" / "great" chances above.
 *
 * Otherwise "normal" rings and amulets will be "good" half the time and
 * "cursed" half the time, unless the ring/amulet is always good or cursed.
 *
 * If "okay" is true, and the object is going to be "great", then there is
 * a chance that an artifact will be created.  This is true even if both the
 * "good" and "great" arguments are false.  As a total hack, if "great" is
 * true, then the item gets 3 extra "attempts" to become an artifact.
 */
void apply_magic(object_type *o_ptr, int lev, u32b mode)
{
	int i, rolls, f1, f2, power;

	if (p_ptr->pseikaku == SEIKAKU_MUNCHKIN) lev += randint0(p_ptr->lev/2+10);

	/* Maximum "level" for various things */
	if (lev > MAX_DEPTH - 1) lev = MAX_DEPTH - 1;

	/* Base chance of being "good" */
	f1 = lev + 10;

	/* Maximal chance of being "good" */
	if (f1 > d_info[dungeon_type].obj_good) f1 = d_info[dungeon_type].obj_good;

	/* Base chance of being "great" */
	f2 = f1 * 2 / 3;

	/* Maximal chance of being "great" */
	if ((p_ptr->pseikaku != SEIKAKU_MUNCHKIN) && (f2 > d_info[dungeon_type].obj_great))
		f2 = d_info[dungeon_type].obj_great;

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
		if (character_dungeon)
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

		/* Hack -- extract the "broken" flag */
		if (!a_ptr->cost) o_ptr->ident |= (IDENT_BROKEN);

		/* Hack -- extract the "cursed" flag */
		if (a_ptr->gen_flags & TRG_CURSED) o_ptr->curse_flags |= (TRC_CURSED);
		if (a_ptr->gen_flags & TRG_HEAVY_CURSE) o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
		if (a_ptr->gen_flags & TRG_PERMA_CURSE) o_ptr->curse_flags |= (TRC_PERMA_CURSE);
		if (a_ptr->gen_flags & (TRG_RANDOM_CURSE0)) o_ptr->curse_flags |= get_curse(0, o_ptr);
		if (a_ptr->gen_flags & (TRG_RANDOM_CURSE1)) o_ptr->curse_flags |= get_curse(1, o_ptr);
		if (a_ptr->gen_flags & (TRG_RANDOM_CURSE2)) o_ptr->curse_flags |= get_curse(2, o_ptr);


		/* Cheat -- peek at the item */
		if (cheat_peek) object_mention(o_ptr);

		/* Done */
		return;
	}


	/* Apply magic */
	switch (o_ptr->tval)
	{
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_BOW:
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		{
			if (power) a_m_aux_1(o_ptr, lev, power);
			break;
		}

		case TV_POLEARM:
		{
			if (power && !(o_ptr->sval == SV_DEATH_SCYTHE)) a_m_aux_1(o_ptr, lev, power);
			break;
		}

		case TV_SWORD:
		{
			if (power && !(o_ptr->sval == SV_DOKUBARI)) a_m_aux_1(o_ptr, lev, power);
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

		/* Hack -- apply extra penalties if needed */
		if (object_is_cursed(o_ptr) || object_is_broken(o_ptr))
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

			/* Hack -- obtain pval */
			if (e_ptr->max_pval)
			{
				if ((o_ptr->name2 == EGO_HA) && (have_flag(o_ptr->art_flags, TR_BLOWS)))
				{
					o_ptr->pval++;
					if ((lev > 60) && one_in_(3) && ((o_ptr->dd*(o_ptr->ds+1)) < 15)) o_ptr->pval++;
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

		/* Cheat -- describe the item */
		if (cheat_peek) object_mention(o_ptr);

		/* Done */
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


/*
 * Hack -- determine if a template is "good"
 */
static bool kind_is_good(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	switch (k_ptr->tval)
	{
		/* Armor -- Good unless damaged */
		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		case TV_DRAG_ARMOR:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		{
			if (k_ptr->to_a < 0) return (FALSE);
			return (TRUE);
		}

		/* Weapons -- Good unless damaged */
		case TV_BOW:
		case TV_SWORD:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_DIGGING:
		{
			if (k_ptr->to_h < 0) return (FALSE);
			if (k_ptr->to_d < 0) return (FALSE);
			return (TRUE);
		}

		/* Ammo -- Arrows/Bolts are good */
		case TV_BOLT:
		case TV_ARROW:
		{
			return (TRUE);
		}

		/* Books -- High level books are good (except Arcane books) */
		case TV_LIFE_BOOK:
		case TV_SORCERY_BOOK:
		case TV_NATURE_BOOK:
		case TV_CHAOS_BOOK:
		case TV_DEATH_BOOK:
		case TV_TRUMP_BOOK:
		case TV_CRAFT_BOOK:
		case TV_DAEMON_BOOK:
		case TV_CRUSADE_BOOK:
		case TV_MUSIC_BOOK:
		case TV_HISSATSU_BOOK:
		case TV_HEX_BOOK:
		{
			if (k_ptr->sval >= SV_BOOK_MIN_GOOD) return (TRUE);
			return (FALSE);
		}

		/* Rings -- Rings of Speed are good */
		case TV_RING:
		{
			if (k_ptr->sval == SV_RING_SPEED) return (TRUE);
			if (k_ptr->sval == SV_RING_LORDLY) return (TRUE);
			return (FALSE);
		}

		/* Amulets -- Amulets of the Magi and Resistance are good */
		case TV_AMULET:
		{
			if (k_ptr->sval == SV_AMULET_THE_MAGI) return (TRUE);
			if (k_ptr->sval == SV_AMULET_RESISTANCE) return (TRUE);
			return (FALSE);
		}
	}

	/* Assume not good */
	return (FALSE);
}


/*
 * Attempt to make an object (normal or good/great)
 *
 * This routine plays nasty games to generate the "special artifacts".
 *
 * This routine uses "object_level" for the "generation level".
 *
 * We assume that the given object has been "wiped".
 */
bool make_object(object_type *j_ptr, u32b mode)
{
	int prob, base;
	byte obj_level;


	/* Chance of "special object" */
	prob = ((mode & AM_GOOD) ? 10 : 1000);

	/* Base level for the object */
	base = ((mode & AM_GOOD) ? (object_level + 10) : object_level);


	/* Generate a special object, or a normal object */
	if (!one_in_(prob) || !make_artifact_special(j_ptr))
	{
		int k_idx;

		/* Good objects */
		if ((mode & AM_GOOD) && !get_obj_num_hook)
		{
			/* Activate restriction (if already specified, use that) */
			get_obj_num_hook = kind_is_good;
		}

		/* Restricted objects - prepare allocation table */
		if (get_obj_num_hook) get_obj_num_prep();

		/* Pick a random object */
		k_idx = get_obj_num(base);

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
	apply_magic(j_ptr, object_level, mode);

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

	obj_level = k_info[j_ptr->k_idx].level;
	if (object_is_fixed_artifact(j_ptr)) obj_level = a_info[j_ptr->name1].level;

	/* Notice "okay" out-of-depth objects */
	if (!object_is_cursed(j_ptr) && !object_is_broken(j_ptr) &&
	    (obj_level > dun_level))
	{
		/* Cheat -- peek at items */
		if (cheat_peek) object_mention(j_ptr);
	}

	/* Success */
	return (TRUE);
}


/*
 * Attempt to place an object (normal or good/great) at the given location.
 *
 * This routine plays nasty games to generate the "special artifacts".
 *
 * This routine uses "object_level" for the "generation level".
 *
 * This routine requires a clean floor grid destination.
 */
void place_object(int y, int x, u32b mode)
{
	s16b o_idx;

	/* Acquire grid */
	cave_type *c_ptr = &cave[y][x];

	object_type forge;
	object_type *q_ptr;


	/* Paranoia -- check bounds */
	if (!in_bounds(y, x)) return;

	/* Require floor space */
	if (!cave_drop_bold(y, x)) return;

	/* Avoid stacking on other objects */
	if (c_ptr->o_idx) return;


	/* Get local object */
	q_ptr = &forge;

	/* Wipe the object */
	object_wipe(q_ptr);

	/* Make an object (if possible) */
	if (!make_object(q_ptr, mode)) return;


	/* Make an object */
	o_idx = o_pop();

	/* Success */
	if (o_idx)
	{
		object_type *o_ptr;

		/* Acquire object */
		o_ptr = &o_list[o_idx];

		/* Structure Copy */
		object_copy(o_ptr, q_ptr);

		/* Location */
		o_ptr->iy = y;
		o_ptr->ix = x;

		/* Build a stack */
		o_ptr->next_o_idx = c_ptr->o_idx;

		/* Place the object */
		c_ptr->o_idx = o_idx;

		/* Notice */
		note_spot(y, x);

		/* Redraw */
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


/*
 * Make a treasure object
 *
 * The location must be a legal, clean, floor grid.
 */
bool make_gold(object_type *j_ptr)
{
	int i;

	s32b base;


	/* Hack -- Pick a Treasure variety */
	i = ((randint1(object_level + 2) + 2) / 2) - 1;

	/* Apply "extra" magic */
	if (one_in_(GREAT_OBJ))
	{
		i += randint1(object_level + 1);
	}

	/* Hack -- Creeping Coins only generate "themselves" */
	if (coin_type) i = coin_type;

	/* Do not create "illegal" Treasure Types */
	if (i >= MAX_GOLD) i = MAX_GOLD - 1;

	/* Prepare a gold object */
	object_prep(j_ptr, OBJ_GOLD_LIST + i);

	/* Hack -- Base coin cost */
	base = k_info[OBJ_GOLD_LIST+i].cost;

	/* Determine how much the treasure is "worth" */
	j_ptr->pval = (base + (8L * randint1(base)) + randint1(8));

	/* Success */
	return (TRUE);
}


/*
 * Places a treasure (Gold or Gems) at given location
 *
 * The location must be a legal, clean, floor grid.
 */
void place_gold(int y, int x)
{
	s16b o_idx;

	/* Acquire grid */
	cave_type *c_ptr = &cave[y][x];


	object_type forge;
	object_type *q_ptr;


	/* Paranoia -- check bounds */
	if (!in_bounds(y, x)) return;

	/* Require floor space */
	if (!cave_drop_bold(y, x)) return;

	/* Avoid stacking on other objects */
	if (c_ptr->o_idx) return;


	/* Get local object */
	q_ptr = &forge;

	/* Wipe the object */
	object_wipe(q_ptr);

	/* Make some gold */
	if (!make_gold(q_ptr)) return;


	/* Make an object */
	o_idx = o_pop();

	/* Success */
	if (o_idx)
	{
		object_type *o_ptr;

		/* Acquire object */
		o_ptr = &o_list[o_idx];

		/* Copy the object */
		object_copy(o_ptr, q_ptr);

		/* Save location */
		o_ptr->iy = y;
		o_ptr->ix = x;

		/* Build a stack */
		o_ptr->next_o_idx = c_ptr->o_idx;

		/* Place the object */
		c_ptr->o_idx = o_idx;

		/* Notice */
		note_spot(y, x);

		/* Redraw */
		lite_spot(y, x);
	}
}


/*
 * Let an object fall to the ground at or near a location.
 *
 * The initial location is assumed to be "in_bounds()".
 *
 * This function takes a parameter "chance".  This is the percentage
 * chance that the item will "disappear" instead of drop.  If the object
 * has been thrown, then this is the chance of disappearance on contact.
 *
 * Hack -- this function uses "chance" to determine if it should produce
 * some form of "description" of the drop event (under the player).
 *
 * We check several locations to see if we can find a location at which
 * the object can combine, stack, or be placed.  Artifacts will try very
 * hard to be placed, including "teleporting" to a useful grid if needed.
 */
s16b drop_near(object_type *j_ptr, int chance, int y, int x)
{
	int i, k, d, s;

	int bs, bn;
	int by, bx;
	int dy, dx;
	int ty, tx = 0;

	s16b o_idx = 0;

	s16b this_o_idx, next_o_idx = 0;

	cave_type *c_ptr;

	char o_name[MAX_NLEN];

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
		/* Message */
#ifdef JP
		msg_format("%sは消えた。", o_name);
#else
		msg_format("The %s disappear%s.",
			   o_name, (plural ? "" : "s"));
#endif


		/* Debug */
#ifdef JP
		if (p_ptr->wizard) msg_print("(破損)");
#else
		if (p_ptr->wizard) msg_print("(breakage)");
#endif


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

			/* Location */
			ty = y + dy;
			tx = x + dx;

			/* Skip illegal grids */
			if (!in_bounds(ty, tx)) continue;

			/* Require line of projection */
			if (!projectable(y, x, ty, tx)) continue;

			/* Obtain grid */
			c_ptr = &cave[ty][tx];

			/* Require floor space */
			if (!cave_drop_bold(ty, tx)) continue;

			/* No objects */
			k = 0;

			/* Scan objects in that grid */
			for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
			{
				object_type *o_ptr;

				/* Acquire object */
				o_ptr = &o_list[this_o_idx];

				/* Acquire next object */
				next_o_idx = o_ptr->next_o_idx;

				/* Check for possible combination */
				if (object_similar(o_ptr, j_ptr)) comb = TRUE;

				/* Count objects */
				k++;
			}

			/* Add new object */
			if (!comb) k++;

			/* Paranoia */
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

			/* Okay */
			flag = TRUE;
		}
	}


	/* Handle lack of space */
	if (!flag && !object_is_artifact(j_ptr))
	{
		/* Message */
#ifdef JP
		msg_format("%sは消えた。", o_name);
#else
		msg_format("The %s disappear%s.",
			   o_name, (plural ? "" : "s"));
#endif


		/* Debug */
#ifdef JP
		if (p_ptr->wizard) msg_print("(床スペースがない)");
#else
		if (p_ptr->wizard) msg_print("(no floor space)");
#endif


		/* Failure */
		return (0);
	}


	/* Find a grid */
	for (i = 0; !flag && (i < 1000); i++)
	{
		/* Bounce around */
		ty = rand_spread(by, 1);
		tx = rand_spread(bx, 1);

		/* Verify location */
		if (!in_bounds(ty, tx)) continue;

		/* Bounce to that location */
		by = ty;
		bx = tx;

		/* Require floor space */
		if (!cave_drop_bold(by, bx)) continue;

		/* Okay */
		flag = TRUE;
	}


	if (!flag)
	{
		int candidates = 0, pick;

		for (ty = 1; ty < cur_hgt - 1; ty++)
		{
			for (tx = 1; tx < cur_wid - 1; tx++)
			{
				/* A valid space found */
				if (cave_drop_bold(ty, tx)) candidates++;
			}
		}

		/* No valid place! */
		if (!candidates)
		{
			/* Message */
#ifdef JP
			msg_format("%sは消えた。", o_name);
#else
			msg_format("The %s disappear%s.", o_name, (plural ? "" : "s"));
#endif

			/* Debug */
#ifdef JP
			if (p_ptr->wizard) msg_print("(床スペースがない)");
#else
			if (p_ptr->wizard) msg_print("(no floor space)");
#endif

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

		for (ty = 1; ty < cur_hgt - 1; ty++)
		{
			for (tx = 1; tx < cur_wid - 1; tx++)
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


	/* Grid */
	c_ptr = &cave[by][bx];

	/* Scan objects in that grid for combination */
	for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Acquire object */
		o_ptr = &o_list[this_o_idx];

		/* Acquire next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Check for combination */
		if (object_similar(o_ptr, j_ptr))
		{
			/* Combine the items */
			object_absorb(o_ptr, j_ptr);

			/* Success */
			done = TRUE;

			/* Done */
			break;
		}
	}

	/* Get new object */
	if (!done) o_idx = o_pop();

	/* Failure */
	if (!done && !o_idx)
	{
		/* Message */
#ifdef JP
		msg_format("%sは消えた。", o_name);
#else
		msg_format("The %s disappear%s.",
			   o_name, (plural ? "" : "s"));
#endif


		/* Debug */
#ifdef JP
		if (p_ptr->wizard) msg_print("(アイテムが多過ぎる)");
#else
		if (p_ptr->wizard) msg_print("(too many objects)");
#endif


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
		object_copy(&o_list[o_idx], j_ptr);

		/* Access new object */
		j_ptr = &o_list[o_idx];

		/* Locate */
		j_ptr->iy = by;
		j_ptr->ix = bx;

		/* No monster */
		j_ptr->held_m_idx = 0;

		/* Build a stack */
		j_ptr->next_o_idx = c_ptr->o_idx;

		/* Place the object */
		c_ptr->o_idx = o_idx;

		/* Success */
		done = TRUE;
	}

	/* Note the spot */
	note_spot(by, bx);

	/* Draw the spot */
	lite_spot(by, bx);

	/* Sound */
	sound(SOUND_DROP);

	/* Mega-Hack -- no message if "dropped" by player */
	/* Message when an object falls under the player */
	if (chance && player_bold(by, bx))
	{
#ifdef JP
		msg_print("何かが足下に転がってきた。");
#else
		msg_print("You feel something roll beneath your feet.");
#endif

	}

	/* XXX XXX XXX */

	/* Result */
	return (o_idx);
}


/*
 * Scatter some "great" objects near the player
 */
void acquirement(int y1, int x1, int num, bool great, bool known)
{
	object_type *i_ptr;
	object_type object_type_body;
	u32b mode = AM_GOOD | (great ? AM_GREAT : 0L);

	/* Acquirement */
	while (num--)
	{
		/* Get local object */
		i_ptr = &object_type_body;

		/* Wipe the object */
		object_wipe(i_ptr);

		/* Make a good (or great) object (if possible) */
		if (!make_object(i_ptr, mode)) continue;

		if (known)
		{
			object_aware(i_ptr);
			object_known(i_ptr);
		}

		/* Drop the object */
		(void)drop_near(i_ptr, -1, y1, x1);
	}
}


#define MAX_NORMAL_TRAPS 18

/* See init_feat_variables() in init2.c */
static s16b normal_traps[MAX_NORMAL_TRAPS];

/*
 * Initialize arrays for normal traps
 */
void init_normal_traps(void)
{
	int cur_trap = 0;

	normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_TRAPDOOR");
	normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_PIT");
	normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_SPIKED_PIT");
	normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_POISON_PIT");
	normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_TY_CURSE");
	normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_TELEPORT");
	normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_FIRE");
	normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_ACID");
	normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_SLOW");
	normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_LOSE_STR");
	normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_LOSE_DEX");
	normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_LOSE_CON");
	normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_BLIND");
	normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_CONFUSE");
	normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_POISON");
	normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_SLEEP");
	normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_TRAPS");
	normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_ALARM");
}

/*
 * Get random trap
 *
 * XXX XXX XXX This routine should be redone to reflect trap "level".
 * That is, it does not make sense to have spiked pits at 50 feet.
 * Actually, it is not this routine, but the "trap instantiation"
 * code, which should also check for "trap doors" on quest levels.
 */
s16b choose_random_trap(void)
{
	s16b feat;

	/* Pick a trap */
	while (1)
	{
		/* Hack -- pick a trap */
		feat = normal_traps[randint0(MAX_NORMAL_TRAPS)];

		/* Accept non-trapdoors */
		if (!have_flag(f_info[feat].flags, FF_MORE)) break;

		/* Hack -- no trap doors on special levels */
		if (p_ptr->inside_arena || quest_number(dun_level)) continue;

		/* Hack -- no trap doors on the deepest level */
		if (dun_level >= d_info[dungeon_type].maxdepth) continue;

		break;
	}

	return feat;
}

/*
 * Disclose an invisible trap
 */
void disclose_grid(int y, int x)
{
	cave_type *c_ptr = &cave[y][x];

	if (cave_have_flag_grid(c_ptr, FF_SECRET))
	{
		/* No longer hidden */
		cave_alter_feat(y, x, FF_SECRET);
	}
	else if (c_ptr->mimic)
	{
		/* No longer hidden */
		c_ptr->mimic = 0;

		/* Notice */
		note_spot(y, x);

		/* Redraw */
		lite_spot(y, x);
	}
}


/*
 * Places a random trap at the given location.
 *
 * The location must be a legal, naked, floor grid.
 *
 * Note that all traps start out as "invisible" and "untyped", and then
 * when they are "discovered" (by detecting them or setting them off),
 * the trap is "instantiated" as a visible, "typed", trap.
 */
void place_trap(int y, int x)
{
	cave_type *c_ptr = &cave[y][x];

	/* Paranoia -- verify location */
	if (!in_bounds(y, x)) return;

	/* Require empty, clean, floor grid */
	if (!cave_clean_bold(y, x)) return;

	/* Place an invisible trap */
	c_ptr->mimic = c_ptr->feat;
	c_ptr->feat = choose_random_trap();
}


/*
 * Describe the charges on an item in the inventory.
 */
void inven_item_charges(int item)
{
	object_type *o_ptr = &inventory[item];

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
		/* Print a message */
		msg_format("You have %d charges remaining.", o_ptr->pval);
	}

	/* Single charge */
	else
	{
		/* Print a message */
		msg_format("You have %d charge remaining.", o_ptr->pval);
	}
#endif

}


/*
 * Describe an item in the inventory.
 */
void inven_item_describe(int item)
{
	object_type *o_ptr = &inventory[item];
	char        o_name[MAX_NLEN];

	/* Get a description */
	object_desc(o_name, o_ptr, 0);

	/* Print a message */
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


/*
 * Increase the "number" of an item in the inventory
 */
void inven_item_increase(int item, int num)
{
	object_type *o_ptr = &inventory[item];

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

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Recalculate mana XXX */
		p_ptr->update |= (PU_MANA);

		/* Combine the pack */
		p_ptr->notice |= (PN_COMBINE);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Hack -- Clear temporary elemental brands if player takes off weapons */
		if (!o_ptr->number && p_ptr->ele_attack)
		{
			if ((item == INVEN_RARM) || (item == INVEN_LARM))
			{
				if (!buki_motteruka(INVEN_RARM + INVEN_LARM - item))
				{
					/* Clear all temporary elemental brands */
					set_ele_attack(0, 0);
				}
			}
		}
	}
}


/*
 * Erase an inventory slot if it has no more items
 */
void inven_item_optimize(int item)
{
	object_type *o_ptr = &inventory[item];

	/* Only optimize real items */
	if (!o_ptr->k_idx) return;

	/* Only optimize empty items */
	if (o_ptr->number) return;

	/* The item is in the pack */
	if (item < INVEN_RARM)
	{
		int i;

		/* One less item */
		inven_cnt--;

		/* Slide everything down */
		for (i = item; i < INVEN_PACK; i++)
		{
			/* Structure copy */
			inventory[i] = inventory[i+1];
		}

		/* Erase the "final" slot */
		object_wipe(&inventory[i]);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN);
	}

	/* The item is being wielded */
	else
	{
		/* One less item */
		equip_cnt--;

		/* Erase the empty slot */
		object_wipe(&inventory[item]);

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Recalculate torch */
		p_ptr->update |= (PU_TORCH);

		/* Recalculate mana XXX */
		p_ptr->update |= (PU_MANA);

		/* Window stuff */
		p_ptr->window |= (PW_EQUIP);
	}

	/* Window stuff */
	p_ptr->window |= (PW_SPELL);
}


/*
 * Describe the charges on an item on the floor.
 */
void floor_item_charges(int item)
{
	object_type *o_ptr = &o_list[item];

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
		/* Print a message */
		msg_format("There are %d charges remaining.", o_ptr->pval);
	}

	/* Single charge */
	else
	{
		/* Print a message */
		msg_format("There is %d charge remaining.", o_ptr->pval);
	}
#endif

}


/*
 * Describe an item in the inventory.
 */
void floor_item_describe(int item)
{
	object_type *o_ptr = &o_list[item];
	char        o_name[MAX_NLEN];

	/* Get a description */
	object_desc(o_name, o_ptr, 0);

	/* Print a message */
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


/*
 * Increase the "number" of an item on the floor
 */
void floor_item_increase(int item, int num)
{
	object_type *o_ptr = &o_list[item];

	/* Apply */
	num += o_ptr->number;

	/* Bounds check */
	if (num > 255) num = 255;
	else if (num < 0) num = 0;

	/* Un-apply */
	num -= o_ptr->number;

	/* Change the number */
	o_ptr->number += num;
}


/*
 * Optimize an item on the floor (destroy "empty" items)
 */
void floor_item_optimize(int item)
{
	object_type *o_ptr = &o_list[item];

	/* Paranoia -- be sure it exists */
	if (!o_ptr->k_idx) return;

	/* Only optimize empty items */
	if (o_ptr->number) return;

	/* Delete the object */
	delete_object_idx(item);
}


/*
 * Check if we have space for an item in the pack without overflow
 */
bool inven_carry_okay(object_type *o_ptr)
{
	int j;

	/* Empty slot? */
	if (inven_cnt < INVEN_PACK) return (TRUE);

	/* Similar slot? */
	for (j = 0; j < INVEN_PACK; j++)
	{
		object_type *j_ptr = &inventory[j];

		/* Skip non-objects */
		if (!j_ptr->k_idx) continue;

		/* Check if the two items can be combined */
		if (object_similar(j_ptr, o_ptr)) return (TRUE);
	}

	/* Nope */
	return (FALSE);
}


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


/*
 * Add an item to the players inventory, and return the slot used.
 *
 * If the new item can combine with an existing item in the inventory,
 * it will do so, using "object_similar()" and "object_absorb()", else,
 * the item will be placed into the "proper" location in the inventory.
 *
 * This function can be used to "over-fill" the player's pack, but only
 * once, and such an action must trigger the "overflow" code immediately.
 * Note that when the pack is being "over-filled", the new item must be
 * placed into the "overflow" slot, and the "overflow" must take place
 * before the pack is reordered, but (optionally) after the pack is
 * combined.  This may be tricky.  See "dungeon.c" for info.
 *
 * Note that this code must remove any location/stack information
 * from the object once it is placed into the inventory.
 */
s16b inven_carry(object_type *o_ptr)
{
	int i, j, k;
	int n = -1;

	object_type *j_ptr;


	/* Check for combining */
	for (j = 0; j < INVEN_PACK; j++)
	{
		j_ptr = &inventory[j];

		/* Skip non-objects */
		if (!j_ptr->k_idx) continue;

		/* Hack -- track last item */
		n = j;

		/* Check if the two items can be combined */
		if (object_similar(j_ptr, o_ptr))
		{
			/* Combine the items */
			object_absorb(j_ptr, o_ptr);

			/* Increase the weight */
			p_ptr->total_weight += (o_ptr->number * o_ptr->weight);

			/* Recalculate bonuses */
			p_ptr->update |= (PU_BONUS);

			/* Window stuff */
			p_ptr->window |= (PW_INVEN);

			/* Success */
			return (j);
		}
	}


	/* Paranoia */
	if (inven_cnt > INVEN_PACK) return (-1);

	/* Find an empty slot */
	for (j = 0; j <= INVEN_PACK; j++)
	{
		j_ptr = &inventory[j];

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
			if (object_sort_comp(o_ptr, o_value, &inventory[j])) break;
		}

		/* Use that slot */
		i = j;

		/* Slide objects */
		for (k = n; k >= i; k--)
		{
			/* Hack -- Slide the item */
			object_copy(&inventory[k+1], &inventory[k]);
		}

		/* Wipe the empty slot */
		object_wipe(&inventory[i]);
	}


	/* Copy the item */
	object_copy(&inventory[i], o_ptr);

	/* Access new object */
	j_ptr = &inventory[i];

	/* Forget stack */
	j_ptr->next_o_idx = 0;

	/* Forget monster */
	j_ptr->held_m_idx = 0;

	/* Forget location */
	j_ptr->iy = j_ptr->ix = 0;

	/* Player touches it, and no longer marked */
	j_ptr->marked = OM_TOUCHED;

	/* Increase the weight */
	p_ptr->total_weight += (j_ptr->number * j_ptr->weight);

	/* Count the items */
	inven_cnt++;

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine and Reorder pack */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN);

	/* Return the slot */
	return (i);
}


/*
 * Take off (some of) a non-cursed equipment item
 *
 * Note that only one item at a time can be wielded per slot.
 *
 * Note that taking off an item when "full" may cause that item
 * to fall to the ground.
 *
 * Return the inventory slot into which the item is placed.
 */
s16b inven_takeoff(int item, int amt)
{
	int slot;

	object_type forge;
	object_type *q_ptr;

	object_type *o_ptr;

	cptr act;

	char o_name[MAX_NLEN];


	/* Get the item to take off */
	o_ptr = &inventory[item];

	/* Paranoia */
	if (amt <= 0) return (-1);

	/* Verify */
	if (amt > o_ptr->number) amt = o_ptr->number;

	/* Get local object */
	q_ptr = &forge;

	/* Obtain a local object */
	object_copy(q_ptr, o_ptr);

	/* Modify quantity */
	q_ptr->number = amt;

	/* Describe the object */
	object_desc(o_name, q_ptr, 0);

	/* Took off weapon */
	if (((item == INVEN_RARM) || (item == INVEN_LARM)) &&
	    object_is_melee_weapon(o_ptr))
	{
#ifdef JP
		act = "を装備からはずした";
#else
		act = "You were wielding";
#endif

	}

	/* Took off bow */
	else if (item == INVEN_BOW)
	{
#ifdef JP
		act = "を装備からはずした";
#else
		act = "You were holding";
#endif

	}

	/* Took off light */
	else if (item == INVEN_LITE)
	{
#ifdef JP
		act = "を光源からはずした";
#else
		act = "You were holding";
#endif

	}

	/* Took off something */
	else
	{
#ifdef JP
		act = "を装備からはずした";
#else
		act = "You were wearing";
#endif

	}

	/* Modify, Optimize */
	inven_item_increase(item, -amt);
	inven_item_optimize(item);

	/* Carry the object */
	slot = inven_carry(q_ptr);

	/* Message */
#ifdef JP
	msg_format("%s(%c)%s。", o_name, index_to_label(slot), act);
#else
	msg_format("%s %s (%c).", act, o_name, index_to_label(slot));
#endif


	/* Return slot */
	return (slot);
}


/*
 * Drop (some of) a non-cursed inventory/equipment item
 *
 * The object will be dropped "near" the current location
 */
void inven_drop(int item, int amt)
{
	object_type forge;
	object_type *q_ptr;

	object_type *o_ptr;

	char o_name[MAX_NLEN];


	/* Access original object */
	o_ptr = &inventory[item];

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
		o_ptr = &inventory[item];
	}


	/* Get local object */
	q_ptr = &forge;

	/* Obtain local object */
	object_copy(q_ptr, o_ptr);

	/* Distribute charges of wands or rods */
	distribute_charges(o_ptr, q_ptr, amt);

	/* Modify quantity */
	q_ptr->number = amt;

	/* Describe local object */
	object_desc(o_name, q_ptr, 0);

	/* Message */
#ifdef JP
	msg_format("%s(%c)を落とした。", o_name, index_to_label(item));
#else
	msg_format("You drop %s (%c).", o_name, index_to_label(item));
#endif


	/* Drop it near the player */
	(void)drop_near(q_ptr, 0, py, px);

	/* Modify, Describe, Optimize */
	inven_item_increase(item, -amt);
	inven_item_describe(item);
	inven_item_optimize(item);
}


/*
 * Combine items in the pack
 *
 * Note special handling of the "overflow" slot
 */
void combine_pack(void)
{
	int             i, j, k;
	object_type     *o_ptr;
	object_type     *j_ptr;
	bool            flag = FALSE, combined;

	do
	{
		combined = FALSE;

		/* Combine the pack (backwards) */
		for (i = INVEN_PACK; i > 0; i--)
		{
			/* Get the item */
			o_ptr = &inventory[i];

			/* Skip empty items */
			if (!o_ptr->k_idx) continue;

			/* Scan the items above that item */
			for (j = 0; j < i; j++)
			{
				int max_num;

				/* Get the item */
				j_ptr = &inventory[j];

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
						inven_cnt--;

						/* Slide everything down */
						for (k = i; k < INVEN_PACK; k++)
						{
							/* Structure copy */
							inventory[k] = inventory[k+1];
						}

						/* Erase the "final" slot */
						object_wipe(&inventory[k]);
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

					/* Window stuff */
					p_ptr->window |= (PW_INVEN);

					/* Take note */
					combined = TRUE;

					/* Done */
					break;
				}
			}
		}
	}
	while (combined);

	/* Message */
#ifdef JP
	if (flag) msg_print("ザックの中のアイテムをまとめ直した。");
#else
	if (flag) msg_print("You combine some items in your pack.");
#endif
}


/*
 * Reorder items in the pack
 *
 * Note special handling of the "overflow" slot
 */
void reorder_pack(void)
{
	int             i, j, k;
	s32b            o_value;
	object_type     forge;
	object_type     *q_ptr;
	object_type     *o_ptr;
	bool            flag = FALSE;


	/* Re-order the pack (forwards) */
	for (i = 0; i < INVEN_PACK; i++)
	{
		/* Mega-Hack -- allow "proper" over-flow */
		if ((i == INVEN_PACK) && (inven_cnt == INVEN_PACK)) break;

		/* Get the item */
		o_ptr = &inventory[i];

		/* Skip empty slots */
		if (!o_ptr->k_idx) continue;

		/* Get the "value" of the item */
		o_value = object_value(o_ptr);

		/* Scan every occupied slot */
		for (j = 0; j < INVEN_PACK; j++)
		{
			if (object_sort_comp(o_ptr, o_value, &inventory[j])) break;
		}

		/* Never move down */
		if (j >= i) continue;

		/* Take note */
		flag = TRUE;

		/* Get local object */
		q_ptr = &forge;

		/* Save a copy of the moving item */
		object_copy(q_ptr, &inventory[i]);

		/* Slide the objects */
		for (k = i; k > j; k--)
		{
			/* Slide the item */
			object_copy(&inventory[k], &inventory[k-1]);
		}

		/* Insert the moving item */
		object_copy(&inventory[j], q_ptr);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN);
	}

	/* Message */
#ifdef JP
	if (flag) msg_print("ザックの中のアイテムを並べ直した。");
#else
	if (flag) msg_print("You reorder some items in your pack.");
#endif

}


/*
 * Hack -- display an object kind in the current window
 *
 * Include list of usable spells for readible books
 */
void display_koff(int k_idx)
{
	int y;

	object_type forge;
	object_type *q_ptr;
	int         sval;
	int         use_realm;

	char o_name[MAX_NLEN];


	/* Erase the window */
	for (y = 0; y < Term->hgt; y++)
	{
		/* Erase the line */
		Term_erase(0, y, 255);
	}

	/* No info */
	if (!k_idx) return;

	/* Get local object */
	q_ptr = &forge;

	/* Prepare the object */
	object_prep(q_ptr, k_idx);

	/* Describe */
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
		byte    spells[64];

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

/* Choose one of items that have warning flag */
object_type *choose_warning_item(void)
{
	int i;
	int choices[INVEN_TOTAL - INVEN_RARM];
	int number = 0;

	/* Paranoia -- Player has no warning ability */
	if (!p_ptr->warning) return NULL;

	/* Search Inventory */
	for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		u32b flgs[TR_FLAG_SIZE];
		object_type *o_ptr = &inventory[i];

		object_flags(o_ptr, flgs);
		if (have_flag(flgs, TR_WARNING))
		{
			choices[number] = i;
			number++;
		}
	}

	/* Choice one of them */
	return number ? &inventory[choices[randint0(number)]] : NULL;
}

/* Calculate spell damages */
static void spell_damcalc(monster_type *m_ptr, int typ, int dam, int limit, int *max)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	int          rlev = r_ptr->level;
	bool         ignore_wraith_form = FALSE;

	if (limit) dam = (dam > limit) ? limit : dam;

	/* Vulnerability, resistance and immunity */
	switch (typ)
	{
	case GF_ELEC:
		if (p_ptr->immune_elec)
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		else
		{
			if (p_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
			if (p_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;
			if (prace_is_(RACE_ANDROID)) dam += dam / 3;
			if (p_ptr->resist_elec) dam = (dam + 2) / 3;
			if (IS_OPPOSE_ELEC())
				dam = (dam + 2) / 3;
		}
		break;

	case GF_POIS:
		if (p_ptr->resist_pois) dam = (dam + 2) / 3;
		if (IS_OPPOSE_POIS()) dam = (dam + 2) / 3;
		break;

	case GF_ACID:
		if (p_ptr->immune_acid)
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		else
		{
			if (p_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
			if (p_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;
			if (p_ptr->resist_acid) dam = (dam + 2) / 3;
			if (IS_OPPOSE_ACID()) dam = (dam + 2) / 3;
		}
		break;

	case GF_COLD:
	case GF_ICE:
		if (p_ptr->immune_cold)
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		else
		{
			if (p_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
			if (p_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;
			if (p_ptr->resist_cold) dam = (dam + 2) / 3;
			if (IS_OPPOSE_COLD()) dam = (dam + 2) / 3;
		}
		break;

	case GF_FIRE:
		if (p_ptr->immune_fire)
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		else
		{
			if (p_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
			if (prace_is_(RACE_ENT)) dam += dam / 3;
			if (p_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;
			if (p_ptr->resist_fire) dam = (dam + 2) / 3;
			if (IS_OPPOSE_FIRE()) dam = (dam + 2) / 3;
		}
		break;

	case GF_PSY_SPEAR:
		ignore_wraith_form = TRUE;
		break;

	case GF_ARROW:
		if (!p_ptr->blind &&
		    ((inventory[INVEN_RARM].k_idx && (inventory[INVEN_RARM].name1 == ART_ZANTETSU)) ||
		     (inventory[INVEN_LARM].k_idx && (inventory[INVEN_LARM].name1 == ART_ZANTETSU))))
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		break;

	case GF_LITE:
		if (p_ptr->resist_lite) dam /= 2; /* Worst case of 4 / (d4 + 7) */
		if (prace_is_(RACE_VAMPIRE) || (p_ptr->mimic_form == MIMIC_VAMPIRE)) dam *= 2;
		else if (prace_is_(RACE_S_FAIRY)) dam = dam * 4 / 3;

		/*
		 * Cannot use "ignore_wraith_form" strictly (for "random one damage")
		 * "dam *= 2;" for later "dam /= 2"
		 */
		if (p_ptr->wraith_form) dam *= 2;
		break;

	case GF_DARK:
		if (prace_is_(RACE_VAMPIRE) || (p_ptr->mimic_form == MIMIC_VAMPIRE) || p_ptr->wraith_form)
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		else if (p_ptr->resist_dark) dam /= 2; /* Worst case of 4 / (d4 + 7) */
		break;

	case GF_SHARDS:
		if (p_ptr->resist_shard) dam = dam * 3 / 4; /* Worst case of 6 / (d4 + 7) */
		break;

	case GF_SOUND:
		if (p_ptr->resist_sound) dam = dam * 5 / 8; /* Worst case of 5 / (d4 + 7) */
		break;

	case GF_CONFUSION:
		if (p_ptr->resist_conf) dam = dam * 5 / 8; /* Worst case of 5 / (d4 + 7) */
		break;

	case GF_CHAOS:
		if (p_ptr->resist_chaos) dam = dam * 3 / 4; /* Worst case of 6 / (d4 + 7) */
		break;

	case GF_NETHER:
		if (prace_is_(RACE_SPECTRE))
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		else if (p_ptr->resist_neth) dam = dam * 3 / 4; /* Worst case of 6 / (d4 + 7) */
		break;

	case GF_DISENCHANT:
		if (p_ptr->resist_disen) dam = dam * 3 / 4; /* Worst case of 6 / (d4 + 7) */
		break;

	case GF_NEXUS:
		if (p_ptr->resist_nexus) dam = dam * 3 / 4; /* Worst case of 6 / (d4 + 7) */
		break;

	case GF_TIME:
		if (p_ptr->resist_time) dam /= 2; /* Worst case of 4 / (d4 + 7) */
		break;

	case GF_GRAVITY:
		if (p_ptr->levitation) dam = (dam * 2) / 3;
		break;

	case GF_ROCKET:
		if (p_ptr->resist_shard) dam /= 2;
		break;

	case GF_NUKE:
		if (p_ptr->resist_pois) dam = (2 * dam + 2) / 5;
		if (IS_OPPOSE_POIS()) dam = (2 * dam + 2) / 5;
		break;

	case GF_DEATH_RAY:
		if (p_ptr->mimic_form)
		{
			if (mimic_info[p_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_NONLIVING)
			{
				dam = 0;
				ignore_wraith_form = TRUE;
			}
		}
		else
		{
			switch (p_ptr->prace)
			{
			case RACE_GOLEM:
			case RACE_SKELETON:
			case RACE_ZOMBIE:
			case RACE_VAMPIRE:
			case RACE_DEMON:
			case RACE_SPECTRE:
				dam = 0;
				ignore_wraith_form = TRUE;
				break;
			}
		}
		break;

	case GF_HOLY_FIRE:
		if (p_ptr->align > 10) dam /= 2;
		else if (p_ptr->align < -10) dam *= 2;
		break;

	case GF_HELL_FIRE:
		if (p_ptr->align > 10) dam *= 2;
		break;

	case GF_MIND_BLAST:
	case GF_BRAIN_SMASH:
		if (100 + rlev / 2 <= MAX(5, p_ptr->skill_sav))
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		break;

	case GF_CAUSE_1:
	case GF_CAUSE_2:
	case GF_CAUSE_3:
	case GF_HAND_DOOM:
		if (100 + rlev / 2 <= p_ptr->skill_sav)
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		break;

	case GF_CAUSE_4:
		if ((100 + rlev / 2 <= p_ptr->skill_sav) && (m_ptr->r_idx != MON_KENSHIROU))
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		break;
	}

	if (p_ptr->wraith_form && !ignore_wraith_form)
	{
		dam /= 2;
		if (!dam) dam = 1;
	}

	if (dam > *max) *max = dam;
}

/* Calculate blow damages */
static int blow_damcalc(monster_type *m_ptr, monster_blow *blow_ptr)
{
	int  dam = blow_ptr->d_dice * blow_ptr->d_side;
	int  dummy_max = 0;
	bool check_wraith_form = TRUE;

	if (blow_ptr->method != RBM_EXPLODE)
	{
		int ac = p_ptr->ac + p_ptr->to_a;

		switch (blow_ptr->effect)
		{
		case RBE_SUPERHURT:
		{
			int tmp_dam = dam - (dam * ((ac < 150) ? ac : 150) / 250);
			dam = MAX(dam, tmp_dam * 2);
			break;
		}

		case RBE_HURT:
		case RBE_SHATTER:
			dam -= (dam * ((ac < 150) ? ac : 150) / 250);
			break;

		case RBE_ACID:
			spell_damcalc(m_ptr, GF_ACID, dam, 0, &dummy_max);
			dam = dummy_max;
			check_wraith_form = FALSE;
			break;

		case RBE_ELEC:
			spell_damcalc(m_ptr, GF_ELEC, dam, 0, &dummy_max);
			dam = dummy_max;
			check_wraith_form = FALSE;
			break;

		case RBE_FIRE:
			spell_damcalc(m_ptr, GF_FIRE, dam, 0, &dummy_max);
			dam = dummy_max;
			check_wraith_form = FALSE;
			break;

		case RBE_COLD:
			spell_damcalc(m_ptr, GF_COLD, dam, 0, &dummy_max);
			dam = dummy_max;
			check_wraith_form = FALSE;
			break;

		case RBE_DR_MANA:
			dam = 0;
			check_wraith_form = FALSE;
			break;
		}

		if (check_wraith_form && p_ptr->wraith_form)
		{
			dam /= 2;
			if (!dam) dam = 1;
		}
	}
	else
	{
		dam = (dam + 1) / 2;
		spell_damcalc(m_ptr, mbe_info[blow_ptr->effect].explode_type, dam, 0, &dummy_max);
		dam = dummy_max;
	}

	return dam;
}

/* Examine the grid (xx,yy) and warn the player if there are any danger */
bool process_warning(int xx, int yy)
{
	int mx, my;
	cave_type *c_ptr;
	char o_name[MAX_NLEN];

#define WARNING_AWARE_RANGE 12
	int dam_max = 0;
	static int old_damage = 0;

	for (mx = xx - WARNING_AWARE_RANGE; mx < xx + WARNING_AWARE_RANGE + 1; mx++)
	{
		for (my = yy - WARNING_AWARE_RANGE; my < yy + WARNING_AWARE_RANGE + 1; my++)
		{
			int dam_max0 = 0;
			monster_type *m_ptr;
			monster_race *r_ptr;

			if (!in_bounds(my, mx) || (distance(my, mx, yy, xx) > WARNING_AWARE_RANGE)) continue;

			c_ptr = &cave[my][mx];

			if (!c_ptr->m_idx) continue;

			m_ptr = &m_list[c_ptr->m_idx];

			if (MON_CSLEEP(m_ptr)) continue;
			if (!is_hostile(m_ptr)) continue;

			r_ptr = &r_info[m_ptr->r_idx];

			/* Monster spells (only powerful ones)*/
			if (projectable(my, mx, yy, xx))
			{
				int breath_dam_div3 = m_ptr->hp / 3;
				int breath_dam_div6 = m_ptr->hp / 6;
				u32b f4 = r_ptr->flags4;
				u32b f5 = r_ptr->flags5;
				u32b f6 = r_ptr->flags6;

				if (!(d_info[dungeon_type].flags1 & DF1_NO_MAGIC))
				{
					int rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);
					int storm_dam = rlev * 4 + 150;
					bool powerful = (bool)(r_ptr->flags2 & RF2_POWERFUL);

					if (f4 & RF4_BA_CHAO) spell_damcalc(m_ptr, GF_CHAOS, rlev * (powerful ? 3 : 2) + 100, 0, &dam_max0);
					if (f5 & RF5_BA_MANA) spell_damcalc(m_ptr, GF_MANA, storm_dam, 0, &dam_max0);
					if (f5 & RF5_BA_DARK) spell_damcalc(m_ptr, GF_DARK, storm_dam, 0, &dam_max0);
					if (f5 & RF5_BA_LITE) spell_damcalc(m_ptr, GF_LITE, storm_dam, 0, &dam_max0);
					if (f6 & RF6_HAND_DOOM) spell_damcalc(m_ptr, GF_HAND_DOOM, p_ptr->chp * 6 / 10, 0, &dam_max0);
					if (f6 & RF6_PSY_SPEAR) spell_damcalc(m_ptr, GF_PSY_SPEAR, powerful ? (rlev * 2 + 150) : (rlev * 3 / 2 + 100), 0, &dam_max0);
				}
				if (f4 & RF4_ROCKET) spell_damcalc(m_ptr, GF_ROCKET, m_ptr->hp / 4, 800, &dam_max0);
				if (f4 & RF4_BR_ACID) spell_damcalc(m_ptr, GF_ACID, breath_dam_div3, 1600, &dam_max0);
				if (f4 & RF4_BR_ELEC) spell_damcalc(m_ptr, GF_ELEC, breath_dam_div3, 1600, &dam_max0);
				if (f4 & RF4_BR_FIRE) spell_damcalc(m_ptr, GF_FIRE, breath_dam_div3, 1600, &dam_max0);
				if (f4 & RF4_BR_COLD) spell_damcalc(m_ptr, GF_COLD, breath_dam_div3, 1600, &dam_max0);
				if (f4 & RF4_BR_POIS) spell_damcalc(m_ptr, GF_POIS, breath_dam_div3, 800, &dam_max0);
				if (f4 & RF4_BR_NETH) spell_damcalc(m_ptr, GF_NETHER, breath_dam_div6, 550, &dam_max0);
				if (f4 & RF4_BR_LITE) spell_damcalc(m_ptr, GF_LITE, breath_dam_div6, 400, &dam_max0);
				if (f4 & RF4_BR_DARK) spell_damcalc(m_ptr, GF_DARK, breath_dam_div6, 400, &dam_max0);
				if (f4 & RF4_BR_CONF) spell_damcalc(m_ptr, GF_CONFUSION, breath_dam_div6, 450, &dam_max0);
				if (f4 & RF4_BR_SOUN) spell_damcalc(m_ptr, GF_SOUND, breath_dam_div6, 450, &dam_max0);
				if (f4 & RF4_BR_CHAO) spell_damcalc(m_ptr, GF_CHAOS, breath_dam_div6, 600, &dam_max0);
				if (f4 & RF4_BR_DISE) spell_damcalc(m_ptr, GF_DISENCHANT, breath_dam_div6, 500, &dam_max0);
				if (f4 & RF4_BR_NEXU) spell_damcalc(m_ptr, GF_NEXUS, breath_dam_div3, 250, &dam_max0);
				if (f4 & RF4_BR_TIME) spell_damcalc(m_ptr, GF_TIME, breath_dam_div3, 150, &dam_max0);
				if (f4 & RF4_BR_INER) spell_damcalc(m_ptr, GF_INERTIA, breath_dam_div6, 200, &dam_max0);
				if (f4 & RF4_BR_GRAV) spell_damcalc(m_ptr, GF_GRAVITY, breath_dam_div3, 200, &dam_max0);
				if (f4 & RF4_BR_SHAR) spell_damcalc(m_ptr, GF_SHARDS, breath_dam_div6, 500, &dam_max0);
				if (f4 & RF4_BR_PLAS) spell_damcalc(m_ptr, GF_PLASMA, breath_dam_div6, 150, &dam_max0);
				if (f4 & RF4_BR_WALL) spell_damcalc(m_ptr, GF_FORCE, breath_dam_div6, 200, &dam_max0);
				if (f4 & RF4_BR_MANA) spell_damcalc(m_ptr, GF_MANA, breath_dam_div3, 250, &dam_max0);
				if (f4 & RF4_BR_NUKE) spell_damcalc(m_ptr, GF_NUKE, breath_dam_div3, 800, &dam_max0);
				if (f4 & RF4_BR_DISI) spell_damcalc(m_ptr, GF_DISINTEGRATE, breath_dam_div6, 150, &dam_max0);
			}

			/* Monster melee attacks */
			if (!(r_ptr->flags1 & RF1_NEVER_BLOW) && !(d_info[dungeon_type].flags1 & DF1_NO_MELEE))
			{
				if (mx <= xx + 1 && mx >= xx - 1 && my <= yy + 1 && my >= yy - 1)
				{
					int m;
					int dam_melee = 0;
					for (m = 0; m < 4; m++)
					{
						/* Skip non-attacks */
						if (!r_ptr->blow[m].method || (r_ptr->blow[m].method == RBM_SHOOT)) continue;

						/* Extract the attack info */
						dam_melee += blow_damcalc(m_ptr, &r_ptr->blow[m]);
						if (r_ptr->blow[m].method == RBM_EXPLODE) break;
					}
					if (dam_melee > dam_max0) dam_max0 = dam_melee;
				}
			}

			/* Contribution from this monster */
			dam_max += dam_max0;
		}
	}

	/* Prevent excessive warning */
	if (dam_max > old_damage)
	{
		old_damage = dam_max * 3 / 2;

		if (dam_max > p_ptr->chp / 2)
		{
			object_type *o_ptr = choose_warning_item();

			if (o_ptr) object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
			else strcpy(o_name, "体"); /* Warning ability without item */
			msg_format("%sが鋭く震えた！", o_name);
#else
			else strcpy(o_name, "body"); /* Warning ability without item */
			msg_format("Your %s pulsates sharply!", o_name);
#endif
			disturb(0, 0);
#ifdef JP
			return get_check("本当にこのまま進むか？");
#else
			return get_check("Really want to go ahead? ");
#endif
		}
	}
	else old_damage = old_damage / 2;

	c_ptr = &cave[yy][xx];
	if (((!easy_disarm && is_trap(c_ptr->feat))
	    || (c_ptr->mimic && is_trap(c_ptr->feat))) && !one_in_(13))
	{
		object_type *o_ptr = choose_warning_item();

		if (o_ptr) object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
		else strcpy(o_name, "体"); /* Warning ability without item */
		msg_format("%sが震えた！", o_name);
#else
		else strcpy(o_name, "body"); /* Warning ability without item */
		msg_format("Your %s pulsates!", o_name);
#endif
		disturb(0, 0);
#ifdef JP
		return get_check("本当にこのまま進むか？");
#else
		return get_check("Really want to go ahead? ");
#endif
	}

	return TRUE;
}


static bool item_tester_hook_melee_ammo(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_DIGGING:
		case TV_BOLT:
		case TV_ARROW:
		case TV_SHOT:
		{
			return (TRUE);
		}
		case TV_SWORD:
		{
			if (o_ptr->sval != SV_DOKUBARI) return (TRUE);
		}
	}

	return (FALSE);
}


/*
 *  A structure for smithing
 */
typedef struct {
	int add;       /* TR flag number or special essence id */
	cptr add_name; /* Name of this ability */
	int type;      /* Menu number */
	int essence;   /* Index for carrying essences */
	int value;     /* Needed value to add this ability */
} essence_type;


/*
 *  Smithing type data for Weapon smith
 */
#ifdef JP
static essence_type essence_info[] = 
{
	{TR_STR, "腕力", 4, TR_STR, 20},
	{TR_INT, "知能", 4, TR_INT, 20},
	{TR_WIS, "賢さ", 4, TR_WIS, 20},
	{TR_DEX, "器用さ", 4, TR_DEX, 20},
	{TR_CON, "耐久力", 4, TR_CON, 20},
	{TR_CHR, "魅力", 4, TR_CHR, 20},
	{TR_MAGIC_MASTERY, "魔力支配", 4, TR_MAGIC_MASTERY, 20},
	{TR_STEALTH, "隠密", 4, TR_STEALTH, 40},
	{TR_SEARCH, "探索", 4, TR_SEARCH, 15},
	{TR_INFRA, "赤外線視力", 4, TR_INFRA, 15},
	{TR_TUNNEL, "採掘", 4, TR_TUNNEL, 15},
	{TR_SPEED, "スピード", 4, TR_SPEED, 12},
	{TR_BLOWS, "追加攻撃", 1, TR_BLOWS, 20},
	{TR_CHAOTIC, "カオス攻撃", 1, TR_CHAOTIC, 15},
	{TR_VAMPIRIC, "吸血攻撃", 1, TR_VAMPIRIC, 60},
	{TR_IMPACT, "地震発動", 7, TR_IMPACT, 15},
	{TR_BRAND_POIS, "毒殺", 1, TR_BRAND_POIS, 20},
	{TR_BRAND_ACID, "溶解", 1, TR_BRAND_ACID, 20},
	{TR_BRAND_ELEC, "電撃", 1, TR_BRAND_ELEC, 20},
	{TR_BRAND_FIRE, "焼棄", 1, TR_BRAND_FIRE, 20},
	{TR_BRAND_COLD, "凍結", 1, TR_BRAND_COLD, 20},
	{TR_SUST_STR, "腕力維持", 3, TR_SUST_STR, 15},
	{TR_SUST_INT, "知能維持", 3, TR_SUST_STR, 15},
	{TR_SUST_WIS, "賢さ維持", 3, TR_SUST_STR, 15},
	{TR_SUST_DEX, "器用さ維持", 3, TR_SUST_STR, 15},
	{TR_SUST_CON, "耐久力維持", 3, TR_SUST_STR, 15},
	{TR_SUST_CHR, "魅力維持", 3, TR_SUST_STR, 15},
	{TR_IM_ACID, "酸免疫", 2, TR_IM_ACID, 20},
	{TR_IM_ELEC, "電撃免疫", 2, TR_IM_ACID, 20},
	{TR_IM_FIRE, "火炎免疫", 2, TR_IM_ACID, 20},
	{TR_IM_COLD, "冷気免疫", 2, TR_IM_ACID, 20},
	{TR_REFLECT, "反射", 2, TR_REFLECT, 20},
	{TR_FREE_ACT, "麻痺知らず", 3, TR_FREE_ACT, 20},
	{TR_HOLD_LIFE, "生命力維持", 3, TR_HOLD_LIFE, 20},
	{TR_RES_ACID, "耐酸", 2, TR_RES_ACID, 15},
	{TR_RES_ELEC, "耐電撃", 2, TR_RES_ELEC, 15},
	{TR_RES_FIRE, "耐火炎", 2, TR_RES_FIRE, 15},
	{TR_RES_COLD, "耐冷気", 2, TR_RES_COLD, 15},
	{TR_RES_POIS, "耐毒", 2, TR_RES_POIS, 25},
	{TR_RES_FEAR, "耐恐怖", 2, TR_RES_FEAR, 20},
	{TR_RES_LITE, "耐閃光", 2, TR_RES_LITE, 20},
	{TR_RES_DARK, "耐暗黒", 2, TR_RES_DARK, 20},
	{TR_RES_BLIND, "耐盲目", 2, TR_RES_BLIND, 20},
	{TR_RES_CONF, "耐混乱", 2, TR_RES_CONF, 20},
	{TR_RES_SOUND, "耐轟音", 2, TR_RES_SOUND, 20},
	{TR_RES_SHARDS, "耐破片", 2, TR_RES_SHARDS, 20},
	{TR_RES_NETHER, "耐地獄", 2, TR_RES_NETHER, 20},
	{TR_RES_NEXUS, "耐因果混乱", 2, TR_RES_NEXUS, 20},
	{TR_RES_CHAOS, "耐カオス", 2, TR_RES_CHAOS, 20},
	{TR_RES_DISEN, "耐劣化", 2, TR_RES_DISEN, 20},
	{TR_SH_FIRE, "", 0, -2, 0},
	{TR_SH_ELEC, "", 0, -2, 0},
	{TR_SH_COLD, "", 0, -2, 0},
	{TR_NO_MAGIC, "反魔法", 3, TR_NO_MAGIC, 15},
	{TR_WARNING, "警告", 3, TR_WARNING, 20},
	{TR_LEVITATION, "浮遊", 3, TR_LEVITATION, 20},
	{TR_LITE, "永久光源", 3, TR_LITE, 15},
	{TR_SEE_INVIS, "可視透明", 3, TR_SEE_INVIS, 20},
	{TR_TELEPATHY, "テレパシー", 6, TR_TELEPATHY, 15},
	{TR_SLOW_DIGEST, "遅消化", 3, TR_SLOW_DIGEST, 15},
	{TR_REGEN, "急速回復", 3, TR_REGEN, 20},
	{TR_TELEPORT, "テレポート", 3, TR_TELEPORT, 25},

	{TR_SLAY_EVIL, "邪悪倍打", 5, TR_SLAY_EVIL, 100},
	{TR_KILL_EVIL, "邪悪倍倍打", 0, TR_SLAY_EVIL, 60},
	{TR_SLAY_ANIMAL, "動物倍打", 5, TR_SLAY_ANIMAL, 20},
	{TR_KILL_ANIMAL, "動物倍倍打", 5, TR_SLAY_ANIMAL, 60},
	{TR_SLAY_UNDEAD, "不死倍打", 5, TR_SLAY_UNDEAD, 20},
	{TR_KILL_UNDEAD, "不死倍倍打", 5, TR_SLAY_UNDEAD, 60},
	{TR_SLAY_DEMON, "悪魔倍打", 5, TR_SLAY_DEMON, 20},
	{TR_KILL_DEMON, "悪魔倍倍打", 5, TR_SLAY_DEMON, 60},
	{TR_SLAY_ORC, "オーク倍打", 5, TR_SLAY_ORC, 15},
	{TR_KILL_ORC, "オーク倍倍打", 5, TR_SLAY_ORC, 60},
	{TR_SLAY_TROLL, "トロル倍打", 5, TR_SLAY_TROLL, 15},
	{TR_KILL_TROLL, "トロル倍倍打", 5, TR_SLAY_TROLL, 60},
	{TR_SLAY_GIANT, "巨人倍打", 5, TR_SLAY_GIANT, 20},
	{TR_KILL_GIANT, "巨人倍倍打", 5, TR_SLAY_GIANT, 60},       
	{TR_SLAY_DRAGON, "竜倍打", 5, TR_SLAY_DRAGON, 20},
	{TR_KILL_DRAGON, "竜倍倍打", 5, TR_SLAY_DRAGON, 60},
	{TR_SLAY_HUMAN, "人間倍打", 5, TR_SLAY_HUMAN, 20},
	{TR_KILL_HUMAN, "人間倍倍打", 5, TR_SLAY_HUMAN, 60},

	{TR_ESP_ANIMAL, "動物ESP", 6, TR_SLAY_ANIMAL, 40},
	{TR_ESP_UNDEAD, "不死ESP", 6, TR_SLAY_UNDEAD, 40}, 
	{TR_ESP_DEMON, "悪魔ESP", 6, TR_SLAY_DEMON, 40},       
	{TR_ESP_ORC, "オークESP", 6, TR_SLAY_ORC, 40},     
	{TR_ESP_TROLL, "トロルESP", 6, TR_SLAY_TROLL, 40},   
	{TR_ESP_GIANT, "巨人ESP", 6, TR_SLAY_GIANT, 40},       
	{TR_ESP_DRAGON, "竜ESP", 6, TR_SLAY_DRAGON, 40},
	{TR_ESP_HUMAN, "人間ESP", 6, TR_SLAY_HUMAN, 40},

	{ESSENCE_ATTACK, "攻撃", 10, TR_ES_ATTACK, 30},
	{ESSENCE_AC, "防御", 10, TR_ES_AC, 15},
	{ESSENCE_TMP_RES_ACID, "酸耐性発動", 7, TR_RES_ACID, 50},
	{ESSENCE_TMP_RES_ELEC, "電撃耐性発動", 7, TR_RES_ELEC, 50},
	{ESSENCE_TMP_RES_FIRE, "火炎耐性発動", 7, TR_RES_FIRE, 50},
	{ESSENCE_TMP_RES_COLD, "冷気耐性発動", 7, TR_RES_COLD, 50},
	{ESSENCE_SH_FIRE, "火炎オーラ", 7, -1, 50},
	{ESSENCE_SH_ELEC, "電撃オーラ", 7, -1, 50},
	{ESSENCE_SH_COLD, "冷気オーラ", 7, -1, 50},
	{ESSENCE_RESISTANCE, "全耐性", 2, -1, 150},
	{ESSENCE_SUSTAIN, "装備保持", 10, -1, 10},
	{ESSENCE_SLAY_GLOVE, "殺戮の小手", 1, TR_ES_ATTACK, 200},

	{-1, NULL, 0, -1, 0}
};
#else
static essence_type essence_info[] = 
{
	{TR_STR, "strength", 4, TR_STR, 20},
	{TR_INT, "intelligence", 4, TR_INT, 20},
	{TR_WIS, "wisdom", 4, TR_WIS, 20},
	{TR_DEX, "dexterity", 4, TR_DEX, 20},
	{TR_CON, "constitution", 4, TR_CON, 20},
	{TR_CHR, "charisma", 4, TR_CHR, 20},
	{TR_MAGIC_MASTERY, "magic mastery", 4, TR_MAGIC_MASTERY, 20},
	{TR_STEALTH, "stealth", 4, TR_STEALTH, 40},
	{TR_SEARCH, "serching", 4, TR_SEARCH, 15},
	{TR_INFRA, "infravision", 4, TR_INFRA, 15},
	{TR_TUNNEL, "digging", 4, TR_TUNNEL, 15},
	{TR_SPEED, "speed", 4, TR_SPEED, 12},
	{TR_BLOWS, "extra attack", 1, TR_BLOWS, 20},
	{TR_CHAOTIC, "chaos brand", 1, TR_CHAOTIC, 15},
	{TR_VAMPIRIC, "vampiric brand", 1, TR_VAMPIRIC, 60},
	{TR_IMPACT, "quake activation", 7, TR_IMPACT, 15},
	{TR_BRAND_POIS, "poison brand", 1, TR_BRAND_POIS, 20},
	{TR_BRAND_ACID, "acid brand", 1, TR_BRAND_ACID, 20},
	{TR_BRAND_ELEC, "electric brand", 1, TR_BRAND_ELEC, 20},
	{TR_BRAND_FIRE, "fire brand", 1, TR_BRAND_FIRE, 20},
	{TR_BRAND_COLD, "cold brand", 1, TR_BRAND_COLD, 20},
	{TR_SUST_STR, "sustain strength", 3, TR_SUST_STR, 15},
	{TR_SUST_INT, "sustain intelligence", 3, TR_SUST_STR, 15},
	{TR_SUST_WIS, "sustain wisdom", 3, TR_SUST_STR, 15},
	{TR_SUST_DEX, "sustain dexterity", 3, TR_SUST_STR, 15},
	{TR_SUST_CON, "sustain constitution", 3, TR_SUST_STR, 15},
	{TR_SUST_CHR, "sustain charisma", 3, TR_SUST_STR, 15},
	{TR_IM_ACID, "acid immunity", 2, TR_IM_ACID, 20},
	{TR_IM_ELEC, "electric immunity", 2, TR_IM_ACID, 20},
	{TR_IM_FIRE, "fire immunity", 2, TR_IM_ACID, 20},
	{TR_IM_COLD, "cold immunity", 2, TR_IM_ACID, 20},
	{TR_REFLECT, "reflection", 2, TR_REFLECT, 20},
	{TR_FREE_ACT, "free action", 3, TR_FREE_ACT, 20},
	{TR_HOLD_LIFE, "hold life", 3, TR_HOLD_LIFE, 20},
	{TR_RES_ACID, "resistance to acid", 2, TR_RES_ACID, 15},
	{TR_RES_ELEC, "resistance to electric", 2, TR_RES_ELEC, 15},
	{TR_RES_FIRE, "resistance to fire", 2, TR_RES_FIRE, 15},
	{TR_RES_COLD, "resistance to cold", 2, TR_RES_COLD, 15},
	{TR_RES_POIS, "resistance to poison", 2, TR_RES_POIS, 25},
	{TR_RES_FEAR, "resistance to fear", 2, TR_RES_FEAR, 20},
	{TR_RES_LITE, "resistance to light", 2, TR_RES_LITE, 20},
	{TR_RES_DARK, "resistance to dark", 2, TR_RES_DARK, 20},
	{TR_RES_BLIND, "resistance to blind", 2, TR_RES_BLIND, 20},
	{TR_RES_CONF, "resistance to confusion", 2, TR_RES_CONF, 20},
	{TR_RES_SOUND, "resistance to sound", 2, TR_RES_SOUND, 20},
	{TR_RES_SHARDS, "resistance to shard", 2, TR_RES_SHARDS, 20},
	{TR_RES_NETHER, "resistance to nether", 2, TR_RES_NETHER, 20},
	{TR_RES_NEXUS, "resistance to nexus", 2, TR_RES_NEXUS, 20},
	{TR_RES_CHAOS, "resistance to chaos", 2, TR_RES_CHAOS, 20},
	{TR_RES_DISEN, "resistance to disenchantment", 2, TR_RES_DISEN, 20},
	{TR_SH_FIRE, "", 0, -2, 0},
	{TR_SH_ELEC, "", 0, -2, 0},
	{TR_SH_COLD, "", 0, -2, 0},
	{TR_NO_MAGIC, "anti magic", 3, TR_NO_MAGIC, 15},
	{TR_WARNING, "warning", 3, TR_WARNING, 20},
	{TR_LEVITATION, "levitation", 3, TR_LEVITATION, 20},
	{TR_LITE, "permanent light", 3, TR_LITE, 15},
	{TR_SEE_INVIS, "see invisible", 3, TR_SEE_INVIS, 20},
	{TR_TELEPATHY, "telepathy", 6, TR_TELEPATHY, 15},
	{TR_SLOW_DIGEST, "slow digestion", 3, TR_SLOW_DIGEST, 15},
	{TR_REGEN, "regeneration", 3, TR_REGEN, 20},
	{TR_TELEPORT, "teleport", 3, TR_TELEPORT, 25},

	{TR_SLAY_EVIL, "slay evil", 5, TR_SLAY_EVIL, 100},
	{TR_SLAY_ANIMAL, "slay animal", 5, TR_SLAY_ANIMAL, 20},
	{TR_KILL_ANIMAL, "kill animal", 5, TR_SLAY_ANIMAL, 60},
	{TR_KILL_EVIL, "kill evil", 0, TR_SLAY_EVIL, 60},
	{TR_SLAY_UNDEAD, "slay undead", 5, TR_SLAY_UNDEAD, 20},
	{TR_KILL_UNDEAD, "kill undead", 5, TR_SLAY_UNDEAD, 60},
	{TR_SLAY_DEMON, "slay demon", 5, TR_SLAY_DEMON, 20},
	{TR_KILL_DEMON, "kill demon", 5, TR_SLAY_DEMON, 60},
	{TR_SLAY_ORC, "slay orc", 5, TR_SLAY_ORC, 15},
	{TR_KILL_ORC, "kill orc", 5, TR_SLAY_ORC, 60},
	{TR_SLAY_TROLL, "slay troll", 5, TR_SLAY_TROLL, 15},
	{TR_KILL_TROLL, "kill troll", 5, TR_SLAY_TROLL, 60},
	{TR_SLAY_GIANT, "slay giant", 5, TR_SLAY_GIANT, 20},
	{TR_KILL_GIANT, "kill giant", 5, TR_SLAY_GIANT, 60},       
	{TR_SLAY_DRAGON, "slay dragon", 5, TR_SLAY_DRAGON, 20},
	{TR_KILL_DRAGON, "kill dragon", 5, TR_SLAY_DRAGON, 60},
	{TR_SLAY_HUMAN, "slay human", 5, TR_SLAY_HUMAN, 20},
	{TR_KILL_HUMAN, "kill human", 5, TR_SLAY_HUMAN, 60},

	{TR_ESP_ANIMAL, "sense animal", 6, TR_SLAY_ANIMAL, 40},
	{TR_ESP_UNDEAD, "sense undead", 6, TR_SLAY_UNDEAD, 40}, 
	{TR_ESP_DEMON, "sense demon", 6, TR_SLAY_DEMON, 40},       
	{TR_ESP_ORC, "sense orc", 6, TR_SLAY_ORC, 40},     
	{TR_ESP_TROLL, "sense troll", 6, TR_SLAY_TROLL, 40},   
	{TR_ESP_GIANT, "sense giant", 6, TR_SLAY_GIANT, 40},       
	{TR_ESP_DRAGON, "sense dragon", 6, TR_SLAY_DRAGON, 40},
	{TR_ESP_HUMAN, "sense human", 6, TR_SLAY_HUMAN, 40},

	{ESSENCE_ATTACK, "weapon enchant", 10, TR_ES_ATTACK, 30},
	{ESSENCE_AC, "armor enchant", 10, TR_ES_AC, 15},
	{ESSENCE_TMP_RES_ACID, "resist acid activation", 7, TR_RES_ACID, 50},
	{ESSENCE_TMP_RES_ELEC, "resist electricity activation", 7, TR_RES_ELEC, 50},
	{ESSENCE_TMP_RES_FIRE, "resist fire activation", 7, TR_RES_FIRE, 50},
	{ESSENCE_TMP_RES_COLD, "resist cold activation", 7, TR_RES_COLD, 50},
	{ESSENCE_SH_FIRE, "fiery sheath", 7, -1, 50},
	{ESSENCE_SH_ELEC, "electric sheath", 7, -1, 50},
	{ESSENCE_SH_COLD, "sheath of coldness", 7, -1, 50},
	{ESSENCE_RESISTANCE, "resistance", 2, -1, 150},
	{ESSENCE_SUSTAIN, "elements proof", 10, -1, 10},
	{ESSENCE_SLAY_GLOVE, "gauntlets of slaying", 1, TR_ES_ATTACK, 200},

	{-1, NULL, 0, -1, 0}
};
#endif


/*
 *  Essense names for Weapon smith
 */
#ifdef JP
static cptr essence_name[] = 
{
	"腕力",
	"知能",
	"賢さ",
	"器用さ",
	"耐久力",
	"魅力",
	"魔力支配",
	"",
	"隠密",
	"探索",
	"赤外線視力",
	"採掘",
	"スピード",
	"追加攻撃",
	"カオス攻撃",
	"吸血攻撃",
	"動物倍打",
	"邪悪倍打",
	"不死倍打",
	"悪魔倍打",
	"オーク倍打",
	"トロル倍打",
	"巨人倍打",
	"竜倍打",
	"",
	"",
	"地震",
	"毒殺",
	"溶解",
	"電撃",
	"焼棄",
	"凍結",
	"能力維持",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"免疫",
	"",
	"",
	"",
	"",
	"反射",
	"麻痺知らず",
	"生命力維持",
	"耐酸",
	"耐電撃",
	"耐火炎",
	"耐冷気",
	"耐毒",
	"耐恐怖",
	"耐閃光",
	"耐暗黒",
	"耐盲目",
	"耐混乱",
	"耐轟音",
	"耐破片",
	"耐地獄",
	"耐因果混乱",
	"耐カオス",
	"耐劣化",
	"",
	"",
	"人間倍打",
	"",
	"",
	"反魔法",
	"",
	"",
	"警告",
	"",
	"",
	"",
	"浮遊",
	"永久光源",
	"可視透明",
	"テレパシー",
	"遅消化",
	"急速回復",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"テレポート",
	"",
	"",
	"攻撃",
	"防御",

	NULL
};

#else

static cptr essence_name[] = 
{
	"strength",
	"intelligen.",
	"wisdom",
	"dexterity",
	"constitut.",
	"charisma",
	"magic mast.",
	"",
	"stealth",
	"serching",
	"infravision",
	"digging",
	"speed",
	"extra atk",
	"chaos brand",
	"vampiric",
	"slay animal",
	"slay evil",
	"slay undead",
	"slay demon",
	"slay orc",
	"slay troll",
	"slay giant",
	"slay dragon",
	"",
	"",
	"quake",
	"pois. brand",
	"acid brand",
	"elec. brand",
	"fire brand",
	"cold brand",
	"sustain",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"immunity",
	"",
	"",
	"",
	"",
	"reflection",
	"free action",
	"hold life",
	"res. acid",
	"res. elec.",
	"res. fire",
	"res. cold",
	"res. poison",
	"res. fear",
	"res. light",
	"res. dark",
	"res. blind",
	"res.confuse",
	"res. sound",
	"res. shard",
	"res. nether",
	"res. nexus",
	"res. chaos",
	"res. disen.",
	"",
	"",
	"slay human",
	"",
	"",
	"anti magic",
	"",
	"",
	"warning",
	"",
	"",
	"",
	"levitation",
	"perm. light",
	"see invis.",
	"telepathy",
	"slow dige.",
	"regen.",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"teleport",
	"",
	"",
	"weapon enc.",
	"armor enc.",

	NULL
};
#endif


static void display_essence(void)
{
	int i, num = 0;

	screen_save();
	for (i = 1; i < 22; i++)
	{
		prt("",i,0);
	}
#ifdef JP
	prt("エッセンス   個数     エッセンス   個数     エッセンス   個数", 1, 8);
#else
	prt("Essence      Num      Essence      Num      Essence      Num ", 1, 8);
#endif
	for (i = 0; essence_name[i]; i++)
	{
		if (!essence_name[i][0]) continue;
		prt(format("%-11s %5d", essence_name[i], p_ptr->magic_num1[i]), 2+num%21, 8+num/21*22);
		num++;
	}
#ifdef JP
	prt("現在所持しているエッセンス", 0, 0);
#else
	prt("List of all essences you have.", 0, 0);
#endif
	(void)inkey();
	screen_load();
	return;
}

static void drain_essence(void)
{
	int drain_value[sizeof(p_ptr->magic_num1) / sizeof(s32b)];
	int i, item;
	int dec = 4;
	bool observe = FALSE;
	int old_ds, old_dd, old_to_h, old_to_d, old_ac, old_to_a, old_pval, old_name2, old_timeout;
	u32b old_flgs[TR_FLAG_SIZE], new_flgs[TR_FLAG_SIZE];
	object_type *o_ptr;
	cptr            q, s;
	byte iy, ix, marked, number;
	s16b next_o_idx, weight;

	for (i = 0; i < sizeof(drain_value) / sizeof(int); i++)
		drain_value[i] = 0;

	item_tester_hook = object_is_weapon_armour_ammo;
	item_tester_no_ryoute = TRUE;

	/* Get an item */
#ifdef JP
	q = "どのアイテムから抽出しますか？";
	s = "抽出できるアイテムがありません。";
#else
	q = "Extract from which item? ";
	s = "You have nothing you can extract from.";
#endif

	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	if (object_is_known(o_ptr) && !object_is_nameless(o_ptr))
	{
		char o_name[MAX_NLEN];
		object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
		if (!get_check(format("本当に%sから抽出してよろしいですか？", o_name))) return;
#else
		if (!get_check(format("Really extract from %s? ", o_name))) return;
#endif
	}

	energy_use = 100;

	object_flags(o_ptr, old_flgs);
	if (have_flag(old_flgs, TR_KILL_DRAGON)) add_flag(old_flgs, TR_SLAY_DRAGON);
	if (have_flag(old_flgs, TR_KILL_ANIMAL)) add_flag(old_flgs, TR_SLAY_ANIMAL);
	if (have_flag(old_flgs, TR_KILL_EVIL)) add_flag(old_flgs, TR_SLAY_EVIL);
	if (have_flag(old_flgs, TR_KILL_UNDEAD)) add_flag(old_flgs, TR_SLAY_UNDEAD);
	if (have_flag(old_flgs, TR_KILL_DEMON)) add_flag(old_flgs, TR_SLAY_DEMON);
	if (have_flag(old_flgs, TR_KILL_ORC)) add_flag(old_flgs, TR_SLAY_ORC);
	if (have_flag(old_flgs, TR_KILL_TROLL)) add_flag(old_flgs, TR_SLAY_TROLL);
	if (have_flag(old_flgs, TR_KILL_GIANT)) add_flag(old_flgs, TR_SLAY_GIANT);
	if (have_flag(old_flgs, TR_KILL_HUMAN)) add_flag(old_flgs, TR_SLAY_HUMAN);

	old_to_a = o_ptr->to_a;
	old_ac = o_ptr->ac;
	old_to_h = o_ptr->to_h;
	old_to_d = o_ptr->to_d;
	old_ds = o_ptr->ds;
	old_dd = o_ptr->dd;
	old_pval = o_ptr->pval;
	old_name2 = o_ptr->name2;
	old_timeout = o_ptr->timeout;
	if (o_ptr->curse_flags & (TRC_CURSED | TRC_HEAVY_CURSE | TRC_PERMA_CURSE)) dec--;
	if (have_flag(old_flgs, TR_AGGRAVATE)) dec--;
	if (have_flag(old_flgs, TR_NO_TELE)) dec--;
	if (have_flag(old_flgs, TR_DRAIN_EXP)) dec--;
	if (have_flag(old_flgs, TR_TY_CURSE)) dec--;

	iy = o_ptr->iy;
	ix = o_ptr->ix;
	next_o_idx = o_ptr->next_o_idx;
	marked = o_ptr->marked;
	weight = o_ptr->weight;
	number = o_ptr->number;

	object_prep(o_ptr, o_ptr->k_idx);

	o_ptr->iy=iy;
	o_ptr->ix=ix;
	o_ptr->next_o_idx=next_o_idx;
	o_ptr->marked=marked;
	o_ptr->number = number;
	if (o_ptr->tval == TV_DRAG_ARMOR) o_ptr->timeout = old_timeout;
	if (item >= 0) p_ptr->total_weight += (o_ptr->weight*o_ptr->number - weight*number);
	o_ptr->ident |= (IDENT_MENTAL);
	object_aware(o_ptr);
	object_known(o_ptr);

	object_flags(o_ptr, new_flgs);

	for (i = 0; essence_info[i].add_name; i++)
	{
		essence_type *es_ptr = &essence_info[i];
		int pval = 0;

		if (es_ptr->add < TR_FLAG_MAX && is_pval_flag(es_ptr->add) && old_pval)
			pval = (have_flag(new_flgs, es_ptr->add)) ? old_pval - o_ptr->pval : old_pval;

		if (es_ptr->add < TR_FLAG_MAX &&
		    (!have_flag(new_flgs, es_ptr->add) || pval) &&
		    have_flag(old_flgs, es_ptr->add))
		{
			if (pval)
			{
				drain_value[es_ptr->essence] += 10 * pval;
			}
			else if (es_ptr->essence != -2)
			{
				drain_value[es_ptr->essence] += 10;
			}
			else if (es_ptr->add == TR_SH_FIRE)
			{
				drain_value[TR_BRAND_FIRE] += 10;
				drain_value[TR_RES_FIRE] += 10;
			}
			else if (es_ptr->add == TR_SH_ELEC)
			{
				drain_value[TR_BRAND_ELEC] += 10;
				drain_value[TR_RES_ELEC] += 10;
			}
			else if (es_ptr->add == TR_SH_COLD)
			{
				drain_value[TR_BRAND_COLD] += 10;
				drain_value[TR_RES_COLD] += 10;
			}
		}
	}

	if ((have_flag(old_flgs, TR_FORCE_WEAPON)) && !(have_flag(new_flgs, TR_FORCE_WEAPON)))
	{
		drain_value[TR_INT] += 5;
		drain_value[TR_WIS] += 5;
	}
	if ((have_flag(old_flgs, TR_VORPAL)) && !(have_flag(new_flgs, TR_VORPAL)))
	{
		drain_value[TR_BRAND_POIS] += 5;
		drain_value[TR_BRAND_ACID] += 5;
		drain_value[TR_BRAND_ELEC] += 5;
		drain_value[TR_BRAND_FIRE] += 5;
		drain_value[TR_BRAND_COLD] += 5;
	}
	if ((have_flag(old_flgs, TR_DEC_MANA)) && !(have_flag(new_flgs, TR_DEC_MANA)))
	{
		drain_value[TR_INT] += 10;
	}
	if ((have_flag(old_flgs, TR_XTRA_MIGHT)) && !(have_flag(new_flgs, TR_XTRA_MIGHT)))
	{
		drain_value[TR_STR] += 10;
	}
	if ((have_flag(old_flgs, TR_XTRA_SHOTS)) && !(have_flag(new_flgs, TR_XTRA_SHOTS)))
	{
		drain_value[TR_DEX] += 10;
	}
	if (old_name2 == EGO_2WEAPON)
	{
		drain_value[TR_DEX] += 20;
	}
	if (object_is_weapon_ammo(o_ptr))
	{
		if (old_ds > o_ptr->ds) drain_value[TR_ES_ATTACK] += (old_ds-o_ptr->ds)*10;

		if (old_dd > o_ptr->dd) drain_value[TR_ES_ATTACK] += (old_dd-o_ptr->dd)*10;
	}
	if (old_to_h > o_ptr->to_h) drain_value[TR_ES_ATTACK] += (old_to_h-o_ptr->to_h)*10;
	if (old_to_d > o_ptr->to_d) drain_value[TR_ES_ATTACK] += (old_to_d-o_ptr->to_d)*10;
	if (old_ac > o_ptr->ac) drain_value[TR_ES_AC] += (old_ac-o_ptr->ac)*10;
	if (old_to_a > o_ptr->to_a) drain_value[TR_ES_AC] += (old_to_a-o_ptr->to_a)*10;

	for (i = 0; i < sizeof(drain_value) / sizeof(int); i++)
	{
		drain_value[i] *= number;
		drain_value[i] = drain_value[i] * dec / 4;
		drain_value[i] = MAX(drain_value[i], 0);
		if ((o_ptr->tval >= TV_SHOT) && (o_ptr->tval <= TV_BOLT)) drain_value[i] /= 10;
		if (drain_value[i])
		{
			observe = TRUE;
		}
	}
	if (!observe)
	{
#ifdef JP
		msg_print("エッセンスは抽出できませんでした。");
#else
		msg_print("You were not able to extract any essence.");
#endif
	}
	else
	{
#ifdef JP
		msg_print("抽出したエッセンス:");
#else
		msg_print("Extracted essences:");
#endif
		for (i = 0; essence_name[i]; i++)
		{
			if (!essence_name[i][0]) continue;
			if (!drain_value[i]) continue;

			p_ptr->magic_num1[i] += drain_value[i];
			p_ptr->magic_num1[i] = MIN(20000, p_ptr->magic_num1[i]);
			msg_print(NULL);
			msg_format("%s...%d", essence_name[i], drain_value[i]);
		}
	}

	/* Combine the pack */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN);
}



static int choose_essence(void)
{
	int mode = 0;
	char choice;
	int menu_line = (use_menu ? 1 : 0);

#ifdef JP
	cptr menu_name[] = {
		"武器属性", 
		"耐性",
		"能力",
		"数値",
		"スレイ",
		"ESP",
		"その他"
	};
#else
	cptr menu_name[] = {
		"Brand weapon",
		"Resistance",
		"Ability",
		"Magic number", 
		"Slay",
		"ESP",
		"Others"
	};
#endif
	const int mode_max = 7;

#ifdef ALLOW_REPEAT
	if (repeat_pull(&mode) && 1 <= mode && mode <= mode_max)
		return mode;
	mode = 0;
#endif /* ALLOW_REPEAT */

	if (use_menu)
	{
		screen_save();

		while(!mode)
		{
			int i;
			for (i = 0; i < mode_max; i++)
#ifdef JP
				prt(format(" %s %s", (menu_line == 1+i) ? "》" : "  ", menu_name[i]), 2 + i, 14);
			prt("どの種類のエッセンス付加を行いますか？", 0, 0);
#else
				prt(format(" %s %s", (menu_line == 1+i) ? "> " : "  ", menu_name[i]), 2 + i, 14);
			prt("Choose from menu.", 0, 0);
#endif

			choice = inkey();
			switch(choice)
			{
			case ESCAPE:
			case 'z':
			case 'Z':
				screen_load();
				return 0;
			case '2':
			case 'j':
			case 'J':
				menu_line++;
				break;
			case '8':
			case 'k':
			case 'K':
				menu_line += mode_max - 1;
				break;
			case '\r':
			case '\n':
			case 'x':
			case 'X':
				mode = menu_line;
				break;
			}
			if (menu_line > mode_max) menu_line -= mode_max;
		}
		screen_load();
	}
	else
	{
		screen_save();
		while (!mode)
		{
			int i;

			for (i = 0; i < mode_max; i++)
				prt(format("  %c) %s", 'a' + i, menu_name[i]), 2 + i, 14);

#ifdef JP
			if (!get_com("何を付加しますか:", &choice, TRUE))
#else
			if (!get_com("Command :", &choice, TRUE))
#endif
			{
				screen_load();
				return 0;
			}

			if (isupper(choice)) choice = tolower(choice);

			if ('a' <= choice && choice <= 'a' + (char)mode_max - 1)
				mode = (int)choice - 'a' + 1;
		}
		screen_load();
	}

#ifdef ALLOW_REPEAT
	repeat_push(mode);
#endif /* ALLOW_REPEAT */
	return mode;
}

static void add_essence(int mode)
{
	int item, max_num = 0;
	int i;
	bool flag,redraw;
	char choice;
	cptr            q, s;
	object_type *o_ptr;
	int ask = TRUE;
	char out_val[160];
	int num[22];
	char o_name[MAX_NLEN];
	int use_essence;
	essence_type *es_ptr;

	int menu_line = (use_menu ? 1 : 0);

	for (i = 0; essence_info[i].add_name; i++)
	{
		es_ptr = &essence_info[i];

		if (es_ptr->type != mode) continue;
		num[max_num++] = i;
	}

#ifdef ALLOW_REPEAT
	if (!repeat_pull(&i) || i<0 || i>=max_num)
	{
#endif /* ALLOW_REPEAT */


	/* Nothing chosen yet */
	flag = FALSE;

	/* No redraw yet */
	redraw = FALSE;

	/* Build a prompt */
#ifdef JP
	(void) strnfmt(out_val, 78, "('*'で一覧, ESCで中断) どの能力を付加しますか？");
#else
	(void)strnfmt(out_val, 78, "(*=List, ESC=exit) Add which ability? ");
#endif
	if (use_menu) screen_save();

	/* Get a spell from the user */

	choice = (always_show_list || use_menu) ? ESCAPE:1;
	while (!flag)
	{
		bool able[22];
		if( choice==ESCAPE ) choice = ' '; 
		else if( !get_com(out_val, &choice, FALSE) )break; 

		if (use_menu && choice != ' ')
		{
			switch(choice)
			{
				case '0':
				{
					screen_load();
					return;
				}

				case '8':
				case 'k':
				case 'K':
				{
					menu_line += (max_num-1);
					break;
				}

				case '2':
				case 'j':
				case 'J':
				{
					menu_line++;
					break;
				}

				case '4':
				case 'h':
				case 'H':
				{
					menu_line = 1;
					break;
				}
				case '6':
				case 'l':
				case 'L':
				{
					menu_line = max_num;
					break;
				}

				case 'x':
				case 'X':
				case '\r':
				case '\n':
				{
					i = menu_line - 1;
					ask = FALSE;
					break;
				}
			}
			if (menu_line > max_num) menu_line -= max_num;
		}
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?') || (use_menu && ask))
		{
			/* Show the list */
			if (!redraw || use_menu)
			{
				byte y, x = 10;
				int ctr;
				char dummy[80], dummy2[80];
				byte col;

				strcpy(dummy, "");

				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				if (!use_menu) screen_save();

				for (y = 1; y < 24; y++)
					prt("", y, x);

				/* Print header(s) */
#ifdef JP
				prt(format("   %-43s %6s/%s", "能力(必要エッセンス)", "必要数", "所持数"), 1, x);

#else
				prt(format("   %-43s %6s/%s", "Ability (needed essence)", "Needs", "Possess"), 1, x);
#endif
				/* Print list */
				for (ctr = 0; ctr < max_num; ctr++)
				{
					es_ptr = &essence_info[num[ctr]];

					if (use_menu)
					{
						if (ctr == (menu_line-1))
#ifdef JP
							strcpy(dummy, "》 ");
#else
							strcpy(dummy, ">  ");
#endif
						else strcpy(dummy, "   ");
						
					}
					/* letter/number for power selection */
					else
					{
						sprintf(dummy, "%c) ",I2A(ctr));
					}

					strcat(dummy, es_ptr->add_name);

					col = TERM_WHITE;
					able[ctr] = TRUE;

					if (es_ptr->essence != -1)
					{
						strcat(dummy, format("(%s)", essence_name[es_ptr->essence]));
						if (p_ptr->magic_num1[es_ptr->essence] < es_ptr->value) able[ctr] = FALSE;
					}
					else
					{
						switch(es_ptr->add)
						{
						case ESSENCE_SH_FIRE:
#ifdef JP
							strcat(dummy, "(焼棄+耐火炎)");
#else
							strcat(dummy, "(brand fire + res.fire)");
#endif
							if (p_ptr->magic_num1[TR_BRAND_FIRE] < es_ptr->value) able[ctr] = FALSE;
							if (p_ptr->magic_num1[TR_RES_FIRE] < es_ptr->value) able[ctr] = FALSE;
							break;
						case ESSENCE_SH_ELEC:
#ifdef JP
							strcat(dummy, "(電撃+耐電撃)");
#else
							strcat(dummy, "(brand elec. + res. elec.)");
#endif
							if (p_ptr->magic_num1[TR_BRAND_ELEC] < es_ptr->value) able[ctr] = FALSE;
							if (p_ptr->magic_num1[TR_RES_ELEC] < es_ptr->value) able[ctr] = FALSE;
							break;
						case ESSENCE_SH_COLD:
#ifdef JP
							strcat(dummy, "(凍結+耐冷気)");
#else
							strcat(dummy, "(brand cold + res. cold)");
#endif
							if (p_ptr->magic_num1[TR_BRAND_COLD] < es_ptr->value) able[ctr] = FALSE;
							if (p_ptr->magic_num1[TR_RES_COLD] < es_ptr->value) able[ctr] = FALSE;
							break;
						case ESSENCE_RESISTANCE:
#ifdef JP
							strcat(dummy, "(耐火炎+耐冷気+耐電撃+耐酸)");
#else
							strcat(dummy, "(r.fire+r.cold+r.elec+r.acid)");
#endif
							if (p_ptr->magic_num1[TR_RES_FIRE] < es_ptr->value) able[ctr] = FALSE;
							if (p_ptr->magic_num1[TR_RES_COLD] < es_ptr->value) able[ctr] = FALSE;
							if (p_ptr->magic_num1[TR_RES_ELEC] < es_ptr->value) able[ctr] = FALSE;
							if (p_ptr->magic_num1[TR_RES_ACID] < es_ptr->value) able[ctr] = FALSE;
							break;
						case ESSENCE_SUSTAIN:
#ifdef JP
							strcat(dummy, "(耐火炎+耐冷気+耐電撃+耐酸)");
#else
							strcat(dummy, "(r.fire+r.cold+r.elec+r.acid)");
#endif
							if (p_ptr->magic_num1[TR_RES_FIRE] < es_ptr->value) able[ctr] = FALSE;
							if (p_ptr->magic_num1[TR_RES_COLD] < es_ptr->value) able[ctr] = FALSE;
							if (p_ptr->magic_num1[TR_RES_ELEC] < es_ptr->value) able[ctr] = FALSE;
							if (p_ptr->magic_num1[TR_RES_ACID] < es_ptr->value) able[ctr] = FALSE;
							break;
						}
					}

					if (!able[ctr]) col = TERM_RED;

					if (es_ptr->essence != -1)
					{
						sprintf(dummy2, "%-49s %3d/%d", dummy, es_ptr->value, (int)p_ptr->magic_num1[es_ptr->essence]);
					}
					else
					{
						sprintf(dummy2, "%-49s %3d/(\?\?)", dummy, es_ptr->value);
					}

					c_prt(col, dummy2, ctr+2, x);
				}
			}

			/* Hide the list */
			else
			{
				/* Hide list */
				redraw = FALSE;

				/* Restore the screen */
				screen_load();
			}

			/* Redo asking */
			continue;
		}

		if (!use_menu)
		{
			/* Note verify */
			ask = (isupper(choice));

			/* Lowercase */
			if (ask) choice = tolower(choice);

			/* Extract request */
			i = (islower(choice) ? A2I(choice) : -1);
		}

		/* Totally Illegal */
		if ((i < 0) || (i >= max_num) || !able[i])
		{
			bell();
			continue;
		}

		/* Verify it */
		if (ask)
		{
			char tmp_val[160];

			/* Prompt */
#ifdef JP
			(void) strnfmt(tmp_val, 78, "%sを付加しますか？ ", essence_info[num[i]].add_name);
#else
			(void) strnfmt(tmp_val, 78, "Add the abilitiy of %s? ", essence_info[num[i]].add_name);
#endif

			/* Belay that order */
			if (!get_check(tmp_val)) continue;
		}

		/* Stop the loop */
		flag = TRUE;
	}

	/* Restore the screen */
	if (redraw) screen_load();

	if (!flag) return;

#ifdef ALLOW_REPEAT
	repeat_push(i);
	}
#endif /* ALLOW_REPEAT */

	es_ptr = &essence_info[num[i]];

	if (es_ptr->add == ESSENCE_SLAY_GLOVE)
		item_tester_tval = TV_GLOVES;
	else if (mode == 1 || mode == 5)
		item_tester_hook = item_tester_hook_melee_ammo;
	else if (es_ptr->add == ESSENCE_ATTACK)
		item_tester_hook = object_allow_enchant_weapon;
	else if (es_ptr->add == ESSENCE_AC)
		item_tester_hook = object_is_armour;
	else
		item_tester_hook = object_is_weapon_armour_ammo;
	item_tester_no_ryoute = TRUE;

	/* Get an item */
#ifdef JP
	q = "どのアイテムを改良しますか？";
	s = "改良できるアイテムがありません。";
#else
	q = "Improve which item? ";
	s = "You have nothing to improve.";
#endif

	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	if ((mode != 10) && (object_is_artifact(o_ptr) || object_is_smith(o_ptr)))
	{
#ifdef JP
		msg_print("そのアイテムはこれ以上改良できない。");
#else
		msg_print("This item is no more able to be improved.");
#endif
		return;
	}

	object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

	use_essence = es_ptr->value;
	if ((o_ptr->tval >= TV_SHOT) && (o_ptr->tval <= TV_BOLT)) use_essence = (use_essence+9)/10;
	if (o_ptr->number > 1)
	{
		use_essence *= o_ptr->number;
#ifdef JP
		msg_format("%d個あるのでエッセンスは%d必要です。", o_ptr->number, use_essence);
#else
		msg_format("It will take %d essences.",use_essence);
#endif

	}

	if (es_ptr->essence != -1)
	{
		if (p_ptr->magic_num1[es_ptr->essence] < use_essence)
		{
#ifdef JP
			msg_print("エッセンスが足りない。");
#else
			msg_print("You don't have enough essences.");
#endif
			return;
		}
		if (is_pval_flag(es_ptr->add))
		{
			if (o_ptr->pval < 0)
			{
#ifdef JP
				msg_print("このアイテムの能力修正を強化することはできない。");
#else
				msg_print("You cannot increase magic number of this item.");
#endif
				return;
			}
			else if (es_ptr->add == TR_BLOWS)
			{
				if (o_ptr->pval > 1)
				{
#ifdef JP
					if (!get_check("修正値は1になります。よろしいですか？")) return;
#else
					if (!get_check("The magic number of this weapon will become 1. Are you sure? ")) return;
#endif
				}

				o_ptr->pval = 1;
#ifdef JP
				msg_format("エッセンスを%d個使用します。", use_essence);
#else
				msg_format("It will take %d essences.", use_essence);
#endif
			}
			else if (o_ptr->pval > 0)
			{
				use_essence *= o_ptr->pval;
#ifdef JP
				msg_format("エッセンスを%d個使用します。", use_essence);
#else
				msg_format("It will take %d essences.", use_essence);
#endif
			}
			else
			{
				char tmp[80];
				char tmp_val[160];
				int pval;
				int limit = MIN(5, p_ptr->magic_num1[es_ptr->essence]/es_ptr->value);

#ifdef JP
				sprintf(tmp, "いくつ付加しますか？ (1-%d): ", limit);
#else
				sprintf(tmp, "Enchant how many? (1-%d): ", limit);
#endif
				strcpy(tmp_val, "1");

				if (!get_string(tmp, tmp_val, 1)) return;
				pval = atoi(tmp_val);
				if (pval > limit) pval = limit;
				else if (pval < 1) pval = 1;
				o_ptr->pval += pval;
				use_essence *= pval;
#ifdef JP
				msg_format("エッセンスを%d個使用します。", use_essence);
#else
				msg_format("It will take %d essences.", use_essence);
#endif
			}

			if (p_ptr->magic_num1[es_ptr->essence] < use_essence)
			{
#ifdef JP
				msg_print("エッセンスが足りない。");
#else
				msg_print("You don't have enough essences.");
#endif
				return;
			}
		}
		else if (es_ptr->add == ESSENCE_SLAY_GLOVE)
		{
			char tmp_val[160];
			int val;
			int get_to_h, get_to_d;

			strcpy(tmp_val, "1");
#ifdef JP
			if (!get_string(format("いくつ付加しますか？ (1-%d):", p_ptr->lev/7+3), tmp_val, 2)) return;
#else
			if (!get_string(format("Enchant how many? (1-%d):", p_ptr->lev/7+3), tmp_val, 2)) return;
#endif
			val = atoi(tmp_val);
			if (val > p_ptr->lev/7+3) val = p_ptr->lev/7+3;
			else if (val < 1) val = 1;
			use_essence *= val;
#ifdef JP
			msg_format("エッセンスを%d個使用します。", use_essence);
#else
			msg_format("It will take %d essences.", use_essence);
#endif
			if (p_ptr->magic_num1[es_ptr->essence] < use_essence)
			{
#ifdef JP
				msg_print("エッセンスが足りない。");
#else
				msg_print("You don't have enough essences.");
#endif
				return;
			}
			get_to_h = ((val+1)/2+randint0(val/2+1));
			get_to_d = ((val+1)/2+randint0(val/2+1));
			o_ptr->xtra4 = (get_to_h<<8)+get_to_d;
			o_ptr->to_h += get_to_h;
			o_ptr->to_d += get_to_d;
		}
		p_ptr->magic_num1[es_ptr->essence] -= use_essence;
		if (es_ptr->add == ESSENCE_ATTACK)
		{
			if ((o_ptr->to_h >= p_ptr->lev/5+5) && (o_ptr->to_d >= p_ptr->lev/5+5))
			{
#ifdef JP
				msg_print("改良に失敗した。");
#else
				msg_print("You failed to enchant.");
#endif
				energy_use = 100;
				return;
			}
			else
			{
				if (o_ptr->to_h < p_ptr->lev/5+5) o_ptr->to_h++;
				if (o_ptr->to_d < p_ptr->lev/5+5) o_ptr->to_d++;
			}
		}
		else if (es_ptr->add == ESSENCE_AC)
		{
			if (o_ptr->to_a >= p_ptr->lev/5+5)
			{
#ifdef JP
				msg_print("改良に失敗した。");
#else
				msg_print("You failed to enchant.");
#endif
				energy_use = 100;
				return;
			}
			else
			{
				if (o_ptr->to_a < p_ptr->lev/5+5) o_ptr->to_a++;
			}
		}
		else
		{
			o_ptr->xtra3 = es_ptr->add + 1;
		}
	}
	else
	{
		bool success = TRUE;

		switch(es_ptr->add)
		{
		case ESSENCE_SH_FIRE:
			if ((p_ptr->magic_num1[TR_BRAND_FIRE] < use_essence) || (p_ptr->magic_num1[TR_RES_FIRE] < use_essence))
			{
				success = FALSE;
				break;
			}
			p_ptr->magic_num1[TR_BRAND_FIRE] -= use_essence;
			p_ptr->magic_num1[TR_RES_FIRE] -= use_essence;
			break;
		case ESSENCE_SH_ELEC:
			if ((p_ptr->magic_num1[TR_BRAND_ELEC] < use_essence) || (p_ptr->magic_num1[TR_RES_ELEC] < use_essence))
			{
				success = FALSE;
				break;
			}
			p_ptr->magic_num1[TR_BRAND_ELEC] -= use_essence;
			p_ptr->magic_num1[TR_RES_ELEC] -= use_essence;
			break;
		case ESSENCE_SH_COLD:
			if ((p_ptr->magic_num1[TR_BRAND_COLD] < use_essence) || (p_ptr->magic_num1[TR_RES_COLD] < use_essence))
			{
				success = FALSE;
				break;
			}
			p_ptr->magic_num1[TR_BRAND_COLD] -= use_essence;
			p_ptr->magic_num1[TR_RES_COLD] -= use_essence;
			break;
		case ESSENCE_RESISTANCE:
		case ESSENCE_SUSTAIN:
			if ((p_ptr->magic_num1[TR_RES_ACID] < use_essence) || (p_ptr->magic_num1[TR_RES_ELEC] < use_essence) || (p_ptr->magic_num1[TR_RES_FIRE] < use_essence) || (p_ptr->magic_num1[TR_RES_COLD] < use_essence))
			{
				success = FALSE;
				break;
			}
			p_ptr->magic_num1[TR_RES_ACID] -= use_essence;
			p_ptr->magic_num1[TR_RES_ELEC] -= use_essence;
			p_ptr->magic_num1[TR_RES_FIRE] -= use_essence;
			p_ptr->magic_num1[TR_RES_COLD] -= use_essence;
			break;
		}
		if (!success)
		{
#ifdef JP
			msg_print("エッセンスが足りない。");
#else
			msg_print("You don't have enough essences.");
#endif
			return;
		}
		if (es_ptr->add == ESSENCE_SUSTAIN)
		{
			add_flag(o_ptr->art_flags, TR_IGNORE_ACID);
			add_flag(o_ptr->art_flags, TR_IGNORE_ELEC);
			add_flag(o_ptr->art_flags, TR_IGNORE_FIRE);
			add_flag(o_ptr->art_flags, TR_IGNORE_COLD);
		}
		else
		{
			o_ptr->xtra3 = es_ptr->add + 1;
		}
	}

	energy_use = 100;

#ifdef JP
	msg_format("%sに%sの能力を付加しました。", o_name, es_ptr->add_name);
#else
	msg_format("You have added ability of %s to %s.", es_ptr->add_name, o_name);
#endif

	/* Combine the pack */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN);
}


static void erase_essence(void)
{
	int item;
	cptr q, s;
	object_type *o_ptr;
	char o_name[MAX_NLEN];
	u32b flgs[TR_FLAG_SIZE];

	item_tester_hook = object_is_smith;

	/* Get an item */
#ifdef JP
	q = "どのアイテムのエッセンスを消去しますか？";
	s = "エッセンスを付加したアイテムがありません。";
#else
	q = "Remove from which item? ";
	s = "You have nothing to remove essence.";
#endif

	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
	if (!get_check(format("よろしいですか？ [%s]", o_name))) return;
#else
	if (!get_check(format("Are you sure? [%s]", o_name))) return;
#endif

	energy_use = 100;

	if (o_ptr->xtra3 == 1+ESSENCE_SLAY_GLOVE)
	{
		o_ptr->to_h -= (o_ptr->xtra4>>8);
		o_ptr->to_d -= (o_ptr->xtra4 & 0x000f);
		o_ptr->xtra4 = 0;
		if (o_ptr->to_h < 0) o_ptr->to_h = 0;
		if (o_ptr->to_d < 0) o_ptr->to_d = 0;
	}
	o_ptr->xtra3 = 0;
	object_flags(o_ptr, flgs);
	if (!(have_pval_flags(flgs))) o_ptr->pval = 0;
#ifdef JP
	msg_print("エッセンスを取り去った。");
#else
	msg_print("You removed all essence you have added.");
#endif

	/* Combine the pack */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN);
}

void do_cmd_kaji(bool only_browse)
{
	int mode = 0;
	char choice;

	int menu_line = (use_menu ? 1 : 0);

	if (!only_browse)
	{
		if (p_ptr->confused)
		{
#ifdef JP
			msg_print("混乱していて作業できない！");
#else
			msg_print("You are too confused!");
#endif

			return;
		}
		if (p_ptr->blind)
		{
#ifdef JP
			msg_print("目が見えなくて作業できない！");
#else
			msg_print("You are blind!");
#endif

			return;
		}
		if (p_ptr->image)
		{
#ifdef JP
			msg_print("うまく見えなくて作業できない！");
#else
			msg_print("You are hallucinating!");
#endif

			return;
		}
	}

#ifdef ALLOW_REPEAT
	if (!(repeat_pull(&mode) && 1 <= mode && mode <= 5))
	{
#endif /* ALLOW_REPEAT */

	if (only_browse) screen_save();
	do {
	if (!only_browse) screen_save();
	if (use_menu)
	{
		while(!mode)
		{
#ifdef JP
			prt(format(" %s エッセンス一覧", (menu_line == 1) ? "》" : "  "), 2, 14);
			prt(format(" %s エッセンス抽出", (menu_line == 2) ? "》" : "  "), 3, 14);
			prt(format(" %s エッセンス消去", (menu_line == 3) ? "》" : "  "), 4, 14);
			prt(format(" %s エッセンス付加", (menu_line == 4) ? "》" : "  "), 5, 14);
			prt(format(" %s 武器/防具強化", (menu_line == 5) ? "》" : "  "), 6, 14);
			prt(format("どの種類の技術を%sますか？", only_browse ? "調べ" : "使い"), 0, 0);
#else
			prt(format(" %s List essences", (menu_line == 1) ? "> " : "  "), 2, 14);
			prt(format(" %s Extract essence", (menu_line == 2) ? "> " : "  "), 3, 14);
			prt(format(" %s Remove essence", (menu_line == 3) ? "> " : "  "), 4, 14);
			prt(format(" %s Add essence", (menu_line == 4) ? "> " : "  "), 5, 14);
			prt(format(" %s Enchant weapon/armor", (menu_line == 5) ? "> " : "  "), 6, 14);
			prt(format("Choose command from menu."), 0, 0);
#endif
			choice = inkey();
			switch(choice)
			{
			case ESCAPE:
			case 'z':
			case 'Z':
				screen_load();
				return;
			case '2':
			case 'j':
			case 'J':
				menu_line++;
				break;
			case '8':
			case 'k':
			case 'K':
				menu_line+= 4;
				break;
			case '\r':
			case '\n':
			case 'x':
			case 'X':
				mode = menu_line;
				break;
			}
			if (menu_line > 5) menu_line -= 5;
		}
	}

	else
	{
		while (!mode)
		{
#ifdef JP
			prt("  a) エッセンス一覧", 2, 14);
			prt("  b) エッセンス抽出", 3, 14);
			prt("  c) エッセンス消去", 4, 14);
			prt("  d) エッセンス付加", 5, 14);
			prt("  e) 武器/防具強化", 6, 14);
			if (!get_com(format("どの能力を%sますか:", only_browse ? "調べ" : "使い"), &choice, TRUE))
#else
			prt("  a) List essences", 2, 14);
			prt("  b) Extract essence", 3, 14);
			prt("  c) Remove essence", 4, 14);
			prt("  d) Add essence", 5, 14);
			prt("  e) Enchant weapon/armor", 6, 14);
			if (!get_com("Command :", &choice, TRUE))
#endif
			{
				screen_load();
				return;
			}
			switch (choice)
			{
			case 'A':
			case 'a':
				mode = 1;
				break;
			case 'B':
			case 'b':
				mode = 2;
				break;
			case 'C':
			case 'c':
				mode = 3;
				break;
			case 'D':
			case 'd':
				mode = 4;
				break;
			case 'E':
			case 'e':
				mode = 5;
				break;
			}
		}
	}

	if (only_browse)
	{
		char temp[62*5];
		int line, j;

		/* Clear lines, position cursor  (really should use strlen here) */
		Term_erase(14, 21, 255);
		Term_erase(14, 20, 255);
		Term_erase(14, 19, 255);
		Term_erase(14, 18, 255);
		Term_erase(14, 17, 255);
		Term_erase(14, 16, 255);

		roff_to_buf(kaji_tips[mode-1], 62, temp, sizeof(temp));
		for(j=0, line = 17;temp[j];j+=(1+strlen(&temp[j])))
		{
			prt(&temp[j], line, 15);
			line++;
		}
		mode = 0;
	}
	if (!only_browse) screen_load();
	} while (only_browse);
#ifdef ALLOW_REPEAT
	repeat_push(mode);
	}
#endif /* ALLOW_REPEAT */

	switch(mode)
	{
		case 1: display_essence();break;
		case 2: drain_essence();break;
		case 3: erase_essence();break;
		case 4:
			mode = choose_essence();
			if (mode == 0)
				break;
			add_essence(mode);
			break;
		case 5: add_essence(10);break;
	}
}
