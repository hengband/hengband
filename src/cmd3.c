/* File: cmd3.c */

/* Purpose: Inventory commands */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"



/*
 * Display inventory
 */
void do_cmd_inven(void)
{
	char out_val[160];


	/* Note that we are in "inventory" mode */
	command_wrk = FALSE;

#ifdef ALLOW_EASY_FLOOR

	/* Note that we are in "inventory" mode */
	if (easy_floor) command_wrk = (USE_INVEN);

#endif /* ALLOW_EASY_FLOOR */

	/* Save screen */
	screen_save();

	/* Hack -- show empty slots */
	item_tester_full = TRUE;

	/* Display the inventory */
	(void)show_inven(0);

	/* Hack -- hide empty slots */
	item_tester_full = FALSE;

#ifdef JP
        sprintf(out_val, "持ち物： 合計 %3d.%1d kg (限界の%ld%%) コマンド: ",
            lbtokg1(p_ptr->total_weight) , lbtokg2(p_ptr->total_weight) ,
            (p_ptr->total_weight * 100) / ((adj_str_wgt[p_ptr->stat_ind[A_STR]] * (p_ptr->pclass == CLASS_BERSERKER ? 150 : 100)) 
/ 2));
#else
	sprintf(out_val, "Inventory: carrying %d.%d pounds (%ld%% of capacity). Command: ",
	    (int)(p_ptr->total_weight / 10), (int)(p_ptr->total_weight % 10),
	    (p_ptr->total_weight * 100) / ((adj_str_wgt[p_ptr->stat_ind[A_STR]] * (p_ptr->pclass == CLASS_BERSERKER ? 150 : 100)) / 2));
#endif


	/* Get a command */
	prt(out_val, 0, 0);

	/* Get a new command */
	command_new = inkey();

	/* Load screen */
	screen_load();


	/* Process "Escape" */
	if (command_new == ESCAPE)
	{
		int wid, hgt;

		/* Get size */
		Term_get_size(&wid, &hgt);

		/* Reset stuff */
		command_new = 0;
		command_gap = wid - 30;
	}

	/* Process normal keys */
	else
	{
		/* Hack -- Use "display" mode */
		command_see = TRUE;
	}
}


/*
 * Display equipment
 */
void do_cmd_equip(void)
{
	char out_val[160];


	/* Note that we are in "equipment" mode */
	command_wrk = TRUE;

#ifdef ALLOW_EASY_FLOOR

	/* Note that we are in "equipment" mode */
	if (easy_floor) command_wrk = (USE_EQUIP);

#endif /* ALLOW_EASY_FLOOR  */

	/* Save the screen */
	screen_save();

	/* Hack -- show empty slots */
	item_tester_full = TRUE;

	/* Display the equipment */
	(void)show_equip(0);

	/* Hack -- undo the hack above */
	item_tester_full = FALSE;

	/* Build a prompt */
#ifdef JP
        sprintf(out_val, "装備： 合計 %3d.%1d kg (限界の%ld%%) コマンド: ",
            lbtokg1(p_ptr->total_weight) , lbtokg2(p_ptr->total_weight) ,
            (p_ptr->total_weight * 100) / ((adj_str_wgt[p_ptr->stat_ind[A_STR]] * (p_ptr->pclass == CLASS_BERSERKER ? 150 : 100)) 
/ 2));
#else
	sprintf(out_val, "Equipment: carrying %d.%d pounds (%ld%% of capacity). Command: ",
	    (int)(p_ptr->total_weight / 10), (int)(p_ptr->total_weight % 10),
	    (p_ptr->total_weight * 100) / ((adj_str_wgt[p_ptr->stat_ind[A_STR]] * (p_ptr->pclass == CLASS_BERSERKER ? 150 : 100)) / 2));
#endif


	/* Get a command */
	prt(out_val, 0, 0);

	/* Get a new command */
	command_new = inkey();

	/* Restore the screen */
	screen_load();


	/* Process "Escape" */
	if (command_new == ESCAPE)
	{
		int wid, hgt;

		/* Get size */
		Term_get_size(&wid, &hgt);

		/* Reset stuff */
		command_new = 0;
		command_gap = wid - 30;
	}

	/* Process normal keys */
	else
	{
		/* Enter "display" mode */
		command_see = TRUE;
	}
}


/*
 * The "wearable" tester
 */
static bool item_tester_hook_wear(object_type *o_ptr)
{
	if ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_ABUNAI_MIZUGI))
		if (p_ptr->psex == SEX_MALE) return FALSE;

	/* Check for a usable slot */
	if (wield_slot(o_ptr) >= INVEN_RARM) return (TRUE);

	/* Assume not wearable */
	return (FALSE);
}


static bool item_tester_hook_mochikae(object_type *o_ptr)
{
	/* Check for a usable slot */
	if (((o_ptr->tval >= TV_DIGGING) && (o_ptr->tval <= TV_SWORD)) ||
	    (o_ptr->tval == TV_SHIELD) || (o_ptr->tval == TV_CAPTURE) ||
	    (o_ptr->tval == TV_CARD)) return (TRUE);

	/* Assume not wearable */
	return (FALSE);
}


static bool item_tester_hook_melee_weapon(object_type *o_ptr)
{
	/* Check for a usable slot */
	if ((o_ptr->tval >= TV_DIGGING) && (o_ptr->tval <= TV_SWORD))return (TRUE);

	/* Assume not wearable */
	return (FALSE);
}


/*
 * Wield or wear a single item from the pack or floor
 */
void do_cmd_wield(void)
{
	int i, item, slot;

	object_type forge;
	object_type *q_ptr;

	object_type *o_ptr;

	cptr act;

	char o_name[MAX_NLEN];

	cptr q, s;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	/* Restrict the choices */
	item_tester_hook = item_tester_hook_wear;

	/* Get an item */
#ifdef JP
	q = "どれを装備しますか? ";
	s = "装備可能なアイテムがない。";
#else
	q = "Wear/Wield which item? ";
	s = "You have nothing you can wear or wield.";
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


	/* Check the slot */
	slot = wield_slot(o_ptr);
#if 1 /* EASY_RING -- TNB */

	if ((o_ptr->tval == TV_RING) && inventory[INVEN_LEFT].k_idx &&
		inventory[INVEN_RIGHT].k_idx)
	{
		/* Restrict the choices */
		item_tester_tval = TV_RING;
		item_tester_no_ryoute = TRUE;

		/* Choose a ring from the equipment only */
#ifdef JP
q = "どちらの指輪と取り替えますか?";
#else
		q = "Replace which ring? ";
#endif

#ifdef JP
s = "おっと。";
#else
		s = "Oops.";
#endif

		if (!get_item(&slot, q, s, (USE_EQUIP)))
			return;
	}

#endif /* EASY_RING -- TNB */

	if (((o_ptr->tval == TV_SHIELD) || (o_ptr->tval == TV_CARD) || (o_ptr->tval == TV_CAPTURE)) &&
		buki_motteruka(INVEN_RARM) && buki_motteruka(INVEN_LARM))
	{
		/* Restrict the choices */
		item_tester_hook = item_tester_hook_melee_weapon;
		item_tester_no_ryoute = TRUE;

		/* Choose a weapon from the equipment only */
#ifdef JP
q = "どちらの武器と取り替えますか?";
#else
		q = "Replace which weapon? ";
#endif

#ifdef JP
s = "おっと。";
#else
		s = "Oops.";
#endif

		if (!get_item(&slot, q, s, (USE_EQUIP)))
			return;
		if (slot == INVEN_RARM)
		{
			object_type *or_ptr = &inventory[INVEN_RARM];
			object_type *ol_ptr = &inventory[INVEN_LARM];
			object_type *otmp_ptr;
			object_type object_tmp;
			char ol_name[MAX_NLEN];

			otmp_ptr = &object_tmp;

			object_desc(ol_name, ol_ptr, FALSE, 0);

			object_copy(otmp_ptr, ol_ptr);
			object_copy(ol_ptr, or_ptr);
			object_copy(or_ptr, otmp_ptr);
#ifdef JP
			msg_format("%sを右手に構えなおした。", ol_name);
#else
			msg_format("You wield %s at right hand.", ol_name);
#endif

			slot = INVEN_LARM;
		}
	}

	/* 二刀流にするかどうか */
	if ((o_ptr->tval >= TV_DIGGING) && (o_ptr->tval <= TV_SWORD) && (slot == INVEN_LARM))
	{
#ifdef JP
		if (!get_check("二刀流で戦いますか？"))
#else
		if (!get_check("Dual wielding? "))
#endif
		{
			slot = INVEN_RARM;
		}
	}

	if ((o_ptr->tval >= TV_DIGGING) && (o_ptr->tval <= TV_SWORD) &&
	    inventory[INVEN_LARM].k_idx &&
		inventory[INVEN_RARM].k_idx)
	{
		/* Restrict the choices */
		item_tester_hook = item_tester_hook_mochikae;

		/* Choose a ring from the equipment only */
#ifdef JP
q = "どちらの手に装備しますか?";
#else
		q = "Equip which hand? ";
#endif

#ifdef JP
s = "おっと。";
#else
		s = "Oops.";
#endif

		if (!get_item(&slot, q, s, (USE_EQUIP)))
			return;
	}

	/* Prevent wielding into a cursed slot */
	if (cursed_p(&inventory[slot]))
	{
		/* Describe it */
		object_desc(o_name, &inventory[slot], FALSE, 0);

		/* Message */
#ifdef JP
                msg_format("%s%sは呪われているようだ。",
                           describe_use(slot) , o_name );
#else
		msg_format("The %s you are %s appears to be cursed.",
		           o_name, describe_use(slot));
#endif


		/* Cancel the command */
		return;
	}

	if (cursed_p(o_ptr) && wear_confirm &&
	    (object_known_p(o_ptr) || (o_ptr->ident & IDENT_SENSE)))
	{
		char dummy[MAX_NLEN+80];

		/* Describe it */
		object_desc(o_name, o_ptr, FALSE, 0);

#ifdef JP
sprintf(dummy, "本当に%s{呪われている}を使いますか？", o_name);
#else
		sprintf(dummy, "Really use the %s {cursed}? ", o_name);
#endif


		if (!get_check(dummy))
			return;
	}

	if ((o_ptr->name1 == ART_STONEMASK) && object_known_p(o_ptr) && (p_ptr->prace != RACE_VAMPIRE) && (p_ptr->prace != RACE_ANDROID))
	{
		char dummy[MAX_NLEN+80];

		/* Describe it */
		object_desc(o_name, o_ptr, FALSE, 0);

#ifdef JP
sprintf(dummy, "%sを装備すると吸血鬼になります。よろしいですか？", o_name);
#else
		msg_format("%s will transforms you into a vampire permanently when equiped.", o_name);
		sprintf(dummy, "Do you become a vampire?");
#endif


		if (!get_check(dummy))
			return;
	}

	/* Check if completed a quest */
	for (i = 0; i < max_quests; i++)
	{
		if ((quest[i].type == QUEST_TYPE_FIND_ARTIFACT) &&
		    (quest[i].status == QUEST_STATUS_TAKEN) &&
		    (quest[i].k_idx == o_ptr->name1))
		{
			if (record_fix_quest) do_cmd_write_nikki(NIKKI_FIX_QUEST_C, i, NULL);
			quest[i].status = QUEST_STATUS_COMPLETED;
			quest[i].complev = (byte)p_ptr->lev;
#ifdef JP
msg_print("クエストを達成した！");
#else
			msg_print("You completed the quest!");
#endif

			msg_print(NULL);
		}
	}

	if (p_ptr->pseikaku == SEIKAKU_MUNCHKIN) identify_item(o_ptr);

	/* Take a turn */
	energy_use = 100;

	/* Get local object */
	q_ptr = &forge;

	/* Obtain local object */
	object_copy(q_ptr, o_ptr);

	/* Modify quantity */
	q_ptr->number = 1;

	/* Decrease the item (from the pack) */
	if (item >= 0)
	{
		inven_item_increase(item, -1);
		inven_item_optimize(item);
	}

	/* Decrease the item (from the floor) */
	else
	{
		floor_item_increase(0 - item, -1);
		floor_item_optimize(0 - item);
	}

	/* Access the wield slot */
	o_ptr = &inventory[slot];

	/* Take off existing item */
	if (o_ptr->k_idx)
	{
		/* Take off existing item */
		(void)inven_takeoff(slot, 255);
	}

	/* Wear the new stuff */
	object_copy(o_ptr, q_ptr);

	/* Increase the weight */
	p_ptr->total_weight += q_ptr->weight;

	/* Increment the equip counter by hand */
	equip_cnt++;

	/* Where is the item now */
	if (slot == INVEN_RARM)
	{
		if((o_ptr->tval != TV_SHIELD) && (o_ptr->tval != TV_CAPTURE) && (o_ptr->tval != TV_CARD) && (empty_hands(FALSE) & 0x00000001) && ((o_ptr->weight > 99) || (o_ptr->tval == TV_POLEARM)) && (!p_ptr->riding || (p_ptr->pet_extra_flags & PF_RYOUTE)))
#ifdef JP
			act = "を両手で構えた";
#else
			act = "You are wielding";
#endif
		else
#ifdef JP
			act = (left_hander ? "を左手に装備した" : "を右手に装備した");
#else
			act = "You are wielding";
#endif

	}
	else if (slot == INVEN_LARM)
	{
#ifdef JP
		act = (left_hander ? "を右手に装備した" : "を左手に装備した");
#else
		act = "You are wielding";
#endif

	}
	else if (slot == INVEN_BOW)
	{
#ifdef JP
		act = "を射撃用に装備した";
#else
		act = "You are shooting with";
#endif

	}
	else if (slot == INVEN_LITE)
	{
#ifdef JP
		act = "を光源にした";
#else
		act = "Your light source is";
#endif

	}
	else
	{
#ifdef JP
		act = "を装備した";
#else
		act = "You are wearing";
#endif

	}

	/* Describe the result */
	object_desc(o_name, o_ptr, TRUE, 3);

	/* Message */
#ifdef JP
        msg_format("%s(%c)%s。", o_name, index_to_label(slot), act );
#else
	msg_format("%s %s (%c).", act, o_name, index_to_label(slot));
#endif


	/* Cursed! */
	if (cursed_p(o_ptr))
	{
		/* Warn the player */
#ifdef JP
		msg_print("うわ！ すさまじく冷たい！");
#else
		msg_print("Oops! It feels deathly cold!");
#endif


		chg_virtue(V_HARMONY, -1);

		/* Note the curse */
		o_ptr->ident |= (IDENT_SENSE);
	}

	if ((o_ptr->name1 == ART_STONEMASK) && (p_ptr->prace != RACE_VAMPIRE) && (p_ptr->prace != RACE_ANDROID))
	{
		int h_percent;
		if (p_ptr->prace < 32)
		{
			p_ptr->old_race1 |= 1L << p_ptr->prace;
		}
		else
		{
			p_ptr->old_race2 = 1L << (p_ptr->prace-32);
		}
		p_ptr->prace = RACE_VAMPIRE;
#ifdef JP
		msg_format("あなたは吸血鬼に変化した！");
#else
		msg_format("You polymorphed into a vampire!");
#endif

		rp_ptr = &race_info[p_ptr->prace];

		/* Experience factor */
		p_ptr->expfact = rp_ptr->r_exp + cp_ptr->c_exp;

		/* Calculate the height/weight for males */
		if (p_ptr->psex == SEX_MALE)
		{
			p_ptr->ht = randnor(rp_ptr->m_b_ht, rp_ptr->m_m_ht);
			h_percent = (int)(p_ptr->ht) * 100 / (int)(rp_ptr->m_b_ht);
			p_ptr->wt = randnor((int)(rp_ptr->m_b_wt) * h_percent /100
					    , (int)(rp_ptr->m_m_wt) * h_percent / 300 );
		}

		/* Calculate the height/weight for females */
		else if (p_ptr->psex == SEX_FEMALE)
		{
			p_ptr->ht = randnor(rp_ptr->f_b_ht, rp_ptr->f_m_ht);
			h_percent = (int)(p_ptr->ht) * 100 / (int)(rp_ptr->f_b_ht);
			p_ptr->wt = randnor((int)(rp_ptr->f_b_wt) * h_percent /100
					    , (int)(rp_ptr->f_m_wt) * h_percent / 300 );
		}

		check_experience();

		/* Hitdice */
		if (p_ptr->pclass == CLASS_SORCERER)
			p_ptr->hitdie = rp_ptr->r_mhp/2 + cp_ptr->c_mhp + ap_ptr->a_mhp;
		else
			p_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;

		do_cmd_rerate(FALSE);

		p_ptr->redraw |= (PR_BASIC);

		p_ptr->update |= (PU_BONUS);

		handle_stuff();
		lite_spot(py, px);
	}

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Recalculate torch */
	p_ptr->update |= (PU_TORCH);

	/* Recalculate mana */
	p_ptr->update |= (PU_MANA);

	p_ptr->redraw |= (PR_EQUIPPY);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	calc_android_exp();
}


void kamaenaoshi(int item)
{
	object_type *o_ptr, *o2_ptr;
	char o_name[MAX_NLEN];

	if ((item == INVEN_RARM) && buki_motteruka(INVEN_LARM))
	{
		o_ptr = &inventory[INVEN_RARM];
		o2_ptr = &inventory[INVEN_LARM];
		object_copy(o_ptr, o2_ptr);
		p_ptr->total_weight += o2_ptr->weight;
		inven_item_increase(INVEN_LARM,-1);
		inven_item_optimize(INVEN_LARM);
		object_desc(o_name, o_ptr, TRUE, 3);
		if (((o_ptr->weight > 99) || (o_ptr->tval == TV_POLEARM)) && (!p_ptr->riding || (p_ptr->pet_extra_flags & PF_RYOUTE)))
#ifdef JP
			msg_format("%sを両手で構えた。", o_name );
#else
			msg_format("You are wielding %s with two-handed.", o_name );
#endif
		 else
#ifdef JP
			msg_format("%sを%sで構えた。", o_name, (left_hander ? "左手" : "右手"));
#else
			msg_format("You are wielding %s with %s hand.", o_name, (left_hander ? "left":"right") );
#endif
	}
	else if ((item == INVEN_LARM) && buki_motteruka(INVEN_RARM))
	{
		o_ptr = &inventory[INVEN_RARM];
		object_desc(o_name, o_ptr, TRUE, 3);
		if (((o_ptr->weight > 99) || (o_ptr->tval == TV_POLEARM)) && (!p_ptr->riding || (p_ptr->pet_extra_flags & PF_RYOUTE)))
#ifdef JP
			msg_format("%sを両手で構えた。", o_name );
#else
			msg_format("You are wielding %s with two-handed.", o_name );
#endif
	}
	else if ((item == INVEN_LARM) && !(empty_hands(FALSE) & 0x0002))
	{
		o_ptr = &inventory[INVEN_LARM];
		o2_ptr = &inventory[INVEN_RARM];
		object_copy(o_ptr, o2_ptr);
		p_ptr->total_weight += o2_ptr->weight;
		inven_item_increase(INVEN_RARM,-1);
		inven_item_optimize(INVEN_RARM);
		object_desc(o_name, o_ptr, TRUE, 3);
#ifdef JP
                msg_format("%sを持ち替えた。", o_name );
#else
                msg_format("You switched hand of %s.", o_name );
#endif
	}
}


/*
 * Take off an item
 */
void do_cmd_takeoff(void)
{
	int item;

	object_type *o_ptr;

	cptr q, s;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	item_tester_no_ryoute = TRUE;
	/* Get an item */
#ifdef JP
	q = "どれを装備からはずしますか? ";
	s = "はずせる装備がない。";
#else
	q = "Take off which item? ";
	s = "You are not wearing anything to take off.";
#endif

	if (!get_item(&item, q, s, (USE_EQUIP))) return;

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


	/* Item is cursed */
	if (cursed_p(o_ptr))
	{
		if ((o_ptr->curse_flags & TRC_PERMA_CURSE) || (p_ptr->pclass != CLASS_BERSERKER))
		{
			/* Oops */
#ifdef JP
			msg_print("ふーむ、どうやら呪われているようだ。");
#else
			msg_print("Hmmm, it seems to be cursed.");
#endif

			/* Nope */
			return;
		}

		if (((o_ptr->curse_flags & TRC_HEAVY_CURSE) && one_in_(7)) || one_in_(4))
		{
#ifdef JP
			msg_print("呪われた装備を力づくで剥がした！");
#else
			msg_print("You teared a cursed equipment off by sheer strength!");
#endif

			/* Hack -- Assume felt */
			o_ptr->ident |= (IDENT_SENSE);

			o_ptr->curse_flags = 0L;

			/* Take note */
			o_ptr->feeling = FEEL_NONE;

			/* Recalculate the bonuses */
			p_ptr->update |= (PU_BONUS);

			/* Window stuff */
			p_ptr->window |= (PW_EQUIP);

#ifdef JP
			msg_print("呪いを打ち破った。");
#else
			msg_print("You break the curse.");
#endif
		}
		else
		{
#ifdef JP
			msg_print("装備を外せなかった。");
#else
			msg_print("You couldn't remove the equipment.");
#endif
			energy_use = 50;
			return;
		}
	}

	/* Take a partial turn */
	energy_use = 50;

	/* Take off the item */
	(void)inven_takeoff(item, 255);

	kamaenaoshi(item);

	calc_android_exp();

	p_ptr->redraw |= (PR_EQUIPPY);
}


/*
 * Drop an item
 */
void do_cmd_drop(void)
{
	int item, amt = 1;

	object_type *o_ptr;

	cptr q, s;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	item_tester_no_ryoute = TRUE;
	/* Get an item */
#ifdef JP
	q = "どのアイテムを落としますか? ";
	s = "落とせるアイテムを持っていない。";
#else
	q = "Drop which item? ";
	s = "You have nothing to drop.";
#endif

	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN))) return;

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


	/* Hack -- Cannot remove cursed items */
	if ((item >= INVEN_RARM) && cursed_p(o_ptr))
	{
		/* Oops */
#ifdef JP
                msg_print("ふーむ、どうやら呪われているようだ。");
#else
		msg_print("Hmmm, it seems to be cursed.");
#endif


		/* Nope */
		return;
	}


	/* See how many items */
	if (o_ptr->number > 1)
	{
		/* Get a quantity */
		amt = get_quantity(NULL, o_ptr->number);

		/* Allow user abort */
		if (amt <= 0) return;
	}


	/* Take a partial turn */
	energy_use = 50;

	/* Drop (some of) the item */
	inven_drop(item, amt);

	if ((item == INVEN_RARM) || (item == INVEN_LARM)) kamaenaoshi(item);

	if (item >= INVEN_RARM) calc_android_exp();

	p_ptr->redraw |= (PR_EQUIPPY);
}


static bool high_level_book(object_type *o_ptr)
{
	if ((o_ptr->tval == TV_LIFE_BOOK) ||
	    (o_ptr->tval == TV_SORCERY_BOOK) ||
	    (o_ptr->tval == TV_NATURE_BOOK) ||
	    (o_ptr->tval == TV_CHAOS_BOOK) ||
	    (o_ptr->tval == TV_DEATH_BOOK) ||
	    (o_ptr->tval == TV_TRUMP_BOOK) ||
	    (o_ptr->tval == TV_ENCHANT_BOOK) ||
	    (o_ptr->tval == TV_DAEMON_BOOK) ||
	    (o_ptr->tval == TV_MUSIC_BOOK))
	{
		if (o_ptr->sval > 1)
			return TRUE;
		else
			return FALSE;
	}

	return FALSE;
}


/*
 * Destroy an item
 */
void do_cmd_destroy(void)
{
	int			item, amt = 1;
	int			old_number;

	bool		force = FALSE;

	object_type		*o_ptr;
	object_type             forge;
	object_type             *q_ptr = &forge;

	char		o_name[MAX_NLEN];

	char		out_val[MAX_NLEN+40];

	cptr q, s;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	/* Hack -- force destruction */
	if (command_arg > 0) force = TRUE;


	/* Get an item */
#ifdef JP
	q = "どのアイテムを壊しますか? ";
	s = "壊せるアイテムを持っていない。";
#else
	q = "Destroy which item? ";
	s = "You have nothing to destroy.";
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


	/* See how many items */
	if (o_ptr->number > 1)
	{
		/* Get a quantity */
		amt = get_quantity(NULL, o_ptr->number);

		/* Allow user abort */
		if (amt <= 0) return;
	}


	/* Describe the object */
	old_number = o_ptr->number;
	o_ptr->number = amt;
	object_desc(o_name, o_ptr, TRUE, 3);
	o_ptr->number = old_number;

	/* Verify unless quantity given */
	if (!force)
	{
		if (confirm_destroy || (object_value(o_ptr) > 0))
		{
			/* Make a verification */
#ifdef JP
		sprintf(out_val, "本当に%sを壊しますか? ", o_name);
#else
			sprintf(out_val, "Really destroy %s? ", o_name);
#endif

			if (!get_check(out_val)) return;
		}
	}

	/* Take a turn */
	energy_use = 100;

	/* Artifacts cannot be destroyed */
	if (artifact_p(o_ptr) || o_ptr->art_name)
	{
		byte feel = FEEL_SPECIAL;

		energy_use = 0;

		/* Message */
#ifdef JP
		msg_format("%sは破壊不可能だ。", o_name);
#else
		msg_format("You cannot destroy %s.", o_name);
#endif


		/* Hack -- Handle icky artifacts */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) feel = FEEL_TERRIBLE;

		/* Hack -- inscribe the artifact */
		o_ptr->feeling = feel;

		/* We have "felt" it (again) */
		o_ptr->ident |= (IDENT_SENSE);

		/* Combine the pack */
		p_ptr->notice |= (PN_COMBINE);

		p_ptr->redraw |= (PR_EQUIPPY);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Done */
		return;
	}

	object_copy(q_ptr, o_ptr);

	/* Message */
#ifdef JP
	msg_format("%sを壊した。", o_name);
#else
	msg_format("You destroy %s.", o_name);
#endif

	sound(SOUND_DESTITEM);

	/* Reduce the charges of rods/wands */
	reduce_charges(o_ptr, amt);

	/* Eliminate the item (from the pack) */
	if (item >= 0)
	{
		inven_item_increase(item, -amt);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Eliminate the item (from the floor) */
	else
	{
		floor_item_increase(0 - item, -amt);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}

	if (high_level_book(q_ptr))
	{
		bool gain_expr = FALSE;

		if (p_ptr->prace == RACE_ANDROID)
		{
		}
		else if ((p_ptr->pclass == CLASS_WARRIOR) || (p_ptr->pclass == CLASS_BERSERKER))
		{
			gain_expr = TRUE;
		}
		else if (p_ptr->pclass == CLASS_PALADIN)
		{
			if (p_ptr->realm1 == REALM_LIFE)
			{
				if (q_ptr->tval != TV_LIFE_BOOK) gain_expr = TRUE;
			}
			else
			{
				if (q_ptr->tval == TV_LIFE_BOOK) gain_expr = TRUE;
			}
		}

		if (gain_expr && (p_ptr->exp < PY_MAX_EXP))
		{
			s32b tester_exp = p_ptr->max_exp / 20;
			if (tester_exp > 10000) tester_exp = 10000;
			if (q_ptr->sval < 3) tester_exp /= 4;
			if (tester_exp<1) tester_exp = 1;

#ifdef JP
msg_print("更に経験を積んだような気がする。");
#else
			msg_print("You feel more experienced.");
#endif

			gain_exp(tester_exp * amt);
		}
		if (high_level_book(q_ptr) && q_ptr->tval == TV_LIFE_BOOK)
		{
			chg_virtue(V_UNLIFE, 1);
			chg_virtue(V_VITALITY, -1);
		}
		else if (high_level_book(q_ptr) && q_ptr->tval == TV_DEATH_BOOK)
		{
			chg_virtue(V_UNLIFE, -1);
			chg_virtue(V_VITALITY, 1);
		}
	
		if (q_ptr->to_a || q_ptr->to_h || q_ptr->to_d)
			chg_virtue(V_ENCHANT, -1);
	
		if (object_value_real(q_ptr) > 30000)
			chg_virtue(V_SACRIFICE, 2);
	
		else if (object_value_real(q_ptr) > 10000)
			chg_virtue(V_SACRIFICE, 1);
	}

	if (q_ptr->to_a != 0 || q_ptr->to_d != 0 || q_ptr->to_h != 0)
		chg_virtue(V_HARMONY, 1);

	if (item >= INVEN_RARM) calc_android_exp();
}


/*
 * Observe an item which has been *identify*-ed
 */
void do_cmd_observe(void)
{
	int			item;

	object_type		*o_ptr;

	char		o_name[MAX_NLEN];

	cptr q, s;

	item_tester_no_ryoute = TRUE;
	/* Get an item */
#ifdef JP
	q = "どのアイテムを調べますか? ";
	s = "調べられるアイテムがない。";
#else
	q = "Examine which item? ";
	s = "You have nothing to examine.";
#endif

	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return;

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


	/* Require full knowledge */
	if (!(o_ptr->ident & IDENT_MENTAL))
	{
#ifdef JP
		msg_print("このアイテムについて特に知っていることはない。");
#else
		msg_print("You have no special knowledge about that item.");
#endif

		return;
	}


	/* Description */
	object_desc(o_name, o_ptr, TRUE, 3);

	/* Describe */
#ifdef JP
	msg_format("%sを調べている...", o_name);
#else
	msg_format("Examining %s...", o_name);
#endif

	/* Describe it fully */
#ifdef JP
	if (!identify_fully_aux(o_ptr)) msg_print("特に変わったところはないようだ。");
#else
	if (!identify_fully_aux(o_ptr)) msg_print("You see nothing special.");
#endif

}



/*
 * Remove the inscription from an object
 * XXX Mention item (when done)?
 */
void do_cmd_uninscribe(void)
{
	int   item;

	object_type *o_ptr;

	cptr q, s;

	item_tester_no_ryoute = TRUE;
	/* Get an item */
#ifdef JP
	q = "どのアイテムの銘を消しますか? ";
	s = "銘を消せるアイテムがない。";
#else
	q = "Un-inscribe which item? ";
	s = "You have nothing to un-inscribe.";
#endif

	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return;

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

	/* Nothing to remove */
	if (!o_ptr->inscription)
	{
#ifdef JP
		msg_print("このアイテムには消すべき銘がない。");
#else
		msg_print("That item had no inscription to remove.");
#endif

		return;
	}

	/* Message */
#ifdef JP
	msg_print("銘を消した。");
#else
	msg_print("Inscription removed.");
#endif


	/* Remove the incription */
	o_ptr->inscription = 0;

	/* Combine the pack */
	p_ptr->notice |= (PN_COMBINE);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);
}

/*
 * Auto flag inscribe
 */

typedef struct flag_insc_table
{
#ifdef JP
	char *japanese;
#endif
	char *english;
	u32b flag;
	int num;
	u32b except_flag;
} flag_insc_table;

#ifdef JP
static flag_insc_table flag_insc_plus[] =
{
	{ "攻", "At", TR1_BLOWS, 1, 0 },
	{ "速", "Sp", TR1_SPEED, 1, 0 },
	{ "腕", "St", TR1_STR, 1, 0 },
	{ "知", "In", TR1_INT, 1, 0 },
	{ "賢", "Wi", TR1_WIS, 1, 0 },
	{ "器", "Dx", TR1_DEX, 1, 0 },
	{ "耐", "Cn", TR1_CON, 1, 0 },
	{ "魅", "Ch", TR1_CHR, 1, 0 },
	{ "隠", "Sl", TR1_STEALTH, 1, 0 },
	{ "探", "Sr", TR1_SEARCH, 1, 0 },
	{ "赤", "If", TR1_INFRA, 1, 0 },
	{ "掘", "Dg", TR1_TUNNEL, 1, 0 },
	{ NULL, 0, 0, 0 }
};

static flag_insc_table flag_insc_immune[] =
{
	{ "酸", "Ac", TR2_IM_ACID, 2, 0 },
	{ "電", "El", TR2_IM_ELEC, 2, 0 },
	{ "火", "Fi", TR2_IM_FIRE, 2, 0 },
	{ "冷", "Co", TR2_IM_COLD, 2, 0 },
	{ NULL, 0, 0, 0 }
};

static flag_insc_table flag_insc_resistance[] =
{
	{ "酸", "Ac", TR2_RES_ACID, 2, TR2_IM_ACID },
	{ "電", "El", TR2_RES_ELEC, 2, TR2_IM_ELEC },
	{ "火", "Fi", TR2_RES_FIRE, 2, TR2_IM_FIRE },
	{ "冷", "Co", TR2_RES_COLD, 2, TR2_IM_COLD },
	{ "毒", "Po", TR2_RES_POIS, 2, 0 },
	{ "閃", "Li", TR2_RES_LITE, 2, 0 },
	{ "暗", "Dk", TR2_RES_DARK, 2, 0 },
	{ "破", "Sh", TR2_RES_SHARDS, 2, 0 },
	{ "盲", "Bl", TR2_RES_BLIND, 2, 0 },
	{ "乱", "Cf", TR2_RES_CONF, 2, 0 },
	{ "轟", "So", TR2_RES_SOUND, 2, 0 },
	{ "獄", "Nt", TR2_RES_NETHER, 2, 0 },
	{ "因", "Nx", TR2_RES_NEXUS, 2, 0 },
	{ "沌", "Ca", TR2_RES_CHAOS, 2, 0 },
	{ "劣", "Di", TR2_RES_DISEN, 2, 0 },
	{ "恐", "Fe", TR2_RES_FEAR, 2, 0 },
	{ NULL, 0, 0, 0 }
};

static flag_insc_table flag_insc_misc[] =
{
	{ "魔力", "Ma", TR3_DEC_MANA, 3, 0 },
	{ "投", "Th", TR2_THROW, 2, 0 },
	{ "反", "Rf", TR2_REFLECT, 2, 0 },
	{ "麻", "Fa", TR2_FREE_ACT, 2, 0 },
	{ "視", "Si", TR3_SEE_INVIS, 3, 0 },
	{ "経", "Hl", TR2_HOLD_LIFE, 2, 0 },
	{ "感", "Esp", TR3_TELEPATHY, 3, 0 },
	{ "遅", "Sd", TR3_SLOW_DIGEST, 3, 0 },
	{ "活", "Rg", TR3_REGEN, 3, 0 },
	{ "浮", "Lv", TR3_FEATHER, 3, 0 },
	{ "明", "Lu", TR3_LITE, 3, 0 },
	{ "警", "Wr", TR3_WARNING, 3, 0 },
        { "倍", "Xm", TR3_XTRA_MIGHT, 3, 0 },
	{ "射", "Xs", TR3_XTRA_SHOTS, 3, 0 },
	{ "怒", "Ag", TR3_AGGRAVATE, 3, 0 },
	{ "祝", "Bs", TR3_BLESSED, 3, 0 },
#if 0
	{ "永呪", "Pc", TR3_PERMA_CURSE, 3, 0 },
	{ "呪", "Cu", TR3_HEAVY_CURSE, 3, TR3_PERMA_CURSE },
	{ "忌", "Ty", TR3_TY_CURSE, 3, 0 },
#endif
	{ NULL, 0, 0, 0 }
};

static flag_insc_table flag_insc_aura[] =
{
	{ "炎", "F", TR3_SH_FIRE, 3, 0 },
	{ "電", "E", TR3_SH_ELEC, 3, 0 },
	{ "冷", "C", TR3_SH_COLD, 3, 0 },
	{ "魔", "M", TR3_NO_MAGIC, 3, 0 },
	{ "瞬", "T", TR3_NO_TELE, 3, 0 },
	{ NULL, 0, 0, 0 }
};

static flag_insc_table flag_insc_brand[] =
{
	{ "酸", "A", TR1_BRAND_ACID, 1, 0 },
	{ "電", "E", TR1_BRAND_ELEC, 1, 0 },
	{ "焼", "F", TR1_BRAND_FIRE, 1, 0 },
	{ "凍", "Co", TR1_BRAND_COLD, 1, 0 },
	{ "毒", "P", TR1_BRAND_POIS, 1, 0 },
	{ "沌", "Ca", TR1_CHAOTIC, 1, 0 },
	{ "吸", "V", TR1_VAMPIRIC, 1, 0 },
	{ "震", "Q", TR1_IMPACT, 1, 0 },
	{ "切", "S", TR1_VORPAL, 1, 0 },
	{ "理", "M", TR1_FORCE_WEAPON, 1, 0 },
	{ NULL, NULL, 0, 0, 0 }
};

static flag_insc_table flag_insc_slay[] =
{
	{ "邪", "*", TR1_SLAY_EVIL, 1, 0 },
	{ "龍", "D", TR1_KILL_DRAGON, 1, 0 },
	{ "竜", "d", TR1_SLAY_DRAGON, 1, TR1_KILL_DRAGON },
	{ "オ", "o", TR1_SLAY_ORC, 1, 0 },
	{ "ト", "T", TR1_SLAY_TROLL, 1, 0 },
	{ "巨", "P", TR1_SLAY_GIANT, 1, 0 },
	{ "デ", "U", TR1_SLAY_DEMON, 1, 0 },
	{ "死", "L", TR1_SLAY_UNDEAD, 1, 0 },
	{ "動", "Z", TR1_SLAY_ANIMAL, 1, 0 },
	{ NULL, NULL, 0, 0, 0 }
};

static flag_insc_table flag_insc_sust[] =
{
	{ "腕", "St", TR2_SUST_STR, 2, 0 },
	{ "知", "In", TR2_SUST_INT, 2, 0 },
	{ "賢", "Wi", TR2_SUST_WIS, 2, 0 },
	{ "器", "Dx", TR2_SUST_DEX, 2, 0 },
	{ "耐", "Cn", TR2_SUST_CON, 2, 0 },
	{ "魅", "Ch", TR2_SUST_CHR, 2, 0 },
	{ NULL, NULL, 0, 0, 0 }
};

#else
static flag_insc_table flag_insc_plus[] =
{
  	{ "At", TR1_BLOWS, 1, 0 },
  	{ "Sp", TR1_SPEED, 1, 0 },
  	{ "St", TR1_STR, 1, 0 },
  	{ "In", TR1_INT, 1, 0 },
  	{ "Wi", TR1_WIS, 1, 0 },
  	{ "Dx", TR1_DEX, 1, 0 },
  	{ "Cn", TR1_CON, 1, 0 },
  	{ "Ch", TR1_CHR, 1, 0 },
  	{ "Sl", TR1_STEALTH, 1, 0 },
  	{ "Sr", TR1_SEARCH, 1, 0 },
  	{ "If", TR1_INFRA, 1, 0 },
  	{ "Dg", TR1_TUNNEL, 1, 0 },
  	{ NULL, 0, 0, 0 }
};

static flag_insc_table flag_insc_immune[] =
{
  	{ "Ac", TR2_IM_ACID, 2, 0 },
  	{ "El", TR2_IM_ELEC, 2, 0 },
  	{ "Fi", TR2_IM_FIRE, 2, 0 },
  	{ "Co", TR2_IM_COLD, 2, 0 },
  	{ NULL, 0, 0, 0 }
};

static flag_insc_table flag_insc_resistance[] =
{
  	{ "Ac", TR2_RES_ACID, 2, TR2_IM_ACID },
  	{ "El", TR2_RES_ELEC, 2, TR2_IM_ELEC },
  	{ "Fi", TR2_RES_FIRE, 2, TR2_IM_FIRE },
  	{ "Co", TR2_RES_COLD, 2, TR2_IM_COLD },
  	{ "Po", TR2_RES_POIS, 2, 0 },
  	{ "Li", TR2_RES_LITE, 2, 0 },
  	{ "Dk", TR2_RES_DARK, 2, 0 },
  	{ "Sh", TR2_RES_SHARDS, 2, 0 },
  	{ "Bl", TR2_RES_BLIND, 2, 0 },
  	{ "Cf", TR2_RES_CONF, 2, 0 },
  	{ "So", TR2_RES_SOUND, 2, 0 },
  	{ "Nt", TR2_RES_NETHER, 2, 0 },
  	{ "Nx", TR2_RES_NEXUS, 2, 0 },
  	{ "Ca", TR2_RES_CHAOS, 2, 0 },
  	{ "Di", TR2_RES_DISEN, 2, 0 },
  	{ "Fe", TR2_RES_FEAR, 2, 0 },
  	{ NULL, 0, 0, 0 }
};

static flag_insc_table flag_insc_misc[] =
{
  	{ "Ma", TR3_DEC_MANA, 3, 0 },
  	{ "Th", TR2_THROW, 2, 0 },
  	{ "Rf", TR2_REFLECT, 2, 0 },
  	{ "Fa", TR2_FREE_ACT, 2, 0 },
  	{ "Si", TR3_SEE_INVIS, 3, 0 },
  	{ "Hl", TR2_HOLD_LIFE, 2, 0 },
  	{ "Esp", TR3_TELEPATHY, 3, 0 },
  	{ "Sd", TR3_SLOW_DIGEST, 3, 0 },
  	{ "Rg", TR3_REGEN, 3, 0 },
  	{ "Lv", TR3_FEATHER, 3, 0 },
  	{ "Lu", TR3_LITE, 3, 0 },
	{ "Wr", TR3_WARNING, 3, 0 },
	{ "Xm", TR3_XTRA_MIGHT, 3, 0 },
  	{ "Xs", TR3_XTRA_SHOTS, 3, 0 },
  	{ "Ag", TR3_AGGRAVATE, 3, 0 },
  	{ "Bs", TR3_BLESSED, 3, 0 },
#if 0
  	{ "Pc", TR3_PERMA_CURSE, 3, 0 },
  	{ "Cu", TR3_HEAVY_CURSE, 3, TR3_PERMA_CURSE },
  	{ "Ty", TR3_TY_CURSE, 3, 0 },
#endif
#if 0
  	{ "De", TR3_DRAIN_EXP, 3, 0 },
#endif
  	{ NULL, 0, 0, 0 }
};

static flag_insc_table flag_insc_aura[] =
{
  	{ "F", TR3_SH_FIRE, 3, 0 },
  	{ "E", TR3_SH_ELEC, 3, 0 },
  	{ "C", TR3_SH_COLD, 3, 0 },
  	{ "M", TR3_NO_MAGIC, 3, 0 },
  	{ "T", TR3_NO_TELE, 3, 0 },
  	{ NULL, 0, 0, 0 }
};

static flag_insc_table flag_insc_brand[] =
{
  	{ "A", TR1_BRAND_ACID, 1, 0 },
  	{ "E", TR1_BRAND_ELEC, 1, 0 },
  	{ "F", TR1_BRAND_FIRE, 1, 0 },
  	{ "Co", TR1_BRAND_COLD, 1, 0 },
  	{ "P", TR1_BRAND_POIS, 1, 0 },
  	{ "Ca", TR1_CHAOTIC, 1, 0 },
  	{ "V", TR1_VAMPIRIC, 1, 0 },
  	{ "Q", TR1_IMPACT, 1, 0 },
  	{ "S", TR1_VORPAL, 1, 0 },
  	{ "M", TR1_FORCE_WEAPON, 1, 0 },
  	{ NULL, 0, 0, 0 }
};

static flag_insc_table flag_insc_slay[] =
{
  	{ "*", TR1_SLAY_EVIL, 1, 0 },
  	{ "D", TR1_KILL_DRAGON, 1, 0 },
  	{ "d", TR1_SLAY_DRAGON, 1, TR1_KILL_DRAGON },
  	{ "o", TR1_SLAY_ORC, 1, 0 },
  	{ "T", TR1_SLAY_TROLL, 1, 0 },
  	{ "P", TR1_SLAY_GIANT, 1, 0 },
  	{ "U", TR1_SLAY_DEMON, 1, 0 },
  	{ "L", TR1_SLAY_UNDEAD, 1, 0 },
  	{ "Z", TR1_SLAY_ANIMAL, 1, 0 },
  	{ NULL, 0, 0, 0 }
};

static flag_insc_table flag_insc_sust[] =
{
  	{ "St", TR2_SUST_STR, 2, 0 },
  	{ "In", TR2_SUST_INT, 2, 0 },
  	{ "Wi", TR2_SUST_WIS, 2, 0 },
  	{ "Dx", TR2_SUST_DEX, 2, 0 },
  	{ "Cn", TR2_SUST_CON, 2, 0 },
  	{ "Ch", TR2_SUST_CHR, 2, 0 },
  	{ NULL, 0, 0, 0 }
};
#endif

#define ADD_INSC(STR) (void)(strcat(ptr, (STR)), ptr += strlen(STR))

static char *inscribe_flags_aux(flag_insc_table *f_ptr, u32b flag[], bool kanji, char *ptr)
{
	while (f_ptr->num)
	{
		if ((flag[f_ptr->num-1] & f_ptr->flag) &&
		    !(flag[f_ptr->num-1] & f_ptr->except_flag))
#ifdef JP
			ADD_INSC(kanji ? f_ptr->japanese : f_ptr->english);
#else
			ADD_INSC(f_ptr->english);
#endif
		f_ptr ++;
	}

	return ptr;
}

static bool have_flag_of(flag_insc_table *f_ptr, u32b flag[])
{
	while (f_ptr->num)
	{
		if ((flag[f_ptr->num-1] & f_ptr->flag) &&
		    !(flag[f_ptr->num-1] & f_ptr->except_flag))
			return (TRUE);
		f_ptr++;
	}

	return (FALSE);
}

s16b inscribe_flags(object_type *o_ptr, cptr out_val)
{
	char buff[1024];
	char *ptr = buff;
	char *prev_ptr = buff;
	int i;

	bool kanji = FALSE;
	bool all = TRUE;
	u32b flag[3];

	/* not fully identified */
	if (!(o_ptr->ident & IDENT_MENTAL))
		return quark_add(out_val);

	/* Extract the flags */
	object_flags(o_ptr, &flag[0], &flag[1], &flag[2]);


	*buff = '\0';
	for (i = 0; out_val[i]; i++)
	{
		if ('%' == out_val[i] )
		{
			cptr start_percent = ptr;
#ifdef JP
			if ('%' == out_val[i+1])
			{
				i++;
				kanji = FALSE;
			}
			else
			{
				kanji = TRUE;
			}
#endif
			if ('a' == out_val[i+1] && 'l' == out_val[i+2] && 'l' == out_val[i+3])
			{
				all = TRUE;
				i += 3;
			}
			else
			{
				all = FALSE;
			}

			/* check for too long inscription */
			if (ptr >= buff + MAX_NLEN) continue;

			/* Remove obvious flags */
			if (!all)
			{
				object_kind *k_ptr = &k_info[o_ptr->k_idx];
				
				/* Base object */
				flag[0] &= ~k_ptr->flags1;
				flag[1] &= ~k_ptr->flags2;
				flag[2] &= ~k_ptr->flags3;

				if (o_ptr->name1)
				{
					artifact_type *a_ptr = &a_info[o_ptr->name1];
					
					flag[0] &= ~a_ptr->flags1;
					flag[1] &= ~a_ptr->flags2;
					flag[2] &= ~(a_ptr->flags3 & ~TR3_TELEPORT);
				}

				if (o_ptr->name2)
				{
					ego_item_type *e_ptr = &e_info[o_ptr->name2];
					
					flag[0] &= ~e_ptr->flags1;
					flag[1] &= ~e_ptr->flags2;
					flag[2] &= ~(e_ptr->flags3 & ~TR3_TELEPORT);
				}
			}


			/* Plusses */
			if (have_flag_of(flag_insc_plus, flag))
			{
				if (kanji)
					ADD_INSC("+");
			}
			ptr = inscribe_flags_aux(flag_insc_plus, flag, kanji, ptr);

			/* Immunity */
			if (have_flag_of(flag_insc_immune, flag))
			{
				if (!kanji && ptr != prev_ptr)
				{
					ADD_INSC(";");
					prev_ptr = ptr;
				}
				ADD_INSC("*");
			}
			ptr = inscribe_flags_aux(flag_insc_immune, flag, kanji, ptr);

			/* Resistance */
			if (have_flag_of(flag_insc_resistance, flag))
			{
				if (kanji)
					ADD_INSC("r");
				else if (ptr != prev_ptr)
				{
					ADD_INSC(";");
					prev_ptr = ptr;
				}
			}
			ptr = inscribe_flags_aux(flag_insc_resistance, flag, kanji, ptr);

			/* Misc Ability */
			if (have_flag_of(flag_insc_misc, flag))
			{
				if (ptr != prev_ptr)
				{
					ADD_INSC(";");
					prev_ptr = ptr;
				}
			}
			ptr = inscribe_flags_aux(flag_insc_misc, flag, kanji, ptr);

			/* Aura */
			if (have_flag_of(flag_insc_aura, flag))
			{
				ADD_INSC("[");
			}
			ptr = inscribe_flags_aux(flag_insc_aura, flag, kanji, ptr);

			/* Brand Weapon */
			if (have_flag_of(flag_insc_brand, flag))
				ADD_INSC("|");
			ptr = inscribe_flags_aux(flag_insc_brand, flag, kanji, ptr);

			/* Slay Weapon */
			if (have_flag_of(flag_insc_slay, flag))
				ADD_INSC("/");
			ptr = inscribe_flags_aux(flag_insc_slay, flag, kanji, ptr);

			/* Random Teleport */
			if (flag[2] & (TR3_TELEPORT))
			{
				ADD_INSC(".");
			}

			/* sustain */
			if (have_flag_of(flag_insc_sust, flag))
			{
				ADD_INSC("(");
			}
			ptr = inscribe_flags_aux(flag_insc_sust, flag, kanji, ptr);

			if (ptr == start_percent)
				ADD_INSC(" ");
		}
		else
		{
			*ptr++ = out_val[i];
			*ptr = '\0';
		}
	}

	/* cut too long inscription */
	if (strlen(buff) >= MAX_NLEN-1)
	{
#ifdef JP
		int n;
		for (n = 0; n < MAX_NLEN; n++)
			if(iskanji(buff[n])) n++;
		if (n == MAX_NLEN) n = MAX_NLEN-2; /* 最後が漢字半分 */
		buff[n] = '\0';
#else
		buff[MAX_NLEN-1] = '\0';
#endif
	}
	return quark_add(buff);
}

/*
 * Inscribe an object with a comment
 */
void do_cmd_inscribe(void)
{
	int			item;

	object_type		*o_ptr;

	char		o_name[MAX_NLEN];

	char		out_val[80];

	cptr q, s;

	item_tester_no_ryoute = TRUE;
	/* Get an item */
#ifdef JP
	q = "どのアイテムに銘を刻みますか? ";
	s = "銘を刻めるアイテムがない。";
#else
	q = "Inscribe which item? ";
	s = "You have nothing to inscribe.";
#endif

	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return;

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

	/* Describe the activity */
	object_desc(o_name, o_ptr, TRUE, 2);

	/* Message */
#ifdef JP
	msg_format("%sに銘を刻む。", o_name);
#else
	msg_format("Inscribing %s.", o_name);
#endif

	msg_print(NULL);

	/* Start with nothing */
	strcpy(out_val, "");

	/* Use old inscription */
	if (o_ptr->inscription)
	{
		/* Start with the old inscription */
		strcpy(out_val, quark_str(o_ptr->inscription));
	}

	/* Get a new inscription (possibly empty) */
#ifdef JP
        if (get_string("銘: ", out_val, 80))
#else
	if (get_string("Inscription: ", out_val, 80))
#endif
	{
		/* Save the inscription */
		o_ptr->inscription = inscribe_flags(o_ptr, out_val);

		/* Combine the pack */
		p_ptr->notice |= (PN_COMBINE);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);
	}
}



/*
 * An "item_tester_hook" for refilling lanterns
 */
static bool item_tester_refill_lantern(object_type *o_ptr)
{
	/* Flasks of oil are okay */
	if (o_ptr->tval == TV_FLASK) return (TRUE);

	/* Laterns are okay */
	if ((o_ptr->tval == TV_LITE) &&
	    (o_ptr->sval == SV_LITE_LANTERN)) return (TRUE);

	/* Assume not okay */
	return (FALSE);
}


/*
 * Refill the players lamp (from the pack or floor)
 */
static void do_cmd_refill_lamp(void)
{
	int item;

	object_type *o_ptr;
	object_type *j_ptr;

	cptr q, s;


	/* Restrict the choices */
	item_tester_hook = item_tester_refill_lantern;

	/* Get an item */
#ifdef JP
	q = "どの油つぼから注ぎますか? ";
	s = "油つぼがない。";
#else
	q = "Refill with which flask? ";
	s = "You have no flasks of oil.";
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


	/* Take a partial turn */
	energy_use = 50;

	/* Access the lantern */
	j_ptr = &inventory[INVEN_LITE];

	/* Refuel */
	j_ptr->xtra4 += o_ptr->xtra4;

	/* Message */
#ifdef JP
	msg_print("ランプに油を注いだ。");
#else
	msg_print("You fuel your lamp.");
#endif

	/* Comment */
	if ((o_ptr->name2 == EGO_LITE_DARKNESS) && (j_ptr->xtra4 > 0))
	{
		j_ptr->xtra4 = 0;
#ifdef JP
		msg_print("ランプが消えてしまった！");
#else
		msg_print("Your lamp has gone out!");
#endif
	}
	else if ((o_ptr->name2 == EGO_LITE_DARKNESS) || (j_ptr->name2 == EGO_LITE_DARKNESS))
	{
		j_ptr->xtra4 = 0;
#ifdef JP
		msg_print("しかしランプは全く光らない。");
#else
		msg_print("Curiously, your lamp doesn't light.");
#endif
	}
	else if (j_ptr->xtra4 >= FUEL_LAMP)
	{
		j_ptr->xtra4 = FUEL_LAMP;
#ifdef JP
		msg_print("ランプの油は一杯だ。");
#else
		msg_print("Your lamp is full.");
#endif

	}

	/* Decrease the item (from the pack) */
	if (item >= 0)
	{
		inven_item_increase(item, -1);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Decrease the item (from the floor) */
	else
	{
		floor_item_increase(0 - item, -1);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}

	/* Recalculate torch */
	p_ptr->update |= (PU_TORCH);
}


/*
 * An "item_tester_hook" for refilling torches
 */
static bool item_tester_refill_torch(object_type *o_ptr)
{
	/* Torches are okay */
	if ((o_ptr->tval == TV_LITE) &&
	    (o_ptr->sval == SV_LITE_TORCH)) return (TRUE);

	/* Assume not okay */
	return (FALSE);
}


/*
 * Refuel the players torch (from the pack or floor)
 */
static void do_cmd_refill_torch(void)
{
	int item;

	object_type *o_ptr;
	object_type *j_ptr;

	cptr q, s;


	/* Restrict the choices */
	item_tester_hook = item_tester_refill_torch;

	/* Get an item */
#ifdef JP
	q = "どの松明で明かりを強めますか? ";
	s = "他に松明がない。";
#else
	q = "Refuel with which torch? ";
	s = "You have no extra torches.";
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


	/* Take a partial turn */
	energy_use = 50;

	/* Access the primary torch */
	j_ptr = &inventory[INVEN_LITE];

	/* Refuel */
	j_ptr->xtra4 += o_ptr->xtra4 + 5;

	/* Message */
#ifdef JP
	msg_print("松明を結合した。");
#else
	msg_print("You combine the torches.");
#endif


	/* Comment */
	if ((o_ptr->name2 == EGO_LITE_DARKNESS) && (j_ptr->xtra4 > 0))
	{
		j_ptr->xtra4 = 0;
#ifdef JP
		msg_print("松明が消えてしまった！");
#else
		msg_print("Your torch has gone out!");
#endif
	}
	else if ((o_ptr->name2 == EGO_LITE_DARKNESS) || (j_ptr->name2 == EGO_LITE_DARKNESS))
	{
		j_ptr->xtra4 = 0;
#ifdef JP
		msg_print("しかし松明は全く光らない。");
#else
		msg_print("Curiously, your torche don't light.");
#endif
	}
	/* Over-fuel message */
	else if (j_ptr->xtra4 >= FUEL_TORCH)
	{
		j_ptr->xtra4 = FUEL_TORCH;
#ifdef JP
		msg_print("松明の寿命は十分だ。");
#else
		msg_print("Your torch is fully fueled.");
#endif

	}

	/* Refuel message */
	else
	{
#ifdef JP
		msg_print("松明はいっそう明るく輝いた。");
#else
		msg_print("Your torch glows more brightly.");
#endif

	}

	/* Decrease the item (from the pack) */
	if (item >= 0)
	{
		inven_item_increase(item, -1);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Decrease the item (from the floor) */
	else
	{
		floor_item_increase(0 - item, -1);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}

	/* Recalculate torch */
	p_ptr->update |= (PU_TORCH);
}


/*
 * Refill the players lamp, or restock his torches
 */
void do_cmd_refill(void)
{
	object_type *o_ptr;

	/* Get the light */
	o_ptr = &inventory[INVEN_LITE];

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	/* It is nothing */
	if (o_ptr->tval != TV_LITE)
	{
#ifdef JP
		msg_print("光源を装備していない。");
#else
		msg_print("You are not wielding a light.");
#endif

	}

	/* It's a lamp */
	else if (o_ptr->sval == SV_LITE_LANTERN)
	{
		do_cmd_refill_lamp();
	}

	/* It's a torch */
	else if (o_ptr->sval == SV_LITE_TORCH)
	{
		do_cmd_refill_torch();
	}

	/* No torch to refill */
	else
	{
#ifdef JP
		msg_print("この光源は寿命を延ばせない。");
#else
		msg_print("Your light cannot be refilled.");
#endif

	}
}


/*
 * Target command
 */
void do_cmd_target(void)
{
	/* Target set */
	if (target_set(TARGET_KILL))
	{
#ifdef JP
		msg_print("ターゲット決定。");
#else
		msg_print("Target Selected.");
#endif

	}

	/* Target aborted */
	else
	{
#ifdef JP
		msg_print("ターゲット解除。");
#else
		msg_print("Target Aborted.");
#endif

	}
}



/*
 * Look command
 */
void do_cmd_look(void)
{
	/* Look around */
	if (target_set(TARGET_LOOK))
	{
#ifdef JP
		msg_print("ターゲット決定。");
#else
		msg_print("Target Selected.");
#endif

	}
}



/*
 * Allow the player to examine other sectors on the map
 */
void do_cmd_locate(void)
{
	int		dir, y1, x1, y2, x2;

	char	tmp_val[80];

	char	out_val[160];

	int wid, hgt;

	/* Get size */
	get_screen_size(&wid, &hgt);


	/* Start at current panel */
	y2 = y1 = panel_row_min;
	x2 = x1 = panel_col_min;

	/* Show panels until done */
	while (1)
	{
		/* Describe the location */
		if ((y2 == y1) && (x2 == x1))
		{
#ifdef JP
                        strcpy(tmp_val, "真上");
#else
			tmp_val[0] = '\0';
#endif

		}
		else
		{
#ifdef JP
			sprintf(tmp_val, "%s%s",
			        ((y2 < y1) ? "北" : (y2 > y1) ? "南" : ""),
			        ((x2 < x1) ? "西" : (x2 > x1) ? "東" : ""));
#else
			sprintf(tmp_val, "%s%s of",
			        ((y2 < y1) ? " North" : (y2 > y1) ? " South" : ""),
			        ((x2 < x1) ? " West" : (x2 > x1) ? " East" : ""));
#endif

		}

		/* Prepare to ask which way to look */
		sprintf(out_val,
#ifdef JP
                        "マップ位置 [%d(%02d),%d(%02d)] (プレイヤーの%s)  方向?",
#else
		        "Map sector [%d(%02d),%d(%02d)], which is%s your sector.  Direction?",
#endif

		        y2 / (hgt / 2), y2 % (hgt / 2),
		        x2 / (wid / 2), x2 % (wid / 2), tmp_val);

		/* Assume no direction */
		dir = 0;

		/* Get a direction */
		while (!dir)
		{
			char command;

			/* Get a command (or Cancel) */
			if (!get_com(out_val, &command, TRUE)) break;

			/* Extract the action (if any) */
			dir = get_keymap_dir(command);

			/* Error */
			if (!dir) bell();
		}

		/* No direction */
		if (!dir) break;

		/* Apply the motion */
		if (change_panel(ddy[dir], ddx[dir]))
		{
			y2 = panel_row_min;
			x2 = panel_col_min;
		}
	}


	/* Recenter the map around the player */
	verify_panel();

	/* Update stuff */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	/* Handle stuff */
	handle_stuff();
}



/*
 * The table of "symbol info" -- each entry is a string of the form
 * "X:desc" where "X" is the trigger, and "desc" is the "info".
 */
static cptr ident_info[] =
{
#ifdef JP
	" :暗闇",
	"!:薬, オイル",
	"\":アミュレット, 頸飾り",
	"#:壁(隠しドア)又は植物",
	"$:財宝(金か宝石)",
	"%:鉱脈(溶岩か石英)",
	"&:箱",
	"':開いたドア",
	"(:軟らかい防具",
	"):盾",
	"*:財宝を含んだ鉱脈または球形の怪物",
	"+:閉じたドア",
	",:食べ物, おばけキノコ",
	"-:魔法棒, ロッド",
	".:床",
	"/:竿状武器(アックス/パイク/等)",
	"0:博物館の入口",
	"1:雑貨屋の入口",
	"2:防具屋の入口",
	"3:武器専門店の入口",
	"4:寺院の入口",
	"5:錬金術の店の入口",
	"6:魔法の店の入口",
	"7:ブラックマーケットの入口",
	"8:我が家の入口",
	"9:書店の入口",
	"::岩石",
	";:回避の絵文字/爆発のルーン",
	"<:上り階段",
	"=:指輪",
	">:下り階段",
	"?:巻物",
	"@:プレイヤー",
	"A:天使",
	"B:鳥",
	"C:犬",
	"D:古代ドラゴン/ワイアーム",
	"E:エレメンタル",
	"F:トンボ",
	"G:ゴースト",
	"H:雑種",
	"I:昆虫",
	"J:ヘビ",
	"K:キラー・ビートル",
	"L:リッチ",
	"M:多首の爬虫類",
	"N:謎の生物",
	"O:オーガ",
	"P:巨大人間型生物",
	"Q:クイルスルグ(脈打つ肉塊)",
	"R:爬虫類/両生類",
	"S:蜘蛛/サソリ/ダニ",
	"T:トロル",
	"U:上級デーモン",
	"V:バンパイア",
	"W:ワイト/レイス/等",
	"X:ゾーン/ザレン/等",
	"Y:イエティ",
	"Z:ハウンド",
	"[:堅いアーマー",
	"\\:鈍器(メイス/ムチ/等)",
	"]:種々の防具",
	"^:トラップ",
	"_:杖",
	"`:人形，彫像",
	"a:アリ",
	"b:コウモリ",
	"c:ムカデ",
	"d:ドラゴン",
	"e:目玉",
	"f:ネコ",
	"g:ゴーレム",
	"h:ホビット/エルフ/ドワーフ",
	"i:ベトベト",
	"j:ゼリー",
	"k:コボルド",
	"l:水棲生物",
	"m:モルド",
	"n:ナーガ",
	"o:オーク",
	"p:人間",
	"q:四足獣",
	"r:ネズミ",
	"s:スケルトン",
	"t:町の人",
	"u:下級デーモン",
	"v:ボルテックス",
	"w:イモムシ/大群",
	/* "x:unused", */
	"y:イーク",
	"z:ゾンビ/ミイラ",
	"{:飛び道具の弾(矢/弾)",
	"|:刀剣類(ソード/ダガー/等)",
	"}:飛び道具(弓/クロスボウ/スリング)",
	"~:水/溶岩流(種々のアイテム)",
#else
	" :A dark grid",
	"!:A potion (or oil)",
	"\":An amulet (or necklace)",
	"#:A wall (or secret door)",
	"$:Treasure (gold or gems)",
	"%:A vein (magma or quartz)",
	"&:A chest",
	"':An open door",
	"(:Soft armor",
	"):A shield",
	"*:A vein with treasure or a ball monster",
	"+:A closed door",
	",:Food (or mushroom patch)",
	"-:A wand (or rod)",
	".:Floor",
	"/:A polearm (Axe/Pike/etc)",
	"0:Entrance to Museum",
	"1:Entrance to General Store",
	"2:Entrance to Armory",
	"3:Entrance to Weaponsmith",
	"4:Entrance to Temple",
	"5:Entrance to Alchemy shop",
	"6:Entrance to Magic store",
	"7:Entrance to Black Market",
	"8:Entrance to your home",
	"9:Entrance to the bookstore",
	"::Rubble",
	";:A glyph of warding / explosive rune",
	"<:An up staircase",
	"=:A ring",
	">:A down staircase",
	"?:A scroll",
	"@:You",
	"A:Angel",
	"B:Bird",
	"C:Canine",
	"D:Ancient Dragon/Wyrm",
	"E:Elemental",
	"F:Dragon Fly",
	"G:Ghost",
	"H:Hybrid",
	"I:Insect",
	"J:Snake",
	"K:Killer Beetle",
	"L:Lich",
	"M:Multi-Headed Reptile",
	"N:Mystery Living",
	"O:Ogre",
	"P:Giant Humanoid",
	"Q:Quylthulg (Pulsing Flesh Mound)",
	"R:Reptile/Amphibian",
	"S:Spider/Scorpion/Tick",
	"T:Troll",
	"U:Major Demon",
	"V:Vampire",
	"W:Wight/Wraith/etc",
	"X:Xorn/Xaren/etc",
	"Y:Yeti",
	"Z:Zephyr Hound",
	"[:Hard armor",
	"\\:A hafted weapon (mace/whip/etc)",
	"]:Misc. armor",
	"^:A trap",
	"_:A staff",
	"`:A figurine or statue",
	"a:Ant",
	"b:Bat",
	"c:Centipede",
	"d:Dragon",
	"e:Floating Eye",
	"f:Feline",
	"g:Golem",
	"h:Hobbit/Elf/Dwarf",
	"i:Icky Thing",
	"j:Jelly",
	"k:Kobold",
	"l:Aquatic monster",
	"m:Mold",
	"n:Naga",
	"o:Orc",
	"p:Person/Human",
	"q:Quadruped",
	"r:Rodent",
	"s:Skeleton",
	"t:Townsperson",
	"u:Minor Demon",
	"v:Vortex",
	"w:Worm/Worm-Mass",
	/* "x:unused", */
	"y:Yeek",
	"z:Zombie/Mummy",
	"{:A missile (arrow/bolt/shot)",
	"|:An edged weapon (sword/dagger/etc)",
	"}:A launcher (bow/crossbow/sling)",
	"~:Fluid terrain (or miscellaneous item)",
#endif

	NULL
};


/*
 * Sorting hook -- Comp function -- see below
 *
 * We use "u" to point to array of monster indexes,
 * and "v" to select the type of sorting to perform on "u".
 */
bool ang_sort_comp_hook(vptr u, vptr v, int a, int b)
{
	u16b *who = (u16b*)(u);

	u16b *why = (u16b*)(v);

	int w1 = who[a];
	int w2 = who[b];

	int z1, z2;


	/* Sort by player kills */
	if (*why >= 4)
	{
		/* Extract player kills */
		z1 = r_info[w1].r_pkills;
		z2 = r_info[w2].r_pkills;

		/* Compare player kills */
		if (z1 < z2) return (TRUE);
		if (z1 > z2) return (FALSE);
	}


	/* Sort by total kills */
	if (*why >= 3)
	{
		/* Extract total kills */
		z1 = r_info[w1].r_tkills;
		z2 = r_info[w2].r_tkills;

		/* Compare total kills */
		if (z1 < z2) return (TRUE);
		if (z1 > z2) return (FALSE);
	}


	/* Sort by monster level */
	if (*why >= 2)
	{
		/* Extract levels */
		z1 = r_info[w1].level;
		z2 = r_info[w2].level;

		/* Compare levels */
		if (z1 < z2) return (TRUE);
		if (z1 > z2) return (FALSE);
	}


	/* Sort by monster experience */
	if (*why >= 1)
	{
		/* Extract experience */
		z1 = r_info[w1].mexp;
		z2 = r_info[w2].mexp;

		/* Compare experience */
		if (z1 < z2) return (TRUE);
		if (z1 > z2) return (FALSE);
	}


	/* Compare indexes */
	return (w1 <= w2);
}


/*
 * Sorting hook -- Swap function -- see below
 *
 * We use "u" to point to array of monster indexes,
 * and "v" to select the type of sorting to perform.
 */
void ang_sort_swap_hook(vptr u, vptr v, int a, int b)
{
	u16b *who = (u16b*)(u);

	u16b holder;

	/* Swap */
	holder = who[a];
	who[a] = who[b];
	who[b] = holder;
}



/*
 * Identify a character, allow recall of monsters
 *
 * Several "special" responses recall "multiple" monsters:
 *   ^A (all monsters)
 *   ^U (all unique monsters)
 *   ^N (all non-unique monsters)
 *
 * The responses may be sorted in several ways, see below.
 *
 * Note that the player ghosts are ignored. XXX XXX XXX
 */
void do_cmd_query_symbol(void)
{
	int		i, n, r_idx;
	char	sym, query;
	char	buf[128];

	bool	all = FALSE;
	bool	uniq = FALSE;
	bool	norm = FALSE;
	char    temp[80] = "";

	bool	recall = FALSE;

	u16b	why = 0;
	u16b	*who;

	/* Get a character, or abort */
#ifdef JP
	if (!get_com("知りたい文字を入力して下さい(記号 or ^A全,^Uユ,^N非ユ,^M名前): ", &sym, FALSE)) return;
#else
	if (!get_com("Enter character to be identified(^A:All,^U:Uniqs,^N:Non uniqs,^M:Name): ", &sym, FALSE)) return;
#endif


	/* Find that character info, and describe it */
	for (i = 0; ident_info[i]; ++i)
	{
		if (sym == ident_info[i][0]) break;
	}

	/* Describe */
	if (sym == KTRL('A'))
	{
		all = TRUE;
#ifdef JP
		strcpy(buf, "全モンスターのリスト");
#else
		strcpy(buf, "Full monster list.");
#endif

	}
	else if (sym == KTRL('U'))
	{
		all = uniq = TRUE;
#ifdef JP
		strcpy(buf, "ユニーク・モンスターのリスト");
#else
		strcpy(buf, "Unique monster list.");
#endif

	}
	else if (sym == KTRL('N'))
	{
		all = norm = TRUE;
#ifdef JP
		strcpy(buf, "ユニーク外モンスターのリスト");
#else
		strcpy(buf, "Non-unique monster list.");
#endif

	}
	/* XTRA HACK WHATSEARCH */
	else if (sym == KTRL('M'))
	{
		all = TRUE;
#ifdef JP
		if (!get_string("名前(英語の場合小文字で可)",temp, 70))
#else
		if (!get_string("Enter name:",temp, 70))
#endif
		{
			temp[0]=0;
			return;
		}
#ifdef JP
		sprintf(buf, "名前:%sにマッチ",temp);
#else
		sprintf(buf, "Monsters with a name \"%s\"",temp);
#endif
	}
	else if (ident_info[i])
	{
		sprintf(buf, "%c - %s.", sym, ident_info[i] + 2);
	}
	else
	{
#ifdef JP
		sprintf(buf, "%c - %s", sym, "無効な文字");
#else
		sprintf(buf, "%c - %s.", sym, "Unknown Symbol");
#endif

	}

	/* Display the result */
	prt(buf, 0, 0);

	/* Allocate the "who" array */
	C_MAKE(who, max_r_idx, u16b);

	/* Collect matching monsters */
	for (n = 0, i = 1; i < max_r_idx; i++)
	{
		monster_race *r_ptr = &r_info[i];

		/* Nothing to recall */
		if (!cheat_know && !r_ptr->r_sights) continue;

		/* Require non-unique monsters if needed */
		if (norm && (r_ptr->flags1 & (RF1_UNIQUE))) continue;

		/* Require unique monsters if needed */
		if (uniq && !(r_ptr->flags1 & (RF1_UNIQUE))) continue;

		/* XTRA HACK WHATSEARCH */
		if (temp[0]){
		  int xx;
		  char temp2[80];
  
		  for (xx=0; temp[xx] && xx<80; xx++){
#ifdef JP
		    if (iskanji( temp[xx])) { xx++; continue; }
#endif
		    if (isupper(temp[xx])) temp[xx]=tolower(temp[xx]);
		  }
  
#ifdef JP
		  strcpy(temp2, r_name+r_ptr->E_name);
#else
		  strcpy(temp2, r_name+r_ptr->name);
#endif
		  for (xx=0; temp2[xx] && xx<80; xx++)
		    if (isupper(temp2[xx])) temp2[xx]=tolower(temp2[xx]);
  
#ifdef JP
		  if (strstr(temp2, temp) || strstr_j(r_name + r_ptr->name, temp) )
#else
		  if (strstr(temp2, temp))
#endif
			  who[n++]=i;
		}else
		/* Collect "appropriate" monsters */
		if (all || (r_ptr->d_char == sym)) who[n++] = i;
	}

	/* Nothing to recall */
	if (!n)
	{
		/* Free the "who" array */
		C_KILL(who, max_r_idx, u16b);

		return;
	}


	/* Prompt XXX XXX XXX */
#ifdef JP
	put_str("思い出を見ますか? (k:殺害順/y/n): ", 0, 36);
#else
	put_str("Recall details? (k/y/n): ", 0, 40);
#endif


	/* Query */
	query = inkey();

	/* Restore */
	prt(buf, 0, 0);

	why = 2;

	/* Select the sort method */
	ang_sort_comp = ang_sort_comp_hook;
	ang_sort_swap = ang_sort_swap_hook;

	/* Sort the array */
	ang_sort(who, &why, n);

	/* Sort by kills (and level) */
	if (query == 'k')
	{
		why = 4;
		query = 'y';
	}

	/* Catch "escape" */
	if (query != 'y')
	{
		/* Free the "who" array */
		C_KILL(who, max_r_idx, u16b);

		return;
	}

	/* Sort if needed */
	if (why == 4)
	{
		/* Select the sort method */
		ang_sort_comp = ang_sort_comp_hook;
		ang_sort_swap = ang_sort_swap_hook;

		/* Sort the array */
		ang_sort(who, &why, n);
	}


	/* Start at the end */
	i = n - 1;

	/* Scan the monster memory */
	while (1)
	{
		/* Extract a race */
		r_idx = who[i];

		/* Hack -- Auto-recall */
		monster_race_track(FALSE, r_idx);

		/* Hack -- Handle stuff */
		handle_stuff();

		/* Hack -- Begin the prompt */
		roff_top(r_idx);

		/* Hack -- Complete the prompt */
#ifdef JP
		Term_addstr(-1, TERM_WHITE, " ['r'思い出, ESC]");
#else
		Term_addstr(-1, TERM_WHITE, " [(r)ecall, ESC]");
#endif


		/* Interact */
		while (1)
		{
			/* Recall */
			if (recall)
			{
				/* Save the screen */
				screen_save();

				/* Recall on screen */
				screen_roff(who[i], 0);

				/* Hack -- Complete the prompt (again) */
#ifdef JP
				Term_addstr(-1, TERM_WHITE, " ['r'思い出, ESC]");
#else
				Term_addstr(-1, TERM_WHITE, " [(r)ecall, ESC]");
#endif

			}

			/* Command */
			query = inkey();

			/* Unrecall */
			if (recall)
			{
				/* Restore */
				screen_load();
			}

			/* Normal commands */
			if (query != 'r') break;

			/* Toggle recall */
			recall = !recall;
		}

		/* Stop scanning */
		if (query == ESCAPE) break;

		/* Move to "prev" monster */
		if (query == '-')
		{
			if (++i == n)
			{
				i = 0;
				if (!expand_list) break;
			}
		}

		/* Move to "next" monster */
		else
		{
			if (i-- == 0)
			{
				i = n - 1;
				if (!expand_list) break;
			}
		}
	}

	/* Free the "who" array */
	C_KILL(who, max_r_idx, u16b);

	/* Re-display the identity */
	prt(buf, 0, 0);
}


/*
 *  research_mon
 *  -KMW-
 */
bool research_mon(void)
{
	int i, n, r_idx;
	char sym, query;
	char buf[128];

	s16b oldkills;
	byte oldwake;
	bool oldcheat;

	bool notpicked;

	bool recall = FALSE;

	u16b why = 0;

	monster_race *r2_ptr;

	u16b	*who;

	/* XTRA HACK WHATSEARCH */
	bool    all = FALSE;
	bool    uniq = FALSE;
	bool    norm = FALSE;
	char temp[80] = "";

	/* XTRA HACK REMEMBER_IDX */
	static int old_sym = '\0';
	static int old_i = 0;

	oldcheat = cheat_know;


	/* Save the screen */
	screen_save();

	/* Get a character, or abort */
#ifdef JP
if (!get_com("モンスターの文字を入力して下さい(記号 or ^A全,^Uユ,^N非ユ,^M名前):", &sym, FALSE)) 
#else
	if (!get_com("Enter character to be identified(^A:All,^U:Uniqs,^N:Non uniqs,^M:Name): ", &sym, FALSE))
#endif

	{
		/* Restore */
		screen_load();

		return (FALSE);
	}

	/* Find that character info, and describe it */
	for (i = 0; ident_info[i]; ++i)
	{
		if (sym == ident_info[i][0]) break;
	}

		/* XTRA HACK WHATSEARCH */
	if (sym == KTRL('A'))
	{
		all = TRUE;
#ifdef JP
		strcpy(buf, "全モンスターのリスト");
#else
		strcpy(buf, "Full monster list.");
#endif
	}
	else if (sym == KTRL('U'))
	{
		all = uniq = TRUE;
#ifdef JP
		strcpy(buf, "ユニーク・モンスターのリスト");
#else
		strcpy(buf, "Unique monster list.");
#endif
	}
	else if (sym == KTRL('N'))
	{
		all = norm = TRUE;
#ifdef JP
		strcpy(buf, "ユニーク外モンスターのリスト");
#else
		strcpy(buf, "Non-unique monster list.");
#endif
	}
        else if (sym == KTRL('M'))
	{
		all = TRUE;
#ifdef JP
		if (!get_string("名前(英語の場合小文字で可)",temp, 70))
#else
		if (!get_string("Enter name:",temp, 70))
#endif
		{
			temp[0]=0;
			return FALSE;
		}
#ifdef JP
		sprintf(buf, "名前:%sにマッチ",temp);
#else
		sprintf(buf, "Monsters with a name \"%s\"",temp);
#endif
	}
	else if (ident_info[i])
	{
		sprintf(buf, "%c - %s.", sym, ident_info[i] + 2);
	}
  	else
  	{
#ifdef JP
sprintf(buf, "%c - %s", sym, "無効な文字");
#else
		sprintf(buf, "%c - %s.", sym, "Unknown Symbol");
#endif

	}

	/* Display the result */
	prt(buf, 16, 10);


	/* Allocate the "who" array */
	C_MAKE(who, max_r_idx, u16b);

	/* Collect matching monsters */
	for (n = 0, i = 1; i < max_r_idx; i++)
	{
		monster_race *r_ptr = &r_info[i];

		cheat_know = TRUE;

		/* XTRA HACK WHATSEARCH */
		/* Require non-unique monsters if needed */
		if (norm && (r_ptr->flags1 & (RF1_UNIQUE))) continue;

		/* Require unique monsters if needed */
		if (uniq && !(r_ptr->flags1 & (RF1_UNIQUE))) continue;

		/* 名前検索 */
		if (temp[0]){
		  int xx;
		  char temp2[80];
  
		  for (xx=0; temp[xx] && xx<80; xx++){
#ifdef JP
		    if (iskanji( temp[xx])) { xx++; continue; }
#endif
		    if (isupper(temp[xx])) temp[xx]=tolower(temp[xx]);
		  }
  
#ifdef JP
		  strcpy(temp2, r_name+r_ptr->E_name);
#else
		  strcpy(temp2, r_name+r_ptr->name);
#endif
		  for (xx=0; temp2[xx] && xx<80; xx++)
		    if (isupper(temp2[xx])) temp2[xx]=tolower(temp2[xx]);
  
#ifdef JP
		  if (strstr(temp2, temp) || strstr_j(r_name + r_ptr->name, temp) )
#else
		  if (strstr(temp2, temp))
#endif
			  who[n++]=i;
		}
		else if (all || (r_ptr->d_char == sym)) who[n++] = i;
	}

	/* Nothing to recall */
	if (!n)
	{
		cheat_know = oldcheat;

		/* Free the "who" array */
		C_KILL(who, max_r_idx, u16b);

		/* Restore */
		screen_load();

		return (FALSE);
	}

	/* Sort by level */
	why = 2;
	query = 'y';

	/* Sort if needed */
	if (why)
	{
		/* Select the sort method */
		ang_sort_comp = ang_sort_comp_hook;
		ang_sort_swap = ang_sort_swap_hook;

		/* Sort the array */
		ang_sort(who, &why, n);
	}


	/* Start at the end */
	/* XTRA HACK REMEMBER_IDX */
	if (old_sym == sym && old_i < n) i = old_i;
	else i = n - 1;

	notpicked = TRUE;

	/* Scan the monster memory */
	while (notpicked)
	{
		/* Extract a race */
		r_idx = who[i];

		/* Save this monster ID */
		p_ptr->monster_race_idx = r_idx;

		/* Hack -- Handle stuff */
		handle_stuff();

		/* Hack -- Begin the prompt */
		roff_top(r_idx);

		/* Hack -- Complete the prompt */
#ifdef JP
Term_addstr(-1, TERM_WHITE, " ['r'思い出, ' 'で続行, ESC]");
#else
		Term_addstr(-1, TERM_WHITE, " [(r)ecall, ESC, space to continue]");
#endif


		/* Interact */
		while (1)
		{
			/* Recall */
			if (recall)
			{
				/* Recall on screen */
				r2_ptr = &r_info[r_idx];

				oldkills = r2_ptr->r_tkills;
				oldwake = r2_ptr->r_wake;
				screen_roff(who[i], 1);
				r2_ptr->r_tkills = oldkills;
				r2_ptr->r_wake = oldwake;
				cheat_know = oldcheat;
				notpicked = FALSE;

				/* XTRA HACK REMEMBER_IDX */
				old_sym = sym;
				old_i = i;
			}

			/* Command */
			query = inkey();

			/* Normal commands */
			if (query != 'r') break;

			/* Toggle recall */
			recall = !recall;
		}

		/* Stop scanning */
		if (query == ESCAPE) break;

		/* Move to "prev" monster */
		if (query == '-')
		{
			if (++i == n)
			{
				i = 0;
				if (!expand_list) break;
			}
		}

		/* Move to "next" monster */
		else
		{
			if (i-- == 0)
			{
				i = n - 1;
				if (!expand_list) break;
			}
		}
	}


	/* Re-display the identity */
	/* prt(buf, 5, 5);*/

	cheat_know = oldcheat;

	/* Free the "who" array */
	C_KILL(who, max_r_idx, u16b);

	/* Restore */
	screen_load();

	return (!notpicked);
}

