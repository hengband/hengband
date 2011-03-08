/* File: cmd6.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

/* Purpose: Object commands */

#include "angband.h"


/*
 * This file includes code for eating food, drinking potions,
 * reading scrolls, aiming wands, using staffs, zapping rods,
 * and activating artifacts.
 *
 * In all cases, if the player becomes "aware" of the item's use
 * by testing it, mark it as "aware" and reward some experience
 * based on the object's level, always rounding up.  If the player
 * remains "unaware", mark that object "kind" as "tried".
 *
 * This code now correctly handles the unstacking of wands, staffs,
 * and rods.  Note the overly paranoid warning about potential pack
 * overflow, which allows the player to use and drop a stacked item.
 *
 * In all "unstacking" scenarios, the "used" object is "carried" as if
 * the player had just picked it up.  In particular, this means that if
 * the use of an item induces pack overflow, that item will be dropped.
 *
 * For simplicity, these routines induce a full "pack reorganization"
 * which not only combines similar items, but also reorganizes various
 * items to obey the current "sorting" method.  This may require about
 * 400 item comparisons, but only occasionally.
 *
 * There may be a BIG problem with any "effect" that can cause "changes"
 * to the inventory.  For example, a "scroll of recharging" can cause
 * a wand/staff to "disappear", moving the inventory up.  Luckily, the
 * scrolls all appear BEFORE the staffs/wands, so this is not a problem.
 * But, for example, a "staff of recharging" could cause MAJOR problems.
 * In such a case, it will be best to either (1) "postpone" the effect
 * until the end of the function, or (2) "change" the effect, say, into
 * giving a staff "negative" charges, or "turning a staff into a stick".
 * It seems as though a "rod of recharging" might in fact cause problems.
 * The basic problem is that the act of recharging (and destroying) an
 * item causes the inducer of that action to "move", causing "o_ptr" to
 * no longer point at the correct item, with horrifying results.
 *
 * Note that food/potions/scrolls no longer use bit-flags for effects,
 * but instead use the "sval" (which is also used to sort the objects).
 */


static void do_cmd_eat_food_aux(int item)
{
	int ident, lev;
	object_type *o_ptr;

	if (music_singing_any()) stop_singing();
	if (hex_spelling_any()) stop_hex_spell_all();

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

	/* Sound */
	sound(SOUND_EAT);

	/* Take a turn */
	energy_use = 100;

	/* Identity not known yet */
	ident = FALSE;

	/* Object level */
	lev = k_info[o_ptr->k_idx].level;

	if (o_ptr->tval == TV_FOOD)
	{
		/* Analyze the food */
		switch (o_ptr->sval)
		{
			case SV_FOOD_POISON:
			{
				if (!(p_ptr->resist_pois || IS_OPPOSE_POIS()))
				{
					if (set_poisoned(p_ptr->poisoned + randint0(10) + 10))
					{
						ident = TRUE;
					}
				}
				break;
			}

			case SV_FOOD_BLINDNESS:
			{
				if (!p_ptr->resist_blind)
				{
					if (set_blind(p_ptr->blind + randint0(200) + 200))
					{
						ident = TRUE;
					}
				}
				break;
			}

			case SV_FOOD_PARANOIA:
			{
				if (!p_ptr->resist_fear)
				{
					if (set_afraid(p_ptr->afraid + randint0(10) + 10))
					{
						ident = TRUE;
					}
				}
				break;
			}

			case SV_FOOD_CONFUSION:
			{
				if (!p_ptr->resist_conf)
				{
					if (set_confused(p_ptr->confused + randint0(10) + 10))
					{
						ident = TRUE;
					}
				}
				break;
			}

			case SV_FOOD_HALLUCINATION:
			{
				if (!p_ptr->resist_chaos)
				{
					if (set_image(p_ptr->image + randint0(250) + 250))
					{
						ident = TRUE;
					}
				}
				break;
			}

			case SV_FOOD_PARALYSIS:
			{
				if (!p_ptr->free_act)
				{
					if (set_paralyzed(p_ptr->paralyzed + randint0(10) + 10))
					{
						ident = TRUE;
					}
				}
				break;
			}

			case SV_FOOD_WEAKNESS:
			{
#ifdef JP
				take_hit(DAMAGE_NOESCAPE, damroll(6, 6), "毒入り食料", -1);
#else
				take_hit(DAMAGE_NOESCAPE, damroll(6, 6), "poisonous food", -1);
#endif

				(void)do_dec_stat(A_STR);
				ident = TRUE;
				break;
			}

			case SV_FOOD_SICKNESS:
			{
#ifdef JP
				take_hit(DAMAGE_NOESCAPE, damroll(6, 6), "毒入り食料", -1);
#else
				take_hit(DAMAGE_NOESCAPE, damroll(6, 6), "poisonous food", -1);
#endif

				(void)do_dec_stat(A_CON);
				ident = TRUE;
				break;
			}

			case SV_FOOD_STUPIDITY:
			{
#ifdef JP
				take_hit(DAMAGE_NOESCAPE, damroll(8, 8), "毒入り食料", -1);
#else
				take_hit(DAMAGE_NOESCAPE, damroll(8, 8), "poisonous food", -1);
#endif

				(void)do_dec_stat(A_INT);
				ident = TRUE;
				break;
			}

			case SV_FOOD_NAIVETY:
			{
#ifdef JP
				take_hit(DAMAGE_NOESCAPE, damroll(8, 8), "毒入り食料", -1);
#else
				take_hit(DAMAGE_NOESCAPE, damroll(8, 8), "poisonous food", -1);
#endif

				(void)do_dec_stat(A_WIS);
				ident = TRUE;
				break;
			}

			case SV_FOOD_UNHEALTH:
			{
#ifdef JP
				take_hit(DAMAGE_NOESCAPE, damroll(10, 10), "毒入り食料", -1);
#else
				take_hit(DAMAGE_NOESCAPE, damroll(10, 10), "poisonous food", -1);
#endif

				(void)do_dec_stat(A_CON);
				ident = TRUE;
				break;
			}

			case SV_FOOD_DISEASE:
			{
#ifdef JP
				take_hit(DAMAGE_NOESCAPE, damroll(10, 10), "毒入り食料", -1);
#else
				take_hit(DAMAGE_NOESCAPE, damroll(10, 10), "poisonous food", -1);
#endif

				(void)do_dec_stat(A_STR);
				ident = TRUE;
				break;
			}

			case SV_FOOD_CURE_POISON:
			{
				if (set_poisoned(0)) ident = TRUE;
				break;
			}

			case SV_FOOD_CURE_BLINDNESS:
			{
				if (set_blind(0)) ident = TRUE;
				break;
			}

			case SV_FOOD_CURE_PARANOIA:
			{
				if (set_afraid(0)) ident = TRUE;
				break;
			}

			case SV_FOOD_CURE_CONFUSION:
			{
				if (set_confused(0)) ident = TRUE;
				break;
			}

			case SV_FOOD_CURE_SERIOUS:
			{
				if (hp_player(damroll(4, 8))) ident = TRUE;
				break;
			}

			case SV_FOOD_RESTORE_STR:
			{
				if (do_res_stat(A_STR)) ident = TRUE;
				break;
			}

			case SV_FOOD_RESTORE_CON:
			{
				if (do_res_stat(A_CON)) ident = TRUE;
				break;
			}

			case SV_FOOD_RESTORING:
			{
				if (do_res_stat(A_STR)) ident = TRUE;
				if (do_res_stat(A_INT)) ident = TRUE;
				if (do_res_stat(A_WIS)) ident = TRUE;
				if (do_res_stat(A_DEX)) ident = TRUE;
				if (do_res_stat(A_CON)) ident = TRUE;
				if (do_res_stat(A_CHR)) ident = TRUE;
				break;
			}


#ifdef JP
			/* それぞれの食べ物の感想をオリジナルより細かく表現 */
			case SV_FOOD_BISCUIT:
			{
				msg_print("甘くてサクサクしてとてもおいしい。");
				ident = TRUE;
				break;
			}

			case SV_FOOD_JERKY:
			{
				msg_print("歯ごたえがあっておいしい。");
				ident = TRUE;
				break;
			}

			case SV_FOOD_SLIME_MOLD:
			{
				msg_print("これはなんとも形容しがたい味だ。");
				ident = TRUE;
				break;
			}

			case SV_FOOD_RATION:
			{
				msg_print("これはおいしい。");
				ident = TRUE;
				break;
			}
#else
			case SV_FOOD_RATION:
			case SV_FOOD_BISCUIT:
			case SV_FOOD_JERKY:
			case SV_FOOD_SLIME_MOLD:
			{
				msg_print("That tastes good.");
				ident = TRUE;
				break;
			}
#endif


			case SV_FOOD_WAYBREAD:
			{
#ifdef JP
				msg_print("これはひじょうに美味だ。");
#else
				msg_print("That tastes good.");
#endif

				(void)set_poisoned(0);
				(void)hp_player(damroll(4, 8));
				ident = TRUE;
				break;
			}

#ifdef JP
			case SV_FOOD_PINT_OF_ALE:
			{
				msg_print("のどごし爽やかだ。");
				ident = TRUE;
				break;
			}

			case SV_FOOD_PINT_OF_WINE:
			{
				msg_print("That tastes good.");
				ident = TRUE;
				break;
			}
#else
			case SV_FOOD_PINT_OF_ALE:
			case SV_FOOD_PINT_OF_WINE:
			{
				msg_print("That tastes good.");
				ident = TRUE;
				break;
			}
#endif

		}
	}

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	if (!(object_is_aware(o_ptr)))
	{
		chg_virtue(V_KNOWLEDGE, -1);
		chg_virtue(V_PATIENCE, -1);
		chg_virtue(V_CHANCE, 1);
	}

	/* We have tried it */
	if (o_ptr->tval == TV_FOOD) object_tried(o_ptr);

	/* The player is now aware of the object */
	if (ident && !object_is_aware(o_ptr))
	{
		object_aware(o_ptr);
		gain_exp((lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);


	/* Food can feed the player */
	if (prace_is_(RACE_VAMPIRE) || (p_ptr->mimic_form == MIMIC_VAMPIRE))
	{
		/* Reduced nutritional benefit */
		(void)set_food(p_ptr->food + (o_ptr->pval / 10));
#ifdef JP
msg_print("あなたのような者にとって食糧など僅かな栄養にしかならない。");
#else
		msg_print("Mere victuals hold scant sustenance for a being such as yourself.");
#endif

		if (p_ptr->food < PY_FOOD_ALERT)   /* Hungry */
#ifdef JP
msg_print("あなたの飢えは新鮮な血によってのみ満たされる！");
#else
			msg_print("Your hunger can only be satisfied with fresh blood!");
#endif

	}
	else if ((prace_is_(RACE_SKELETON) ||
		  prace_is_(RACE_GOLEM) ||
		  prace_is_(RACE_ZOMBIE) ||
		  prace_is_(RACE_SPECTRE)) &&
		 (o_ptr->tval == TV_STAFF || o_ptr->tval == TV_WAND))
	{
		cptr staff;

		if (o_ptr->tval == TV_STAFF &&
		    (item < 0) && (o_ptr->number > 1))
		{
#ifdef JP
			msg_print("まずは杖を拾わなければ。");
#else
			msg_print("You must first pick up the staffs.");
#endif
			return;
		}

#ifdef JP
		staff = (o_ptr->tval == TV_STAFF) ? "杖" : "魔法棒";
#else
		staff = (o_ptr->tval == TV_STAFF) ? "staff" : "wand";
#endif

		/* "Eat" charges */
		if (o_ptr->pval == 0)
		{
#ifdef JP
			msg_format("この%sにはもう魔力が残っていない。", staff);
#else
			msg_format("The %s has no charges left.", staff);
#endif

			o_ptr->ident |= (IDENT_EMPTY);

			/* Combine / Reorder the pack (later) */
			p_ptr->notice |= (PN_COMBINE | PN_REORDER);
			p_ptr->window |= (PW_INVEN);

			return;
		}

#ifdef JP
		msg_format("あなたは%sの魔力をエネルギー源として吸収した。", staff);
#else
		msg_format("You absorb mana of the %s as your energy.", staff);
#endif

		/* Use a single charge */
		o_ptr->pval--;

		/* Eat a charge */
		set_food(p_ptr->food + 5000);

		/* XXX Hack -- unstack if necessary */
		if (o_ptr->tval == TV_STAFF &&
		    (item >= 0) && (o_ptr->number > 1))
		{
			object_type forge;
			object_type *q_ptr;

			/* Get local object */
			q_ptr = &forge;

			/* Obtain a local object */
			object_copy(q_ptr, o_ptr);

			/* Modify quantity */
			q_ptr->number = 1;

			/* Restore the charges */
			o_ptr->pval++;

			/* Unstack the used item */
			o_ptr->number--;
			p_ptr->total_weight -= q_ptr->weight;
			item = inven_carry(q_ptr);

			/* Message */
#ifdef JP
			msg_format("杖をまとめなおした。");
#else
			msg_print("You unstack your staff.");
#endif
		}

		/* Describe charges in the pack */
		if (item >= 0)
		{
			inven_item_charges(item);
		}

		/* Describe charges on the floor */
		else
		{
			floor_item_charges(0 - item);
		}

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Don't eat a staff/wand itself */
		return;
	}
	else if ((prace_is_(RACE_DEMON) ||
		 (mimic_info[p_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_DEMON)) &&
		 (o_ptr->tval == TV_CORPSE && o_ptr->sval == SV_CORPSE &&
		  my_strchr("pht", r_info[o_ptr->pval].d_char)))
	{
		/* Drain vitality of humanoids */
		char o_name[MAX_NLEN];

		object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

#ifdef JP
		msg_format("%sは燃え上り灰になった。精力を吸収した気がする。", o_name);
#else
		msg_format("%^s is burnt to ashes.  You absorb its vitality!", o_name);
#endif
		(void)set_food(PY_FOOD_MAX - 1);
	}
	else if (prace_is_(RACE_SKELETON))
	{
#if 0
		if (o_ptr->tval == TV_SKELETON ||
		    (o_ptr->tval == TV_CORPSE && o_ptr->sval == SV_SKELETON))
		{
#ifdef JP
			msg_print("あなたは骨で自分の体を補った。");
#else
			msg_print("Your body absorbs the bone.");
#endif
			set_food(p_ptr->food + 5000);
		}
		else 
#endif

		if (!((o_ptr->sval == SV_FOOD_WAYBREAD) ||
		      (o_ptr->sval < SV_FOOD_BISCUIT)))
		{
			object_type forge;
			object_type *q_ptr = &forge;

#ifdef JP
msg_print("食べ物がアゴを素通りして落ちた！");
#else
			msg_print("The food falls through your jaws!");
#endif


			/* Create the item */
			object_prep(q_ptr, lookup_kind(o_ptr->tval, o_ptr->sval));

			/* Drop the object from heaven */
			(void)drop_near(q_ptr, -1, py, px);
		}
		else
		{
#ifdef JP
msg_print("食べ物がアゴを素通りして落ち、消えた！");
#else
			msg_print("The food falls through your jaws and vanishes!");
#endif

		}
	}
	else if (prace_is_(RACE_GOLEM) ||
		 prace_is_(RACE_ZOMBIE) ||
		 prace_is_(RACE_ENT) ||
		 prace_is_(RACE_DEMON) ||
		 prace_is_(RACE_ANDROID) ||
		 prace_is_(RACE_SPECTRE) ||
		 (mimic_info[p_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_NONLIVING))
	{
#ifdef JP
msg_print("生者の食物はあなたにとってほとんど栄養にならない。");
#else
		msg_print("The food of mortals is poor sustenance for you.");
#endif

		set_food(p_ptr->food + ((o_ptr->pval) / 20));
	}
	else if (o_ptr->tval == TV_FOOD && o_ptr->sval == SV_FOOD_WAYBREAD)
	{
		/* Waybread is always fully satisfying. */
		set_food(MAX(p_ptr->food, PY_FOOD_MAX - 1));
	}
	else
	{
		/* Food can feed the player */
		(void)set_food(p_ptr->food + o_ptr->pval);
	}

	/* Destroy a food in the pack */
	if (item >= 0)
	{
		inven_item_increase(item, -1);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Destroy a food on the floor */
	else
	{
		floor_item_increase(0 - item, -1);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}
}


/*
 * Hook to determine if an object is eatable
 */
static bool item_tester_hook_eatable(object_type *o_ptr)
{
	if (o_ptr->tval==TV_FOOD) return TRUE;

#if 0
	if (prace_is_(RACE_SKELETON))
	{
		if (o_ptr->tval == TV_SKELETON ||
		    (o_ptr->tval == TV_CORPSE && o_ptr->sval == SV_SKELETON))
			return TRUE;
	}
	else 
#endif

	if (prace_is_(RACE_SKELETON) ||
	    prace_is_(RACE_GOLEM) ||
	    prace_is_(RACE_ZOMBIE) ||
	    prace_is_(RACE_SPECTRE))
	{
		if (o_ptr->tval == TV_STAFF || o_ptr->tval == TV_WAND)
			return TRUE;
	}
	else if (prace_is_(RACE_DEMON) ||
		 (mimic_info[p_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_DEMON))
	{
		if (o_ptr->tval == TV_CORPSE &&
		    o_ptr->sval == SV_CORPSE &&
		    my_strchr("pht", r_info[o_ptr->pval].d_char))
			return TRUE;
	}

	/* Assume not */
	return (FALSE);
}


/*
 * Eat some food (from the pack or floor)
 */
void do_cmd_eat_food(void)
{
	int         item;
	cptr        q, s;


	if (p_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
	{
		set_action(ACTION_NONE);
	}

	/* Restrict choices to food */
	item_tester_hook = item_tester_hook_eatable;

	/* Get an item */
#ifdef JP
	q = "どれを食べますか? ";
	s = "食べ物がない。";
#else
	q = "Eat which item? ";
	s = "You have nothing to eat.";
#endif

	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;

	/* Eat the object */
	do_cmd_eat_food_aux(item);
}


/*
 * Quaff a potion (from the pack or the floor)
 */
static void do_cmd_quaff_potion_aux(int item)
{
	int         ident, lev;
	object_type *o_ptr;
	object_type forge;
	object_type *q_ptr;


	/* Take a turn */
	energy_use = 100;

	if (world_player)
	{
		if (flush_failure) flush();
#ifdef JP
		msg_print("瓶から水が流れ出てこない！");
#else
		msg_print("The potion doesn't flow out from a bottle.");
#endif

		sound(SOUND_FAIL);
		return;
	}

	if (music_singing_any()) stop_singing();
	if (hex_spelling_any())
	{
		if (!hex_spelling(HEX_INHAIL)) stop_hex_spell_all();
	}

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

	/* Get local object */
	q_ptr = &forge;

	/* Obtain a local object */
	object_copy(q_ptr, o_ptr);

	/* Single object */
	q_ptr->number = 1;

	/* Reduce and describe inventory */
	if (item >= 0)
	{
		inven_item_increase(item, -1);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Reduce and describe floor item */
	else
	{
		floor_item_increase(0 - item, -1);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}

	/* Sound */
	sound(SOUND_QUAFF);


	/* Not identified yet */
	ident = FALSE;

	/* Object level */
	lev = k_info[q_ptr->k_idx].level;

	/* Analyze the potion */
	if (q_ptr->tval == TV_POTION)
	{
		switch (q_ptr->sval)
		{
#ifdef JP
			/* 飲みごたえをオリジナルより細かく表現 */
		case SV_POTION_WATER:
			msg_print("口の中がさっぱりした。");
			msg_print("のどの渇きが少しおさまった。");
			ident = TRUE;
			break;

		case SV_POTION_APPLE_JUICE:
			msg_print("甘くてサッパリとしていて、とてもおいしい。");
			msg_print("のどの渇きが少しおさまった。");
			ident = TRUE;
			break;

		case SV_POTION_SLIME_MOLD:
			msg_print("なんとも不気味な味だ。");
			msg_print("のどの渇きが少しおさまった。");
			ident = TRUE;
			break;

#else
		case SV_POTION_WATER:
		case SV_POTION_APPLE_JUICE:
		case SV_POTION_SLIME_MOLD:
			msg_print("You feel less thirsty.");
			ident = TRUE;
			break;
#endif

		case SV_POTION_SLOWNESS:
			if (set_slow(randint1(25) + 15, FALSE)) ident = TRUE;
			break;

		case SV_POTION_SALT_WATER:
#ifdef JP
			msg_print("うぇ！思わず吐いてしまった。");
#else
			msg_print("The potion makes you vomit!");
#endif

			if (!(prace_is_(RACE_GOLEM) ||
			      prace_is_(RACE_ZOMBIE) ||
			      prace_is_(RACE_DEMON) ||
			      prace_is_(RACE_ANDROID) ||
			      prace_is_(RACE_SPECTRE) ||
			      (mimic_info[p_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_NONLIVING)))
			{
				/* Only living creatures get thirsty */
				(void)set_food(PY_FOOD_STARVE - 1);
			}

			(void)set_poisoned(0);
			(void)set_paralyzed(p_ptr->paralyzed + 4);
			ident = TRUE;
			break;

		case SV_POTION_POISON:
			if (!(p_ptr->resist_pois || IS_OPPOSE_POIS()))
			{
				if (set_poisoned(p_ptr->poisoned + randint0(15) + 10))
				{
					ident = TRUE;
				}
			}
			break;

		case SV_POTION_BLINDNESS:
			if (!p_ptr->resist_blind)
			{
				if (set_blind(p_ptr->blind + randint0(100) + 100))
				{
					ident = TRUE;
				}
			}
			break;

		case SV_POTION_CONFUSION: /* Booze */
			if (p_ptr->pclass != CLASS_MONK) chg_virtue(V_HARMONY, -1);
			else if (!p_ptr->resist_conf) p_ptr->special_attack |= ATTACK_SUIKEN;
			if (!p_ptr->resist_conf)
			{
				if (set_confused(randint0(20) + 15))
				{
					ident = TRUE;
				}
			}

			if (!p_ptr->resist_chaos)
			{
				if (one_in_(2))
				{
					if (set_image(p_ptr->image + randint0(150) + 150))
					{
						ident = TRUE;
					}
				}
				if (one_in_(13) && (p_ptr->pclass != CLASS_MONK))
				{
					ident = TRUE;
					if (one_in_(3)) lose_all_info();
					else wiz_dark();
					(void)teleport_player_aux(100, TELEPORT_NONMAGICAL | TELEPORT_PASSIVE);
					wiz_dark();
#ifdef JP
					msg_print("知らない場所で目が醒めた。頭痛がする。");
					msg_print("何も思い出せない。どうやってここへ来たのかも分からない！");
#else
					msg_print("You wake up somewhere with a sore head...");
					msg_print("You can't remember a thing, or how you got here!");
#endif

				}
			}
			break;

		case SV_POTION_SLEEP:
			if (!p_ptr->free_act)
			{
#ifdef JP
		msg_print("あなたは眠ってしまった。");
#else
		msg_print("You fall asleep.");
#endif


				if (ironman_nightmare)
				{
#ifdef JP
msg_print("恐ろしい光景が頭に浮かんできた。");
#else
					msg_print("A horrible vision enters your mind.");
#endif


					/* Pick a nightmare */
					get_mon_num_prep(get_nightmare, NULL);

					/* Have some nightmares */
					have_nightmare(get_mon_num(MAX_DEPTH));

					/* Remove the monster restriction */
					get_mon_num_prep(NULL, NULL);
				}
				if (set_paralyzed(p_ptr->paralyzed + randint0(4) + 4))
				{
					ident = TRUE;
				}
			}
			break;

		case SV_POTION_LOSE_MEMORIES:
			if (!p_ptr->hold_life && (p_ptr->exp > 0))
			{
#ifdef JP
				msg_print("過去の記憶が薄れていく気がする。");
#else
				msg_print("You feel your memories fade.");
#endif
				chg_virtue(V_KNOWLEDGE, -5);

				lose_exp(p_ptr->exp / 4);
				ident = TRUE;
			}
			break;

		case SV_POTION_RUINATION:
#ifdef JP
			msg_print("身も心も弱ってきて、精気が抜けていくようだ。");
			take_hit(DAMAGE_LOSELIFE, damroll(10, 10), "破滅の薬", -1);
#else
			msg_print("Your nerves and muscles feel weak and lifeless!");
			take_hit(DAMAGE_LOSELIFE, damroll(10, 10), "a potion of Ruination", -1);
#endif

			(void)dec_stat(A_DEX, 25, TRUE);
			(void)dec_stat(A_WIS, 25, TRUE);
			(void)dec_stat(A_CON, 25, TRUE);
			(void)dec_stat(A_STR, 25, TRUE);
			(void)dec_stat(A_CHR, 25, TRUE);
			(void)dec_stat(A_INT, 25, TRUE);
			ident = TRUE;
			break;

		case SV_POTION_DEC_STR:
			if (do_dec_stat(A_STR)) ident = TRUE;
			break;

		case SV_POTION_DEC_INT:
			if (do_dec_stat(A_INT)) ident = TRUE;
			break;

		case SV_POTION_DEC_WIS:
			if (do_dec_stat(A_WIS)) ident = TRUE;
			break;

		case SV_POTION_DEC_DEX:
			if (do_dec_stat(A_DEX)) ident = TRUE;
			break;

		case SV_POTION_DEC_CON:
			if (do_dec_stat(A_CON)) ident = TRUE;
			break;

		case SV_POTION_DEC_CHR:
			if (do_dec_stat(A_CHR)) ident = TRUE;
			break;

		case SV_POTION_DETONATIONS:
#ifdef JP
			msg_print("体の中で激しい爆発が起きた！");
			take_hit(DAMAGE_NOESCAPE, damroll(50, 20), "爆発の薬", -1);
#else
			msg_print("Massive explosions rupture your body!");
			take_hit(DAMAGE_NOESCAPE, damroll(50, 20), "a potion of Detonation", -1);
#endif

			(void)set_stun(p_ptr->stun + 75);
			(void)set_cut(p_ptr->cut + 5000);
			ident = TRUE;
			break;

		case SV_POTION_DEATH:
			chg_virtue(V_VITALITY, -1);
			chg_virtue(V_UNLIFE, 5);
#ifdef JP
			msg_print("死の予感が体中を駆けめぐった。");
			take_hit(DAMAGE_LOSELIFE, 5000, "死の薬", -1);
#else
			msg_print("A feeling of Death flows through your body.");
			take_hit(DAMAGE_LOSELIFE, 5000, "a potion of Death", -1);
#endif

			ident = TRUE;
			break;

		case SV_POTION_INFRAVISION:
			if (set_tim_infra(p_ptr->tim_infra + 100 + randint1(100), FALSE))
			{
				ident = TRUE;
			}
			break;

		case SV_POTION_DETECT_INVIS:
			if (set_tim_invis(p_ptr->tim_invis + 12 + randint1(12), FALSE))
			{
				ident = TRUE;
			}
			break;

		case SV_POTION_SLOW_POISON:
			if (set_poisoned(p_ptr->poisoned / 2)) ident = TRUE;
			break;

		case SV_POTION_CURE_POISON:
			if (set_poisoned(0)) ident = TRUE;
			break;

		case SV_POTION_BOLDNESS:
			if (set_afraid(0)) ident = TRUE;
			break;

		case SV_POTION_SPEED:
			if (!p_ptr->fast)
			{
				if (set_fast(randint1(25) + 15, FALSE)) ident = TRUE;
			}
			else
			{
				(void)set_fast(p_ptr->fast + 5, FALSE);
			}
			break;

		case SV_POTION_RESIST_HEAT:
			if (set_oppose_fire(p_ptr->oppose_fire + randint1(10) + 10, FALSE))
			{
				ident = TRUE;
			}
			break;

		case SV_POTION_RESIST_COLD:
			if (set_oppose_cold(p_ptr->oppose_cold + randint1(10) + 10, FALSE))
			{
				ident = TRUE;
			}
			break;

		case SV_POTION_HEROISM:
			if (set_afraid(0)) ident = TRUE;
			if (set_hero(p_ptr->hero + randint1(25) + 25, FALSE)) ident = TRUE;
			if (hp_player(10)) ident = TRUE;
			break;

		case SV_POTION_BESERK_STRENGTH:
			if (set_afraid(0)) ident = TRUE;
			if (set_shero(p_ptr->shero + randint1(25) + 25, FALSE)) ident = TRUE;
			if (hp_player(30)) ident = TRUE;
			break;

		case SV_POTION_CURE_LIGHT:
			if (hp_player(damroll(2, 8))) ident = TRUE;
			if (set_blind(0)) ident = TRUE;
			if (set_cut(p_ptr->cut - 10)) ident = TRUE;
			if (set_shero(0,TRUE)) ident = TRUE;
			break;

		case SV_POTION_CURE_SERIOUS:
			if (hp_player(damroll(4, 8))) ident = TRUE;
			if (set_blind(0)) ident = TRUE;
			if (set_confused(0)) ident = TRUE;
			if (set_cut((p_ptr->cut / 2) - 50)) ident = TRUE;
			if (set_shero(0,TRUE)) ident = TRUE;
			break;

		case SV_POTION_CURE_CRITICAL:
			if (hp_player(damroll(6, 8))) ident = TRUE;
			if (set_blind(0)) ident = TRUE;
			if (set_confused(0)) ident = TRUE;
			if (set_poisoned(0)) ident = TRUE;
			if (set_stun(0)) ident = TRUE;
			if (set_cut(0)) ident = TRUE;
			if (set_shero(0,TRUE)) ident = TRUE;
			break;

		case SV_POTION_HEALING:
			if (hp_player(300)) ident = TRUE;
			if (set_blind(0)) ident = TRUE;
			if (set_confused(0)) ident = TRUE;
			if (set_poisoned(0)) ident = TRUE;
			if (set_stun(0)) ident = TRUE;
			if (set_cut(0)) ident = TRUE;
			if (set_shero(0,TRUE)) ident = TRUE;
			break;

		case SV_POTION_STAR_HEALING:
			if (hp_player(1200)) ident = TRUE;
			if (set_blind(0)) ident = TRUE;
			if (set_confused(0)) ident = TRUE;
			if (set_poisoned(0)) ident = TRUE;
			if (set_stun(0)) ident = TRUE;
			if (set_cut(0)) ident = TRUE;
			if (set_shero(0,TRUE)) ident = TRUE;
			break;

		case SV_POTION_LIFE:
			chg_virtue(V_VITALITY, 1);
			chg_virtue(V_UNLIFE, -5);
#ifdef JP
			msg_print("体中に生命力が満ちあふれてきた！");
#else
			msg_print("You feel life flow through your body!");
#endif

			restore_level();
			(void)set_poisoned(0);
			(void)set_blind(0);
			(void)set_confused(0);
			(void)set_image(0);
			(void)set_stun(0);
			(void)set_cut(0);
			(void)do_res_stat(A_STR);
			(void)do_res_stat(A_CON);
			(void)do_res_stat(A_DEX);
			(void)do_res_stat(A_WIS);
			(void)do_res_stat(A_INT);
			(void)do_res_stat(A_CHR);
			(void)set_shero(0,TRUE);
			update_stuff();
			hp_player(5000);
			ident = TRUE;
			break;

		case SV_POTION_RESTORE_MANA:
			if (p_ptr->pclass == CLASS_MAGIC_EATER)
			{
				int i;
				for (i = 0; i < EATER_EXT*2; i++)
				{
					p_ptr->magic_num1[i] += (p_ptr->magic_num2[i] < 10) ? EATER_CHARGE * 3 : p_ptr->magic_num2[i]*EATER_CHARGE/3;
					if (p_ptr->magic_num1[i] > p_ptr->magic_num2[i]*EATER_CHARGE) p_ptr->magic_num1[i] = p_ptr->magic_num2[i]*EATER_CHARGE;
				}
				for (; i < EATER_EXT*3; i++)
				{
					int k_idx = lookup_kind(TV_ROD, i-EATER_EXT*2);
					p_ptr->magic_num1[i] -= ((p_ptr->magic_num2[i] < 10) ? EATER_ROD_CHARGE*3 : p_ptr->magic_num2[i]*EATER_ROD_CHARGE/3)*k_info[k_idx].pval;
					if (p_ptr->magic_num1[i] < 0) p_ptr->magic_num1[i] = 0;
				}
#ifdef JP
				msg_print("頭がハッキリとした。");
#else
				msg_print("You feel your head clear.");
#endif
				p_ptr->window |= (PW_PLAYER);
				ident = TRUE;
			}
			else if (p_ptr->csp < p_ptr->msp)
			{
				p_ptr->csp = p_ptr->msp;
				p_ptr->csp_frac = 0;
#ifdef JP
				msg_print("頭がハッキリとした。");
#else
				msg_print("You feel your head clear.");
#endif

				p_ptr->redraw |= (PR_MANA);
				p_ptr->window |= (PW_PLAYER);
				p_ptr->window |= (PW_SPELL);
				ident = TRUE;
			}
			if (set_shero(0,TRUE)) ident = TRUE;
			break;

		case SV_POTION_RESTORE_EXP:
			if (restore_level()) ident = TRUE;
			break;

		case SV_POTION_RES_STR:
			if (do_res_stat(A_STR)) ident = TRUE;
			break;

		case SV_POTION_RES_INT:
			if (do_res_stat(A_INT)) ident = TRUE;
			break;

		case SV_POTION_RES_WIS:
			if (do_res_stat(A_WIS)) ident = TRUE;
			break;

		case SV_POTION_RES_DEX:
			if (do_res_stat(A_DEX)) ident = TRUE;
			break;

		case SV_POTION_RES_CON:
			if (do_res_stat(A_CON)) ident = TRUE;
			break;

		case SV_POTION_RES_CHR:
			if (do_res_stat(A_CHR)) ident = TRUE;
			break;

		case SV_POTION_INC_STR:
			if (do_inc_stat(A_STR)) ident = TRUE;
			break;

		case SV_POTION_INC_INT:
			if (do_inc_stat(A_INT)) ident = TRUE;
			break;

		case SV_POTION_INC_WIS:
			if (do_inc_stat(A_WIS)) ident = TRUE;
			break;

		case SV_POTION_INC_DEX:
			if (do_inc_stat(A_DEX)) ident = TRUE;
			break;

		case SV_POTION_INC_CON:
			if (do_inc_stat(A_CON)) ident = TRUE;
			break;

		case SV_POTION_INC_CHR:
			if (do_inc_stat(A_CHR)) ident = TRUE;
			break;

		case SV_POTION_AUGMENTATION:
			if (do_inc_stat(A_STR)) ident = TRUE;
			if (do_inc_stat(A_INT)) ident = TRUE;
			if (do_inc_stat(A_WIS)) ident = TRUE;
			if (do_inc_stat(A_DEX)) ident = TRUE;
			if (do_inc_stat(A_CON)) ident = TRUE;
			if (do_inc_stat(A_CHR)) ident = TRUE;
			break;

		case SV_POTION_ENLIGHTENMENT:
#ifdef JP
			msg_print("自分の置かれている状況が脳裏に浮かんできた...");
#else
			msg_print("An image of your surroundings forms in your mind...");
#endif

			chg_virtue(V_KNOWLEDGE, 1);
			chg_virtue(V_ENLIGHTEN, 1);
			wiz_lite(FALSE);
			ident = TRUE;
			break;

		case SV_POTION_STAR_ENLIGHTENMENT:
#ifdef JP
			msg_print("更なる啓蒙を感じた...");
#else
			msg_print("You begin to feel more enlightened...");
#endif

			chg_virtue(V_KNOWLEDGE, 1);
			chg_virtue(V_ENLIGHTEN, 2);
			msg_print(NULL);
			wiz_lite(FALSE);
			(void)do_inc_stat(A_INT);
			(void)do_inc_stat(A_WIS);
			(void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
			(void)detect_doors(DETECT_RAD_DEFAULT);
			(void)detect_stairs(DETECT_RAD_DEFAULT);
			(void)detect_treasure(DETECT_RAD_DEFAULT);
			(void)detect_objects_gold(DETECT_RAD_DEFAULT);
			(void)detect_objects_normal(DETECT_RAD_DEFAULT);
			identify_pack();
			self_knowledge();
			ident = TRUE;
			break;

		case SV_POTION_SELF_KNOWLEDGE:
#ifdef JP
			msg_print("自分自身のことが少しは分かった気がする...");
#else
			msg_print("You begin to know yourself a little better...");
#endif

			msg_print(NULL);
			self_knowledge();
			ident = TRUE;
			break;

		case SV_POTION_EXPERIENCE:
			if (p_ptr->prace == RACE_ANDROID) break;
			chg_virtue(V_ENLIGHTEN, 1);
			if (p_ptr->exp < PY_MAX_EXP)
			{
				s32b ee = (p_ptr->exp / 2) + 10;
				if (ee > 100000L) ee = 100000L;
#ifdef JP
				msg_print("更に経験を積んだような気がする。");
#else
				msg_print("You feel more experienced.");
#endif

				gain_exp(ee);
				ident = TRUE;
			}
			break;

		case SV_POTION_RESISTANCE:
			(void)set_oppose_acid(p_ptr->oppose_acid + randint1(20) + 20, FALSE);
			(void)set_oppose_elec(p_ptr->oppose_elec + randint1(20) + 20, FALSE);
			(void)set_oppose_fire(p_ptr->oppose_fire + randint1(20) + 20, FALSE);
			(void)set_oppose_cold(p_ptr->oppose_cold + randint1(20) + 20, FALSE);
			(void)set_oppose_pois(p_ptr->oppose_pois + randint1(20) + 20, FALSE);
			ident = TRUE;
			break;

		case SV_POTION_CURING:
			if (hp_player(50)) ident = TRUE;
			if (set_blind(0)) ident = TRUE;
			if (set_poisoned(0)) ident = TRUE;
			if (set_confused(0)) ident = TRUE;
			if (set_stun(0)) ident = TRUE;
			if (set_cut(0)) ident = TRUE;
			if (set_image(0)) ident = TRUE;
			break;

		case SV_POTION_INVULNERABILITY:
			(void)set_invuln(p_ptr->invuln + randint1(4) + 4, FALSE);
			ident = TRUE;
			break;

		case SV_POTION_NEW_LIFE:
			do_cmd_rerate(FALSE);
			get_max_stats();
			p_ptr->update |= PU_BONUS;
			if (p_ptr->muta1 || p_ptr->muta2 || p_ptr->muta3)
			{
				chg_virtue(V_CHANCE, -5);
#ifdef JP
msg_print("全ての突然変異が治った。");
#else
				msg_print("You are cured of all mutations.");
#endif

				p_ptr->muta1 = p_ptr->muta2 = p_ptr->muta3 = 0;
				p_ptr->update |= PU_BONUS;
				handle_stuff();
				mutant_regenerate_mod = calc_mutant_regenerate_mod();
			}
			ident = TRUE;
			break;

		case SV_POTION_NEO_TSUYOSHI:
			(void)set_image(0);
			(void)set_tsuyoshi(p_ptr->tsuyoshi + randint1(100) + 100, FALSE);
			ident = TRUE;
			break;

		case SV_POTION_TSUYOSHI:
#ifdef JP
msg_print("「オクレ兄さん！」");
#else
			msg_print("Brother OKURE!");
#endif
			msg_print(NULL);
			p_ptr->tsuyoshi = 1;
			(void)set_tsuyoshi(0, TRUE);
			if (!p_ptr->resist_chaos)
			{
				(void)set_image(50 + randint1(50));
			}
			ident = TRUE;
			break;
		
		case SV_POTION_POLYMORPH:
			if ((p_ptr->muta1 || p_ptr->muta2 || p_ptr->muta3) && one_in_(23))
			{
				chg_virtue(V_CHANCE, -5);
#ifdef JP
msg_print("全ての突然変異が治った。");
#else
				msg_print("You are cured of all mutations.");
#endif

				p_ptr->muta1 = p_ptr->muta2 = p_ptr->muta3 = 0;
				p_ptr->update |= PU_BONUS;
				handle_stuff();
			}
			else
			{
				do
				{
					if (one_in_(2))
					{
						if(gain_random_mutation(0)) ident = TRUE;
					}
					else if (lose_mutation(0)) ident = TRUE;
				} while(!ident || one_in_(2));
			}
			break;
		}
	}

	if (prace_is_(RACE_SKELETON))
	{
#ifdef JP
msg_print("液体の一部はあなたのアゴを素通りして落ちた！");
#else
		msg_print("Some of the fluid falls through your jaws!");
#endif

		(void)potion_smash_effect(0, py, px, q_ptr->k_idx);
	}

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	if (!(object_is_aware(q_ptr)))
	{
		chg_virtue(V_PATIENCE, -1);
		chg_virtue(V_CHANCE, 1);
		chg_virtue(V_KNOWLEDGE, -1);
	}

	/* The item has been tried */
	object_tried(q_ptr);

	/* An identification was made */
	if (ident && !object_is_aware(q_ptr))
	{
		object_aware(q_ptr);
		gain_exp((lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Potions can feed the player */
	switch (p_ptr->mimic_form)
	{
	case MIMIC_NONE:
		switch (p_ptr->prace)
		{
			case RACE_VAMPIRE:
				(void)set_food(p_ptr->food + (q_ptr->pval / 10));
				break;
			case RACE_SKELETON:
				/* Do nothing */
				break;
			case RACE_GOLEM:
			case RACE_ZOMBIE:
			case RACE_DEMON:
			case RACE_SPECTRE:
				set_food(p_ptr->food + ((q_ptr->pval) / 20));
				break;
			case RACE_ANDROID:
				if (q_ptr->tval == TV_FLASK)
				{
#ifdef JP
					msg_print("オイルを補給した。");
#else
					msg_print("You replenish yourself with the oil.");
#endif
					set_food(p_ptr->food + 5000);
				}
				else
				{
					set_food(p_ptr->food + ((q_ptr->pval) / 20));
				}
				break;
			case RACE_ENT:
#ifdef JP
				msg_print("水分を取り込んだ。");
#else
				msg_print("You are moistened.");
#endif
				set_food(MIN(p_ptr->food + q_ptr->pval + MAX(0, q_ptr->pval * 10) + 2000, PY_FOOD_MAX - 1));
				break;
			default:
				(void)set_food(p_ptr->food + q_ptr->pval);
				break;
		}
		break;
	case MIMIC_DEMON:
	case MIMIC_DEMON_LORD:
		set_food(p_ptr->food + ((q_ptr->pval) / 20));
		break;
	case MIMIC_VAMPIRE:
		(void)set_food(p_ptr->food + (q_ptr->pval / 10));
		break;
	default:
		(void)set_food(p_ptr->food + q_ptr->pval);
		break;
	}
}


/*
 * Hook to determine if an object can be quaffed
 */
static bool item_tester_hook_quaff(object_type *o_ptr)
{
	if (o_ptr->tval == TV_POTION) return TRUE;

	if (prace_is_(RACE_ANDROID))
	{
		if (o_ptr->tval == TV_FLASK && o_ptr->sval == SV_FLASK_OIL)
			return TRUE;
	}

	return FALSE;
}


/*
 * Quaff some potion (from the pack or floor)
 */
void do_cmd_quaff_potion(void)
{
	int  item;
	cptr q, s;

	if (p_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
	{
		set_action(ACTION_NONE);
	}

	/* Restrict choices to potions */
	item_tester_hook = item_tester_hook_quaff;

	/* Get an item */
#ifdef JP
	q = "どの薬を飲みますか? ";
	s = "飲める薬がない。";
#else
	q = "Quaff which potion? ";
	s = "You have no potions to quaff.";
#endif

	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;

	/* Quaff the potion */
	do_cmd_quaff_potion_aux(item);
}


/*
 * Read a scroll (from the pack or floor).
 *
 * Certain scrolls can be "aborted" without losing the scroll.  These
 * include scrolls with no effects but recharge or identify, which are
 * cancelled before use.  XXX Reading them still takes a turn, though.
 */
static void do_cmd_read_scroll_aux(int item, bool known)
{
	int         k, used_up, ident, lev;
	object_type *o_ptr;
	char        Rumor[1024];


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


	/* Take a turn */
	energy_use = 100;

	if (world_player)
	{
		if (flush_failure) flush();
#ifdef JP
		msg_print("止まった時の中ではうまく働かないようだ。");
#else
		msg_print("Nothing happen.");
#endif

		sound(SOUND_FAIL);
		return;
	}

	if (p_ptr->pclass == CLASS_BERSERKER)
	{
#ifdef JP
		msg_print("巻物なんて読めない。");
#else
		msg_print("You cannot read.");
#endif
		return;
	}

	if (music_singing_any()) stop_singing();

	/* Hex */
	if (hex_spelling_any() && ((p_ptr->lev < 35) || hex_spell_fully())) stop_hex_spell_all();

	/* Not identified yet */
	ident = FALSE;

	/* Object level */
	lev = k_info[o_ptr->k_idx].level;

	/* Assume the scroll will get used up */
	used_up = TRUE;

	if (o_ptr->tval == TV_SCROLL)
	{
	/* Analyze the scroll */
	switch (o_ptr->sval)
	{
		case SV_SCROLL_DARKNESS:
		{
			if (!(p_ptr->resist_blind) && !(p_ptr->resist_dark))
			{
				(void)set_blind(p_ptr->blind + 3 + randint1(5));
			}
			if (unlite_area(10, 3)) ident = TRUE;
			break;
		}

		case SV_SCROLL_AGGRAVATE_MONSTER:
		{
#ifdef JP
			msg_print("カン高くうなる様な音が辺りを覆った。");
#else
			msg_print("There is a high pitched humming noise.");
#endif

			aggravate_monsters(0);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_CURSE_ARMOR:
		{
			if (curse_armor()) ident = TRUE;
			break;
		}

		case SV_SCROLL_CURSE_WEAPON:
		{
			k = 0;
			if (buki_motteruka(INVEN_RARM))
			{
				k = INVEN_RARM;
				if (buki_motteruka(INVEN_LARM) && one_in_(2)) k = INVEN_LARM;
			}
			else if (buki_motteruka(INVEN_LARM)) k = INVEN_LARM;
			if (k && curse_weapon(FALSE, k)) ident = TRUE;
			break;
		}

		case SV_SCROLL_SUMMON_MONSTER:
		{
			for (k = 0; k < randint1(3); k++)
			{
				if (summon_specific(0, py, px, dun_level, 0, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)))
				{
					ident = TRUE;
				}
			}
			break;
		}

		case SV_SCROLL_SUMMON_UNDEAD:
		{
			for (k = 0; k < randint1(3); k++)
			{
				if (summon_specific(0, py, px, dun_level, SUMMON_UNDEAD, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)))
				{
					ident = TRUE;
				}
			}
			break;
		}

		case SV_SCROLL_SUMMON_PET:
		{
			if (summon_specific(-1, py, px, dun_level, 0, (PM_ALLOW_GROUP | PM_FORCE_PET)))
			{
				ident = TRUE;
			}
			break;
		}

		case SV_SCROLL_SUMMON_KIN:
		{
			if (summon_kin_player(p_ptr->lev, py, px, (PM_FORCE_PET | PM_ALLOW_GROUP)))
			{
				ident = TRUE;
			}
			break;
		}

		case SV_SCROLL_TRAP_CREATION:
		{
			if (trap_creation(py, px)) ident = TRUE;
			break;
		}

		case SV_SCROLL_PHASE_DOOR:
		{
			teleport_player(10, 0L);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_TELEPORT:
		{
			teleport_player(100, 0L);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_TELEPORT_LEVEL:
		{
			(void)teleport_level(0);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_WORD_OF_RECALL:
		{
			if (!word_of_recall()) used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_IDENTIFY:
		{
			if (!ident_spell(FALSE)) used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_STAR_IDENTIFY:
		{
			if (!identify_fully(FALSE)) used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_REMOVE_CURSE:
		{
			if (remove_curse())
			{
#ifdef JP
				msg_print("誰かに見守られているような気がする。");
#else
				msg_print("You feel as if someone is watching over you.");
#endif

				ident = TRUE;
			}
			break;
		}

		case SV_SCROLL_STAR_REMOVE_CURSE:
		{
			if (remove_all_curse())
			{
#ifdef JP
				msg_print("誰かに見守られているような気がする。");
#else
				msg_print("You feel as if someone is watching over you.");
#endif
			}
			ident = TRUE;
			break;
		}

		case SV_SCROLL_ENCHANT_ARMOR:
		{
			ident = TRUE;
			if (!enchant_spell(0, 0, 1)) used_up = FALSE;
			break;
		}

		case SV_SCROLL_ENCHANT_WEAPON_TO_HIT:
		{
			if (!enchant_spell(1, 0, 0)) used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_ENCHANT_WEAPON_TO_DAM:
		{
			if (!enchant_spell(0, 1, 0)) used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_STAR_ENCHANT_ARMOR:
		{
			if (!enchant_spell(0, 0, randint1(3) + 2)) used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_STAR_ENCHANT_WEAPON:
		{
			if (!enchant_spell(randint1(3), randint1(3), 0)) used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_RECHARGING:
		{
			if (!recharge(130)) used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_MUNDANITY:
		{
			ident = TRUE;
			if (!mundane_spell(FALSE)) used_up = FALSE;
			break;
		}

		case SV_SCROLL_LIGHT:
		{
			if (lite_area(damroll(2, 8), 2)) ident = TRUE;
			break;
		}

		case SV_SCROLL_MAPPING:
		{
			map_area(DETECT_RAD_MAP);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_DETECT_GOLD:
		{
			if (detect_treasure(DETECT_RAD_DEFAULT)) ident = TRUE;
			if (detect_objects_gold(DETECT_RAD_DEFAULT)) ident = TRUE;
			break;
		}

		case SV_SCROLL_DETECT_ITEM:
		{
			if (detect_objects_normal(DETECT_RAD_DEFAULT)) ident = TRUE;
			break;
		}

		case SV_SCROLL_DETECT_TRAP:
		{
			if (detect_traps(DETECT_RAD_DEFAULT, known)) ident = TRUE;
			break;
		}

		case SV_SCROLL_DETECT_DOOR:
		{
			if (detect_doors(DETECT_RAD_DEFAULT)) ident = TRUE;
			if (detect_stairs(DETECT_RAD_DEFAULT)) ident = TRUE;
			break;
		}

		case SV_SCROLL_DETECT_INVIS:
		{
			if (detect_monsters_invis(DETECT_RAD_DEFAULT)) ident = TRUE;
			break;
		}

		case SV_SCROLL_SATISFY_HUNGER:
		{
			if (set_food(PY_FOOD_MAX - 1)) ident = TRUE;
			break;
		}

		case SV_SCROLL_BLESSING:
		{
			if (set_blessed(p_ptr->blessed + randint1(12) + 6, FALSE)) ident = TRUE;
			break;
		}

		case SV_SCROLL_HOLY_CHANT:
		{
			if (set_blessed(p_ptr->blessed + randint1(24) + 12, FALSE)) ident = TRUE;
			break;
		}

		case SV_SCROLL_HOLY_PRAYER:
		{
			if (set_blessed(p_ptr->blessed + randint1(48) + 24, FALSE)) ident = TRUE;
			break;
		}

		case SV_SCROLL_MONSTER_CONFUSION:
		{
			if (!(p_ptr->special_attack & ATTACK_CONFUSE))
			{
#ifdef JP
				msg_print("手が輝き始めた。");
#else
				msg_print("Your hands begin to glow.");
#endif

				p_ptr->special_attack |= ATTACK_CONFUSE;
				p_ptr->redraw |= (PR_STATUS);
				ident = TRUE;
			}
			break;
		}

		case SV_SCROLL_PROTECTION_FROM_EVIL:
		{
			k = 3 * p_ptr->lev;
			if (set_protevil(p_ptr->protevil + randint1(25) + k, FALSE)) ident = TRUE;
			break;
		}

		case SV_SCROLL_RUNE_OF_PROTECTION:
		{
			warding_glyph();
			ident = TRUE;
			break;
		}

		case SV_SCROLL_TRAP_DOOR_DESTRUCTION:
		{
			if (destroy_doors_touch()) ident = TRUE;
			break;
		}

		case SV_SCROLL_STAR_DESTRUCTION:
		{
			if (destroy_area(py, px, 13 + randint0(5), FALSE))
				ident = TRUE;
			else
#ifdef JP
msg_print("ダンジョンが揺れた...");
#else
				msg_print("The dungeon trembles...");
#endif


			break;
		}

		case SV_SCROLL_DISPEL_UNDEAD:
		{
			if (dispel_undead(80)) ident = TRUE;
			break;
		}

		case SV_SCROLL_SPELL:
		{
			if ((p_ptr->pclass == CLASS_WARRIOR) || (p_ptr->pclass == CLASS_IMITATOR) || (p_ptr->pclass == CLASS_MINDCRAFTER) || (p_ptr->pclass == CLASS_SORCERER) || (p_ptr->pclass == CLASS_ARCHER) || (p_ptr->pclass == CLASS_MAGIC_EATER) || (p_ptr->pclass == CLASS_RED_MAGE) || (p_ptr->pclass == CLASS_SAMURAI) || (p_ptr->pclass == CLASS_BLUE_MAGE) || (p_ptr->pclass == CLASS_CAVALRY) || (p_ptr->pclass == CLASS_BERSERKER) || (p_ptr->pclass == CLASS_SMITH) || (p_ptr->pclass == CLASS_MIRROR_MASTER) || (p_ptr->pclass == CLASS_NINJA)) break;
			p_ptr->add_spells++;
			p_ptr->update |= (PU_SPELLS);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_GENOCIDE:
		{
			(void)symbol_genocide(300, TRUE);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_MASS_GENOCIDE:
		{
			(void)mass_genocide(300, TRUE);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_ACQUIREMENT:
		{
			acquirement(py, px, 1, TRUE, FALSE);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_STAR_ACQUIREMENT:
		{
			acquirement(py, px, randint1(2) + 1, TRUE, FALSE);
			ident = TRUE;
			break;
		}

		/* New Hengband scrolls */
		case SV_SCROLL_FIRE:
		{
			fire_ball(GF_FIRE, 0, 666, 4);
			/* Note: "Double" damage since it is centered on the player ... */
			if (!(IS_OPPOSE_FIRE() || p_ptr->resist_fire || p_ptr->immune_fire))
#ifdef JP
take_hit(DAMAGE_NOESCAPE, 50+randint1(50), "炎の巻物", -1);
#else
				take_hit(DAMAGE_NOESCAPE, 50 + randint1(50), "a Scroll of Fire", -1);
#endif

			ident = TRUE;
			break;
		}


		case SV_SCROLL_ICE:
		{
			fire_ball(GF_ICE, 0, 777, 4);
			if (!(IS_OPPOSE_COLD() || p_ptr->resist_cold || p_ptr->immune_cold))
#ifdef JP
take_hit(DAMAGE_NOESCAPE, 100+randint1(100), "氷の巻物", -1);
#else
				take_hit(DAMAGE_NOESCAPE, 100 + randint1(100), "a Scroll of Ice", -1);
#endif

			ident = TRUE;
			break;
		}

		case SV_SCROLL_CHAOS:
		{
			fire_ball(GF_CHAOS, 0, 1000, 4);
			if (!p_ptr->resist_chaos)
#ifdef JP
take_hit(DAMAGE_NOESCAPE, 111+randint1(111), "ログルスの巻物", -1);
#else
				take_hit(DAMAGE_NOESCAPE, 111 + randint1(111), "a Scroll of Logrus", -1);
#endif

			ident = TRUE;
			break;
		}

		case SV_SCROLL_RUMOR:
		{
			errr err = 0;

			switch (randint1(20))
			{
				case 1:
#ifdef JP
err = get_rnd_line("chainswd_j.txt", 0, Rumor);
#else
					err = get_rnd_line("chainswd.txt", 0, Rumor);
#endif

					break;
				case 2:
#ifdef JP
err = get_rnd_line("error_j.txt", 0, Rumor);
#else
					err = get_rnd_line("error.txt", 0, Rumor);
#endif

					break;
				case 3:
				case 4:
				case 5:
#ifdef JP
err = get_rnd_line("death_j.txt", 0, Rumor);
#else
					err = get_rnd_line("death.txt", 0, Rumor);
#endif

					break;
				default:
#ifdef JP
err = get_rnd_line_jonly("rumors_j.txt", 0, Rumor, 10);
#else
					err = get_rnd_line("rumors.txt", 0, Rumor);
#endif

					break;
			}

			/* An error occured */
#ifdef JP
if (err) strcpy(Rumor, "嘘の噂もある。");
#else
			if (err) strcpy(Rumor, "Some rumors are wrong.");
#endif


#ifdef JP
msg_print("巻物にはメッセージが書かれている:");
#else
			msg_print("There is message on the scroll. It says:");
#endif

			msg_print(NULL);
			msg_format("%s", Rumor);
			msg_print(NULL);
#ifdef JP
msg_print("巻物は煙を立てて消え去った！");
#else
			msg_print("The scroll disappears in a puff of smoke!");
#endif

			ident = TRUE;
			break;
		}

		case SV_SCROLL_ARTIFACT:
		{
			ident = TRUE;
			if (!artifact_scroll()) used_up = FALSE;
			break;
		}

		case SV_SCROLL_RESET_RECALL:
		{
			ident = TRUE;
			if (!reset_recall()) used_up = FALSE;
			break;
		}
	}
	}
	else if (o_ptr->name1 == ART_GHB)
	{
#ifdef JP
		msg_print("私は苦労して『グレーター・ヘル=ビースト』を倒した。");
		msg_print("しかし手に入ったのはこのきたないＴシャツだけだった。");
#else
		msg_print("I had a very hard time to kill the Greater hell-beast, ");
		msg_print("but all I got was this lousy t-shirt!");
#endif
		used_up = FALSE;
	}
	else if (o_ptr->name1 == ART_POWER)
	{
#ifdef JP
		msg_print("「一つの指輪は全てを統べ、");
		msg_print(NULL);
		msg_print("一つの指輪は全てを見つけ、");
		msg_print(NULL);
		msg_print("一つの指輪は全てを捕らえて");
		msg_print(NULL);
		msg_print("暗闇の中に繋ぎとめる。」");
#else
		msg_print("'One Ring to rule them all, ");
		msg_print(NULL);
		msg_print("One Ring to find them, ");
		msg_print(NULL);
		msg_print("One Ring to bring them all ");
		msg_print(NULL);
		msg_print("and in the darkness bind them.'");
#endif
		used_up = FALSE;
	}
	else if (o_ptr->tval==TV_PARCHMENT)
	{
		cptr q;
		char o_name[MAX_NLEN];
		char buf[1024];

		/* Save screen */
		screen_save();

		q=format("book-%d_jp.txt",o_ptr->sval);

		/* Display object description */
		object_desc(o_name, o_ptr, OD_NAME_ONLY);

		/* Build the filename */
		path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, q);

		/* Peruse the help file */
		(void)show_file(TRUE, buf, o_name, 0, 0);

		/* Load screen */
		screen_load();

		used_up=FALSE;
	}


	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	if (!(object_is_aware(o_ptr)))
	{
		chg_virtue(V_PATIENCE, -1);
		chg_virtue(V_CHANCE, 1);
		chg_virtue(V_KNOWLEDGE, -1);
	}

	/* The item was tried */
	object_tried(o_ptr);

	/* An identification was made */
	if (ident && !object_is_aware(o_ptr))
	{
		object_aware(o_ptr);
		gain_exp((lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);


	/* Hack -- allow certain scrolls to be "preserved" */
	if (!used_up)
	{
		return;
	}

	sound(SOUND_SCROLL);

	/* Destroy a scroll in the pack */
	if (item >= 0)
	{
		inven_item_increase(item, -1);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Destroy a scroll on the floor */
	else
	{
		floor_item_increase(0 - item, -1);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}
}


/*
 * Hook to determine if an object is readable
 */
static bool item_tester_hook_readable(object_type *o_ptr)
{
	if ((o_ptr->tval==TV_SCROLL) || (o_ptr->tval==TV_PARCHMENT) || (o_ptr->name1 == ART_GHB) || (o_ptr->name1 == ART_POWER)) return (TRUE);

	/* Assume not */
	return (FALSE);
}


void do_cmd_read_scroll(void)
{
	object_type *o_ptr;
	int  item;
	cptr q, s;

	if (p_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
	{
		set_action(ACTION_NONE);
	}

	/* Check some conditions */
	if (p_ptr->blind)
	{
#ifdef JP
		msg_print("目が見えない。");
#else
		msg_print("You can't see anything.");
#endif

		return;
	}
	if (no_lite())
	{
#ifdef JP
		msg_print("明かりがないので、暗くて読めない。");
#else
		msg_print("You have no light to read by.");
#endif

		return;
	}
	if (p_ptr->confused)
	{
#ifdef JP
		msg_print("混乱していて読めない。");
#else
		msg_print("You are too confused!");
#endif

		return;
	}


	/* Restrict choices to scrolls */
	item_tester_hook = item_tester_hook_readable;

	/* Get an item */
#ifdef JP
	q = "どの巻物を読みますか? ";
	s = "読める巻物がない。";
#else
	q = "Read which scroll? ";
	s = "You have no scrolls to read.";
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

	/* Read the scroll */
	do_cmd_read_scroll_aux(item, object_is_aware(o_ptr));
}


static int staff_effect(int sval, bool *use_charge, bool magic, bool known)
{
	int k;
	int ident = FALSE;

	/* Analyze the staff */
	switch (sval)
	{
		case SV_STAFF_DARKNESS:
		{
			if (!(p_ptr->resist_blind) && !(p_ptr->resist_dark))
			{
				if (set_blind(p_ptr->blind + 3 + randint1(5))) ident = TRUE;
			}
			if (unlite_area(10, 3)) ident = TRUE;
			break;
		}

		case SV_STAFF_SLOWNESS:
		{
			if (set_slow(p_ptr->slow + randint1(30) + 15, FALSE)) ident = TRUE;
			break;
		}

		case SV_STAFF_HASTE_MONSTERS:
		{
			if (speed_monsters()) ident = TRUE;
			break;
		}

		case SV_STAFF_SUMMONING:
		{
			for (k = 0; k < randint1(4); k++)
			{
				if (summon_specific(0, py, px, dun_level, 0, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)))
				{
					ident = TRUE;
				}
			}
			break;
		}

		case SV_STAFF_TELEPORTATION:
		{
			teleport_player(100, 0L);
			ident = TRUE;
			break;
		}

		case SV_STAFF_IDENTIFY:
		{
			if (!ident_spell(FALSE)) *use_charge = FALSE;
			ident = TRUE;
			break;
		}

		case SV_STAFF_REMOVE_CURSE:
		{
			if (remove_curse())
			{
				if (magic)
				{
#ifdef JP
					msg_print("誰かに見守られているような気がする。");
#else
					msg_print("You feel as if someone is watching over you.");
#endif
				}
				else if (!p_ptr->blind)
				{
#ifdef JP
					msg_print("杖は一瞬ブルーに輝いた...");
#else
					msg_print("The staff glows blue for a moment...");
#endif

				}
				ident = TRUE;
			}
			break;
		}

		case SV_STAFF_STARLITE:
		{
			int num = damroll(5, 3);
			int y, x;
			int attempts;

			if (!p_ptr->blind && !magic)
			{
#ifdef JP
				msg_print("杖の先が明るく輝いた...");
#else
				msg_print("The end of the staff glows brightly...");
#endif

			}
			for (k = 0; k < num; k++)
			{
				attempts = 1000;

				while (attempts--)
				{
					scatter(&y, &x, py, px, 4, 0);

					if (!cave_have_flag_bold(y, x, FF_PROJECT)) continue;

					if (!player_bold(y, x)) break;
				}

				project(0, 0, y, x, damroll(6 + p_ptr->lev / 8, 10), GF_LITE_WEAK,
						  (PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_KILL), -1);
			}
			ident = TRUE;
			break;
		}

		case SV_STAFF_LITE:
		{
			if (lite_area(damroll(2, 8), 2)) ident = TRUE;
			break;
		}

		case SV_STAFF_MAPPING:
		{
			map_area(DETECT_RAD_MAP);
			ident = TRUE;
			break;
		}

		case SV_STAFF_DETECT_GOLD:
		{
			if (detect_treasure(DETECT_RAD_DEFAULT)) ident = TRUE;
			if (detect_objects_gold(DETECT_RAD_DEFAULT)) ident = TRUE;
			break;
		}

		case SV_STAFF_DETECT_ITEM:
		{
			if (detect_objects_normal(DETECT_RAD_DEFAULT)) ident = TRUE;
			break;
		}

		case SV_STAFF_DETECT_TRAP:
		{
			if (detect_traps(DETECT_RAD_DEFAULT, known)) ident = TRUE;
			break;
		}

		case SV_STAFF_DETECT_DOOR:
		{
			if (detect_doors(DETECT_RAD_DEFAULT)) ident = TRUE;
			if (detect_stairs(DETECT_RAD_DEFAULT)) ident = TRUE;
			break;
		}

		case SV_STAFF_DETECT_INVIS:
		{
			if (detect_monsters_invis(DETECT_RAD_DEFAULT)) ident = TRUE;
			break;
		}

		case SV_STAFF_DETECT_EVIL:
		{
			if (detect_monsters_evil(DETECT_RAD_DEFAULT)) ident = TRUE;
			break;
		}

		case SV_STAFF_CURE_LIGHT:
		{
			if (hp_player(damroll(2, 8))) ident = TRUE;
			if (set_shero(0,TRUE)) ident = TRUE;
			break;
		}

		case SV_STAFF_CURING:
		{
			if (set_blind(0)) ident = TRUE;
			if (set_poisoned(0)) ident = TRUE;
			if (set_confused(0)) ident = TRUE;
			if (set_stun(0)) ident = TRUE;
			if (set_cut(0)) ident = TRUE;
			if (set_image(0)) ident = TRUE;
			if (set_shero(0,TRUE)) ident = TRUE;
			break;
		}

		case SV_STAFF_HEALING:
		{
			if (hp_player(300)) ident = TRUE;
			if (set_stun(0)) ident = TRUE;
			if (set_cut(0)) ident = TRUE;
			if (set_shero(0,TRUE)) ident = TRUE;
			break;
		}

		case SV_STAFF_THE_MAGI:
		{
			if (do_res_stat(A_INT)) ident = TRUE;
			if (p_ptr->csp < p_ptr->msp)
			{
				p_ptr->csp = p_ptr->msp;
				p_ptr->csp_frac = 0;
				ident = TRUE;
#ifdef JP
				msg_print("頭がハッキリとした。");
#else
				msg_print("You feel your head clear.");
#endif

				p_ptr->redraw |= (PR_MANA);
				p_ptr->window |= (PW_PLAYER);
				p_ptr->window |= (PW_SPELL);
			}
			if (set_shero(0,TRUE)) ident = TRUE;
			break;
		}

		case SV_STAFF_SLEEP_MONSTERS:
		{
			if (sleep_monsters()) ident = TRUE;
			break;
		}

		case SV_STAFF_SLOW_MONSTERS:
		{
			if (slow_monsters()) ident = TRUE;
			break;
		}

		case SV_STAFF_SPEED:
		{
			if (set_fast(randint1(30) + 15, FALSE)) ident = TRUE;
			break;
		}

		case SV_STAFF_PROBING:
		{
			probing();
			ident = TRUE;
			break;
		}

		case SV_STAFF_DISPEL_EVIL:
		{
			if (dispel_evil(80)) ident = TRUE;
			break;
		}

		case SV_STAFF_POWER:
		{
			if (dispel_monsters(150)) ident = TRUE;
			break;
		}

		case SV_STAFF_HOLINESS:
		{
			if (dispel_evil(150)) ident = TRUE;
			k = 3 * p_ptr->lev;
			if (set_protevil((magic ? 0 : p_ptr->protevil) + randint1(25) + k, FALSE)) ident = TRUE;
			if (set_poisoned(0)) ident = TRUE;
			if (set_afraid(0)) ident = TRUE;
			if (hp_player(50)) ident = TRUE;
			if (set_stun(0)) ident = TRUE;
			if (set_cut(0)) ident = TRUE;
			break;
		}

		case SV_STAFF_GENOCIDE:
		{
			(void)symbol_genocide((magic ? p_ptr->lev + 50 : 200), TRUE);
			ident = TRUE;
			break;
		}

		case SV_STAFF_EARTHQUAKES:
		{
			if (earthquake(py, px, 10))
				ident = TRUE;
			else
#ifdef JP
msg_print("ダンジョンが揺れた。");
#else
				msg_print("The dungeon trembles.");
#endif


			break;
		}

		case SV_STAFF_DESTRUCTION:
		{
			if (destroy_area(py, px, 13 + randint0(5), FALSE))
				ident = TRUE;

			break;
		}

		case SV_STAFF_ANIMATE_DEAD:
		{
			if (animate_dead(0, py, px))
				ident = TRUE;

			break;
		}

		case SV_STAFF_MSTORM:
		{
#ifdef JP
			msg_print("強力な魔力が敵を引き裂いた！");
#else
			msg_print("Mighty magics rend your enemies!");
#endif
			project(0, 5, py, px,
				(randint1(200) + 300) * 2, GF_MANA, PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID, -1);
			if ((p_ptr->pclass != CLASS_MAGE) && (p_ptr->pclass != CLASS_HIGH_MAGE) && (p_ptr->pclass != CLASS_SORCERER) && (p_ptr->pclass != CLASS_MAGIC_EATER) && (p_ptr->pclass != CLASS_BLUE_MAGE))
			{
#ifdef JP
				(void)take_hit(DAMAGE_NOESCAPE, 50, "コントロールし難い強力な魔力の解放", -1);
#else
				(void)take_hit(DAMAGE_NOESCAPE, 50, "unleashing magics too mighty to control", -1);
#endif
			}
			ident = TRUE;

			break;
		}

		case SV_STAFF_NOTHING:
		{
#ifdef JP
			msg_print("何も起らなかった。");
#else
			msg_print("Nothing happen.");
#endif
			if (prace_is_(RACE_SKELETON) || prace_is_(RACE_GOLEM) ||
				prace_is_(RACE_ZOMBIE) || prace_is_(RACE_SPECTRE))
#ifdef JP
				msg_print("もったいない事をしたような気がする。食べ物は大切にしなくては。");
#else
				msg_print("What a waste.  It's your food!");
#endif
			break;
		}
	}
	return ident;
}

/*
 * Use a staff.			-RAK-
 *
 * One charge of one staff disappears.
 *
 * Hack -- staffs of identify can be "cancelled".
 */
static void do_cmd_use_staff_aux(int item)
{
	int         ident, chance, lev;
	object_type *o_ptr;


	/* Hack -- let staffs of identify get aborted */
	bool use_charge = TRUE;


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


	/* Mega-Hack -- refuse to use a pile from the ground */
	if ((item < 0) && (o_ptr->number > 1))
	{
#ifdef JP
		msg_print("まずは杖を拾わなければ。");
#else
		msg_print("You must first pick up the staffs.");
#endif

		return;
	}


	/* Take a turn */
	energy_use = 100;

	/* Extract the item level */
	lev = k_info[o_ptr->k_idx].level;
	if (lev > 50) lev = 50 + (lev - 50)/2;

	/* Base chance of success */
	chance = p_ptr->skill_dev;

	/* Confusion hurts skill */
	if (p_ptr->confused) chance = chance / 2;

	/* Hight level objects are harder */
	chance = chance - lev;

	/* Give everyone a (slight) chance */
	if ((chance < USE_DEVICE) && one_in_(USE_DEVICE - chance + 1))
	{
		chance = USE_DEVICE;
	}

	if (world_player)
	{
		if (flush_failure) flush();
#ifdef JP
		msg_print("止まった時の中ではうまく働かないようだ。");
#else
		msg_print("Nothing happen. Maybe this staff is freezing too.");
#endif

		sound(SOUND_FAIL);
		return;
	}

	/* Roll for usage */
	if ((chance < USE_DEVICE) || (randint1(chance) < USE_DEVICE) || (p_ptr->pclass == CLASS_BERSERKER))
	{
		if (flush_failure) flush();
#ifdef JP
		msg_print("杖をうまく使えなかった。");
#else
		msg_print("You failed to use the staff properly.");
#endif

		sound(SOUND_FAIL);
		return;
	}

	/* Notice empty staffs */
	if (o_ptr->pval <= 0)
	{
		if (flush_failure) flush();
#ifdef JP
		msg_print("この杖にはもう魔力が残っていない。");
#else
		msg_print("The staff has no charges left.");
#endif

		o_ptr->ident |= (IDENT_EMPTY);

		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);
		p_ptr->window |= (PW_INVEN);

		return;
	}


	/* Sound */
	sound(SOUND_ZAP);

	ident = staff_effect(o_ptr->sval, &use_charge, FALSE, object_is_aware(o_ptr));

	if (!(object_is_aware(o_ptr)))
	{
		chg_virtue(V_PATIENCE, -1);
		chg_virtue(V_CHANCE, 1);
		chg_virtue(V_KNOWLEDGE, -1);
	}

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Tried the item */
	object_tried(o_ptr);

	/* An identification was made */
	if (ident && !object_is_aware(o_ptr))
	{
		object_aware(o_ptr);
		gain_exp((lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);


	/* Hack -- some uses are "free" */
	if (!use_charge) return;


	/* Use a single charge */
	o_ptr->pval--;

	/* XXX Hack -- unstack if necessary */
	if ((item >= 0) && (o_ptr->number > 1))
	{
		object_type forge;
		object_type *q_ptr;

		/* Get local object */
		q_ptr = &forge;

		/* Obtain a local object */
		object_copy(q_ptr, o_ptr);

		/* Modify quantity */
		q_ptr->number = 1;

		/* Restore the charges */
		o_ptr->pval++;

		/* Unstack the used item */
		o_ptr->number--;
		p_ptr->total_weight -= q_ptr->weight;
		item = inven_carry(q_ptr);

		/* Message */
#ifdef JP
		msg_print("杖をまとめなおした。");
#else
		msg_print("You unstack your staff.");
#endif

	}

	/* Describe charges in the pack */
	if (item >= 0)
	{
		inven_item_charges(item);
	}

	/* Describe charges on the floor */
	else
	{
		floor_item_charges(0 - item);
	}
}


void do_cmd_use_staff(void)
{
	int  item;
	cptr q, s;

	if (p_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
	{
		set_action(ACTION_NONE);
	}

	/* Restrict choices to wands */
	item_tester_tval = TV_STAFF;

	/* Get an item */
#ifdef JP
	q = "どの杖を使いますか? ";
	s = "使える杖がない。";
#else
	q = "Use which staff? ";
	s = "You have no staff to use.";
#endif

	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;

	do_cmd_use_staff_aux(item);
}


static int wand_effect(int sval, int dir, bool magic)
{
	int ident = FALSE;

	/* XXX Hack -- Wand of wonder can do anything before it */
	if (sval == SV_WAND_WONDER)
	{
		int vir = virtue_number(V_CHANCE);
		sval = randint0(SV_WAND_WONDER);

		if (vir)
		{
			if (p_ptr->virtues[vir - 1] > 0)
			{
				while (randint1(300) < p_ptr->virtues[vir - 1]) sval++;
				if (sval > SV_WAND_COLD_BALL) sval = randint0(4) + SV_WAND_ACID_BALL;
			}
			else
			{
				while (randint1(300) < (0-p_ptr->virtues[vir - 1])) sval--;
				if (sval < SV_WAND_HEAL_MONSTER) sval = randint0(3) + SV_WAND_HEAL_MONSTER;
			}
		}
		if (sval < SV_WAND_TELEPORT_AWAY)
			chg_virtue(V_CHANCE, 1);
	}

	/* Analyze the wand */
	switch (sval)
	{
		case SV_WAND_HEAL_MONSTER:
		{
			if (heal_monster(dir, damroll(10, 10))) ident = TRUE;
			break;
		}

		case SV_WAND_HASTE_MONSTER:
		{
			if (speed_monster(dir)) ident = TRUE;
			break;
		}

		case SV_WAND_CLONE_MONSTER:
		{
			if (clone_monster(dir)) ident = TRUE;
			break;
		}

		case SV_WAND_TELEPORT_AWAY:
		{
			if (teleport_monster(dir)) ident = TRUE;
			break;
		}

		case SV_WAND_DISARMING:
		{
			if (disarm_trap(dir)) ident = TRUE;
			break;
		}

		case SV_WAND_TRAP_DOOR_DEST:
		{
			if (destroy_door(dir)) ident = TRUE;
			break;
		}

		case SV_WAND_STONE_TO_MUD:
		{
			if (wall_to_mud(dir)) ident = TRUE;
			break;
		}

		case SV_WAND_LITE:
		{
#ifdef JP
			msg_print("青く輝く光線が放たれた。");
#else
			msg_print("A line of blue shimmering light appears.");
#endif

			(void)lite_line(dir);
			ident = TRUE;
			break;
		}

		case SV_WAND_SLEEP_MONSTER:
		{
			if (sleep_monster(dir)) ident = TRUE;
			break;
		}

		case SV_WAND_SLOW_MONSTER:
		{
			if (slow_monster(dir)) ident = TRUE;
			break;
		}

		case SV_WAND_CONFUSE_MONSTER:
		{
			if (confuse_monster(dir, p_ptr->lev)) ident = TRUE;
			break;
		}

		case SV_WAND_FEAR_MONSTER:
		{
			if (fear_monster(dir, p_ptr->lev)) ident = TRUE;
			break;
		}

		case SV_WAND_DRAIN_LIFE:
		{
			if (drain_life(dir, 80 + p_ptr->lev)) ident = TRUE;
			break;
		}

		case SV_WAND_POLYMORPH:
		{
			if (poly_monster(dir)) ident = TRUE;
			break;
		}

		case SV_WAND_STINKING_CLOUD:
		{
			fire_ball(GF_POIS, dir, 12 + p_ptr->lev / 4, 2);
			ident = TRUE;
			break;
		}

		case SV_WAND_MAGIC_MISSILE:
		{
			fire_bolt_or_beam(20, GF_MISSILE, dir, damroll(2 + p_ptr->lev / 10, 6));
			ident = TRUE;
			break;
		}

		case SV_WAND_ACID_BOLT:
		{
			fire_bolt_or_beam(20, GF_ACID, dir, damroll(6 + p_ptr->lev / 7, 8));
			ident = TRUE;
			break;
		}

		case SV_WAND_CHARM_MONSTER:
		{
			if (charm_monster(dir, MAX(20, p_ptr->lev)))
			ident = TRUE;
			break;
		}

		case SV_WAND_FIRE_BOLT:
		{
			fire_bolt_or_beam(20, GF_FIRE, dir, damroll(7 + p_ptr->lev / 6, 8));
			ident = TRUE;
			break;
		}

		case SV_WAND_COLD_BOLT:
		{
			fire_bolt_or_beam(20, GF_COLD, dir, damroll(5 + p_ptr->lev / 8, 8));
			ident = TRUE;
			break;
		}

		case SV_WAND_ACID_BALL:
		{
			fire_ball(GF_ACID, dir, 60 + 3 * p_ptr->lev / 4, 2);
			ident = TRUE;
			break;
		}

		case SV_WAND_ELEC_BALL:
		{
			fire_ball(GF_ELEC, dir, 40 + 3 * p_ptr->lev / 4, 2);
			ident = TRUE;
			break;
		}

		case SV_WAND_FIRE_BALL:
		{
			fire_ball(GF_FIRE, dir, 70 + 3 * p_ptr->lev / 4, 2);
			ident = TRUE;
			break;
		}

		case SV_WAND_COLD_BALL:
		{
			fire_ball(GF_COLD, dir, 50 + 3 * p_ptr->lev / 4, 2);
			ident = TRUE;
			break;
		}

		case SV_WAND_WONDER:
		{
#ifdef JP
			msg_print("おっと、謎の魔法棒を始動させた。");
#else
			msg_print("Oops.  Wand of wonder activated.");
#endif

			break;
		}

		case SV_WAND_DRAGON_FIRE:
		{
			fire_ball(GF_FIRE, dir, 200, -3);
			ident = TRUE;
			break;
		}

		case SV_WAND_DRAGON_COLD:
		{
			fire_ball(GF_COLD, dir, 180, -3);
			ident = TRUE;
			break;
		}

		case SV_WAND_DRAGON_BREATH:
		{
			switch (randint1(5))
			{
				case 1:
				{
					fire_ball(GF_ACID, dir, 240, -3);
					break;
				}

				case 2:
				{
					fire_ball(GF_ELEC, dir, 210, -3);
					break;
				}

				case 3:
				{
					fire_ball(GF_FIRE, dir, 240, -3);
					break;
				}

				case 4:
				{
					fire_ball(GF_COLD, dir, 210, -3);
					break;
				}

				default:
				{
					fire_ball(GF_POIS, dir, 180, -3);
					break;
				}
			}

			ident = TRUE;
			break;
		}

		case SV_WAND_DISINTEGRATE:
		{
			fire_ball(GF_DISINTEGRATE, dir, 200 + randint1(p_ptr->lev * 2), 2);
			ident = TRUE;
			break;
		}

		case SV_WAND_ROCKETS:
		{
#ifdef JP
msg_print("ロケットを発射した！");
#else
			msg_print("You launch a rocket!");
#endif

			fire_rocket(GF_ROCKET, dir, 250 + p_ptr->lev * 3, 2);
			ident = TRUE;
			break;
		}

		case SV_WAND_STRIKING:
		{
			fire_bolt(GF_METEOR, dir, damroll(15 + p_ptr->lev / 3, 13));
			ident = TRUE;
			break;
		}

		case SV_WAND_GENOCIDE:
		{
			fire_ball_hide(GF_GENOCIDE, dir, magic ? p_ptr->lev + 50 : 250, 0);
			ident = TRUE;
			break;
		}
	}
	return ident;
}


/*
 * Aim a wand (from the pack or floor).
 *
 * Use a single charge from a single item.
 * Handle "unstacking" in a logical manner.
 *
 * For simplicity, you cannot use a stack of items from the
 * ground.  This would require too much nasty code.
 *
 * There are no wands which can "destroy" themselves, in the inventory
 * or on the ground, so we can ignore this possibility.  Note that this
 * required giving "wand of wonder" the ability to ignore destruction
 * by electric balls.
 *
 * All wands can be "cancelled" at the "Direction?" prompt for free.
 *
 * Note that the basic "bolt" wands do slightly less damage than the
 * basic "bolt" rods, but the basic "ball" wands do the same damage
 * as the basic "ball" rods.
 */
static void do_cmd_aim_wand_aux(int item)
{
	int         lev, ident, chance, dir;
	object_type *o_ptr;
	bool old_target_pet = target_pet;

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

	/* Mega-Hack -- refuse to aim a pile from the ground */
	if ((item < 0) && (o_ptr->number > 1))
	{
#ifdef JP
		msg_print("まずは魔法棒を拾わなければ。");
#else
		msg_print("You must first pick up the wands.");
#endif

		return;
	}


	/* Allow direction to be cancelled for free */
	if (object_is_aware(o_ptr) && (o_ptr->sval == SV_WAND_HEAL_MONSTER
				      || o_ptr->sval == SV_WAND_HASTE_MONSTER))
			target_pet = TRUE;
	if (!get_aim_dir(&dir))
	{
		target_pet = old_target_pet;
		return;
	}
	target_pet = old_target_pet;

	/* Take a turn */
	energy_use = 100;

	/* Get the level */
	lev = k_info[o_ptr->k_idx].level;
	if (lev > 50) lev = 50 + (lev - 50)/2;

	/* Base chance of success */
	chance = p_ptr->skill_dev;

	/* Confusion hurts skill */
	if (p_ptr->confused) chance = chance / 2;

	/* Hight level objects are harder */
	chance = chance - lev;

	/* Give everyone a (slight) chance */
	if ((chance < USE_DEVICE) && one_in_(USE_DEVICE - chance + 1))
	{
		chance = USE_DEVICE;
	}

	if (world_player)
	{
		if (flush_failure) flush();
#ifdef JP
		msg_print("止まった時の中ではうまく働かないようだ。");
#else
		msg_print("Nothing happen. Maybe this wand is freezing too.");
#endif

		sound(SOUND_FAIL);
		return;
	}

	/* Roll for usage */
	if ((chance < USE_DEVICE) || (randint1(chance) < USE_DEVICE) || (p_ptr->pclass == CLASS_BERSERKER))
	{
		if (flush_failure) flush();
#ifdef JP
		msg_print("魔法棒をうまく使えなかった。");
#else
		msg_print("You failed to use the wand properly.");
#endif

		sound(SOUND_FAIL);
		return;
	}

	/* The wand is already empty! */
	if (o_ptr->pval <= 0)
	{
		if (flush_failure) flush();
#ifdef JP
		msg_print("この魔法棒にはもう魔力が残っていない。");
#else
		msg_print("The wand has no charges left.");
#endif

		o_ptr->ident |= (IDENT_EMPTY);

		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);
		p_ptr->window |= (PW_INVEN);

		return;
	}

	/* Sound */
	sound(SOUND_ZAP);

	ident = wand_effect(o_ptr->sval, dir, FALSE);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	if (!(object_is_aware(o_ptr)))
	{
		chg_virtue(V_PATIENCE, -1);
		chg_virtue(V_CHANCE, 1);
		chg_virtue(V_KNOWLEDGE, -1);
	}

	/* Mark it as tried */
	object_tried(o_ptr);

	/* Apply identification */
	if (ident && !object_is_aware(o_ptr))
	{
		object_aware(o_ptr);
		gain_exp((lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);


	/* Use a single charge */
	o_ptr->pval--;

	/* Describe the charges in the pack */
	if (item >= 0)
	{
		inven_item_charges(item);
	}

	/* Describe the charges on the floor */
	else
	{
		floor_item_charges(0 - item);
	}
}


void do_cmd_aim_wand(void)
{
	int     item;
	cptr    q, s;

	/* Restrict choices to wands */
	item_tester_tval = TV_WAND;

	if (p_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
	{
		set_action(ACTION_NONE);
	}

	/* Get an item */
#ifdef JP
	q = "どの魔法棒で狙いますか? ";
	s = "使える魔法棒がない。";
#else
	q = "Aim which wand? ";
	s = "You have no wand to aim.";
#endif

	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;

	/* Aim the wand */
	do_cmd_aim_wand_aux(item);
}


static int rod_effect(int sval, int dir, bool *use_charge, bool magic)
{
	int ident = FALSE;

	/* Unused */
	(void)magic;

	/* Analyze the rod */
	switch (sval)
	{
		case SV_ROD_DETECT_TRAP:
		{
			if (detect_traps(DETECT_RAD_DEFAULT, (bool)(dir ? FALSE : TRUE))) ident = TRUE;
			break;
		}

		case SV_ROD_DETECT_DOOR:
		{
			if (detect_doors(DETECT_RAD_DEFAULT)) ident = TRUE;
			if (detect_stairs(DETECT_RAD_DEFAULT)) ident = TRUE;
			break;
		}

		case SV_ROD_IDENTIFY:
		{
			if (!ident_spell(FALSE)) *use_charge = FALSE;
			ident = TRUE;
			break;
		}

		case SV_ROD_RECALL:
		{
			if (!word_of_recall()) *use_charge = FALSE;
			ident = TRUE;
			break;
		}

		case SV_ROD_ILLUMINATION:
		{
			if (lite_area(damroll(2, 8), 2)) ident = TRUE;
			break;
		}

		case SV_ROD_MAPPING:
		{
			map_area(DETECT_RAD_MAP);
			ident = TRUE;
			break;
		}

		case SV_ROD_DETECTION:
		{
			detect_all(DETECT_RAD_DEFAULT);
			ident = TRUE;
			break;
		}

		case SV_ROD_PROBING:
		{
			probing();
			ident = TRUE;
			break;
		}

		case SV_ROD_CURING:
		{
			if (set_blind(0)) ident = TRUE;
			if (set_poisoned(0)) ident = TRUE;
			if (set_confused(0)) ident = TRUE;
			if (set_stun(0)) ident = TRUE;
			if (set_cut(0)) ident = TRUE;
			if (set_image(0)) ident = TRUE;
			if (set_shero(0,TRUE)) ident = TRUE;
			break;
		}

		case SV_ROD_HEALING:
		{
			if (hp_player(500)) ident = TRUE;
			if (set_stun(0)) ident = TRUE;
			if (set_cut(0)) ident = TRUE;
			if (set_shero(0,TRUE)) ident = TRUE;
			break;
		}

		case SV_ROD_RESTORATION:
		{
			if (restore_level()) ident = TRUE;
			if (do_res_stat(A_STR)) ident = TRUE;
			if (do_res_stat(A_INT)) ident = TRUE;
			if (do_res_stat(A_WIS)) ident = TRUE;
			if (do_res_stat(A_DEX)) ident = TRUE;
			if (do_res_stat(A_CON)) ident = TRUE;
			if (do_res_stat(A_CHR)) ident = TRUE;
			break;
		}

		case SV_ROD_SPEED:
		{
			if (set_fast(randint1(30) + 15, FALSE)) ident = TRUE;
			break;
		}

		case SV_ROD_PESTICIDE:
		{
			if (dispel_monsters(4)) ident = TRUE;
			break;
		}

		case SV_ROD_TELEPORT_AWAY:
		{
			if (teleport_monster(dir)) ident = TRUE;
			break;
		}

		case SV_ROD_DISARMING:
		{
			if (disarm_trap(dir)) ident = TRUE;
			break;
		}

		case SV_ROD_LITE:
		{
#ifdef JP
			msg_print("青く輝く光線が放たれた。");
#else
			msg_print("A line of blue shimmering light appears.");
#endif

			(void)lite_line(dir);
			ident = TRUE;
			break;
		}

		case SV_ROD_SLEEP_MONSTER:
		{
			if (sleep_monster(dir)) ident = TRUE;
			break;
		}

		case SV_ROD_SLOW_MONSTER:
		{
			if (slow_monster(dir)) ident = TRUE;
			break;
		}

		case SV_ROD_DRAIN_LIFE:
		{
			if (drain_life(dir, 70 + 3 * p_ptr->lev / 2)) ident = TRUE;
			break;
		}

		case SV_ROD_POLYMORPH:
		{
			if (poly_monster(dir)) ident = TRUE;
			break;
		}

		case SV_ROD_ACID_BOLT:
		{
			fire_bolt_or_beam(10, GF_ACID, dir, damroll(6 + p_ptr->lev / 7, 8));
			ident = TRUE;
			break;
		}

		case SV_ROD_ELEC_BOLT:
		{
			fire_bolt_or_beam(10, GF_ELEC, dir, damroll(4 + p_ptr->lev / 9, 8));
			ident = TRUE;
			break;
		}

		case SV_ROD_FIRE_BOLT:
		{
			fire_bolt_or_beam(10, GF_FIRE, dir, damroll(7 + p_ptr->lev / 6, 8));
			ident = TRUE;
			break;
		}

		case SV_ROD_COLD_BOLT:
		{
			fire_bolt_or_beam(10, GF_COLD, dir, damroll(5 + p_ptr->lev / 8, 8));
			ident = TRUE;
			break;
		}

		case SV_ROD_ACID_BALL:
		{
			fire_ball(GF_ACID, dir, 60 + p_ptr->lev, 2);
			ident = TRUE;
			break;
		}

		case SV_ROD_ELEC_BALL:
		{
			fire_ball(GF_ELEC, dir, 40 + p_ptr->lev, 2);
			ident = TRUE;
			break;
		}

		case SV_ROD_FIRE_BALL:
		{
			fire_ball(GF_FIRE, dir, 70 + p_ptr->lev, 2);
			ident = TRUE;
			break;
		}

		case SV_ROD_COLD_BALL:
		{
			fire_ball(GF_COLD, dir, 50 + p_ptr->lev, 2);
			ident = TRUE;
			break;
		}

		case SV_ROD_HAVOC:
		{
			call_chaos();
			ident = TRUE;
			break;
		}

		case SV_ROD_STONE_TO_MUD:
		{
			if (wall_to_mud(dir)) ident = TRUE;
			break;
		}

		case SV_ROD_AGGRAVATE:
		{
			aggravate_monsters(0);
			ident = TRUE;
			break;
		}
	}
	return ident;
}

/*
 * Activate (zap) a Rod
 *
 * Unstack fully charged rods as needed.
 *
 * Hack -- rods of perception/genocide can be "cancelled"
 * All rods can be cancelled at the "Direction?" prompt
 *
 * pvals are defined for each rod in k_info. -LM-
 */
static void do_cmd_zap_rod_aux(int item)
{
	int ident, chance, lev, fail;
	int dir = 0;
	object_type *o_ptr;
	bool success;

	/* Hack -- let perception get aborted */
	bool use_charge = TRUE;

	object_kind *k_ptr;

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


	/* Mega-Hack -- refuse to zap a pile from the ground */
	if ((item < 0) && (o_ptr->number > 1))
	{
#ifdef JP
		msg_print("まずはロッドを拾わなければ。");
#else
		msg_print("You must first pick up the rods.");
#endif

		return;
	}


	/* Get a direction (unless KNOWN not to need it) */
	if (((o_ptr->sval >= SV_ROD_MIN_DIRECTION) && (o_ptr->sval != SV_ROD_HAVOC) && (o_ptr->sval != SV_ROD_AGGRAVATE) && (o_ptr->sval != SV_ROD_PESTICIDE)) ||
	     !object_is_aware(o_ptr))
	{
		/* Get a direction, allow cancel */
		if (!get_aim_dir(&dir)) return;
	}


	/* Take a turn */
	energy_use = 100;

	/* Extract the item level */
	lev = k_info[o_ptr->k_idx].level;

	/* Base chance of success */
	chance = p_ptr->skill_dev;

	/* Confusion hurts skill */
	if (p_ptr->confused) chance = chance / 2;

	fail = lev+5;
	if (chance > fail) fail -= (chance - fail)*2;
	else chance -= (fail - chance)*2;
	if (fail < USE_DEVICE) fail = USE_DEVICE;
	if (chance < USE_DEVICE) chance = USE_DEVICE;

	if (world_player)
	{
		if (flush_failure) flush();
#ifdef JP
		msg_print("止まった時の中ではうまく働かないようだ。");
#else
		msg_print("Nothing happen. Maybe this rod is freezing too.");
#endif

		sound(SOUND_FAIL);
		return;
	}

	if (p_ptr->pclass == CLASS_BERSERKER) success = FALSE;
	else if (chance > fail)
	{
		if (randint0(chance*2) < fail) success = FALSE;
		else success = TRUE;
	}
	else
	{
		if (randint0(fail*2) < chance) success = TRUE;
		else success = FALSE;
	}

	/* Roll for usage */
	if (!success)
	{
		if (flush_failure) flush();
#ifdef JP
		msg_print("うまくロッドを使えなかった。");
#else
		msg_print("You failed to use the rod properly.");
#endif

		sound(SOUND_FAIL);
		return;
	}

	k_ptr = &k_info[o_ptr->k_idx];

	/* A single rod is still charging */
	if ((o_ptr->number == 1) && (o_ptr->timeout))
	{
		if (flush_failure) flush();
#ifdef JP
		msg_print("このロッドはまだ魔力を充填している最中だ。");
#else
		msg_print("The rod is still charging.");
#endif

		return;
	}
	/* A stack of rods lacks enough energy. */
	else if ((o_ptr->number > 1) && (o_ptr->timeout > k_ptr->pval * (o_ptr->number - 1)))
	{
		if (flush_failure) flush();
#ifdef JP
msg_print("そのロッドはまだ充填中です。");
#else
		msg_print("The rods are all still charging.");
#endif

		return;
	}

	/* Sound */
	sound(SOUND_ZAP);

	ident = rod_effect(o_ptr->sval, dir, &use_charge, FALSE);

	/* Increase the timeout by the rod kind's pval. -LM- */
	if (use_charge) o_ptr->timeout += k_ptr->pval;

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	if (!(object_is_aware(o_ptr)))
	{
		chg_virtue(V_PATIENCE, -1);
		chg_virtue(V_CHANCE, 1);
		chg_virtue(V_KNOWLEDGE, -1);
	}

	/* Tried the object */
	object_tried(o_ptr);

	/* Successfully determined the object function */
	if (ident && !object_is_aware(o_ptr))
	{
		object_aware(o_ptr);
		gain_exp((lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
}


void do_cmd_zap_rod(void)
{
	int item;
	cptr q, s;

	if (p_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
	{
		set_action(ACTION_NONE);
	}

	/* Restrict choices to rods */
	item_tester_tval = TV_ROD;

	/* Get an item */
#ifdef JP
	q = "どのロッドを振りますか? ";
	s = "使えるロッドがない。";
#else
	q = "Zap which rod? ";
	s = "You have no rod to zap.";
#endif

	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;

	/* Zap the rod */
	do_cmd_zap_rod_aux(item);
}


/*
 * Hook to determine if an object is activatable
 */
static bool item_tester_hook_activate(object_type *o_ptr)
{
	u32b flgs[TR_FLAG_SIZE];

	/* Not known */
	if (!object_is_known(o_ptr)) return (FALSE);

	/* Extract the flags */
	object_flags(o_ptr, flgs);

	/* Check activation flag */
	if (have_flag(flgs, TR_ACTIVATE)) return (TRUE);

	/* Assume not */
	return (FALSE);
}


/*
 * Hack -- activate the ring of power
 */
void ring_of_power(int dir)
{
	/* Pick a random effect */
	switch (randint1(10))
	{
		case 1:
		case 2:
		{
			/* Message */
#ifdef JP
			msg_print("あなたは悪性のオーラに包み込まれた。");
#else
			msg_print("You are surrounded by a malignant aura.");
#endif

			sound(SOUND_EVIL);

			/* Decrease all stats (permanently) */
			(void)dec_stat(A_STR, 50, TRUE);
			(void)dec_stat(A_INT, 50, TRUE);
			(void)dec_stat(A_WIS, 50, TRUE);
			(void)dec_stat(A_DEX, 50, TRUE);
			(void)dec_stat(A_CON, 50, TRUE);
			(void)dec_stat(A_CHR, 50, TRUE);

			/* Lose some experience (permanently) */
			p_ptr->exp -= (p_ptr->exp / 4);
			p_ptr->max_exp -= (p_ptr->exp / 4);
			check_experience();

			break;
		}

		case 3:
		{
			/* Message */
#ifdef JP
			msg_print("あなたは強力なオーラに包み込まれた。");
#else
			msg_print("You are surrounded by a powerful aura.");
#endif


			/* Dispel monsters */
			dispel_monsters(1000);

			break;
		}

		case 4:
		case 5:
		case 6:
		{
			/* Mana Ball */
			fire_ball(GF_MANA, dir, 600, 3);

			break;
		}

		case 7:
		case 8:
		case 9:
		case 10:
		{
			/* Mana Bolt */
			fire_bolt(GF_MANA, dir, 500);

			break;
		}
	}
}


static bool ang_sort_comp_pet(vptr u, vptr v, int a, int b)
{
	u16b *who = (u16b*)(u);

	int w1 = who[a];
	int w2 = who[b];

	monster_type *m_ptr1 = &m_list[w1];
	monster_type *m_ptr2 = &m_list[w2];
	monster_race *r_ptr1 = &r_info[m_ptr1->r_idx];
	monster_race *r_ptr2 = &r_info[m_ptr2->r_idx];

	/* Unused */
	(void)v;

	if (m_ptr1->nickname && !m_ptr2->nickname) return TRUE;
	if (m_ptr2->nickname && !m_ptr1->nickname) return FALSE;

	if ((r_ptr1->flags1 & RF1_UNIQUE) && !(r_ptr2->flags1 & RF1_UNIQUE)) return TRUE;
	if ((r_ptr2->flags1 & RF1_UNIQUE) && !(r_ptr1->flags1 & RF1_UNIQUE)) return FALSE;

	if (r_ptr1->level > r_ptr2->level) return TRUE;
	if (r_ptr2->level > r_ptr1->level) return FALSE;

	if (m_ptr1->hp > m_ptr2->hp) return TRUE;
	if (m_ptr2->hp > m_ptr1->hp) return FALSE;
	
	return w1 <= w2;
}

/*
 * Activate a wielded object.  Wielded objects never stack.
 * And even if they did, activatable objects never stack.
 *
 * Currently, only (some) artifacts, and Dragon Scale Mail, can be activated.
 * But one could, for example, easily make an activatable "Ring of Plasma".
 *
 * Note that it always takes a turn to activate an artifact, even if
 * the user hits "escape" at the "direction" prompt.
 */
static void do_cmd_activate_aux(int item)
{
	int         k, dir, lev, chance, fail;
	object_type *o_ptr;
	bool success;


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

	/* Take a turn */
	energy_use = 100;

	/* Extract the item level */
	lev = k_info[o_ptr->k_idx].level;

	/* Hack -- use artifact level instead */
	if (object_is_fixed_artifact(o_ptr)) lev = a_info[o_ptr->name1].level;
	else if (o_ptr->art_name)
	{
		switch (o_ptr->xtra2)
		{
			case ACT_SUNLIGHT:
			case ACT_BO_MISS_1:
			case ACT_BA_POIS_1:
			case ACT_CONFUSE:
			case ACT_SLEEP:
			case ACT_CURE_LW:
			case ACT_CURE_POISON:
			case ACT_BERSERK:
			case ACT_LIGHT:
			case ACT_DEST_DOOR:
			case ACT_TELEPORT:
				lev = 10;
				break;
			case ACT_BO_ELEC_1:
			case ACT_BO_ACID_1:
			case ACT_BO_COLD_1:
			case ACT_BO_FIRE_1:
			case ACT_MAP_LIGHT:
			case ACT_STONE_MUD:
			case ACT_CURE_MW:
			case ACT_QUAKE:
				lev = 20;
				break;
			case ACT_DRAIN_1:
			case ACT_TELE_AWAY:
			case ACT_ESP:
			case ACT_RESIST_ALL:
			case ACT_DETECT_ALL:
			case ACT_RECALL:
			case ACT_SATIATE:
			case ACT_RECHARGE:
				lev = 30;
				break;
			case ACT_BA_COLD_1:
			case ACT_BA_FIRE_1:
			case ACT_TERROR:
			case ACT_PROT_EVIL:
			case ACT_ID_PLAIN:
			case ACT_REST_LIFE:
			case ACT_SPEED:
			case ACT_BANISH_EVIL:
				lev = 40;
				break;
			case ACT_DRAIN_2:
			case ACT_VAMPIRE_1:
			case ACT_BO_MISS_2:
			case ACT_BA_FIRE_2:
			case ACT_WHIRLWIND:
			case ACT_CHARM_ANIMAL:
			case ACT_SUMMON_ANIMAL:
			case ACT_DISP_EVIL:
			case ACT_DISP_GOOD:
			case ACT_XTRA_SPEED:
			case ACT_DETECT_XTRA:
			case ACT_ID_FULL:
				lev = 50;
				break;
			case ACT_VAMPIRE_2:
			case ACT_BA_COLD_3:
			case ACT_BA_ELEC_3:
			case ACT_GENOCIDE:
			case ACT_CHARM_UNDEAD:
			case ACT_CHARM_OTHER:
			case ACT_SUMMON_PHANTOM:
			case ACT_SUMMON_ELEMENTAL:
			case ACT_RUNE_EXPLO:
				lev = 60;
				break;
			case ACT_MASS_GENO:
			case ACT_CHARM_ANIMALS:
			case ACT_CHARM_OTHERS:
			case ACT_CURE_700:
			case ACT_RUNE_PROT:
			case ACT_ALCHEMY:
			case ACT_REST_ALL:
				lev = 70;
				break;
			case ACT_CALL_CHAOS:
			case ACT_ROCKET:
			case ACT_BA_MISS_3:
			case ACT_CURE_1000:
			case ACT_DIM_DOOR:
			case ACT_SUMMON_UNDEAD:
			case ACT_SUMMON_DEMON:
				lev = 80;
				break;
			case ACT_WRAITH:
			case ACT_INVULN:
				lev = 100;
				break;
			default:
				lev = 0;
		}
	}
	else if (((o_ptr->tval == TV_RING) || (o_ptr->tval == TV_AMULET)) && o_ptr->name2) lev = e_info[o_ptr->name2].level;

	/* Base chance of success */
	chance = p_ptr->skill_dev;

	/* Confusion hurts skill */
	if (p_ptr->confused) chance = chance / 2;

	fail = lev+5;
	if (chance > fail) fail -= (chance - fail)*2;
	else chance -= (fail - chance)*2;
	if (fail < USE_DEVICE) fail = USE_DEVICE;
	if (chance < USE_DEVICE) chance = USE_DEVICE;

	if (world_player)
	{
		if (flush_failure) flush();
#ifdef JP
		msg_print("止まった時の中ではうまく働かないようだ。");
#else
		msg_print("It shows no reaction.");
#endif

		sound(SOUND_FAIL);
		return;
	}

	if (p_ptr->pclass == CLASS_BERSERKER) success = FALSE;
	else if (chance > fail)
	{
		if (randint0(chance*2) < fail) success = FALSE;
		else success = TRUE;
	}
	else
	{
		if (randint0(fail*2) < chance) success = TRUE;
		else success = FALSE;
	}

	/* Roll for usage */
	if (!success)
	{
		if (flush_failure) flush();
#ifdef JP
		msg_print("うまく始動させることができなかった。");
#else
		msg_print("You failed to activate it properly.");
#endif

		sound(SOUND_FAIL);
		return;
	}

	/* Check the recharge */
	if (o_ptr->timeout)
	{
#ifdef JP
		msg_print("それは微かに音を立て、輝き、消えた...");
#else
		msg_print("It whines, glows and fades...");
#endif

		return;
	}


	/* Activate the artifact */
#ifdef JP
	msg_print("始動させた...");
#else
	msg_print("You activate it...");
#endif


	/* Sound */
	sound(SOUND_ZAP);


	if (o_ptr->art_name && o_ptr->xtra2)
	{
		(void)activate_random_artifact(o_ptr);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Success */
		return;
	}

	/* Artifacts */
	else if (object_is_fixed_artifact(o_ptr))
	{
		/* Choose effect */
		switch (o_ptr->name1)
		{
			case ART_GALADRIEL:
			{
#ifdef JP
				msg_print("玻璃瓶から澄んだ光があふれ出た...");
#else
				msg_print("The phial wells with clear light...");
#endif

				lite_area(damroll(2, 15), 3);
				o_ptr->timeout = randint0(10) + 10;
				break;
			}

			case ART_ELENDIL:
			{
#ifdef JP
				msg_print("星が眩しく輝いた...");
#else
				msg_print("The star shines brightly...");
#endif

				map_area(DETECT_RAD_MAP);
				lite_area(damroll(2, 15), 3);
				o_ptr->timeout = randint0(50) + 50;
				break;
			}

			case ART_JUDGE:
			{
#ifdef JP
msg_print("その宝石は赤く明るく光った！");
#else
				msg_print("The Jewel flashes bright red!");
#endif

				chg_virtue(V_KNOWLEDGE, 1);
				chg_virtue(V_ENLIGHTEN, 1);
				wiz_lite(FALSE);
#ifdef JP
msg_print("その宝石はあなたの体力を奪った...");
take_hit(DAMAGE_LOSELIFE, damroll(3,8), "審判の宝石", -1);
#else
				msg_print("The Jewel drains your vitality...");
				take_hit(DAMAGE_LOSELIFE, damroll(3, 8), "the Jewel of Judgement", -1);
#endif

				(void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
				(void)detect_doors(DETECT_RAD_DEFAULT);
				(void)detect_stairs(DETECT_RAD_DEFAULT);

#ifdef JP
if (get_check("帰還の力を使いますか？"))
#else
				if (get_check("Activate recall? "))
#endif

				{
					(void)word_of_recall();
				}

				o_ptr->timeout = randint0(20) + 20;
				break;
			}

			case ART_CARLAMMAS:
			{
#ifdef JP
				msg_print("アミュレットから鋭い音が流れ出た...");
#else
				msg_print("The amulet lets out a shrill wail...");
#endif

				k = 3 * p_ptr->lev;
				(void)set_protevil(randint1(25) + k, FALSE);
				o_ptr->timeout = randint0(225) + 225;
				break;
			}

			case ART_INGWE:
			{
#ifdef JP
				msg_print("アミュレットは辺りを善のオーラで満たした...");
#else
				msg_print("The amulet floods the area with goodness...");
#endif

				dispel_evil(p_ptr->lev * 5);
				o_ptr->timeout = randint0(200) + 200;
				break;
			}

			case ART_YATA:
			{
#ifdef JP
				msg_print("鏡は辺りを善のオーラで満たした...");
#else
				msg_print("The mirror floods the area with goodness...");
#endif

				dispel_evil(p_ptr->lev * 5);
				o_ptr->timeout = randint0(200) + 200;
				break;
			}

			case ART_FRAKIR:
			{
#ifdef JP
msg_print("あなたはフラキアに敵を締め殺すよう命じた。");
#else
				msg_print("You order Frakir to strangle your opponent.");
#endif

				if (!get_aim_dir(&dir)) return;
				if (drain_life(dir, 100))
				o_ptr->timeout = randint0(100) + 100;
				break;
			}

			case ART_TULKAS:
			{
#ifdef JP
				msg_print("指輪は明るく輝いた...");
#else
				msg_print("The ring glows brightly...");
#endif

				(void)set_fast(randint1(75) + 75, FALSE);
				o_ptr->timeout = randint0(150) + 150;
				break;
			}

			case ART_NARYA:
			{
#ifdef JP
				msg_print("指輪は深紅に輝いた...");
#else
				msg_print("The ring glows deep red...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_FIRE, dir, 300, 3);
				o_ptr->timeout = randint0(225) + 225;
				break;
			}

			case ART_NENYA:
			{
#ifdef JP
				msg_print("指輪は白く明るく輝いた...");
#else
				msg_print("The ring glows bright white...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_COLD, dir, 400, 3);
				o_ptr->timeout = randint0(325) + 325;
				break;
			}

			case ART_VILYA:
			case ART_GOURYU:
			{
#ifdef JP
				msg_format("%sは深いブルーに輝いた...", o_ptr->name1 == ART_VILYA ? "指輪" : "ソード");
#else
				msg_format("The %s glows deep blue...", o_ptr->name1 == ART_VILYA ? "ring" : "sword");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_ELEC, dir, 500, 3);
				o_ptr->timeout = randint0(425) + 425;
				break;
			}

			case ART_POWER:
			case ART_AHO:
			{
#ifdef JP
				msg_print("指輪は漆黒に輝いた...");
#else
				msg_print("The ring glows intensely black...");
#endif

				if (!get_aim_dir(&dir)) return;
				ring_of_power(dir);
				o_ptr->timeout = randint0(450) + 450;
				break;
			}

			case ART_RAZORBACK:
			{
				int num = damroll(5, 3);
				int y, x;
				int attempts;

#ifdef JP
				msg_print("鎧が稲妻で覆われた...");
#else
				msg_print("Your armor is surrounded by lightning...");
#endif


				for (k = 0; k < num; k++)
				{
					attempts = 1000;

					while (attempts--)
					{
						scatter(&y, &x, py, px, 4, 0);

						if (!cave_have_flag_bold(y, x, FF_PROJECT)) continue;

						if (!player_bold(y, x)) break;
					}

					project(0, 3, y, x, 150, GF_ELEC,
							  (PROJECT_THRU | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL), -1);
				}

				o_ptr->timeout = 1000;
				break;
			}

			case ART_BLADETURNER:
			{
				if (!get_aim_dir(&dir)) return;
#ifdef JP
				msg_print("あなたはエレメントのブレスを吐いた。");
#else
				msg_print("You breathe the elements.");
#endif

				fire_ball(GF_MISSILE, dir, 300, 4);
#ifdef JP
				msg_print("鎧が様々な色に輝いた...");
#else
				msg_print("Your armor glows many colours...");
#endif

				(void)set_afraid(0);
				(void)set_hero(randint1(50) + 50, FALSE);
				(void)hp_player(10);
				(void)set_blessed(randint1(50) + 50, FALSE);
				(void)set_oppose_acid(randint1(50) + 50, FALSE);
				(void)set_oppose_elec(randint1(50) + 50, FALSE);
				(void)set_oppose_fire(randint1(50) + 50, FALSE);
				(void)set_oppose_cold(randint1(50) + 50, FALSE);
				(void)set_oppose_pois(randint1(50) + 50, FALSE);
				o_ptr->timeout = 400;
				break;
			}

			case ART_SOULKEEPER:
			{
#ifdef JP
				msg_print("鎧が白く明るく輝いた...");
				msg_print("ひじょうに気分がよい...");
#else
				msg_print("Your armor glows a bright white...");
				msg_print("You feel much better...");
#endif

				(void)hp_player(1000);
				(void)set_cut(0);
				o_ptr->timeout = 888;
				break;
			}

			case ART_LOHENGRIN:
			{
#ifdef JP
msg_print("天国の歌が聞こえる...");
#else
				msg_print("A heavenly choir sings...");
#endif

				(void)set_poisoned(0);
				(void)set_cut(0);
				(void)set_stun(0);
				(void)set_confused(0);
				(void)set_blind(0);
				(void)set_afraid(0);
				(void)set_hero(randint1(25) + 25, FALSE);
				(void)hp_player(777);
				o_ptr->timeout = 300;
				break;
			}

			case ART_JULIAN:
			{
#ifdef JP
				msg_print("鎧が深いブルーに輝いた...");
#else
				msg_print("Your armor glows deep blue...");
#endif

				(void)symbol_genocide(200, TRUE);
				o_ptr->timeout = 500;
				break;
			}

			case ART_CASPANION:
			{
#ifdef JP
				msg_print("鎧が赤く明るく輝いた...");
#else
				msg_print("Your armor glows bright red...");
#endif

				destroy_doors_touch();
				o_ptr->timeout = 10;
				break;
			}

			case ART_DOR:
			case ART_TERROR:
			case ART_STONEMASK:
			{
				turn_monsters(40 + p_ptr->lev);
				o_ptr->timeout = 3 * (p_ptr->lev + 10);

				break;
			}

			case ART_HOLHENNETH:
			{
#ifdef JP
				msg_print("ヘルメットが白く明るく輝いた...");
				msg_print("心にイメージが浮かんできた...");
#else
				msg_print("Your helm glows bright white...");
				msg_print("An image forms in your mind...");
#endif

				detect_all(DETECT_RAD_DEFAULT);
				o_ptr->timeout = randint0(55) + 55;
				break;
			}

			case ART_AMBER:
			{
#ifdef JP
				msg_print("王冠が深いブルーに輝いた...");
				msg_print("体内に暖かい鼓動が感じられる...");
#else
				msg_print("Your crown glows deep blue...");
				msg_print("You feel a warm tingling inside...");
#endif

				(void)hp_player(700);
				(void)set_cut(0);
				o_ptr->timeout = 250;
				break;
			}

			case ART_COLLUIN:
			case ART_SEIRYU:
			{
#ifdef JP
				msg_format("%sが様々な色に輝いた...", o_ptr->name1 == ART_COLLUIN ? "クローク" : "鎧");
#else
				msg_format("Your %s glows many colours...", o_ptr->name1 == ART_COLLUIN ? "cloak" : "armor");
#endif

				(void)set_oppose_acid(randint1(20) + 20, FALSE);
				(void)set_oppose_elec(randint1(20) + 20, FALSE);
				(void)set_oppose_fire(randint1(20) + 20, FALSE);
				(void)set_oppose_cold(randint1(20) + 20, FALSE);
				(void)set_oppose_pois(randint1(20) + 20, FALSE);
				o_ptr->timeout = 111;
				break;
			}

			case ART_HOLCOLLETH:
			{
#ifdef JP
				msg_print("クロークが深いブルーに輝いた...");
#else
				msg_print("Your cloak glows deep blue...");
#endif

				sleep_monsters_touch();
				o_ptr->timeout = 55;
				break;
			}

			case ART_THINGOL:
			{
#ifdef JP
				msg_print("クロークが黄色く明るく輝いた...");
#else
				msg_print("Your cloak glows bright yellow...");
#endif

				recharge(130);
				o_ptr->timeout = 70;
				break;
			}

			case ART_COLANNON:
			{
#ifdef JP
				msg_print("クロークが辺りの空間をゆがませた...");
#else
				msg_print("Your cloak twists space around you...");
#endif

				teleport_player(100, 0L);
				o_ptr->timeout = 45;
				break;
			}

			case ART_LUTHIEN:
			{
#ifdef JP
				msg_print("クロークが深紅に輝いた...");
#else
				msg_print("Your cloak glows a deep red...");
#endif

				restore_level();
				o_ptr->timeout = 450;
				break;
			}

			case ART_HEAVENLY_MAIDEN:
			{
#ifdef JP
				msg_print("クロークが柔らかく白く輝いた...");
#else
				msg_print("Your cloak glows soft white...");
#endif
				if (!word_of_recall()) return;
				o_ptr->timeout = 200;
				break;
			}

			case ART_CAMMITHRIM:
			{
#ifdef JP
				msg_print("グローブが眩しいくらいに明るく輝いた...");
#else
				msg_print("Your gloves glow extremely brightly...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_bolt(GF_MISSILE, dir, damroll(2, 6));
				o_ptr->timeout = 2;
				break;
			}

			case ART_PAURHACH:
			{
#ifdef JP
				msg_print("ガントレットが炎に覆われた...");
#else
				msg_print("Your gauntlets are covered in fire...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_bolt(GF_FIRE, dir, damroll(9, 8));
				o_ptr->timeout = randint0(8) + 8;
				break;
			}

			case ART_PAURNIMMEN:
			{
#ifdef JP
				msg_print("ガントレットが冷気に覆われた...");
#else
				msg_print("Your gauntlets are covered in frost...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_bolt(GF_COLD, dir, damroll(6, 8));
				o_ptr->timeout = randint0(7) + 7;
				break;
			}

			case ART_PAURAEGEN:
			{
#ifdef JP
				msg_print("ガントレットが火花に覆われた...");
#else
				msg_print("Your gauntlets are covered in sparks...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_bolt(GF_ELEC, dir, damroll(4, 8));
				o_ptr->timeout = randint0(5) + 5;
				break;
			}

			case ART_PAURNEN:
			{
#ifdef JP
				msg_print("ガントレットが酸に覆われた...");
#else
				msg_print("Your gauntlets are covered in acid...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_bolt(GF_ACID, dir, damroll(5, 8));
				o_ptr->timeout = randint0(6) + 6;
				break;
			}

			case ART_FINGOLFIN:
			{
#ifdef JP
				msg_print("セスタスに魔法のトゲが現れた...");
#else
				msg_print("Your cesti grows magical spikes...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_bolt(GF_ARROW, dir, 150);
				o_ptr->timeout = randint0(90) + 90;
				break;
			}

			case ART_FEANOR:
			{
#ifdef JP
				msg_print("ブーツがグリーンに明るく輝いた...");
#else
				msg_print("Your boots glow bright green...");
#endif

				(void)set_fast(randint1(20) + 20, FALSE);
				o_ptr->timeout = 200;
				break;
			}

			case ART_FLORA:
			{
#ifdef JP
				msg_print("ブーツが深いブルーに輝いた...");
#else
				msg_print("Your boots glow deep blue...");
#endif

				(void)set_afraid(0);
				(void)set_poisoned(0);
				o_ptr->timeout = 5;
				break;
			}

			case ART_NARTHANC:
			{
#ifdef JP
				msg_print("ダガーが炎に覆われた...");
#else
				msg_print("Your dagger is covered in fire...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_bolt(GF_FIRE, dir, damroll(9, 8));
				o_ptr->timeout = randint0(8) + 8;
				break;
			}

			case ART_NIMTHANC:
			{
#ifdef JP
				msg_print("ダガーが冷気に覆われた...");
#else
				msg_print("Your dagger is covered in frost...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_bolt(GF_COLD, dir, damroll(6, 8));
				o_ptr->timeout = randint0(7) + 7;
				break;
			}

			case ART_DETHANC:
			{
#ifdef JP
				msg_print("ダガーが火花に覆われた...");
#else
				msg_print("Your dagger is covered in sparks...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_bolt(GF_ELEC, dir, damroll(4, 8));
				o_ptr->timeout = randint0(5) + 5;
				break;
			}

			case ART_RILIA:
			{
#ifdef JP
				msg_print("ダガーが深い緑色に鼓動している...");
#else
				msg_print("Your dagger throbs deep green...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_POIS, dir, 12, 3);
				o_ptr->timeout = randint0(4) + 4;
				break;
			}

			case ART_NUMAHOKO:
			{
#ifdef JP
				msg_print("矛が深い青色に鼓動している...");
#else
				msg_print("Your dagger throbs deep blue...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_WATER, dir, 200, 3);
				o_ptr->timeout = 250;
				break;
			}

			case ART_FIONA:
			{
#ifdef JP
				msg_print("ダガーが冷気に覆われた...");
#else
				msg_print("Your dagger is covered in frost...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_COLD, dir, 48, 2);
				o_ptr->timeout = randint0(5) + 5;
				break;
			}

			case ART_KUSANAGI:
			case ART_WEREWINDLE:
			{
				switch (randint1(13))
				{
				case 1: case 2: case 3: case 4: case 5:
					teleport_player(10, 0L);
					break;
				case 6: case 7: case 8: case 9: case 10:
					teleport_player(222, 0L);
					break;
				case 11: case 12:
					(void)stair_creation();
					break;
				default:
#ifdef JP
if (get_check("この階を去りますか？"))
#else
					if (get_check("Leave this level? "))
#endif

					{
						if (autosave_l) do_cmd_save_game(TRUE);

						/* Leaving */
						p_ptr->leaving = TRUE;
					}
				}
				o_ptr->timeout = 35;
				break;
			}

			case ART_KAMUI:
			{
				teleport_player(222, 0L);
				o_ptr->timeout = 25;
				break;
			}

			case ART_RINGIL:
			{
#ifdef JP
				msg_print("ソードが青く激しく輝いた...");
#else
				msg_print("Your sword glows an intense blue...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_COLD, dir, 100, 2);
				o_ptr->timeout = 200;
				break;
			}

			case ART_DAWN:
			{
#ifdef JP
msg_print("暁の師団を召喚した。");
#else
				msg_print("You summon the Legion of the Dawn.");
#endif

				(void)summon_specific(-1, py, px, dun_level, SUMMON_DAWN, (PM_ALLOW_GROUP | PM_FORCE_PET));
				o_ptr->timeout = 500 + randint1(500);
				break;
			}

			case ART_ANDURIL:
			{
#ifdef JP
				msg_print("ソードが赤く激しく輝いた...");
#else
				msg_print("Your sword glows an intense red...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_FIRE, dir, 72, 2);
				o_ptr->timeout = 400;
				break;
			}

			case ART_THEODEN:
			{
#ifdef JP
				 msg_print("アックスの刃が黒く輝いた...");
#else
				msg_print("Your axe blade glows black...");
#endif

				if (!get_aim_dir(&dir)) return;
				drain_life(dir, 120);
				o_ptr->timeout = 400;
				break;
			}

			case ART_RUNESPEAR:
			{
#ifdef JP
msg_print("あなたの槍は電気でスパークしている...");
#else
				msg_print("Your spear crackles with electricity...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_ELEC, dir, 100, 3);
				o_ptr->timeout = 200;
				break;
			}

			case ART_AEGLOS:
			{
#ifdef JP
				msg_print("スピアが白く明るく輝いた...");
#else
				msg_print("Your spear glows a bright white...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_COLD, dir, 100, 3);
				o_ptr->timeout = 200;
				break;
			}

			case ART_DESTINY:
			{
#ifdef JP
				msg_print("スピアが鼓動した...");
#else
				msg_print("Your spear pulsates...");
#endif

				if (!get_aim_dir(&dir)) return;
				wall_to_mud(dir);
				o_ptr->timeout = 5;
				break;
			}

			case ART_NAIN:
			{
#ifdef JP
				msg_print("つるはしが鼓動した...");
#else
				msg_print("Your mattock pulsates...");
#endif

				if (!get_aim_dir(&dir)) return;
				wall_to_mud(dir);
				o_ptr->timeout = 2;
				break;
			}

			case ART_EONWE:
			{
#ifdef JP
				msg_print("アックスからひどく鋭い音が流れ出た...");
#else
				msg_print("Your axe lets out a long, shrill note...");
#endif

				(void)mass_genocide(200, TRUE);
				o_ptr->timeout = 1000;
				break;
			}

			case ART_LOTHARANG:
			{
#ifdef JP
				msg_print("バトル・アックスが深紫の光を放射した...");
#else
				msg_print("Your battle axe radiates deep purple...");
#endif

				hp_player(damroll(4, 8));
				(void)set_cut((p_ptr->cut / 2) - 50);
				o_ptr->timeout = randint0(3) + 3;
				break;
			}

			case ART_ULMO:
			{
#ifdef JP
				msg_print("トライデントが深紅に輝いた...");
#else
				msg_print("Your trident glows deep red...");
#endif

				if (!get_aim_dir(&dir)) return;
				teleport_monster(dir);
				o_ptr->timeout = 150;
				break;
			}

			case ART_AVAVIR:
			{
#ifdef JP
				msg_print("大鎌が柔らかく白く輝いた...");
#else
				msg_print("Your scythe glows soft white...");
#endif
				if (!word_of_recall()) return;
				o_ptr->timeout = 200;
				break;
			}

			case ART_MAGATAMA:
			{
#ifdef JP
				msg_print("勾玉が柔らかく白く輝いた...");
#else
				msg_print("Your scythe glows soft white...");
#endif
				if (!word_of_recall()) return;
				o_ptr->timeout = 200;
				break;
			}

			case ART_TOTILA:
			{
#ifdef JP
				msg_print("フレイルが様々な色の火花を発した...");
#else
				msg_print("Your flail glows in scintillating colours...");
#endif

				if (!get_aim_dir(&dir)) return;
				confuse_monster(dir, 20);
				o_ptr->timeout = 15;
				break;
			}

			case ART_FIRESTAR:
			{
#ifdef JP
				msg_print("モーニングスターから炎が吹き出した...");
#else
				msg_print("Your morning star rages in fire...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_FIRE, dir, 72, 3);
				o_ptr->timeout = 100;
				break;
			}

			case ART_GOTHMOG:
			{
#ifdef JP
				msg_print("ムチが深い赤色に輝いた...");
#else
				msg_print("Your whip glows deep red...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_FIRE, dir, 120, 3);
				o_ptr->timeout = 15;
				break;
			}

			case ART_TARATOL:
			{
#ifdef JP
				msg_print("メイスがグリーンに明るく輝いた...");
#else
				msg_print("Your mace glows bright green...");
#endif

				(void)set_fast(randint1(20) + 20, FALSE);
				o_ptr->timeout = randint0(100) + 100;
				break;
			}

			case ART_ERIRIL:
			{
#ifdef JP
				msg_print("クォータースタッフが黄色く輝いた...");
#else
				msg_print("Your quarterstaff glows yellow...");
#endif

				if (!ident_spell(FALSE)) return;
				o_ptr->timeout = 10;
				break;
			}

			case ART_GANDALF:
			{
#ifdef JP
				msg_print("杖が明るく輝いた...");
#else
				msg_print("Your quarterstaff glows brightly...");
#endif

				detect_all(DETECT_RAD_DEFAULT);
				probing();
				identify_fully(FALSE);
				o_ptr->timeout = 100;
				break;
			}

			case ART_TURMIL:
			{
#ifdef JP
				msg_print("ハンマーが白く輝いた...");
#else
				msg_print("Your hammer glows white...");
#endif

				if (!get_aim_dir(&dir)) return;
				drain_life(dir, 90);
				o_ptr->timeout = 70;
				break;
			}

			case ART_BRAND:
			case ART_HELLFIRE:
			{
#ifdef JP
				msg_print("クロスボウが深紅に輝いた...");
#else
				msg_print("Your crossbow glows deep red...");
#endif

				(void)brand_bolts();
				o_ptr->timeout = 999;
				break;
			}
			case ART_CRIMSON:
			{
				int num = 1;
				int i;
				int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
				int tx, ty;
#ifdef JP
				msg_print("せっかくだから『クリムゾン』をぶっぱなすぜ！");
#else
				msg_print("I'll fire CRIMSON! SEKKAKUDAKARA!");
#endif

				if (!get_aim_dir(&dir)) return;

				/* Use the given direction */
				tx = px + 99 * ddx[dir];
				ty = py + 99 * ddy[dir];

				/* Hack -- Use an actual "target" */
				if ((dir == 5) && target_okay())
				{
					tx = target_col;
					ty = target_row;
				}

				if (p_ptr->pclass == CLASS_ARCHER)
				{
					/* Extra shot at level 10 */
					if (p_ptr->lev >= 10) num++;

					/* Extra shot at level 30 */
					if (p_ptr->lev >= 30) num++;

					/* Extra shot at level 45 */
					if (p_ptr->lev >= 45) num++;
				}

				for (i = 0; i < num; i++)
					project(0, p_ptr->lev/20+1, ty, tx, p_ptr->lev*p_ptr->lev*6/50, GF_ROCKET, flg, -1);
				o_ptr->timeout = 15;
				break;
			}
			case ART_PALANTIR:
			{
				monster_type *m_ptr;
				monster_race *r_ptr;
				int i;

#ifdef JP
				msg_print("奇妙な場所が頭の中に浮かんだ．．．");
#else
				msg_print("Some strange places show up in your mind. And you see ...");
#endif

				/* Process the monsters (backwards) */
				for (i = m_max - 1; i >= 1; i--)
				{
					/* Access the monster */
					m_ptr = &m_list[i];

					/* Ignore "dead" monsters */
					if (!m_ptr->r_idx) continue;

					r_ptr = &r_info[m_ptr->r_idx];

					if(r_ptr->flags1 & RF1_UNIQUE)
					{
#ifdef JP
						msg_format("%s． ",r_name + r_ptr->name);
#else
						msg_format("%s. ",r_name + r_ptr->name);
#endif
					}
				}
				o_ptr->timeout = 200;
				break;
			}

			case ART_STONE_LORE:
			{
#ifdef JP
				msg_print("石が隠された秘密を写し出した．．．");
#else
				msg_print("The stone reveals hidden mysteries...");
#endif
				if (!ident_spell(FALSE)) return;

				if (mp_ptr->spell_book)
				{
					/* Sufficient mana */
					if (20 <= p_ptr->csp)
					{
						/* Use some mana */
						p_ptr->csp -= 20;
					}

					/* Over-exert the player */
					else
					{
						int oops = 20 - p_ptr->csp;

						/* No mana left */
						p_ptr->csp = 0;
						p_ptr->csp_frac = 0;

						/* Message */
#ifdef JP
						msg_print("石を制御できない！");
#else
						msg_print("You are too weak to control the stone!");
#endif

						/* Hack -- Bypass free action */
						(void)set_paralyzed(p_ptr->paralyzed +
							randint1(5 * oops + 1));

						/* Confusing. */
						(void)set_confused(p_ptr->confused +
							randint1(5 * oops + 1));
					}

					/* Redraw mana */
					p_ptr->redraw |= (PR_MANA);
				}

#ifdef JP
				take_hit(DAMAGE_LOSELIFE, damroll(1, 12), "危険な秘密", -1);
#else
				take_hit(DAMAGE_LOSELIFE, damroll(1, 12), "perilous secrets", -1);
#endif

				/* Confusing. */
				if (one_in_(5)) (void)set_confused(p_ptr->confused +
					randint1(10));

				/* Exercise a little care... */
				if (one_in_(20))
#ifdef JP
					take_hit(DAMAGE_LOSELIFE, damroll(4, 10), "危険な秘密", -1);
#else
					take_hit(DAMAGE_LOSELIFE, damroll(4, 10), "perilous secrets", -1);
#endif
				o_ptr->timeout = 0;
				break;
			}

			case ART_BOROMIR:
			{
				if (music_singing_any()) stop_singing();
				if (hex_spelling_any()) stop_hex_spell_all();
#ifdef JP
				msg_print("あなたは力強い突風を吹き鳴らした。周囲の敵が震え上っている!");
#else
				msg_print("You wind a mighty blast; your enemies tremble!");
#endif
				(void)turn_monsters((3 * p_ptr->lev / 2) + 10);
				o_ptr->timeout = randint0(40) + 40;
				break;
			}
			case ART_FARAMIR:
			{
#ifdef JP
				msg_print("あなたは害虫を一掃した。");
#else
				msg_print("You exterminate small life.");
#endif
				(void)dispel_monsters(4);
				o_ptr->timeout = randint0(55) + 55;
				break;
			}

			case ART_HIMRING:
			{
#ifdef JP
				msg_print("鈍い音が辺りを包みこんだ。");
#else
				msg_print("A shrill wailing sound surrounds you.");
#endif
				(void)set_protevil(randint1(25) + p_ptr->lev, FALSE);
				o_ptr->timeout = randint0(200) + 200;
				break;
			}

			case ART_ICANUS:
			{

#ifdef JP
				msg_print("ローブが純粋な魔力で震えた。");
#else
				msg_print("The robe pulsates with raw mana...");
#endif
				if (!get_aim_dir(&dir)) return;
				fire_bolt(GF_MANA, dir, 120);
				o_ptr->timeout = randint0(120) + 120;
				break;
			}
			case ART_HURIN:
			{
				(void)set_fast(randint1(50) + 50, FALSE);
				hp_player(10);
				set_afraid(0);
				set_hero(randint1(50) + 50, FALSE);
				o_ptr->timeout = randint0(200) + 100;
				break;
			}
			case ART_GIL_GALAD:
			{
#ifdef JP
				msg_print("シールドが眩しい光で輝いた．．．");
#else
				msg_print("Your shield gleams with blinding light...");
#endif
				fire_ball(GF_LITE, 0, 300, 6);
				confuse_monsters(3 * p_ptr->lev / 2);
				o_ptr->timeout = 250;
				break;
			}
			case ART_YENDOR:
			{
#ifdef JP
				msg_print("カードが白く輝いた．．．");
#else
				msg_print("Your card gleams with blinding light...");
#endif
				if (!recharge(1000)) return;
				o_ptr->timeout = 200;
				break;
			}
			case ART_MURAMASA:
			{
#ifdef JP
				if (get_check("本当に使いますか？"))
#else
				if (get_check("Are you sure?!"))
#endif
				{
#ifdef JP
					msg_print("村正が震えた．．．");
#else
					msg_print("The Muramasa pulsates...");
#endif
					do_inc_stat(A_STR);
					if (one_in_(2))
					{
#ifdef JP
						msg_print("村正は壊れた！");
#else
						msg_print("The Muramasa is destroyed!");
#endif
						curse_weapon(TRUE, item);
					}
				}
				break;
			}
			case ART_FLY_STONE:
			{
#ifdef JP
				msg_print("石が青白く光った．．．");
#else
				msg_print("Your stone glows pale...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_MANA, dir, 400, 4);
				o_ptr->timeout = randint0(250) + 250;
				break;
			}
			case ART_TAIKOBO:
			{
				int x, y;

				if (!get_rep_dir2(&dir)) return;
				y = py+ddy[dir];
				x = px+ddx[dir];
				tsuri_dir = dir;
				if (!cave_have_flag_bold(y, x, FF_WATER))
				{
#ifdef JP
					msg_print("そこは水辺ではない。");
#else
					msg_print("There is no fishing place.");
#endif
					return;
				}
				else if (cave[y][x].m_idx)
				{
					char m_name[80];
					monster_desc(m_name, &m_list[cave[y][x].m_idx], 0);
#ifdef JP
					msg_format("%sが邪魔だ！", m_name);
#else
					msg_format("%^s is stand in your way.", m_name);
#endif
					energy_use = 0;
					return;
				}
				set_action(ACTION_FISH);
				p_ptr->redraw |= (PR_STATE);
				break;
			}
			case ART_JONES:
			{
				if (!get_aim_dir(&dir)) return;
#ifdef JP
				msg_print("ムチを伸ばした。");
#else
				msg_print("You stretched your whip.");
#endif

				fetch(dir, 500, TRUE);
				o_ptr->timeout = randint0(25) + 25;
				break;
			}
			case ART_ARRYU:
			{
				u32b mode = PM_ALLOW_GROUP;
				bool pet = !one_in_(5);
				if (pet) mode |= PM_FORCE_PET;
				else mode |= PM_NO_PET;

				if (summon_specific((pet ? -1 : 0), py, px, ((p_ptr->lev * 3) / 2), SUMMON_HOUND, mode))
				{

					if (pet)
#ifdef JP
						msg_print("ハウンドがあなたの下僕として出現した。");
#else
					msg_print("A group of hounds appear as your servant.");
#endif

					else
#ifdef JP
						msg_print("ハウンドはあなたに牙を向けている！");
#else
						msg_print("A group of hounds appear as your enemy!");
#endif

				}

				o_ptr->timeout = 300 + randint1(150);
				break;
			}

			case ART_GAEBOLG:
			{
#ifdef JP
				msg_print("スピアは眩しく輝いた...");
#else
				msg_print("Your spear grows brightly...");
#endif

				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_LITE, dir, 200, 3);
				o_ptr->timeout = randint0(200) + 200;
				break;
			}

			case ART_INROU:
			{
				int count = 0, i;
				monster_type *m_ptr;
#ifndef JP
				cptr kakusan = "";
#endif

				if (summon_named_creature(0, py, px, MON_SUKE, PM_FORCE_PET))
				{
#ifdef JP
					msg_print("『助さん』が現れた。");
#else
					msg_print("Suke-san apperars.");
					kakusan = "Suke-san";
#endif
					count++;
				}
				if (summon_named_creature(0, py, px, MON_KAKU, PM_FORCE_PET))
				{
#ifdef JP
					msg_print("『格さん』が現れた。");
#else
					msg_print("Kaku-san appears.");
					kakusan = "Kaku-san";
#endif
					count++;
				}
				if (!count)
				{
					for (i = m_max - 1; i > 0; i--)
					{
						m_ptr = &m_list[i];
						if (!m_ptr->r_idx) continue;
						if (!((m_ptr->r_idx == MON_SUKE) || (m_ptr->r_idx == MON_KAKU))) continue;
						if (!los(m_ptr->fy, m_ptr->fx, py, px)) continue;
						if (!projectable(m_ptr->fy, m_ptr->fx, py, px)) continue;
						count++;
						break;
					}
				}

				if (count)
				{
#ifdef JP
					msg_print("「者ども、ひかえおろう！！！このお方をどなたとこころえる。」");
#else
					msg_format("%^s says 'WHO do you think this person is! Bow your head, down your knees!'", kakusan);
#endif

					sukekaku = TRUE;
					stun_monsters(120);
					confuse_monsters(120);
					turn_monsters(120);
					stasis_monsters(120);
					sukekaku = FALSE;
				}
				else
				{
#ifdef JP
					msg_print("しかし、何も起きなかった。");
#else
					msg_print("Nothing happen.");
#endif
				}
				o_ptr->timeout = randint0(150) + 150;
				break;
			}

			case ART_HYOUSIGI:
			{
#ifdef JP
				msg_print("拍子木を打った。");
#else
				msg_print("You beat Your wooden clappers.");
#endif
				aggravate_monsters(0);
				break;
			}

			case ART_MATOI:
			case ART_AEGISFANG:
			{
				(void)set_afraid(0);
				set_hero(randint1(25)+25, FALSE);
				hp_player(10);
				o_ptr->timeout = randint0(30) + 30;
				break;
			}

			case ART_EARENDIL:
			{
				(void)set_poisoned(0);
				(void)set_confused(0);
				(void)set_blind(0);
				(void)set_stun(0);
				(void)set_cut(0);
				(void)set_image(0);

				o_ptr->timeout = 100;
				break;
			}

			case ART_BOLISHOI:
			{
				if (!get_aim_dir(&dir)) return;
				(void)charm_animal(dir, p_ptr->lev);

				o_ptr->timeout = 200;
				break;
			}

			case ART_ARUNRUTH:
			{
#ifdef JP
				msg_print("ソードが淡いブルーに輝いた...");
#else
				msg_print("Your sword glows a pale blue...");
#endif
				if (!get_aim_dir(&dir)) return;
				fire_bolt(GF_COLD, dir, damroll(12, 8));
				o_ptr->timeout = 50;
				break;
			}
			case ART_BLOOD:
			{
#ifdef JP
				msg_print("鎌が明るく輝いた...");
#else
				msg_print("Your scythe glows brightly!");
#endif
				get_bloody_moon_flags(o_ptr);
				o_ptr->timeout = 3333;
				if (p_ptr->prace == RACE_ANDROID) calc_android_exp();
				p_ptr->update |= (PU_BONUS | PU_HP);
				break;
			}
			case ART_KESHO:
			{
#ifdef JP
				msg_print("力強く四股を踏んだ。");
#else
				msg_print("You stamp. (as if you are in a ring.)");
#endif
				(void)set_afraid(0);
				(void)set_hero(randint1(20) + 20, FALSE);
				dispel_evil(p_ptr->lev * 3);
				o_ptr->timeout = 100 + randint1(100);
				break;
			}
			case ART_MOOK:
			{
#ifdef JP
				msg_print("クロークが白く輝いた...");
#else
				msg_print("Your cloak grows white.");
#endif
				(void)set_oppose_cold(randint1(20) + 20, FALSE);
				o_ptr->timeout = 40 + randint1(40);
				break;
			}
			case ART_HERMIT:
			{
#ifdef JP
				msg_print("ムチから鋭い音が流れ出た...");
#else
				msg_print("The whip lets out a shrill wail...");
#endif

				k = 3 * p_ptr->lev;
				(void)set_protevil(randint1(25) + k, FALSE);
				o_ptr->timeout = randint0(225) + 225;
				break;
			}
			case ART_JIZO:
			{
				u32b mode = PM_ALLOW_GROUP;
				bool pet = !one_in_(5);
				if (pet) mode |= PM_FORCE_PET;

				if (summon_named_creature(0, py, px, MON_JIZOTAKO, mode))
				{
					if (pet)
#ifdef JP
						msg_print("蛸があなたの下僕として出現した。");
#else
					msg_print("A group of octopuses appear as your servant.");
#endif

					else
#ifdef JP
						msg_print("蛸はあなたを睨んでいる！");
#else
						msg_print("A group of octopuses appear as your enemy!");
#endif

				}

				o_ptr->timeout = 300 + randint1(150);
				break;
			}

			case ART_FUNDIN:
			{
#ifdef JP
				msg_print("鉄球は辺りを善のオーラで満たした...");
#else
				msg_print("The iron ball floods the area with goodness...");
#endif

				dispel_evil(p_ptr->lev * 5);
				o_ptr->timeout = randint0(100) + 100;
				break;
			}

			case ART_AESCULAPIUS:
			{
#ifdef JP
				msg_print("六尺棒は濃緑色に輝いている...");
#else
				msg_print("The jo staff glows a deep green...");
#endif

				(void)do_res_stat(A_STR);
				(void)do_res_stat(A_INT);
				(void)do_res_stat(A_WIS);
				(void)do_res_stat(A_DEX);
				(void)do_res_stat(A_CON);
				(void)do_res_stat(A_CHR);
				(void)restore_level();
				o_ptr->timeout = 750;
				break;
			}

			case ART_NIGHT:
			{
#ifdef JP
				msg_print("アミュレットが深い闇に覆われた...");
#else
				msg_print("Your amulet is coverd in pitch-darkness...");
#endif
				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_DARK, dir, 250, 4);
				o_ptr->timeout = randint0(150) + 150;
				break;
			}
			case ART_HELL:
			{
#ifdef JP
				msg_print("首輪が深い闇に覆われた...");
#else
				msg_print("Your collar harness is coverd in pitch-darkness...");
#endif
				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_DARK, dir, 250, 4);
				o_ptr->timeout = randint0(150) + 150;
				break;
			}
			case ART_SACRED_KNIGHTS:
			{
#ifdef JP
				msg_print("首飾りが真実を照らし出す...");
#else
				msg_print("Your amulet exhibits the truth...");
#endif
				if (remove_all_curse())
				{
#ifdef JP
					msg_print("誰かに見守られているような気がする。");
#else
					msg_print("You feel as if someone is watching over you.");
#endif
				}
				(void)probing();
				break;
			}
			case ART_CHARMED:
			{
#ifdef JP
				msg_print("ペンダントが青白く光った．．．");
#else
				msg_print("Your pendant glows pale...");
#endif
				if (p_ptr->pclass == CLASS_MAGIC_EATER)
				{
					int i;
					for (i = 0; i < EATER_EXT*2; i++)
					{
						p_ptr->magic_num1[i] += (p_ptr->magic_num2[i] < 10) ? EATER_CHARGE * 3 : p_ptr->magic_num2[i]*EATER_CHARGE/3;
						if (p_ptr->magic_num1[i] > p_ptr->magic_num2[i]*EATER_CHARGE) p_ptr->magic_num1[i] = p_ptr->magic_num2[i]*EATER_CHARGE;
					}
					for (; i < EATER_EXT*3; i++)
					{
						int k_idx = lookup_kind(TV_ROD, i-EATER_EXT*2);
						p_ptr->magic_num1[i] -= ((p_ptr->magic_num2[i] < 10) ? EATER_ROD_CHARGE*3 : p_ptr->magic_num2[i]*EATER_ROD_CHARGE/3)*k_info[k_idx].pval;
						if (p_ptr->magic_num1[i] < 0) p_ptr->magic_num1[i] = 0;
					}
#ifdef JP
					msg_print("頭がハッキリとした。");
#else
					msg_print("You feel your head clear.");
#endif
					p_ptr->window |= (PW_PLAYER);
				}
				else if (p_ptr->csp < p_ptr->msp)
				{
					p_ptr->csp = p_ptr->msp;
					p_ptr->csp_frac = 0;
#ifdef JP
					msg_print("頭がハッキリとした。");
#else
					msg_print("You feel your head clear.");
#endif

					p_ptr->redraw |= (PR_MANA);
					p_ptr->window |= (PW_PLAYER);
					p_ptr->window |= (PW_SPELL);
				}
				o_ptr->timeout = 777;
				break;
			}
		}

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Done */
		return;
	}

	if (object_is_smith(o_ptr))
	{
		switch (o_ptr->xtra3-1)
		{
		case ESSENCE_TMP_RES_ACID:
			(void)set_oppose_acid(randint1(20) + 20, FALSE);
			o_ptr->timeout = randint0(50) + 50;
			return;

		case ESSENCE_TMP_RES_ELEC:
			(void)set_oppose_elec(randint1(20) + 20, FALSE);
			o_ptr->timeout = randint0(50) + 50;
			return;

		case ESSENCE_TMP_RES_FIRE:
			(void)set_oppose_fire(randint1(20) + 20, FALSE);
			o_ptr->timeout = randint0(50) + 50;
			return;

		case ESSENCE_TMP_RES_COLD:
			(void)set_oppose_cold(randint1(20) + 20, FALSE);
			o_ptr->timeout = randint0(50) + 50;
			return;

		case TR_IMPACT:
			earthquake(py, px, 5);
			o_ptr->timeout = 100 + randint1(100);
			
			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP);

			/* Done */
			return;
		}
	}


	if (o_ptr->name2 == EGO_TRUMP)
	{
		teleport_player(100, 0L);
		o_ptr->timeout = 50 + randint1(50);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Done */
		return;
	}


	if (o_ptr->name2 == EGO_LITE_ILLUMINATION)
	{
		if (!o_ptr->xtra4 && ((o_ptr->sval == SV_LITE_TORCH) || (o_ptr->sval == SV_LITE_LANTERN)))
		{
#ifdef JP
			msg_print("燃料がない。");
#else
			msg_print("It has no fuel.");
#endif
			energy_use = 0;
			return;
		}
		lite_area(damroll(2, 15), 3);
		o_ptr->timeout = randint0(10) + 10;

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		return;
	}


	if (o_ptr->name2 == EGO_EARTHQUAKES)
	{
		earthquake(py, px, 5);
		o_ptr->timeout = 100 + randint1(100);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Done */
		return;
	}


	if (o_ptr->name2 == EGO_JUMP)
	{
		teleport_player(10, 0L);
		o_ptr->timeout = 10 + randint1(10);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Done */
		return;
	}


	/* Hack -- Dragon Scale Mail can be activated as well */
	if (o_ptr->tval == TV_DRAG_ARMOR)
	{
		/* Get a direction for breathing (or abort) */
		if (!get_aim_dir(&dir)) return;

		if (music_singing_any()) stop_singing();
		if (hex_spelling_any()) stop_hex_spell_all();

		/* Branch on the sub-type */
		switch (o_ptr->sval)
		{
			case SV_DRAGON_BLUE:
			{
#ifdef JP
				msg_print("あなたは稲妻のブレスを吐いた。");
#else
				msg_print("You breathe lightning.");
#endif

				fire_ball(GF_ELEC, dir, 100, -2);
				o_ptr->timeout = randint0(150) + 150;
				break;
			}

			case SV_DRAGON_WHITE:
			{
#ifdef JP
				msg_print("あなたは冷気のブレスを吐いた。");
#else
				msg_print("You breathe frost.");
#endif

				fire_ball(GF_COLD, dir, 110, -2);
				o_ptr->timeout = randint0(150) + 150;
				break;
			}

			case SV_DRAGON_BLACK:
			{
#ifdef JP
				msg_print("あなたは酸のブレスを吐いた。");
#else
				msg_print("You breathe acid.");
#endif

				fire_ball(GF_ACID, dir, 130, -2);
				o_ptr->timeout = randint0(150) + 150;
				break;
			}

			case SV_DRAGON_GREEN:
			{
#ifdef JP
				msg_print("あなたは毒ガスのブレスを吐いた。");
#else
				msg_print("You breathe poison gas.");
#endif

				fire_ball(GF_POIS, dir, 150, -2);
				o_ptr->timeout = randint0(180) + 180;
				break;
			}

			case SV_DRAGON_RED:
			{
#ifdef JP
				msg_print("あなたは火炎のブレスを吐いた。");
#else
				msg_print("You breathe fire.");
#endif

				fire_ball(GF_FIRE, dir, 200, -2);
				o_ptr->timeout = randint0(200) + 200;
				break;
			}

			case SV_DRAGON_MULTIHUED:
			{
				chance = randint0(5);
#ifdef JP
				msg_format("あなたは%sのブレスを吐いた。",
					   ((chance == 1) ? "稲妻" :
					    ((chance == 2) ? "冷気" :
					     ((chance == 3) ? "酸" :
					      ((chance == 4) ? "毒ガス" : "火炎")))));
#else
				msg_format("You breathe %s.",
					   ((chance == 1) ? "lightning" :
					    ((chance == 2) ? "frost" :
					     ((chance == 3) ? "acid" :
					      ((chance == 4) ? "poison gas" : "fire")))));
#endif

				fire_ball(((chance == 1) ? GF_ELEC :
					   ((chance == 2) ? GF_COLD :
					    ((chance == 3) ? GF_ACID :
					     ((chance == 4) ? GF_POIS : GF_FIRE)))),
					  dir, 250, -2);
				o_ptr->timeout = randint0(200) + 200;
				break;
			}

			case SV_DRAGON_BRONZE:
			{
#ifdef JP
				msg_print("あなたは混乱のブレスを吐いた。");
#else
				msg_print("You breathe confusion.");
#endif

				fire_ball(GF_CONFUSION, dir, 120, -2);
				o_ptr->timeout = randint0(180) + 180;
				break;
			}

			case SV_DRAGON_GOLD:
			{
#ifdef JP
				msg_print("あなたは轟音のブレスを吐いた。");
#else
				msg_print("You breathe sound.");
#endif

				fire_ball(GF_SOUND, dir, 130, -2);
				o_ptr->timeout = randint0(180) + 180;
				break;
			}

			case SV_DRAGON_CHAOS:
			{
				chance = randint0(2);
#ifdef JP
				msg_format("あなたは%sのブレスを吐いた。",
					   ((chance == 1 ? "カオス" : "劣化")));
#else
				msg_format("You breathe %s.",
					   ((chance == 1 ? "chaos" : "disenchantment")));
#endif

				fire_ball((chance == 1 ? GF_CHAOS : GF_DISENCHANT),
					  dir, 220, -2);
				o_ptr->timeout = randint0(200) + 200;
				break;
			}

			case SV_DRAGON_LAW:
			{
				chance = randint0(2);
#ifdef JP
				msg_format("あなたは%sのブレスを吐いた。",
					   ((chance == 1 ? "轟音" : "破片")));
#else
				msg_format("You breathe %s.",
					   ((chance == 1 ? "sound" : "shards")));
#endif

				fire_ball((chance == 1 ? GF_SOUND : GF_SHARDS),
					  dir, 230, -2);
				o_ptr->timeout = randint0(200) + 200;
				break;
			}

			case SV_DRAGON_BALANCE:
			{
				chance = randint0(4);
#ifdef JP
				msg_format("あなたは%sのブレスを吐いた",
					   ((chance == 1) ? "カオス" :
					    ((chance == 2) ? "劣化" :
					     ((chance == 3) ? "轟音" : "破片"))));
#else
				msg_format("You breathe %s.",
					   ((chance == 1) ? "chaos" :
					    ((chance == 2) ? "disenchantment" :
					     ((chance == 3) ? "sound" : "shards"))));
#endif

				fire_ball(((chance == 1) ? GF_CHAOS :
					   ((chance == 2) ? GF_DISENCHANT :
					    ((chance == 3) ? GF_SOUND : GF_SHARDS))),
					  dir, 250, -2);
				o_ptr->timeout = randint0(200) + 200;
				break;
			}

			case SV_DRAGON_SHINING:
			{
				chance = randint0(2);
#ifdef JP
				msg_format("あなたは%sのブレスを吐いた。",
					   ((chance == 0 ? "閃光" : "暗黒")));
#else
				msg_format("You breathe %s.",
					   ((chance == 0 ? "light" : "darkness")));
#endif

				fire_ball((chance == 0 ? GF_LITE : GF_DARK), dir, 200, -2);
				o_ptr->timeout = randint0(200) + 200;
				break;
			}

			case SV_DRAGON_POWER:
			{
#ifdef JP
msg_print("あなたはエレメントのブレスを吐いた。");
#else
				msg_print("You breathe the elements.");
#endif

				fire_ball(GF_MISSILE, dir, 300, -3);
				o_ptr->timeout = randint0(200) + 200;
				break;
			}
		}

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Success */
		return;
	}

	else if (o_ptr->tval == TV_RING)
	{
		if (object_is_ego(o_ptr))
		{
			bool success = TRUE;

			switch (o_ptr->name2)
			{
			case EGO_RING_HERO:
				(void)set_afraid(0);
				(void)set_hero(randint1(25) + 25, FALSE);
				(void)hp_player(10);
				o_ptr->timeout = randint1(100)+100;
				break;
			case EGO_RING_MAGIC_MIS:
				if (!get_aim_dir(&dir)) return;
				fire_bolt(GF_MISSILE, dir, damroll(2, 6));
				o_ptr->timeout = 2;
				break;
			case EGO_RING_FIRE_BOLT:
				if (!get_aim_dir(&dir)) return;
				fire_bolt(GF_FIRE, dir, damroll(9, 8));
				o_ptr->timeout = randint0(8) + 8;
				break;
			case EGO_RING_COLD_BOLT:
				if (!get_aim_dir(&dir)) return;
				fire_bolt(GF_COLD, dir, damroll(6, 8));
				o_ptr->timeout = randint0(7) + 7;
				break;
			case EGO_RING_ELEC_BOLT:
				if (!get_aim_dir(&dir)) return;
				fire_bolt(GF_ELEC, dir, damroll(4, 8));
				o_ptr->timeout = randint0(5) + 5;
				break;
			case EGO_RING_ACID_BOLT:
				if (!get_aim_dir(&dir)) return;
				fire_bolt(GF_ACID, dir, damroll(5, 8));
				o_ptr->timeout = randint0(6) + 6;
				break;
			case EGO_RING_MANA_BOLT:
				if (!get_aim_dir(&dir)) return;
				fire_bolt(GF_MANA, dir, 120);
				o_ptr->timeout = randint0(120)+120;
				break;
			case EGO_RING_FIRE_BALL:
				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_FIRE, dir, 100, 2);
				o_ptr->timeout = randint0(80) + 80;
				break;
			case EGO_RING_COLD_BALL:
				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_COLD, dir, 100, 2);
				o_ptr->timeout = randint0(80) + 80;
				break;
			case EGO_RING_ELEC_BALL:
				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_ELEC, dir, 100, 2);
				o_ptr->timeout = randint0(80) + 80;
				break;
			case EGO_RING_ACID_BALL:
				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_ACID, dir, 100, 2);
				o_ptr->timeout = randint0(80) + 80;
				break;
			case EGO_RING_MANA_BALL:
				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_MANA, dir, 250, 2);
				o_ptr->timeout = 300;
				break;
			case EGO_RING_DRAGON_F:
				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_FIRE, dir, 200, -2);
				if (o_ptr->sval == SV_RING_FLAMES)
				{
					(void)set_oppose_fire(randint1(20) + 20, FALSE);
					o_ptr->timeout = 200;
				}
				else o_ptr->timeout = 250;
				break;
			case EGO_RING_DRAGON_C:
				if (!get_aim_dir(&dir)) return;
				fire_ball(GF_COLD, dir, 200, -2);
				if (o_ptr->sval == SV_RING_ICE)
				{
					(void)set_oppose_cold(randint1(20) + 20, FALSE);
					o_ptr->timeout = 200;
				}
				else o_ptr->timeout = 250;
				break;
			case EGO_RING_M_DETECT:
				(void)detect_monsters_invis(255);
				(void)detect_monsters_normal(255);
				o_ptr->timeout = 150;
				break;
			case EGO_RING_D_SPEED:
				(void)set_fast(randint1(30) + 15, FALSE);
				o_ptr->timeout = 100;
				break;
			case EGO_RING_BERSERKER:
				(void)set_afraid(0);
				(void)set_shero(randint1(25) + 25, FALSE);
				o_ptr->timeout = randint0(75)+75;
				break;
			case EGO_RING_TELE_AWAY:
				if (!get_aim_dir(&dir)) return;
				teleport_monster(dir);
				o_ptr->timeout = 150;
				break;
			case EGO_RING_TRUE:
			{
				int v = randint1(25)+25;
				(void)set_afraid(0);
				(void)set_hero(v, FALSE);
				(void)hp_player(10);
				(void)set_blessed(v, FALSE);
				(void)set_oppose_acid(v, FALSE);
				(void)set_oppose_elec(v, FALSE);
				(void)set_oppose_fire(v, FALSE);
				(void)set_oppose_cold(v, FALSE);
				(void)set_oppose_pois(v, FALSE);
				(void)set_ultimate_res(v, FALSE);
				o_ptr->timeout = 777;
				break;
			}
			default:
				success = FALSE;
				break;
			}
			if (success) return;
		}

		/* Get a direction for breathing (or abort) */
		if (!get_aim_dir(&dir)) return;

		switch (o_ptr->sval)
		{
			case SV_RING_ACID:
			{
				fire_ball(GF_ACID, dir, 100, 2);
				(void)set_oppose_acid(randint1(20) + 20, FALSE);
				o_ptr->timeout = randint0(50) + 50;
				break;
			}

			case SV_RING_ICE:
			{
				fire_ball(GF_COLD, dir, 100, 2);
				(void)set_oppose_cold(randint1(20) + 20, FALSE);
				o_ptr->timeout = randint0(50) + 50;
				break;
			}

			case SV_RING_FLAMES:
			{
				fire_ball(GF_FIRE, dir, 100, 2);
				(void)set_oppose_fire(randint1(20) + 20, FALSE);
				o_ptr->timeout = randint0(50) + 50;
				break;
			}

			case SV_RING_ELEC:
			{
				fire_ball(GF_ELEC, dir, 100, 2);
				(void)set_oppose_elec(randint1(20) + 20, FALSE);
				o_ptr->timeout = randint0(50) + 50;
				break;
			}
		}

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Success */
		return;
	}

	else if (o_ptr->tval == TV_AMULET)
	{
		if (object_is_ego(o_ptr))
		{
			switch (o_ptr->name2)
			{
			case EGO_AMU_IDENT:
				if (!ident_spell(FALSE)) return;
				o_ptr->timeout = 10;
				break;
			case EGO_AMU_CHARM:
				if (!get_aim_dir(&dir)) return;
				charm_monster(dir, MAX(20, p_ptr->lev));
				o_ptr->timeout = 200;
				break;
			case EGO_AMU_JUMP:
				teleport_player(10, 0L);
				o_ptr->timeout = randint0(10) + 10;
				break;
			case EGO_AMU_TELEPORT:
				teleport_player(100, 0L);
				o_ptr->timeout = randint0(50) + 50;
				break;
			case EGO_AMU_D_DOOR:
				(void)dimension_door();
				o_ptr->timeout = 200;
				break;
			case EGO_AMU_RES_FIRE_:
				(void)set_oppose_fire(randint1(20) + 20, FALSE);
				o_ptr->timeout = randint0(50) + 50;
				break;
			case EGO_AMU_RES_COLD_:
				(void)set_oppose_cold(randint1(20) + 20, FALSE);
				o_ptr->timeout = randint0(50) + 50;
				break;
			case EGO_AMU_RES_ELEC_:
				(void)set_oppose_elec(randint1(20) + 20, FALSE);
				o_ptr->timeout = randint0(50) + 50;
				break;
			case EGO_AMU_RES_ACID_:
				(void)set_oppose_acid(randint1(20) + 20, FALSE);
				o_ptr->timeout = randint0(50) + 50;
				break;
			case EGO_AMU_DETECTION:
				detect_all(DETECT_RAD_DEFAULT);
				o_ptr->timeout = randint0(55)+55;
				break;
			}
		}
		return;
	}

	else if (o_ptr->tval == TV_WHISTLE)
	{
		if (music_singing_any()) stop_singing();
		if (hex_spelling_any()) stop_hex_spell_all();

#if 0
		if (object_is_cursed(o_ptr))
		{
#ifdef JP
			msg_print("カン高い音が響き渡った。");
#else
			msg_print("You produce a shrill whistling sound.");
#endif
			aggravate_monsters(0);
		}
		else
#endif
		{
			int pet_ctr, i;
			u16b *who;
			int max_pet = 0;
			u16b dummy_why;

			/* Allocate the "who" array */
			C_MAKE(who, max_m_idx, u16b);

			/* Process the monsters (backwards) */
			for (pet_ctr = m_max - 1; pet_ctr >= 1; pet_ctr--)
			{
				if (is_pet(&m_list[pet_ctr]) && (p_ptr->riding != pet_ctr))
				  who[max_pet++] = pet_ctr;
			}

			/* Select the sort method */
			ang_sort_comp = ang_sort_comp_pet;
			ang_sort_swap = ang_sort_swap_hook;

			ang_sort(who, &dummy_why, max_pet);

			/* Process the monsters (backwards) */
			for (i = 0; i < max_pet; i++)
			{
				pet_ctr = who[i];
				teleport_monster_to(pet_ctr, py, px, 100, TELEPORT_PASSIVE);
			}

			/* Free the "who" array */
			C_KILL(who, max_m_idx, u16b);
		}
		o_ptr->timeout = 100+randint1(100);
		return;
	}
	else if (o_ptr->tval == TV_CAPTURE)
	{
		if(!o_ptr->pval)
		{
			bool old_target_pet = target_pet;
			target_pet = TRUE;
			if (!get_aim_dir(&dir))
			{
				target_pet = old_target_pet;
				return;
			}
			target_pet = old_target_pet;

			if(fire_ball(GF_CAPTURE, dir, 0, 0))
			{
				o_ptr->pval = cap_mon;
				o_ptr->xtra3 = cap_mspeed;
				o_ptr->xtra4 = cap_hp;
				o_ptr->xtra5 = cap_maxhp;
				if (cap_nickname)
				{
					cptr t;
					char *s;
					char buf[80] = "";

					if (o_ptr->inscription)
						strcpy(buf, quark_str(o_ptr->inscription));
					s = buf;
					for (s = buf;*s && (*s != '#'); s++)
					{
#ifdef JP
						if (iskanji(*s)) s++;
#endif
					}
					*s = '#';
					s++;
#ifdef JP
 /*nothing*/
#else
					*s++ = '\'';
#endif
					t = quark_str(cap_nickname);
					while (*t)
					{
						*s = *t;
						s++;
						t++;
					}
#ifdef JP
 /*nothing*/
#else
					*s++ = '\'';
#endif
					*s = '\0';
					o_ptr->inscription = quark_add(buf);
				}
			}
		}
		else
		{
			bool success = FALSE;
			if (!get_rep_dir2(&dir)) return;
			if (monster_can_enter(py + ddy[dir], px + ddx[dir], &r_info[o_ptr->pval], 0))
			{
				if (place_monster_aux(0, py + ddy[dir], px + ddx[dir], o_ptr->pval, (PM_FORCE_PET | PM_NO_KAGE)))
				{
					if (o_ptr->xtra3) m_list[hack_m_idx_ii].mspeed = o_ptr->xtra3;
					if (o_ptr->xtra5) m_list[hack_m_idx_ii].max_maxhp = o_ptr->xtra5;
					if (o_ptr->xtra4) m_list[hack_m_idx_ii].hp = o_ptr->xtra4;
					m_list[hack_m_idx_ii].maxhp = m_list[hack_m_idx_ii].max_maxhp;
					if (o_ptr->inscription)
					{
						char buf[80];
						cptr t;
#ifndef JP
						bool quote = FALSE;
#endif

						t = quark_str(o_ptr->inscription);
						for (t = quark_str(o_ptr->inscription);*t && (*t != '#'); t++)
						{
#ifdef JP
							if (iskanji(*t)) t++;
#endif
						}
						if (*t)
						{
							char *s = buf;
							t++;
#ifdef JP
							/* nothing */
#else
							if (*t =='\'')
							{
								t++;
								quote = TRUE;
							}
#endif
							while(*t)
							{
								*s = *t;
								t++;
								s++;
							}
#ifdef JP
							/* nothing */
#else
							if (quote && *(s-1) =='\'')
								s--;
#endif
							*s = '\0';
							m_list[hack_m_idx_ii].nickname = quark_add(buf);
							t = quark_str(o_ptr->inscription);
							s = buf;
							while(*t && (*t != '#'))
							{
								*s = *t;
								t++;
								s++;
							}
							*s = '\0';
							o_ptr->inscription = quark_add(buf);
						}
					}
					o_ptr->pval = 0;
					o_ptr->xtra3 = 0;
					o_ptr->xtra4 = 0;
					o_ptr->xtra5 = 0;
					success = TRUE;
				}
			}
			if (!success)
#ifdef JP
				msg_print("おっと、解放に失敗した。");
#else
				msg_print("Oops.  You failed to release your pet.");
#endif
		}
		return;
	}

	/* Mistake */
#ifdef JP
	msg_print("おっと、このアイテムは始動できない。");
#else
	msg_print("Oops.  That object cannot be activated.");
#endif

}


void do_cmd_activate(void)
{
	int     item;
	cptr    q, s;


	if (p_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
	{
		set_action(ACTION_NONE);
	}

	item_tester_no_ryoute = TRUE;
	/* Prepare the hook */
	item_tester_hook = item_tester_hook_activate;

	/* Get an item */
#ifdef JP
	q = "どのアイテムを始動させますか? ";
	s = "始動できるアイテムを装備していない。";
#else
	q = "Activate which item? ";
	s = "You have nothing to activate.";
#endif

	if (!get_item(&item, q, s, (USE_EQUIP))) return;

	/* Activate the item */
	do_cmd_activate_aux(item);
}


/*
 * Hook to determine if an object is useable
 */
static bool item_tester_hook_use(object_type *o_ptr)
{
	u32b flgs[TR_FLAG_SIZE];

	/* Ammo */
	if (o_ptr->tval == p_ptr->tval_ammo)
		return (TRUE);

	/* Useable object */
	switch (o_ptr->tval)
	{
		case TV_SPIKE:
		case TV_STAFF:
		case TV_WAND:
		case TV_ROD:
		case TV_SCROLL:
		case TV_POTION:
		case TV_FOOD:
		{
			return (TRUE);
		}

		default:
		{
			int i;

			/* Not known */
			if (!object_is_known(o_ptr)) return (FALSE);

			/* HACK - only items from the equipment can be activated */
			for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
			{
				if (&inventory[i] == o_ptr)
				{
					/* Extract the flags */
					object_flags(o_ptr, flgs);

					/* Check activation flag */
					if (have_flag(flgs, TR_ACTIVATE)) return (TRUE);
				}
			}
		}
	}

	/* Assume not */
	return (FALSE);
}


/*
 * Use an item
 * XXX - Add actions for other item types
 */
void do_cmd_use(void)
{
	int         item;
	object_type *o_ptr;
	cptr        q, s;

	if (p_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
	{
		set_action(ACTION_NONE);
	}

	item_tester_no_ryoute = TRUE;
	/* Prepare the hook */
	item_tester_hook = item_tester_hook_use;

	/* Get an item */
#ifdef JP
q = "どれを使いますか？";
s = "使えるものがありません。";
#else
	q = "Use which item? ";
	s = "You have nothing to use.";
#endif

	if (!get_item(&item, q, s, (USE_INVEN | USE_EQUIP | USE_FLOOR))) return;

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

	switch (o_ptr->tval)
	{
		/* Spike a door */
		case TV_SPIKE:
		{
			do_cmd_spike();
			break;
		}

		/* Eat some food */
		case TV_FOOD:
		{
			do_cmd_eat_food_aux(item);
			break;
		}

		/* Aim a wand */
		case TV_WAND:
		{
			do_cmd_aim_wand_aux(item);
			break;
		}

		/* Use a staff */
		case TV_STAFF:
		{
			do_cmd_use_staff_aux(item);
			break;
		}

		/* Zap a rod */
		case TV_ROD:
		{
			do_cmd_zap_rod_aux(item);
			break;
		}

		/* Quaff a potion */
		case TV_POTION:
		{
			do_cmd_quaff_potion_aux(item);
			break;
		}

		/* Read a scroll */
		case TV_SCROLL:
		{
			/* Check some conditions */
			if (p_ptr->blind)
			{
#ifdef JP
msg_print("目が見えない。");
#else
				msg_print("You can't see anything.");
#endif

				return;
			}
			if (no_lite())
			{
#ifdef JP
msg_print("明かりがないので、暗くて読めない。");
#else
				msg_print("You have no light to read by.");
#endif

				return;
			}
			if (p_ptr->confused)
			{
#ifdef JP
msg_print("混乱していて読めない！");
#else
				msg_print("You are too confused!");
#endif

				return;
			}

		  do_cmd_read_scroll_aux(item, TRUE);
		  break;
		}

		/* Fire ammo */
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		{
			do_cmd_fire_aux(item, &inventory[INVEN_BOW]);
			break;
		}

		/* Activate an artifact */
		default:
		{
			do_cmd_activate_aux(item);
			break;
		}
	}
}

static int select_magic_eater(bool only_browse)
{
	int ext=0;
	char choice;
	bool flag, request_list;
	int tval = 0;
	int             ask = TRUE, i = 0;
	char            out_val[160];

	int menu_line = (use_menu ? 1 : 0);

#ifdef ALLOW_REPEAT
	int sn;
	if (repeat_pull(&sn))
	{
		/* Verify the spell */
		if (sn >= EATER_EXT*2 && !(p_ptr->magic_num1[sn] > k_info[lookup_kind(TV_ROD, sn-EATER_EXT*2)].pval * (p_ptr->magic_num2[sn] - 1) * EATER_ROD_CHARGE))
			return sn;
		else if (sn < EATER_EXT*2 && !(p_ptr->magic_num1[sn] < EATER_CHARGE))
			return sn;
	}
	
#endif /* ALLOW_REPEAT */

	for (i = 0; i < 108; i++)
	{
		if (p_ptr->magic_num2[i]) break;
	}
	if (i == 108)
	{
#ifdef JP
		msg_print("魔法を覚えていない！");
#else
		msg_print("You don't have any magic!");
#endif
		return -1;
	}

	if (use_menu)
	{
		screen_save();

		while(!tval)
		{
#ifdef JP
			prt(format(" %s 杖", (menu_line == 1) ? "》" : "  "), 2, 14);
			prt(format(" %s 魔法棒", (menu_line == 2) ? "》" : "  "), 3, 14);
			prt(format(" %s ロッド", (menu_line == 3) ? "》" : "  "), 4, 14);
			prt("どの種類の魔法を使いますか？", 0, 0);
#else
			prt(format(" %s staff", (menu_line == 1) ? "> " : "  "), 2, 14);
			prt(format(" %s wand", (menu_line == 2) ? "> " : "  "), 3, 14);
			prt(format(" %s rod", (menu_line == 3) ? "> " : "  "), 4, 14);
			prt("Which type of magic do you use?", 0, 0);
#endif
			choice = inkey();
			switch(choice)
			{
			case ESCAPE:
			case 'z':
			case 'Z':
				screen_load();
				return -1;
			case '2':
			case 'j':
			case 'J':
				menu_line++;
				break;
			case '8':
			case 'k':
			case 'K':
				menu_line+= 2;
				break;
			case '\r':
			case 'x':
			case 'X':
				ext = (menu_line-1)*EATER_EXT;
				if (menu_line == 1) tval = TV_STAFF;
				else if (menu_line == 2) tval = TV_WAND;
				else tval = TV_ROD;
				break;
			}
			if (menu_line > 3) menu_line -= 3;
		}
		screen_load();
	}
	else
	{
	while (TRUE)
	{
#ifdef JP
		if (!get_com("[A] 杖, [B] 魔法棒, [C] ロッド:", &choice, TRUE))
#else
		if (!get_com("[A] staff, [B] wand, [C] rod:", &choice, TRUE))
#endif
		{
			return -1;
		}
		if (choice == 'A' || choice == 'a')
		{
			ext = 0;
			tval = TV_STAFF;
			break;
		}
		if (choice == 'B' || choice == 'b')
		{
			ext = EATER_EXT;
			tval = TV_WAND;
			break;
		}
		if (choice == 'C' || choice == 'c')
		{
			ext = EATER_EXT*2;
			tval = TV_ROD;
			break;
		}
	}
	}
	for (i = ext; i < ext + EATER_EXT; i++)
	{
		if (p_ptr->magic_num2[i])
		{
			if (use_menu) menu_line = i-ext+1;
			break;
		}
	}
	if (i == ext+EATER_EXT)
	{
#ifdef JP
		msg_print("その種類の魔法は覚えていない！");
#else
		msg_print("You don't have that type of magic!");
#endif
		return -1;
	}

	/* Nothing chosen yet */
	flag = FALSE;

	/* Build a prompt */
#ifdef JP
(void) strnfmt(out_val, 78, "('*'で一覧, ESCで中断) どの魔力を使いますか？");
#else
	(void)strnfmt(out_val, 78, "(*=List, ESC=exit) Use which power? ");
#endif
	
	/* Save the screen */
	screen_save();

	request_list = always_show_list;

	/* Get a spell from the user */
	while (!flag)
	{
		/* Show the list */
		if (request_list || use_menu)
		{
			byte y, x = 0;
			int ctr, chance;
			int k_idx;
			char dummy[80];
			int x1, y1, level;
			byte col;

			strcpy(dummy, "");

			for (y = 1; y < 20; y++)
				prt("", y, x);

			y = 1;

			/* Print header(s) */
#ifdef JP
			prt(format("                           %s 失率                           %s 失率", (tval == TV_ROD ? "  状態  " : "使用回数"), (tval == TV_ROD ? "  状態  " : "使用回数")), y++, x);
#else
			prt(format("                           %s Fail                           %s Fail", (tval == TV_ROD ? "  Stat  " : " Charges"), (tval == TV_ROD ? "  Stat  " : " Charges")), y++, x);
#endif

			/* Print list */
			for (ctr = 0; ctr < EATER_EXT; ctr++)
			{
				if (!p_ptr->magic_num2[ctr+ext]) continue;

				k_idx = lookup_kind(tval, ctr);

				if (use_menu)
				{
					if (ctr == (menu_line-1))
#ifdef JP
						strcpy(dummy, "》");
#else
					strcpy(dummy, "> ");
#endif
					else strcpy(dummy, "  ");
						
				}
				/* letter/number for power selection */
				else
				{
					char letter;
					if (ctr < 26)
						letter = I2A(ctr);
					else
						letter = '0' + ctr - 26;
					sprintf(dummy, "%c)",letter);
				}
				x1 = ((ctr < EATER_EXT/2) ? x : x + 40);
				y1 = ((ctr < EATER_EXT/2) ? y + ctr : y + ctr - EATER_EXT/2);
				level = (tval == TV_ROD ? k_info[k_idx].level * 5 / 6 - 5 : k_info[k_idx].level);
				chance = level * 4 / 5 + 20;
				chance -= 3 * (adj_mag_stat[p_ptr->stat_ind[mp_ptr->spell_stat]] - 1);
				level /= 2;
				if (p_ptr->lev > level)
				{
					chance -= 3 * (p_ptr->lev - level);
				}
				chance = mod_spell_chance_1(chance);
				chance = MAX(chance, adj_mag_fail[p_ptr->stat_ind[mp_ptr->spell_stat]]);
				/* Stunning makes spells harder */
				if (p_ptr->stun > 50) chance += 25;
				else if (p_ptr->stun) chance += 15;

				if (chance > 95) chance = 95;

				chance = mod_spell_chance_2(chance);

				col = TERM_WHITE;

				if (k_idx)
				{
					if (tval == TV_ROD)
					{
						strcat(dummy, format(
#ifdef JP
							       " %-22.22s 充填:%2d/%2d%3d%%",
#else
							       " %-22.22s   (%2d/%2d) %3d%%",
#endif
							       k_name + k_info[k_idx].name, 
							       p_ptr->magic_num1[ctr+ext] ? 
							       (p_ptr->magic_num1[ctr+ext] - 1) / (EATER_ROD_CHARGE * k_info[k_idx].pval) +1 : 0, 
							       p_ptr->magic_num2[ctr+ext], chance));
						if (p_ptr->magic_num1[ctr+ext] > k_info[k_idx].pval * (p_ptr->magic_num2[ctr+ext]-1) * EATER_ROD_CHARGE) col = TERM_RED;
					}
					else
					{
						strcat(dummy, format(" %-22.22s    %2d/%2d %3d%%", k_name + k_info[k_idx].name, (s16b)(p_ptr->magic_num1[ctr+ext]/EATER_CHARGE), p_ptr->magic_num2[ctr+ext], chance));
						if (p_ptr->magic_num1[ctr+ext] < EATER_CHARGE) col = TERM_RED;
					}
				}
				else
					strcpy(dummy, "");
				c_prt(col, dummy, y1, x1);
			}
		}

		if (!get_com(out_val, &choice, FALSE)) break;

		if (use_menu && choice != ' ')
		{
			switch (choice)
			{
				case '0':
				{
					screen_load();
					return 0;
				}

				case '8':
				case 'k':
				case 'K':
				{
					do
					{
						menu_line += EATER_EXT - 1;
						if (menu_line > EATER_EXT) menu_line -= EATER_EXT;
					} while(!p_ptr->magic_num2[menu_line+ext-1]);
					break;
				}

				case '2':
				case 'j':
				case 'J':
				{
					do
					{
						menu_line++;
						if (menu_line > EATER_EXT) menu_line -= EATER_EXT;
					} while(!p_ptr->magic_num2[menu_line+ext-1]);
					break;
				}

				case '4':
				case 'h':
				case 'H':
				case '6':
				case 'l':
				case 'L':
				{
					bool reverse = FALSE;
					if ((choice == '4') || (choice == 'h') || (choice == 'H')) reverse = TRUE;
					if (menu_line > EATER_EXT/2)
					{
						menu_line -= EATER_EXT/2;
						reverse = TRUE;
					}
					else menu_line+=EATER_EXT/2;
					while(!p_ptr->magic_num2[menu_line+ext-1])
					{
						if (reverse)
						{
							menu_line--;
							if (menu_line < 2) reverse = FALSE;
						}
						else
						{
							menu_line++;
							if (menu_line > EATER_EXT-1) reverse = TRUE;
						}
					}
					break;
				}

				case 'x':
				case 'X':
				case '\r':
				{
					i = menu_line - 1;
					ask = FALSE;
					break;
				}
			}
		}

		/* Request redraw */
		if (use_menu && ask) continue;

		/* Request redraw */
		if (!use_menu && ((choice == ' ') || (choice == '*') || (choice == '?')))
		{
			/* Hide the list */
			if (request_list)
			{
				/* Hide list */
				request_list = FALSE;
				
				/* Restore the screen */
				screen_load();
				screen_save();
			}
			else
				request_list = TRUE;

			/* Redo asking */
			continue;
		}

		if (!use_menu)
		{
			if (isalpha(choice))
			{
				/* Note verify */
				ask = (isupper(choice));

				/* Lowercase */
				if (ask) choice = tolower(choice);

				/* Extract request */
				i = (islower(choice) ? A2I(choice) : -1);
			}
			else
			{
				ask = FALSE; /* Can't uppercase digits */

				i = choice - '0' + 26;
			}
		}

		/* Totally Illegal */
		if ((i < 0) || (i > EATER_EXT) || !p_ptr->magic_num2[i+ext])
		{
			bell();
			continue;
		}

		if (!only_browse)
		{
			/* Verify it */
			if (ask)
			{
				char tmp_val[160];

				/* Prompt */
#ifdef JP
				(void) strnfmt(tmp_val, 78, "%sを使いますか？ ", k_name + k_info[lookup_kind(tval ,i)].name);
#else
				(void) strnfmt(tmp_val, 78, "Use %s?", k_name + k_info[lookup_kind(tval ,i)].name);
#endif

				/* Belay that order */
				if (!get_check(tmp_val)) continue;
			}
			if (tval == TV_ROD)
			{
				if (p_ptr->magic_num1[ext+i]  > k_info[lookup_kind(tval, i)].pval * (p_ptr->magic_num2[ext+i] - 1) * EATER_ROD_CHARGE)
				{
#ifdef JP
					msg_print("その魔法はまだ充填している最中だ。");
#else
					msg_print("The magic are still charging.");
#endif
					msg_print(NULL);
					if (use_menu) ask = TRUE;
					continue;
				}
			}
			else
			{
				if (p_ptr->magic_num1[ext+i] < EATER_CHARGE)
				{
#ifdef JP
					msg_print("その魔法は使用回数が切れている。");
#else
					msg_print("The magic has no charges left.");
#endif
					msg_print(NULL);
					if (use_menu) ask = TRUE;
					continue;
				}
			}
		}

		/* Browse */
		else
		{
			int line, j;
			char temp[70 * 20];

			/* Clear lines, position cursor  (really should use strlen here) */
			Term_erase(7, 23, 255);
			Term_erase(7, 22, 255);
			Term_erase(7, 21, 255);
			Term_erase(7, 20, 255);

			roff_to_buf(k_text + k_info[lookup_kind(tval, i)].text, 62, temp, sizeof(temp));
			for (j = 0, line = 21; temp[j]; j += 1 + strlen(&temp[j]))
			{
				prt(&temp[j], line, 10);
				line++;
			}
	
#ifdef JP
			prt("何かキーを押して下さい。",0,0);
#else
			prt("Hit any key.",0,0);
#endif
			(void)inkey();
			continue;
		}

		/* Stop the loop */
		flag = TRUE;
	}

	/* Restore the screen */
	screen_load();

	if (!flag) return -1;

#ifdef ALLOW_REPEAT
	repeat_push(ext+i);
#endif /* ALLOW_REPEAT */
	return ext+i;
}


/*
 *  Use eaten rod, wand or staff
 */
void do_cmd_magic_eater(bool only_browse)
{
	int item, chance, level, k_idx, tval, sval;
	bool use_charge = TRUE;

	/* Not when confused */
	if (!only_browse && p_ptr->confused)
	{
#ifdef JP
msg_print("混乱していて唱えられない！");
#else
		msg_print("You are too confused!");
#endif

		return;
	}

	item = select_magic_eater(only_browse);
	if (item == -1)
	{
		energy_use = 0;
		return;
	}
	if (item >= EATER_EXT*2) {tval = TV_ROD;sval = item - EATER_EXT*2;}
	else if (item >= EATER_EXT) {tval = TV_WAND;sval = item - EATER_EXT;}
	else {tval = TV_STAFF;sval = item;}
	k_idx = lookup_kind(tval, sval);

	level = (tval == TV_ROD ? k_info[k_idx].level * 5 / 6 - 5 : k_info[k_idx].level);
	chance = level * 4 / 5 + 20;
	chance -= 3 * (adj_mag_stat[p_ptr->stat_ind[mp_ptr->spell_stat]] - 1);
	level /= 2;
	if (p_ptr->lev > level)
	{
		chance -= 3 * (p_ptr->lev - level);
	}
	chance = mod_spell_chance_1(chance);
	chance = MAX(chance, adj_mag_fail[p_ptr->stat_ind[mp_ptr->spell_stat]]);
	/* Stunning makes spells harder */
	if (p_ptr->stun > 50) chance += 25;
	else if (p_ptr->stun) chance += 15;

	if (chance > 95) chance = 95;

	chance = mod_spell_chance_2(chance);

	if (randint0(100) < chance)
	{
		if (flush_failure) flush();

#ifdef JP
msg_print("呪文をうまく唱えられなかった！");
#else
		msg_format("You failed to get the magic off!");
#endif

		sound(SOUND_FAIL);
		if (randint1(100) >= chance)
			chg_virtue(V_CHANCE,-1);
		energy_use = 100;

		return;
	}
	else
	{
		int dir = 0;

		if (tval == TV_ROD)
		{
			if ((sval >= SV_ROD_MIN_DIRECTION) && (sval != SV_ROD_HAVOC) && (sval != SV_ROD_AGGRAVATE) && (sval != SV_ROD_PESTICIDE))
				if (!get_aim_dir(&dir)) return;
			rod_effect(sval, dir, &use_charge, TRUE);
			if (!use_charge) return;
		}
		else if (tval == TV_WAND)
		{
			if (!get_aim_dir(&dir)) return;
			wand_effect(sval, dir, TRUE);
		}
		else
		{
			staff_effect(sval, &use_charge, TRUE, TRUE);
			if (!use_charge) return;
		}
		if (randint1(100) < chance)
			chg_virtue(V_CHANCE,1);
	}
	energy_use = 100;
	if (tval == TV_ROD) p_ptr->magic_num1[item] += k_info[k_idx].pval * EATER_ROD_CHARGE;
	else p_ptr->magic_num1[item] -= EATER_CHARGE;
}
