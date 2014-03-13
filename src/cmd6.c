/*!
 * @file cmd6.c
 * @brief プレイヤーのアイテムに関するコマンドの実装2 / Spell/Prayer commands
 * @date 2014/01/27
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.\n
 * </pre>
 * @details
 * <pre>
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
 * </pre>
 */

#include "angband.h"


/*!
 * @brief 食料を食べるコマンドのサブルーチン
 * @param item 食べるオブジェクトの所持品ID
 * @return なし
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
				take_hit(DAMAGE_NOESCAPE, damroll(6, 6), _("毒入り食料", "poisonous food"), -1);
				(void)do_dec_stat(A_STR);
				ident = TRUE;
				break;
			}

			case SV_FOOD_SICKNESS:
			{
				take_hit(DAMAGE_NOESCAPE, damroll(6, 6), _("毒入り食料", "poisonous food"), -1);
				(void)do_dec_stat(A_CON);
				ident = TRUE;
				break;
			}

			case SV_FOOD_STUPIDITY:
			{
				take_hit(DAMAGE_NOESCAPE, damroll(8, 8), _("毒入り食料", "poisonous food"), -1);
				(void)do_dec_stat(A_INT);
				ident = TRUE;
				break;
			}

			case SV_FOOD_NAIVETY:
			{
				take_hit(DAMAGE_NOESCAPE, damroll(8, 8), _("毒入り食料", "poisonous food"), -1);
				(void)do_dec_stat(A_WIS);
				ident = TRUE;
				break;
			}

			case SV_FOOD_UNHEALTH:
			{
				take_hit(DAMAGE_NOESCAPE, damroll(10, 10), _("毒入り食料", "poisonous food"), -1);
				(void)do_dec_stat(A_CON);
				ident = TRUE;
				break;
			}

			case SV_FOOD_DISEASE:
			{
				take_hit(DAMAGE_NOESCAPE, damroll(10, 10), _("毒入り食料", "poisonous food"), -1);
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
				msg_print(_("これはひじょうに美味だ。", "That tastes good."));
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
		msg_print(_("あなたのような者にとって食糧など僅かな栄養にしかならない。", 
					"Mere victuals hold scant sustenance for a being such as yourself."));

		if (p_ptr->food < PY_FOOD_ALERT)   /* Hungry */
		msg_print(_("あなたの飢えは新鮮な血によってのみ満たされる！", 
					"Your hunger can only be satisfied with fresh blood!"));
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
			msg_print(_("まずは杖を拾わなければ。", "You must first pick up the staffs."));
			return;
		}
		staff = (o_ptr->tval == TV_STAFF) ? _("杖", "staff") : _("魔法棒", "wand");

		/* "Eat" charges */
		if (o_ptr->pval == 0)
		{
			msg_format(_("この%sにはもう魔力が残っていない。", "The %s has no charges left."), staff);
			o_ptr->ident |= (IDENT_EMPTY);

			/* Combine / Reorder the pack (later) */
			p_ptr->notice |= (PN_COMBINE | PN_REORDER);
			p_ptr->window |= (PW_INVEN);

			return;
		}
		msg_format(_("あなたは%sの魔力をエネルギー源として吸収した。", "You absorb mana of the %s as your energy."), staff);

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
			msg_format(_("杖をまとめなおした。", "You unstack your staff."));
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
		msg_format(_("%sは燃え上り灰になった。精力を吸収した気がする。", "%^s is burnt to ashes.  You absorb its vitality!"), o_name);
		(void)set_food(PY_FOOD_MAX - 1);
	}
	else if (prace_is_(RACE_SKELETON))
	{
#if 0
		if (o_ptr->tval == TV_SKELETON ||
		    (o_ptr->tval == TV_CORPSE && o_ptr->sval == SV_SKELETON))
		{
			msg_print(_("あなたは骨で自分の体を補った。", "Your body absorbs the bone."));
			set_food(p_ptr->food + 5000);
		}
		else 
#endif

		if (!((o_ptr->sval == SV_FOOD_WAYBREAD) ||
		      (o_ptr->sval < SV_FOOD_BISCUIT)))
		{
			object_type forge;
			object_type *q_ptr = &forge;
			
			msg_print(_("食べ物がアゴを素通りして落ちた！", "The food falls through your jaws!"));

			/* Create the item */
			object_prep(q_ptr, lookup_kind(o_ptr->tval, o_ptr->sval));

			/* Drop the object from heaven */
			(void)drop_near(q_ptr, -1, py, px);
		}
		else
		{
			msg_print(_("食べ物がアゴを素通りして落ち、消えた！", "The food falls through your jaws and vanishes!"));
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
		msg_print(_("生者の食物はあなたにとってほとんど栄養にならない。", "The food of mortals is poor sustenance for you."));
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


/*!
 * @brief オブジェクトをプレイヤーが食べることができるかを判定する /
 * Hook to determine if an object is eatable
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return 食べることが可能ならばTRUEを返す
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


/*!
 * @brief 食料を食べるコマンドのメインルーチン /
 * Eat some food (from the pack or floor)
 * @return なし
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
	q = _("どれを食べますか? ", "Eat which item? ");
	s = _("食べ物がない。", "You have nothing to eat.");

	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;

	/* Eat the object */
	do_cmd_eat_food_aux(item);
}


/*!
 * @brief 薬を飲むコマンドのサブルーチン /
 * Quaff a potion (from the pack or the floor)
 * @param item 飲む薬オブジェクトの所持品ID
 * @return なし
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
		msg_print(_("瓶から水が流れ出てこない！", "The potion doesn't flow out from a bottle."));

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
			/* 飲みごたえをオリジナルより細かく表現 */
		case SV_POTION_WATER:
			msg_print(_("口の中がさっぱりした。", ""));
			msg_print(_("のどの渇きが少しおさまった。", "You feel less thirsty."));
			ident = TRUE;
			break;

		case SV_POTION_APPLE_JUICE:
			msg_print(_("甘くてサッパリとしていて、とてもおいしい。", ""));
			msg_print(_("のどの渇きが少しおさまった。", "You feel less thirsty."));
			ident = TRUE;
			break;

		case SV_POTION_SLIME_MOLD:
			msg_print(_("なんとも不気味な味だ。", ""));
			msg_print(_("のどの渇きが少しおさまった。", "You feel less thirsty."));
			ident = TRUE;
			break;

		case SV_POTION_SLOWNESS:
			if (set_slow(randint1(25) + 15, FALSE)) ident = TRUE;
			break;

		case SV_POTION_SALT_WATER:
			msg_print(_("うぇ！思わず吐いてしまった。", "The potion makes you vomit!"));

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
					msg_print(_("知らない場所で目が醒めた。頭痛がする。", "You wake up somewhere with a sore head..."));
					msg_print(_("何も思い出せない。どうやってここへ来たのかも分からない！", "You can't remember a thing, or how you got here!"));
				}
			}
			break;

		case SV_POTION_SLEEP:
			if (!p_ptr->free_act)
			{
				msg_print(_("あなたは眠ってしまった。", "You fall asleep."));

				if (ironman_nightmare)
				{
					msg_print(_("恐ろしい光景が頭に浮かんできた。", "A horrible vision enters your mind."));

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
			if (!p_ptr->hold_exp && (p_ptr->exp > 0))
			{
				msg_print(_("過去の記憶が薄れていく気がする。", "You feel your memories fade."));
				chg_virtue(V_KNOWLEDGE, -5);

				lose_exp(p_ptr->exp / 4);
				ident = TRUE;
			}
			break;

		case SV_POTION_RUINATION:
			msg_print(_("身も心も弱ってきて、精気が抜けていくようだ。", "Your nerves and muscles feel weak and lifeless!"));
			take_hit(DAMAGE_LOSELIFE, damroll(10, 10), _("破滅の薬", "a potion of Ruination"), -1);

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
			msg_print(_("体の中で激しい爆発が起きた！", "Massive explosions rupture your body!"));
			take_hit(DAMAGE_NOESCAPE, damroll(50, 20), _("爆発の薬", "a potion of Detonation"), -1);

			(void)set_stun(p_ptr->stun + 75);
			(void)set_cut(p_ptr->cut + 5000);
			ident = TRUE;
			break;

		case SV_POTION_DEATH:
			chg_virtue(V_VITALITY, -1);
			chg_virtue(V_UNLIFE, 5);
			msg_print(_("死の予感が体中を駆けめぐった。", "A feeling of Death flows through your body."));
			take_hit(DAMAGE_LOSELIFE, 5000, _("死の薬", "a potion of Death"), -1);
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
			msg_print(_("体中に生命力が満ちあふれてきた！", "You feel life flow through your body!"));
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
				msg_print(_("頭がハッキリとした。", "You feel your head clear."));
				p_ptr->window |= (PW_PLAYER);
				ident = TRUE;
			}
			else if (p_ptr->csp < p_ptr->msp)
			{
				p_ptr->csp = p_ptr->msp;
				p_ptr->csp_frac = 0;
				msg_print(_("頭がハッキリとした。", "You feel your head clear."));

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
			msg_print(_("自分の置かれている状況が脳裏に浮かんできた...", "An image of your surroundings forms in your mind..."));
			chg_virtue(V_KNOWLEDGE, 1);
			chg_virtue(V_ENLIGHTEN, 1);
			wiz_lite(FALSE);
			ident = TRUE;
			break;

		case SV_POTION_STAR_ENLIGHTENMENT:
			msg_print(_("更なる啓蒙を感じた...", "You begin to feel more enlightened..."));
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
			msg_print(_("自分自身のことが少しは分かった気がする...", "You begin to know yourself a little better..."));
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
				msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));
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
				msg_print(_("全ての突然変異が治った。", "You are cured of all mutations."));
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
			msg_print(_("「オクレ兄さん！」", "Brother OKURE!"));
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
				msg_print(_("全ての突然変異が治った。", "You are cured of all mutations."));
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
		msg_print(_("液体の一部はあなたのアゴを素通りして落ちた！", "Some of the fluid falls through your jaws!"));
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
					msg_print(_("オイルを補給した。", "You replenish yourself with the oil."));
					set_food(p_ptr->food + 5000);
				}
				else
				{
					set_food(p_ptr->food + ((q_ptr->pval) / 20));
				}
				break;
			case RACE_ENT:
				msg_print(_("水分を取り込んだ。", "You are moistened."));
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


/*!
 * @brief オブジェクトをプレイヤーが飲むことができるかを判定する /
 * Hook to determine if an object can be quaffed
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return 飲むことが可能ならばTRUEを返す
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


/*!
 * @brief 薬を飲むコマンドのメインルーチン /
 * Quaff some potion (from the pack or floor)
 * @return なし
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
	q = _("どの薬を飲みますか? ", "Quaff which potion? ");
	s = _("飲める薬がない。", "You have no potions to quaff.");

	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;

	/* Quaff the potion */
	do_cmd_quaff_potion_aux(item);
}


/*!
 * @brief 巻物を読むコマンドのサブルーチン
 * Read a scroll (from the pack or floor).
 * @param item 読むオブジェクトの所持品ID
 * @param known 判明済ならばTRUE
 * @return なし
 * @details
 * <pre>
 * Certain scrolls can be "aborted" without losing the scroll.  These
 * include scrolls with no effects but recharge or identify, which are
 * cancelled before use.  XXX Reading them still takes a turn, though.
 * </pre>
 */
static void do_cmd_read_scroll_aux(int item, bool known)
{
	int         k, used_up, ident, lev;
	object_type *o_ptr;


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
		msg_print(_("止まった時の中ではうまく働かないようだ。", "Nothing happen."));
		sound(SOUND_FAIL);
		return;
	}

	if (p_ptr->pclass == CLASS_BERSERKER)
	{
		msg_print(_("巻物なんて読めない。", "You cannot read."));
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
			msg_print(_("カン高くうなる様な音が辺りを覆った。", "There is a high pitched humming noise."));
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
				msg_print(_("誰かに見守られているような気がする。", "You feel as if someone is watching over you."));
				ident = TRUE;
			}
			break;
		}

		case SV_SCROLL_STAR_REMOVE_CURSE:
		{
			if (remove_all_curse())
			{
				msg_print(_("誰かに見守られているような気がする。", "You feel as if someone is watching over you."));
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
				msg_print(_("手が輝き始めた。", "Your hands begin to glow."));
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
				msg_print(_("ダンジョンが揺れた...", "The dungeon trembles..."));

			break;
		}

		case SV_SCROLL_DISPEL_UNDEAD:
		{
			if (dispel_undead(80)) ident = TRUE;
			break;
		}

		case SV_SCROLL_SPELL:
		{
			if ((p_ptr->pclass == CLASS_WARRIOR) ||
				(p_ptr->pclass == CLASS_IMITATOR) ||
				(p_ptr->pclass == CLASS_MINDCRAFTER) ||
				(p_ptr->pclass == CLASS_SORCERER) ||
				(p_ptr->pclass == CLASS_ARCHER) ||
				(p_ptr->pclass == CLASS_MAGIC_EATER) ||
				(p_ptr->pclass == CLASS_RED_MAGE) ||
				(p_ptr->pclass == CLASS_SAMURAI) ||
				(p_ptr->pclass == CLASS_BLUE_MAGE) ||
				(p_ptr->pclass == CLASS_CAVALRY) ||
				(p_ptr->pclass == CLASS_BERSERKER) ||
				(p_ptr->pclass == CLASS_SMITH) ||
				(p_ptr->pclass == CLASS_MIRROR_MASTER) ||
				(p_ptr->pclass == CLASS_NINJA) ||
				(p_ptr->pclass == CLASS_SNIPER)) break;
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
			acquirement(py, px, 1, TRUE, FALSE, FALSE);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_STAR_ACQUIREMENT:
		{
			acquirement(py, px, randint1(2) + 1, TRUE, FALSE, FALSE);
			ident = TRUE;
			break;
		}

		/* New Hengband scrolls */
		case SV_SCROLL_FIRE:
		{
			fire_ball(GF_FIRE, 0, 666, 4);
			/* Note: "Double" damage since it is centered on the player ... */
			if (!(IS_OPPOSE_FIRE() || p_ptr->resist_fire || p_ptr->immune_fire))
				take_hit(DAMAGE_NOESCAPE, 50+randint1(50), _("炎の巻物", "a Scroll of Fire"), -1);

			ident = TRUE;
			break;
		}


		case SV_SCROLL_ICE:
		{
			fire_ball(GF_ICE, 0, 777, 4);
			if (!(IS_OPPOSE_COLD() || p_ptr->resist_cold || p_ptr->immune_cold))
				take_hit(DAMAGE_NOESCAPE, 100+randint1(100), _("氷の巻物", "a Scroll of Ice"), -1);

			ident = TRUE;
			break;
		}

		case SV_SCROLL_CHAOS:
		{
			fire_ball(GF_CHAOS, 0, 1000, 4);
			if (!p_ptr->resist_chaos)
				take_hit(DAMAGE_NOESCAPE, 111+randint1(111), _("ログルスの巻物", "a Scroll of Logrus"), -1);

			ident = TRUE;
			break;
		}

		case SV_SCROLL_RUMOR:
		{
			msg_print(_("巻物にはメッセージが書かれている:", "There is message on the scroll. It says:"));
			msg_print(NULL);
			display_rumor(TRUE);
			msg_print(NULL);
			msg_print(_("巻物は煙を立てて消え去った！", "The scroll disappears in a puff of smoke!"));

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

		case SV_SCROLL_AMUSEMENT:
		{
			ident = TRUE;
			amusement(py, px, 1, FALSE);
			break;
		}

		case SV_SCROLL_STAR_AMUSEMENT:
		{
			ident = TRUE;
			amusement(py, px,  randint1(2) + 1, FALSE);
			break;
		}
	}
	}
	else if (o_ptr->name1 == ART_GHB)
	{
		msg_print(_("私は苦労して『グレーター・ヘル=ビースト』を倒した。", "I had a very hard time to kill the Greater hell-beast, "));
		msg_print(_("しかし手に入ったのはこのきたないＴシャツだけだった。", "but all I got was this lousy t-shirt!"));
		used_up = FALSE;
	}
	else if (o_ptr->name1 == ART_POWER)
	{
		msg_print(_("「一つの指輪は全てを統べ、", "'One Ring to rule them all, "));
		msg_print(NULL);
		msg_print(_("一つの指輪は全てを見つけ、", "One Ring to find them, "));
		msg_print(NULL);
		msg_print(_("一つの指輪は全てを捕らえて", "One Ring to bring them all "));
		msg_print(NULL);
		msg_print(_("暗闇の中に繋ぎとめる。」", "and in the darkness bind them.'"));
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

/*!
 * @brief オブジェクトをプレイヤーが読むことができるかを判定する /
 * Hook to determine if an object is readable
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return 読むことが可能ならばTRUEを返す
 */
static bool item_tester_hook_readable(object_type *o_ptr)
{
	if ((o_ptr->tval==TV_SCROLL) || (o_ptr->tval==TV_PARCHMENT) || (o_ptr->name1 == ART_GHB) || (o_ptr->name1 == ART_POWER)) return (TRUE);

	/* Assume not */
	return (FALSE);
}

/*!
 * @brief 読むコマンドのメインルーチン /
 * Eat some food (from the pack or floor)
 * @return なし
 */
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
		msg_print(_("目が見えない。", "You can't see anything."));
		return;
	}
	if (no_lite())
	{
		msg_print(_("明かりがないので、暗くて読めない。", "You have no light to read by."));
		return;
	}
	if (p_ptr->confused)
	{
		msg_print(_("混乱していて読めない。", "You are too confused!"));
		return;
	}


	/* Restrict choices to scrolls */
	item_tester_hook = item_tester_hook_readable;

	/* Get an item */
	q = _("どの巻物を読みますか? ", "Read which scroll? ");
	s = _("読める巻物がない。", "You have no scrolls to read.");

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

/*!
 * @brief 杖の効果を発動する
 * @param sval オブジェクトのsval
 * @param use_charge 使用回数を消費したかどうかを返す参照ポインタ
 * @param powerful 強力発動上の処理ならばTRUE
 * @param magic 魔道具術上の処理ならばTRUE
 * @param known 判明済ならばTRUE
 * @return 発動により効果内容が確定したならばTRUEを返す
 */
static int staff_effect(int sval, bool *use_charge, bool powerful, bool magic, bool known)
{
	int k;
	int ident = FALSE;
	int lev = powerful ? p_ptr->lev * 2 : p_ptr->lev;
	int detect_rad = powerful ? DETECT_RAD_DEFAULT * 3 / 2 : DETECT_RAD_DEFAULT;

	/* Analyze the staff */
	switch (sval)
	{
		case SV_STAFF_DARKNESS:
		{
			if (!(p_ptr->resist_blind) && !(p_ptr->resist_dark))
			{
				if (set_blind(p_ptr->blind + 3 + randint1(5))) ident = TRUE;
			}
			if (unlite_area(10, (powerful ? 6 : 3))) ident = TRUE;
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
			const int times = randint1(powerful ? 8 : 4);
			for (k = 0; k < times; k++)
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
			teleport_player((powerful ? 150 : 100), 0L);
			ident = TRUE;
			break;
		}

		case SV_STAFF_IDENTIFY:
		{
			if (powerful) {
				if (!identify_fully(FALSE)) *use_charge = FALSE;
			} else {
				if (!ident_spell(FALSE)) *use_charge = FALSE;
			}
			ident = TRUE;
			break;
		}

		case SV_STAFF_REMOVE_CURSE:
		{
			bool result = powerful ? remove_all_curse() : remove_curse();
			if (result)
			{
				if (magic)
				{
					msg_print(_("誰かに見守られているような気がする。", "You feel as if someone is watching over you."));
				}
				else if (!p_ptr->blind)
				{
					msg_print(_("杖は一瞬ブルーに輝いた...", "The staff glows blue for a moment..."));

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
				msg_print(_("杖の先が明るく輝いた...", "The end of the staff glows brightly..."));
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

				project(0, 0, y, x, damroll(6 + lev / 8, 10), GF_LITE_WEAK,
						  (PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_KILL), -1);
			}
			ident = TRUE;
			break;
		}

		case SV_STAFF_LITE:
		{
			if (lite_area(damroll(2, 8), (powerful ? 4 : 2))) ident = TRUE;
			break;
		}

		case SV_STAFF_MAPPING:
		{
			map_area(powerful ? DETECT_RAD_MAP * 3 / 2 : DETECT_RAD_MAP);
			ident = TRUE;
			break;
		}

		case SV_STAFF_DETECT_GOLD:
		{
			if (detect_treasure(detect_rad)) ident = TRUE;
			if (detect_objects_gold(detect_rad)) ident = TRUE;
			break;
		}

		case SV_STAFF_DETECT_ITEM:
		{
			if (detect_objects_normal(detect_rad)) ident = TRUE;
			break;
		}

		case SV_STAFF_DETECT_TRAP:
		{
			if (detect_traps(detect_rad, known)) ident = TRUE;
			break;
		}

		case SV_STAFF_DETECT_DOOR:
		{
			if (detect_doors(detect_rad)) ident = TRUE;
			if (detect_stairs(detect_rad)) ident = TRUE;
			break;
		}

		case SV_STAFF_DETECT_INVIS:
		{
			if (detect_monsters_invis(detect_rad)) ident = TRUE;
			break;
		}

		case SV_STAFF_DETECT_EVIL:
		{
			if (detect_monsters_evil(detect_rad)) ident = TRUE;
			break;
		}

		case SV_STAFF_CURE_LIGHT:
		{
			if (hp_player(damroll((powerful ? 4 : 2), 8))) ident = TRUE;
			if (powerful) {
				if (set_blind(0)) ident = TRUE;
				if (set_poisoned(0)) ident = TRUE;
				if (set_cut(p_ptr->cut - 10)) ident = TRUE;
			}
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
			if (hp_player(powerful ? 500 : 300)) ident = TRUE;
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
				msg_print(_("頭がハッキリとした。", "You feel your head clear."));

				p_ptr->redraw |= (PR_MANA);
				p_ptr->window |= (PW_PLAYER);
				p_ptr->window |= (PW_SPELL);
			}
			if (set_shero(0,TRUE)) ident = TRUE;
			break;
		}

		case SV_STAFF_SLEEP_MONSTERS:
		{
			if (sleep_monsters(lev)) ident = TRUE;
			break;
		}

		case SV_STAFF_SLOW_MONSTERS:
		{
			if (slow_monsters(lev)) ident = TRUE;
			break;
		}

		case SV_STAFF_SPEED:
		{
			if (set_fast(randint1(30) + (powerful ? 30 : 15), FALSE)) ident = TRUE;
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
			if (dispel_evil(powerful ? 120 : 80)) ident = TRUE;
			break;
		}

		case SV_STAFF_POWER:
		{
			if (dispel_monsters(powerful ? 225 : 150)) ident = TRUE;
			break;
		}

		case SV_STAFF_HOLINESS:
		{
			if (dispel_evil(powerful ? 225 : 150)) ident = TRUE;
			k = 3 * lev;
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
			(void)symbol_genocide((magic ? lev + 50 : 200), TRUE);
			ident = TRUE;
			break;
		}

		case SV_STAFF_EARTHQUAKES:
		{
			if (earthquake(py, px, (powerful ? 15 : 10)))
				ident = TRUE;
			else
				msg_print(_("ダンジョンが揺れた。", "The dungeon trembles."));

			break;
		}

		case SV_STAFF_DESTRUCTION:
		{
			if (destroy_area(py, px, (powerful ? 18 : 13) + randint0(5), FALSE))
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
			msg_print(_("強力な魔力が敵を引き裂いた！", "Mighty magics rend your enemies!"));
			project(0, (powerful ? 7 : 5), py, px,
				(randint1(200) + (powerful ? 500 : 300)) * 2, GF_MANA, PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID, -1);
			if ((p_ptr->pclass != CLASS_MAGE) && (p_ptr->pclass != CLASS_HIGH_MAGE) && (p_ptr->pclass != CLASS_SORCERER) && (p_ptr->pclass != CLASS_MAGIC_EATER) && (p_ptr->pclass != CLASS_BLUE_MAGE))
			{
				(void)take_hit(DAMAGE_NOESCAPE, 50, _("コントロールし難い強力な魔力の解放", "unleashing magics too mighty to control"), -1);
			}
			ident = TRUE;

			break;
		}

		case SV_STAFF_NOTHING:
		{
			msg_print(_("何も起らなかった。", "Nothing happen."));
			if (prace_is_(RACE_SKELETON) || prace_is_(RACE_GOLEM) ||
				prace_is_(RACE_ZOMBIE) || prace_is_(RACE_SPECTRE))
				msg_print(_("もったいない事をしたような気がする。食べ物は大切にしなくては。", "What a waste.  It's your food!"));
			break;
		}
	}
	return ident;
}

/*!
 * @brief 杖を使うコマンドのサブルーチン / 
 * Use a staff.			-RAK-
 * @param item 使うオブジェクトの所持品ID
 * @return なし
 * @details
 * One charge of one staff disappears.
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
		msg_print(_("まずは杖を拾わなければ。", "You must first pick up the staffs."));
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
		msg_print(_("止まった時の中ではうまく働かないようだ。", "Nothing happen. Maybe this staff is freezing too."));
		sound(SOUND_FAIL);
		return;
	}

	/* Roll for usage */
	if ((chance < USE_DEVICE) || (randint1(chance) < USE_DEVICE) || (p_ptr->pclass == CLASS_BERSERKER))
	{
		if (flush_failure) flush();
		msg_print(_("杖をうまく使えなかった。", "You failed to use the staff properly."));
		sound(SOUND_FAIL);
		return;
	}

	/* Notice empty staffs */
	if (o_ptr->pval <= 0)
	{
		if (flush_failure) flush();
		msg_print(_("この杖にはもう魔力が残っていない。", "The staff has no charges left."));
		o_ptr->ident |= (IDENT_EMPTY);

		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);
		p_ptr->window |= (PW_INVEN);

		return;
	}


	/* Sound */
	sound(SOUND_ZAP);

	ident = staff_effect(o_ptr->sval, &use_charge, FALSE, FALSE, object_is_aware(o_ptr));

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
		msg_print(_("杖をまとめなおした。", "You unstack your staff."));
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

/*!
 * @brief 杖を使うコマンドのメインルーチン /
 * @return なし
 */
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
	q = _("どの杖を使いますか? ", "Use which staff? ");
	s = _("使える杖がない。", "You have no staff to use.");

	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;

	do_cmd_use_staff_aux(item);
}

/*!
 * @brief 魔法棒の効果を発動する
 * @param sval オブジェクトのsval
 * @param dir 発動の方向ID
 * @param powerful 強力発動上の処理ならばTRUE
 * @param magic 魔道具術上の処理ならばTRUE
 * @return 発動により効果内容が確定したならばTRUEを返す
 */
static int wand_effect(int sval, int dir, bool powerful, bool magic)
{
	int ident = FALSE;
	int lev = powerful ? p_ptr->lev * 2 : p_ptr->lev;
	int rad = powerful ? 3 : 2;

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
			int dam = damroll((powerful ? 20 : 10), 10);
			if (heal_monster(dir, dam)) ident = TRUE;
			break;
		}

		case SV_WAND_HASTE_MONSTER:
		{
			if (speed_monster(dir, lev)) ident = TRUE;
			break;
		}

		case SV_WAND_CLONE_MONSTER:
		{
			if (clone_monster(dir)) ident = TRUE;
			break;
		}

		case SV_WAND_TELEPORT_AWAY:
		{
			int distance = MAX_SIGHT * (powerful ? 8 : 5);
			if (teleport_monster(dir, distance)) ident = TRUE;
			break;
		}

		case SV_WAND_DISARMING:
		{
			if (disarm_trap(dir)) ident = TRUE;
			if (powerful && disarm_traps_touch()) ident = TRUE;
			break;
		}

		case SV_WAND_TRAP_DOOR_DEST:
		{
			if (destroy_door(dir)) ident = TRUE;
			if (powerful && destroy_doors_touch()) ident = TRUE;
			break;
		}

		case SV_WAND_STONE_TO_MUD:
		{
			int dam = powerful ? 40 + randint1(60) : 20 + randint1(30);
			if (wall_to_mud(dir, dam)) ident = TRUE;
			break;
		}

		case SV_WAND_LITE:
		{
			int dam = damroll((powerful ? 12 : 6), 8);
			msg_print(_("青く輝く光線が放たれた。", "A line of blue shimmering light appears."));
			(void)lite_line(dir, dam);
			ident = TRUE;
			break;
		}

		case SV_WAND_SLEEP_MONSTER:
		{
			if (sleep_monster(dir, lev)) ident = TRUE;
			break;
		}

		case SV_WAND_SLOW_MONSTER:
		{
			if (slow_monster(dir, lev)) ident = TRUE;
			break;
		}

		case SV_WAND_CONFUSE_MONSTER:
		{
			if (confuse_monster(dir, lev)) ident = TRUE;
			break;
		}

		case SV_WAND_FEAR_MONSTER:
		{
			if (fear_monster(dir, lev)) ident = TRUE;
			break;
		}

		case SV_WAND_DRAIN_LIFE:
		{
			if (drain_life(dir, 80 + lev)) ident = TRUE;
			break;
		}

		case SV_WAND_POLYMORPH:
		{
			if (poly_monster(dir, lev)) ident = TRUE;
			break;
		}

		case SV_WAND_STINKING_CLOUD:
		{
			fire_ball(GF_POIS, dir, 12 + lev / 4, rad);
			ident = TRUE;
			break;
		}

		case SV_WAND_MAGIC_MISSILE:
		{
			fire_bolt_or_beam(20, GF_MISSILE, dir, damroll(2 + lev / 10, 6));
			ident = TRUE;
			break;
		}

		case SV_WAND_ACID_BOLT:
		{
			fire_bolt_or_beam(20, GF_ACID, dir, damroll(6 + lev / 7, 8));
			ident = TRUE;
			break;
		}

		case SV_WAND_CHARM_MONSTER:
		{
			if (charm_monster(dir, MAX(20, lev)))
			ident = TRUE;
			break;
		}

		case SV_WAND_FIRE_BOLT:
		{
			fire_bolt_or_beam(20, GF_FIRE, dir, damroll(7 + lev / 6, 8));
			ident = TRUE;
			break;
		}

		case SV_WAND_COLD_BOLT:
		{
			fire_bolt_or_beam(20, GF_COLD, dir, damroll(5 + lev / 8, 8));
			ident = TRUE;
			break;
		}

		case SV_WAND_ACID_BALL:
		{
			fire_ball(GF_ACID, dir, 60 + 3 * lev / 4, rad);
			ident = TRUE;
			break;
		}

		case SV_WAND_ELEC_BALL:
		{
			fire_ball(GF_ELEC, dir, 40 + 3 * lev / 4, rad);
			ident = TRUE;
			break;
		}

		case SV_WAND_FIRE_BALL:
		{
			fire_ball(GF_FIRE, dir, 70 + 3 * lev / 4, rad);
			ident = TRUE;
			break;
		}

		case SV_WAND_COLD_BALL:
		{
			fire_ball(GF_COLD, dir, 50 + 3 * lev / 4, rad);
			ident = TRUE;
			break;
		}

		case SV_WAND_WONDER:
		{
			msg_print(_("おっと、謎の魔法棒を始動させた。", "Oops.  Wand of wonder activated."));
			break;
		}

		case SV_WAND_DRAGON_FIRE:
		{
			fire_ball(GF_FIRE, dir, (powerful ? 300 : 200), -3);
			ident = TRUE;
			break;
		}

		case SV_WAND_DRAGON_COLD:
		{
			fire_ball(GF_COLD, dir, (powerful ? 270 : 180), -3);
			ident = TRUE;
			break;
		}

		case SV_WAND_DRAGON_BREATH:
		{
			int dam;
			int typ;

			switch (randint1(5))
			{
				case 1:
					dam = 240;
					typ = GF_ACID;
					break;
				case 2:
					dam = 210;
					typ = GF_ELEC;
					break;
				case 3:
					dam = 240;
					typ = GF_FIRE;
					break;
				case 4:
					dam = 210;
					typ = GF_COLD;
					break;
				default:
					dam = 180;
					typ = GF_POIS;
					break;
			}

			if (powerful) dam = (dam * 3) / 2;

			fire_ball(typ, dir, dam, -3);

			ident = TRUE;
			break;
		}

		case SV_WAND_DISINTEGRATE:
		{
			fire_ball(GF_DISINTEGRATE, dir, 200 + randint1(lev * 2), rad);
			ident = TRUE;
			break;
		}

		case SV_WAND_ROCKETS:
		{
			msg_print(_("ロケットを発射した！", "You launch a rocket!"));
			fire_rocket(GF_ROCKET, dir, 250 + lev * 3, rad);
			ident = TRUE;
			break;
		}

		case SV_WAND_STRIKING:
		{
			fire_bolt(GF_METEOR, dir, damroll(15 + lev / 3, 13));
			ident = TRUE;
			break;
		}

		case SV_WAND_GENOCIDE:
		{
			fire_ball_hide(GF_GENOCIDE, dir, magic ? lev + 50 : 250, 0);
			ident = TRUE;
			break;
		}
	}
	return ident;
}

/*!
 * @brief 魔法棒を使うコマンドのサブルーチン / 
 * Aim a wand (from the pack or floor).
 * @param item 使うオブジェクトの所持品ID
 * @return なし
 * @details
 * <pre>
 * Use a single charge from a single item.
 * Handle "unstacking" in a logical manner.
 * For simplicity, you cannot use a stack of items from the
 * ground.  This would require too much nasty code.
 * There are no wands which can "destroy" themselves, in the inventory
 * or on the ground, so we can ignore this possibility.  Note that this
 * required giving "wand of wonder" the ability to ignore destruction
 * by electric balls.
 * All wands can be "cancelled" at the "Direction?" prompt for free.
 * Note that the basic "bolt" wands do slightly less damage than the
 * basic "bolt" rods, but the basic "ball" wands do the same damage
 * as the basic "ball" rods.
 * </pre>
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
		msg_print(_("まずは魔法棒を拾わなければ。", "You must first pick up the wands."));
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
		msg_print(_("止まった時の中ではうまく働かないようだ。", "Nothing happen. Maybe this wand is freezing too."));
		sound(SOUND_FAIL);
		return;
	}

	/* Roll for usage */
	if ((chance < USE_DEVICE) || (randint1(chance) < USE_DEVICE) || (p_ptr->pclass == CLASS_BERSERKER))
	{
		if (flush_failure) flush();
		msg_print(_("魔法棒をうまく使えなかった。", "You failed to use the wand properly."));
		sound(SOUND_FAIL);
		return;
	}

	/* The wand is already empty! */
	if (o_ptr->pval <= 0)
	{
		if (flush_failure) flush();
		msg_print(_("この魔法棒にはもう魔力が残っていない。", "The wand has no charges left."));
		o_ptr->ident |= (IDENT_EMPTY);

		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);
		p_ptr->window |= (PW_INVEN);

		return;
	}

	/* Sound */
	sound(SOUND_ZAP);

	ident = wand_effect(o_ptr->sval, dir, FALSE, FALSE);

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

/*!
 * @brief 魔法棒を使うコマンドのメインルーチン /
 * @return なし
 */
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
	q = _("どの魔法棒で狙いますか? ", "Aim which wand? ");
	s = _("使える魔法棒がない。", "You have no wand to aim.");
	
	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;

	/* Aim the wand */
	do_cmd_aim_wand_aux(item);
}

/*!
 * @brief ロッドの効果を発動する
 * @param sval オブジェクトのsval
 * @param dir 発動目標の方向ID
 * @param use_charge チャージを消費したかどうかを返す参照ポインタ
 * @param powerful 強力発動上の処理ならばTRUE
 * @param magic 魔道具術上の処理ならばTRUE
 * @return 発動により効果内容が確定したならばTRUEを返す
 */
static int rod_effect(int sval, int dir, bool *use_charge, bool powerful, bool magic)
{
	int ident = FALSE;
	int lev = powerful ? p_ptr->lev * 2 : p_ptr->lev;
	int detect_rad = powerful ? DETECT_RAD_DEFAULT * 3 / 2 : DETECT_RAD_DEFAULT;
	int rad = powerful ? 3 : 2;

	/* Unused */
	(void)magic;

	/* Analyze the rod */
	switch (sval)
	{
		case SV_ROD_DETECT_TRAP:
		{
			if (detect_traps(detect_rad, (bool)(dir ? FALSE : TRUE))) ident = TRUE;
			break;
		}

		case SV_ROD_DETECT_DOOR:
		{
			if (detect_doors(detect_rad)) ident = TRUE;
			if (detect_stairs(detect_rad)) ident = TRUE;
			break;
		}

		case SV_ROD_IDENTIFY:
		{
			if (powerful) {
				if (!identify_fully(FALSE)) *use_charge = FALSE;
			} else {
				if (!ident_spell(FALSE)) *use_charge = FALSE;
			}
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
			if (lite_area(damroll(2, 8), (powerful ? 4 : 2))) ident = TRUE;
			break;
		}

		case SV_ROD_MAPPING:
		{
			map_area(powerful ? DETECT_RAD_MAP * 3 / 2 : DETECT_RAD_MAP);
			ident = TRUE;
			break;
		}

		case SV_ROD_DETECTION:
		{
			detect_all(detect_rad);
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
			if (hp_player(powerful ? 750 : 500)) ident = TRUE;
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
			if (set_fast(randint1(30) + (powerful ? 30 : 15), FALSE)) ident = TRUE;
			break;
		}

		case SV_ROD_PESTICIDE:
		{
			if (dispel_monsters(powerful ? 8 : 4)) ident = TRUE;
			break;
		}

		case SV_ROD_TELEPORT_AWAY:
		{
			int distance = MAX_SIGHT * (powerful ? 8 : 5);
			if (teleport_monster(dir, distance)) ident = TRUE;
			break;
		}

		case SV_ROD_DISARMING:
		{
			if (disarm_trap(dir)) ident = TRUE;
			if (powerful && disarm_traps_touch()) ident = TRUE;
			break;
		}

		case SV_ROD_LITE:
		{
			int dam = damroll((powerful ? 12 : 6), 8);
			msg_print(_("青く輝く光線が放たれた。", "A line of blue shimmering light appears."));
			(void)lite_line(dir, dam);
			ident = TRUE;
			break;
		}

		case SV_ROD_SLEEP_MONSTER:
		{
			if (sleep_monster(dir, lev)) ident = TRUE;
			break;
		}

		case SV_ROD_SLOW_MONSTER:
		{
			if (slow_monster(dir, lev)) ident = TRUE;
			break;
		}

		case SV_ROD_DRAIN_LIFE:
		{
			if (drain_life(dir, 70 + 3 * lev / 2)) ident = TRUE;
			break;
		}

		case SV_ROD_POLYMORPH:
		{
			if (poly_monster(dir, lev)) ident = TRUE;
			break;
		}

		case SV_ROD_ACID_BOLT:
		{
			fire_bolt_or_beam(10, GF_ACID, dir, damroll(6 + lev / 7, 8));
			ident = TRUE;
			break;
		}

		case SV_ROD_ELEC_BOLT:
		{
			fire_bolt_or_beam(10, GF_ELEC, dir, damroll(4 + lev / 9, 8));
			ident = TRUE;
			break;
		}

		case SV_ROD_FIRE_BOLT:
		{
			fire_bolt_or_beam(10, GF_FIRE, dir, damroll(7 + lev / 6, 8));
			ident = TRUE;
			break;
		}

		case SV_ROD_COLD_BOLT:
		{
			fire_bolt_or_beam(10, GF_COLD, dir, damroll(5 + lev / 8, 8));
			ident = TRUE;
			break;
		}

		case SV_ROD_ACID_BALL:
		{
			fire_ball(GF_ACID, dir, 60 + lev, rad);
			ident = TRUE;
			break;
		}

		case SV_ROD_ELEC_BALL:
		{
			fire_ball(GF_ELEC, dir, 40 + lev, rad);
			ident = TRUE;
			break;
		}

		case SV_ROD_FIRE_BALL:
		{
			fire_ball(GF_FIRE, dir, 70 + lev, rad);
			ident = TRUE;
			break;
		}

		case SV_ROD_COLD_BALL:
		{
			fire_ball(GF_COLD, dir, 50 + lev, rad);
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
			int dam = powerful ? 40 + randint1(60) : 20 + randint1(30);
			if (wall_to_mud(dir, dam)) ident = TRUE;
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

/*!
 * @brief 魔法棒を使うコマンドのサブルーチン / 
 * Activate (zap) a Rod
 * @param item 使うオブジェクトの所持品ID
 * @return なし
 * @details
 * <pre>
 * Unstack fully charged rods as needed.
 * Hack -- rods of perception/genocide can be "cancelled"
 * All rods can be cancelled at the "Direction?" prompt
 * pvals are defined for each rod in k_info. -LM-
 * </pre>
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
		msg_print(_("まずはロッドを拾わなければ。", "You must first pick up the rods."));
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
		msg_print(_("止まった時の中ではうまく働かないようだ。", "Nothing happen. Maybe this rod is freezing too."));
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
		msg_print(_("うまくロッドを使えなかった。", "You failed to use the rod properly."));
		sound(SOUND_FAIL);
		return;
	}

	k_ptr = &k_info[o_ptr->k_idx];

	/* A single rod is still charging */
	if ((o_ptr->number == 1) && (o_ptr->timeout))
	{
		if (flush_failure) flush();
		msg_print(_("このロッドはまだ魔力を充填している最中だ。", "The rod is still charging."));
		return;
	}
	/* A stack of rods lacks enough energy. */
	else if ((o_ptr->number > 1) && (o_ptr->timeout > k_ptr->pval * (o_ptr->number - 1)))
	{
		if (flush_failure) flush();
		msg_print(_("そのロッドはまだ充填中です。", "The rods are all still charging."));
		return;
	}

	/* Sound */
	sound(SOUND_ZAP);

	ident = rod_effect(o_ptr->sval, dir, &use_charge, FALSE, FALSE);

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

/*!
 * @brief ロッドを使うコマンドのメインルーチン /
 * @return なし
 */
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
	q = _("どのロッドを振りますか? ", "Zap which rod? ");
	s = _("使えるロッドがない。", "You have no rod to zap.");

	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;

	/* Zap the rod */
	do_cmd_zap_rod_aux(item);
}

/*!
 * @brief オブジェクトをプレイヤーが魔道具として発動できるかを判定する /
 * Hook to determine if an object is activatable
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return 魔道具として発動可能ならばTRUEを返す
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

/*!
 * @brief 『一つの指輪』の効果処理 /
 * Hack -- activate the ring of power
 * @param dir 発動の方向ID
 * @return なし
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
			msg_print(_("あなたは悪性のオーラに包み込まれた。", "You are surrounded by a malignant aura."));
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
			msg_print(_("あなたは強力なオーラに包み込まれた。", "You are surrounded by a powerful aura."));

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

/*!
 * @brief ペット入りモンスターボールをソートするための比較関数
 * @param u 所持品配列の参照ポインタ
 * @param v 未使用
 * @param a 所持品ID1
 * @param b 所持品ID2
 * @return 1の方が大であればTRUE
 */
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


/*!
 * @brief 装備を発動するコマンドのサブルーチン /
 * Activate a wielded object.  Wielded objects never stack.
 * And even if they did, activatable objects never stack.
 * @param item 発動するオブジェクトの所持品ID
 * @return なし
 * @details
 * <pre>
 * Currently, only (some) artifacts, and Dragon Scale Mail, can be activated.
 * But one could, for example, easily make an activatable "Ring of Plasma".
 * Note that it always takes a turn to activate an artifact, even if
 * the user hits "escape" at the "direction" prompt.
 * </pre>
 */
static void do_cmd_activate_aux(int item)
{
	int         dir, lev, chance, fail;
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
	else if (object_is_random_artifact(o_ptr))
	{
		const activation_type* const act_ptr = find_activation_info(o_ptr);
		if (act_ptr) {
			lev = act_ptr->level;
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
		msg_print(_("止まった時の中ではうまく働かないようだ。", "It shows no reaction."));
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
		msg_print(_("うまく始動させることができなかった。", "You failed to activate it properly."));
		sound(SOUND_FAIL);
		return;
	}

	/* Check the recharge */
	if (o_ptr->timeout)
	{
		msg_print(_("それは微かに音を立て、輝き、消えた...", "It whines, glows and fades..."));
		return;
	}

	/* Some lights need enough fuel for activation */
	if (!o_ptr->xtra4 && (o_ptr->tval == TV_FLASK) &&
		((o_ptr->sval == SV_LITE_TORCH) || (o_ptr->sval == SV_LITE_LANTERN)))
	{
		msg_print(_("燃料がない。", "It has no fuel."));
		energy_use = 0;
		return;
	}

	/* Activate the artifact */
	msg_print(_("始動させた...", "You activate it..."));

	/* Sound */
	sound(SOUND_ZAP);

	/* Activate object */
	if (activation_index(o_ptr))
	{
		(void)activate_random_artifact(o_ptr);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Success */
		return;
	}

	/* Special items */
	else if (o_ptr->tval == TV_WHISTLE)
	{
		if (music_singing_any()) stop_singing();
		if (hex_spelling_any()) stop_hex_spell_all();

#if 0
		if (object_is_cursed(o_ptr))
		{
			msg_print(_("カン高い音が響き渡った。", "You produce a shrill whistling sound."));
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
				msg_print(_("おっと、解放に失敗した。", "Oops.  You failed to release your pet."));
		}
		return;
	}

	/* Mistake */
	msg_print(_("おっと、このアイテムは始動できない。", "Oops.  That object cannot be activated."));
}

/*!
 * @brief 装備を発動するコマンドのメインルーチン /
 * @return なし
 */
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
	q = _("どのアイテムを始動させますか? ", "Activate which item? ");
	s = _("始動できるアイテムを装備していない。", "You have nothing to activate.");

	if (!get_item(&item, q, s, (USE_EQUIP))) return;

	/* Activate the item */
	do_cmd_activate_aux(item);
}


/*!
 * @brief オブジェクトをプレイヤーが簡易使用コマンドで利用できるかを判定する /
 * Hook to determine if an object is useable
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return 利用可能ならばTRUEを返す
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


/*!
 * @brief アイテムを汎用的に「使う」コマンドのメインルーチン /
 * Use an item
 * @return なし
 * @details
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
	q = _("どれを使いますか？", "Use which item? ");
	s = _("使えるものがありません。", "You have nothing to use.");

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
				msg_print(_("目が見えない。", "You can't see anything."));
				return;
			}
			if (no_lite())
			{
				msg_print(_("明かりがないので、暗くて読めない。", "You have no light to read by."));
				return;
			}
			if (p_ptr->confused)
			{
				msg_print(_("混乱していて読めない！", "You are too confused!"));
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

/*!
 * @brief 魔道具術師の取り込んだ魔力一覧から選択/閲覧する /
 * @param only_browse 閲覧するだけならばTRUE
 * @return 選択した魔力のID、キャンセルならば-1を返す
 */
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
		msg_print(_("魔法を覚えていない！", "You don't have any magic!"));
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
#else
			prt(format(" %s staff", (menu_line == 1) ? "> " : "  "), 2, 14);
			prt(format(" %s wand", (menu_line == 2) ? "> " : "  "), 3, 14);
			prt(format(" %s rod", (menu_line == 3) ? "> " : "  "), 4, 14);
#endif

			if (only_browse) prt(_("どの種類の魔法を見ますか？", "Which type of magic do you browse?"), 0, 0);
			else prt(_("どの種類の魔法を使いますか？", "Which type of magic do you use?"), 0, 0);

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
		if (!get_com(_("[A] 杖, [B] 魔法棒, [C] ロッド:", "[A] staff, [B] wand, [C] rod:"), &choice, TRUE))
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
		msg_print(_("その種類の魔法は覚えていない！", "You don't have that type of magic!"));
		return -1;
	}

	/* Nothing chosen yet */
	flag = FALSE;

	/* Build a prompt */
	if (only_browse) strnfmt(out_val, 78, _("('*'で一覧, ESCで中断) どの魔力を見ますか？",
											"(*=List, ESC=exit) Browse which power? "));
	else strnfmt(out_val, 78, _("('*'で一覧, ESCで中断) どの魔力を使いますか？",
								"(*=List, ESC=exit) Use which power? "));
	
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
						strcpy(dummy, _("》", "> "));
					else
						strcpy(dummy, "  ");
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
							       _(" %-22.22s 充填:%2d/%2d%3d%%", " %-22.22s   (%2d/%2d) %3d%%"),
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
				(void) strnfmt(tmp_val, 78, _("%sを使いますか？ ", "Use %s?"), k_name + k_info[lookup_kind(tval ,i)].name);

				/* Belay that order */
				if (!get_check(tmp_val)) continue;
			}
			if (tval == TV_ROD)
			{
				if (p_ptr->magic_num1[ext+i]  > k_info[lookup_kind(tval, i)].pval * (p_ptr->magic_num2[ext+i] - 1) * EATER_ROD_CHARGE)
				{
					msg_print(_("その魔法はまだ充填している最中だ。", "The magic are still charging."));
					msg_print(NULL);
					if (use_menu) ask = TRUE;
					continue;
				}
			}
			else
			{
				if (p_ptr->magic_num1[ext+i] < EATER_CHARGE)
				{
					msg_print(_("その魔法は使用回数が切れている。", "The magic has no charges left."));
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


/*!
 * @brief 取り込んだ魔力を利用するコマンドのメインルーチン /
 * Use eaten rod, wand or staff
 * @param only_browse 閲覧するだけならばTRUE
 * @param powerful 強力発動中の処理ならばTRUE
 * @return 実際にコマンドを実行したならばTRUEを返す。
 */
bool do_cmd_magic_eater(bool only_browse, bool powerful)
{
	int item, chance, level, k_idx, tval, sval;
	bool use_charge = TRUE;

	/* Not when confused */
	if (!only_browse && p_ptr->confused)
	{
		msg_print(_("混乱していて唱えられない！", "You are too confused!"));
		return FALSE;
	}

	item = select_magic_eater(only_browse);
	if (item == -1)
	{
		energy_use = 0;
		return FALSE;
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
		
		msg_print(_("呪文をうまく唱えられなかった！", "You failed to get the magic off!"));
		sound(SOUND_FAIL);
		if (randint1(100) >= chance)
			chg_virtue(V_CHANCE,-1);
		energy_use = 100;

		return TRUE;
	}
	else
	{
		int dir = 0;

		if (tval == TV_ROD)
		{
			if ((sval >= SV_ROD_MIN_DIRECTION) && (sval != SV_ROD_HAVOC) && (sval != SV_ROD_AGGRAVATE) && (sval != SV_ROD_PESTICIDE))
				if (!get_aim_dir(&dir)) return FALSE;
			rod_effect(sval, dir, &use_charge, powerful, TRUE);
			if (!use_charge) return FALSE;
		}
		else if (tval == TV_WAND)
		{
			if (!get_aim_dir(&dir)) return FALSE;
			wand_effect(sval, dir, powerful, TRUE);
		}
		else
		{
			staff_effect(sval, &use_charge, powerful, TRUE, TRUE);
			if (!use_charge) return FALSE;
		}
		if (randint1(100) < chance)
			chg_virtue(V_CHANCE,1);
	}
	energy_use = 100;
	if (tval == TV_ROD) p_ptr->magic_num1[item] += k_info[k_idx].pval * EATER_ROD_CHARGE;
	else p_ptr->magic_num1[item] -= EATER_CHARGE;

        return TRUE;
}
