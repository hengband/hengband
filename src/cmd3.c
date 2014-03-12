/*!
 *  @file cmd3.c
 *  @brief プレイヤーのアイテムに関するコマンドの実装1 / Inventory commands
 *  @date 2014/01/02
 *  @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */


#include "angband.h"



/*!
 * @brief 持ち物一覧を表示するコマンドのメインルーチン / Display inventory
 * @return なし 
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
	    (int)lbtokg1(p_ptr->total_weight) , (int)lbtokg2(p_ptr->total_weight) ,
	    (long int)((p_ptr->total_weight * 100) / weight_limit()));
#else
	sprintf(out_val, "Inventory: carrying %d.%d pounds (%ld%% of capacity). Command: ",
	    (int)(p_ptr->total_weight / 10), (int)(p_ptr->total_weight % 10),
	    (p_ptr->total_weight * 100) / weight_limit());
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


/*!
 * @brief 装備一覧を表示するコマンドのメインルーチン / Display equipment
 * @return なし 
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
	    (int)lbtokg1(p_ptr->total_weight) , (int)lbtokg2(p_ptr->total_weight) ,
	    (long int)((p_ptr->total_weight * 100) / weight_limit()));
#else
	sprintf(out_val, "Equipment: carrying %d.%d pounds (%ld%% of capacity). Command: ",
	    (int)(p_ptr->total_weight / 10), (int)(p_ptr->total_weight % 10),
	    (long int)((p_ptr->total_weight * 100) / weight_limit()));
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


/*!
 * @brief オブジェクトを防具として装備できるかの判定 / The "wearable" tester
 * @param o_ptr 判定するオブジェクトの構造体参照ポインタ
 * @return オブジェクトが防具として装備できるならTRUEを返す。
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


/*!
 * @brief オブジェクトがどちらの手にも装備できる武器かどうかの判定
 * @param o_ptr 判定するオブジェクトの構造体参照ポインタ
 * @return 左右両方の手で装備できるならばTRUEを返す。
 */
static bool item_tester_hook_mochikae(object_type *o_ptr)
{
	/* Check for a usable slot */
	if (((o_ptr->tval >= TV_DIGGING) && (o_ptr->tval <= TV_SWORD)) ||
	    (o_ptr->tval == TV_SHIELD) || (o_ptr->tval == TV_CAPTURE) ||
	    (o_ptr->tval == TV_CARD)) return (TRUE);

	/* Assume not wearable */
	return (FALSE);
}

/*!
 * @brief オブジェクトが右手か左手に装備できる武器かどうかの判定
 * @param o_ptr 判定するオブジェクトの構造体参照ポインタ
 * @return 右手か左手の武器として装備できるならばTRUEを返す。
 */
static bool item_tester_hook_melee_weapon(object_type *o_ptr)
{
	/* Check for a usable slot */
	if ((o_ptr->tval >= TV_DIGGING) && (o_ptr->tval <= TV_SWORD))return (TRUE);

	/* Assume not wearable */
	return (FALSE);
}


bool select_ring_slot = FALSE;

/*!
 * @brief 装備するコマンドのメインルーチン / Wield or wear a single item from the pack or floor
 * @return なし 
 */
void do_cmd_wield(void)
{
	int item, slot;

	object_type forge;
	object_type *q_ptr;

	object_type *o_ptr;

	cptr act;

	char o_name[MAX_NLEN];

	cptr q, s;

	int need_switch_wielding = 0;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	/* Restrict the choices */
	item_tester_hook = item_tester_hook_wear;

	/* Get an item */
	q = _("どれを装備しますか? ", "Wear/Wield which item? ");
	s = _("装備可能なアイテムがない。", "You have nothing you can wear or wield.");

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

	switch (o_ptr->tval)
	{
	/* Shields and some misc. items */
	case TV_CAPTURE:
	case TV_SHIELD:
	case TV_CARD:
		/* Dual wielding */
		if (buki_motteruka(INVEN_RARM) && buki_motteruka(INVEN_LARM))
		{
			/* Restrict the choices */
			item_tester_hook = item_tester_hook_melee_weapon;
			item_tester_no_ryoute = TRUE;

			/* Choose a weapon from the equipment only */
			q = _("どちらの武器と取り替えますか?", "Replace which weapon? ");
			s = _("おっと。", "Oops.");
			if (!get_item(&slot, q, s, (USE_EQUIP))) return;
			if (slot == INVEN_RARM) need_switch_wielding = INVEN_LARM;
		}

		else if (buki_motteruka(INVEN_LARM)) slot = INVEN_RARM;

		/* Both arms are already used by non-weapon */
		else if (inventory[INVEN_RARM].k_idx && !object_is_melee_weapon(&inventory[INVEN_RARM]) &&
		         inventory[INVEN_LARM].k_idx && !object_is_melee_weapon(&inventory[INVEN_LARM]))
		{
			/* Restrict the choices */
			item_tester_hook = item_tester_hook_mochikae;

			/* Choose a hand */
			q = _("どちらの手に装備しますか?", "Equip which hand? ");
			s = _("おっと。", "Oops.");
			if (!get_item(&slot, q, s, (USE_EQUIP))) return;
		}
		break;

	/* Melee weapons */
	case TV_DIGGING:
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_SWORD:
		/* Asking for dual wielding */
		if (slot == INVEN_LARM)
		{
			if (!get_check(_("二刀流で戦いますか？", "Dual wielding? "))) slot = INVEN_RARM;
		}

		else if (!inventory[INVEN_RARM].k_idx && buki_motteruka(INVEN_LARM))
		{
			if (!get_check(_("二刀流で戦いますか？", "Dual wielding? "))) slot = INVEN_LARM;
		}

		/* Both arms are already used */
		else if (inventory[INVEN_LARM].k_idx && inventory[INVEN_RARM].k_idx)
		{
			/* Restrict the choices */
			item_tester_hook = item_tester_hook_mochikae;

			/* Choose a hand */
			q = _("どちらの手に装備しますか?", "Equip which hand? ");
			s = _("おっと。", "Oops.");
			
			if (!get_item(&slot, q, s, (USE_EQUIP))) return;
			if ((slot == INVEN_LARM) && !buki_motteruka(INVEN_RARM))
				need_switch_wielding = INVEN_RARM;
		}
		break;

	/* Rings */
	case TV_RING:
		/* Choose a ring slot */
		if (inventory[INVEN_LEFT].k_idx && inventory[INVEN_RIGHT].k_idx)
		{
			q = _("どちらの指輪と取り替えますか?", "Replace which ring? ");
		}
		else
		{
			q = _("どちらの手に装備しますか?", "Equip which hand? ");
		}
		s = _("おっと。", "Oops.");

		/* Restrict the choices */
		select_ring_slot = TRUE;
		item_tester_no_ryoute = TRUE;

		if (!get_item(&slot, q, s, (USE_EQUIP)))
		{
			select_ring_slot = FALSE;
			return;
		}
		select_ring_slot = FALSE;
		break;
	}

	/* Prevent wielding into a cursed slot */
	if (object_is_cursed(&inventory[slot]))
	{
		/* Describe it */
		object_desc(o_name, &inventory[slot], (OD_OMIT_PREFIX | OD_NAME_ONLY));

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

	if (confirm_wear &&
		((object_is_cursed(o_ptr) && object_is_known(o_ptr)) ||
		((o_ptr->ident & IDENT_SENSE) &&
			(FEEL_BROKEN <= o_ptr->feeling) && (o_ptr->feeling <= FEEL_CURSED))))
	{
		char dummy[MAX_NLEN+80];

		/* Describe it */
		object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
		sprintf(dummy, _("本当に%s{呪われている}を使いますか？", "Really use the %s {cursed}? "), o_name);

		if (!get_check(dummy)) return;
	}

	if ((o_ptr->name1 == ART_STONEMASK) && object_is_known(o_ptr) && (p_ptr->prace != RACE_VAMPIRE) && (p_ptr->prace != RACE_ANDROID))
	{
		char dummy[MAX_NLEN+80];

		/* Describe it */
		object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

#ifdef JP
		sprintf(dummy, "%sを装備すると吸血鬼になります。よろしいですか？", o_name);
#else
		msg_format("%s will transforms you into a vampire permanently when equiped.", o_name);
		sprintf(dummy, "Do you become a vampire?");
#endif

		if (!get_check(dummy)) return;
	}

	if (need_switch_wielding && !object_is_cursed(&inventory[need_switch_wielding]))
	{
		object_type *slot_o_ptr = &inventory[slot];
		object_type *switch_o_ptr = &inventory[need_switch_wielding];
		object_type object_tmp;
		object_type *otmp_ptr = &object_tmp;
		char switch_name[MAX_NLEN];

		object_desc(switch_name, switch_o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

		object_copy(otmp_ptr, switch_o_ptr);
		object_copy(switch_o_ptr, slot_o_ptr);
		object_copy(slot_o_ptr, otmp_ptr);
		
		msg_format(_("%sを%sに構えなおした。", "You wield %s at %s hand."), switch_name, 
					(slot == INVEN_RARM) ? (left_hander ? _("左手", "left") : _("右手", "right")) : 
										   (left_hander ? _("右手", "right") : _("左手", "left")));
		slot = need_switch_wielding;
	}

	check_find_art_quest_completion(o_ptr);

	if (p_ptr->pseikaku == SEIKAKU_MUNCHKIN)
	{
		identify_item(o_ptr);

		/* Auto-inscription */
		autopick_alter_item(item, FALSE);
	}

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

	/* Player touches it */
	o_ptr->marked |= OM_TOUCHED;

	/* Increase the weight */
	p_ptr->total_weight += q_ptr->weight;

	/* Increment the equip counter by hand */
	equip_cnt++;

#ifdef JP
#define STR_WIELD_RARM "%s(%c)を右手に装備した。"
#define STR_WIELD_LARM "%s(%c)を左手に装備した。"
#define STR_WIELD_ARMS "%s(%c)を両手で構えた。"
#else
#define STR_WIELD_RARM "You are wielding %s (%c) in your right hand."
#define STR_WIELD_LARM "You are wielding %s (%c) in your left hand."
#define STR_WIELD_ARMS "You are wielding %s (%c) with both hands."
#endif

	/* Where is the item now */
	switch (slot)
	{
	case INVEN_RARM:
		if (object_allow_two_hands_wielding(o_ptr) && (empty_hands(FALSE) == EMPTY_HAND_LARM) && CAN_TWO_HANDS_WIELDING())
			act = STR_WIELD_ARMS;
		else
			act = (left_hander ? STR_WIELD_LARM : STR_WIELD_RARM);
		break;

	case INVEN_LARM:
		if (object_allow_two_hands_wielding(o_ptr) && (empty_hands(FALSE) == EMPTY_HAND_RARM) && CAN_TWO_HANDS_WIELDING())
			act = STR_WIELD_ARMS;
		else
			act = (left_hander ? STR_WIELD_RARM : STR_WIELD_LARM);
		break;

	case INVEN_BOW:
		act = _("%s(%c)を射撃用に装備した。", "You are shooting with %s (%c).");
		break;

	case INVEN_LITE:
		act = _("%s(%c)を光源にした。", "Your light source is %s (%c).");
		break;

	default:
		act = _("%s(%c)を装備した。", "You are wearing %s (%c).");
		break;
	}

	/* Describe the result */
	object_desc(o_name, o_ptr, 0);

	/* Message */
	msg_format(act, o_name, index_to_label(slot));


	/* Cursed! */
	if (object_is_cursed(o_ptr))
	{
		/* Warn the player */
		msg_print(_("うわ！ すさまじく冷たい！", "Oops! It feels deathly cold!"));
		chg_virtue(V_HARMONY, -1);

		/* Note the curse */
		o_ptr->ident |= (IDENT_SENSE);
	}

	/* The Stone Mask make the player turn into a vampire! */
	if ((o_ptr->name1 == ART_STONEMASK) && (p_ptr->prace != RACE_VAMPIRE) && (p_ptr->prace != RACE_ANDROID))
	{
		/* Turn into a vampire */
		change_race(RACE_VAMPIRE, "");
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

/*!
 * @brief 持ち替え処理
 * @param item 持ち替えを行いたい装備部位ID
 * @return なし
 */
void kamaenaoshi(int item)
{
	object_type *o_ptr, *new_o_ptr;
	char o_name[MAX_NLEN];

	if (item == INVEN_RARM)
	{
		if (buki_motteruka(INVEN_LARM))
		{
			o_ptr = &inventory[INVEN_LARM];
			object_desc(o_name, o_ptr, 0);

			if (!object_is_cursed(o_ptr))
			{
				new_o_ptr = &inventory[INVEN_RARM];
				object_copy(new_o_ptr, o_ptr);
				p_ptr->total_weight += o_ptr->weight;
				inven_item_increase(INVEN_LARM, -((int)o_ptr->number));
				inven_item_optimize(INVEN_LARM);
				if (object_allow_two_hands_wielding(o_ptr) && CAN_TWO_HANDS_WIELDING())
					msg_format(_("%sを両手で構えた。", "You are wielding %s with both hands."), o_name);
				 else
				msg_format(_("%sを%sで構えた。", "You are wielding %s in your %s hand."), o_name, 
							(left_hander ? _("左手", "left") : _("右手", "right")));
			}
			else
			{
				if (object_allow_two_hands_wielding(o_ptr) && CAN_TWO_HANDS_WIELDING())
					msg_format(_("%sを両手で構えた。", "You are wielding %s with both hands."), o_name);
			}
		}
	}
	else if (item == INVEN_LARM)
	{
		o_ptr = &inventory[INVEN_RARM];
		if (o_ptr->k_idx) object_desc(o_name, o_ptr, 0);

		if (buki_motteruka(INVEN_RARM))
		{
			if (object_allow_two_hands_wielding(o_ptr) && CAN_TWO_HANDS_WIELDING())
				msg_format(_("%sを両手で構えた。", "You are wielding %s with both hands."), o_name);
		}
		else if (!(empty_hands(FALSE) & EMPTY_HAND_RARM) && !object_is_cursed(o_ptr))
		{
			new_o_ptr = &inventory[INVEN_LARM];
			object_copy(new_o_ptr, o_ptr);
			p_ptr->total_weight += o_ptr->weight;
			inven_item_increase(INVEN_RARM, -((int)o_ptr->number));
			inven_item_optimize(INVEN_RARM);
			msg_format(_("%sを持ち替えた。", "You switched hand of %s."), o_name);
		}
	}
}


/*!
 * @brief 装備を外すコマンドのメインルーチン / Take off an item
 * @return なし
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
	if (object_is_cursed(o_ptr))
	{
		if ((o_ptr->curse_flags & TRC_PERMA_CURSE) || (p_ptr->pclass != CLASS_BERSERKER))
		{
			/* Oops */
			msg_print(_("ふーむ、どうやら呪われているようだ。", "Hmmm, it seems to be cursed."));

			/* Nope */
			return;
		}

		if (((o_ptr->curse_flags & TRC_HEAVY_CURSE) && one_in_(7)) || one_in_(4))
		{
			msg_print(_("呪われた装備を力づくで剥がした！", "You teared a cursed equipment off by sheer strength!"));

			/* Hack -- Assume felt */
			o_ptr->ident |= (IDENT_SENSE);

			o_ptr->curse_flags = 0L;

			/* Take note */
			o_ptr->feeling = FEEL_NONE;

			/* Recalculate the bonuses */
			p_ptr->update |= (PU_BONUS);

			/* Window stuff */
			p_ptr->window |= (PW_EQUIP);

			msg_print(_("呪いを打ち破った。", "You break the curse."));
		}
		else
		{
			msg_print(_("装備を外せなかった。", "You couldn't remove the equipment."));
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


/*!
 * @brief アイテムを落とすコマンドのメインルーチン / Drop an item
 * @return なし
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
	if ((item >= INVEN_RARM) && object_is_cursed(o_ptr))
	{
		/* Oops */
		msg_print(_("ふーむ、どうやら呪われているようだ。", "Hmmm, it seems to be cursed."));
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

	if (item >= INVEN_RARM)
	{
		kamaenaoshi(item);
		calc_android_exp();
	}

	p_ptr->redraw |= (PR_EQUIPPY);
}

/*!
 * @brief オブジェクトが高位の魔法書かどうかを判定する
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトが高位の魔法書ならばTRUEを返す
 */
static bool high_level_book(object_type *o_ptr)
{
	if ((o_ptr->tval == TV_LIFE_BOOK) ||
	    (o_ptr->tval == TV_SORCERY_BOOK) ||
	    (o_ptr->tval == TV_NATURE_BOOK) ||
	    (o_ptr->tval == TV_CHAOS_BOOK) ||
	    (o_ptr->tval == TV_DEATH_BOOK) ||
	    (o_ptr->tval == TV_TRUMP_BOOK) ||
	    (o_ptr->tval == TV_CRAFT_BOOK) ||
	    (o_ptr->tval == TV_DAEMON_BOOK) ||
	    (o_ptr->tval == TV_CRUSADE_BOOK) ||
	    (o_ptr->tval == TV_MUSIC_BOOK) ||
		(o_ptr->tval == TV_HEX_BOOK))
	{
		if (o_ptr->sval > 1)
			return TRUE;
		else
			return FALSE;
	}

	return FALSE;
}


/*!
 * @brief アイテムを破壊するコマンドのメインルーチン / Destroy an item
 * @return なし
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

	/* Verify unless quantity given beforehand */
	if (!force && (confirm_destroy || (object_value(o_ptr) > 0)))
	{
		object_desc(o_name, o_ptr, OD_OMIT_PREFIX);

		/* Make a verification */
		sprintf(out_val, _("本当に%sを壊しますか? [y/n/Auto]", "Really destroy %s? [y/n/Auto]"), o_name);
		msg_print(NULL);

		/* HACK : Add the line to message buffer */
		message_add(out_val);
		p_ptr->window |= (PW_MESSAGE);
		window_stuff();

		/* Get an acceptable answer */
		while (TRUE)
		{
			char i;

			/* Prompt */
			prt(out_val, 0, 0);

			i = inkey();

			/* Erase the prompt */
			prt("", 0, 0);


			if (i == 'y' || i == 'Y')
			{
				break;
			}
			if (i == ESCAPE || i == 'n' || i == 'N')
			{
				/* Cancel */
				return;
			}
			if (i == 'A')
			{
				/* Add an auto-destroy preference line */
				if (autopick_autoregister(o_ptr))
				{
					/* Auto-destroy it */
					autopick_alter_item(item, TRUE);
				}

				/* The object is already destroyed. */
				return;
			}
		} /* while (TRUE) */
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
	object_desc(o_name, o_ptr, 0);
	o_ptr->number = old_number;

	/* Take a turn */
	energy_use = 100;

	/* Artifacts cannot be destroyed */
	if (!can_player_destroy_object(o_ptr))
	{
		energy_use = 0;

		/* Message */
		msg_format(_("%sは破壊不可能だ。", "You cannot destroy %s."), o_name);
		/* Done */
		return;
	}

	object_copy(q_ptr, o_ptr);

	/* Message */
	msg_format(_("%sを壊した。", "You destroy %s."), o_name);
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
			if (is_good_realm(p_ptr->realm1))
			{
				if (!is_good_realm(tval2realm(q_ptr->tval))) gain_expr = TRUE;
			}
			else
			{
				if (is_good_realm(tval2realm(q_ptr->tval))) gain_expr = TRUE;
			}
		}

		if (gain_expr && (p_ptr->exp < PY_MAX_EXP))
		{
			s32b tester_exp = p_ptr->max_exp / 20;
			if (tester_exp > 10000) tester_exp = 10000;
			if (q_ptr->sval < 3) tester_exp /= 4;
			if (tester_exp<1) tester_exp = 1;

			msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));
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


/*!
 * @brief アイテムを調査するコマンドのメインルーチン / Observe an item which has been *identify*-ed
 * @return なし
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
		msg_print(_("このアイテムについて特に知っていることはない。", "You have no special knowledge about that item."));
		return;
	}


	/* Description */
	object_desc(o_name, o_ptr, 0);

	/* Describe */
	msg_format(_("%sを調べている...", "Examining %s..."), o_name);
	/* Describe it fully */
	if (!screen_object(o_ptr, SCROBJ_FORCE_DETAIL)) msg_print(_("特に変わったところはないようだ。", "You see nothing special."));
}



/*!
 * @brief アイテムの銘を消すコマンドのメインルーチン
 * Remove the inscription from an object XXX Mention item (when done)?
 * @return なし
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
		msg_print(_("このアイテムには消すべき銘がない。", "That item had no inscription to remove."));
		return;
	}

	/* Message */
	msg_print(_("銘を消した。", "Inscription removed."));

	/* Remove the incription */
	o_ptr->inscription = 0;

	/* Combine the pack */
	p_ptr->notice |= (PN_COMBINE);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

	/* .や$の関係で, 再計算が必要なはず -- henkma */
	p_ptr->update |= (PU_BONUS);

}


/*!
 * @brief アイテムの銘を刻むコマンドのメインルーチン
 * Inscribe an object with a comment
 * @return なし
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
	object_desc(o_name, o_ptr, OD_OMIT_INSCRIPTION);

	/* Message */
	msg_format(_("%sに銘を刻む。", "Inscribing %s."), o_name);
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
	if (get_string(_("銘: ", "Inscription: "), out_val, 80))
	{
		/* Save the inscription */
		o_ptr->inscription = quark_add(out_val);

		/* Combine the pack */
		p_ptr->notice |= (PN_COMBINE);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* .や$の関係で, 再計算が必要なはず -- henkma */
		p_ptr->update |= (PU_BONUS);
	}
}



/*!
 * @brief オブジェクトがランタンの燃料になるかどうかを判定する
 * An "item_tester_hook" for refilling lanterns
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトがランタンの燃料になるならばTRUEを返す
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


/*!
 * @brief ランタンに燃料を加えるコマンドのメインルーチン
 * Refill the players lamp (from the pack or floor)
 * @return なし
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
	msg_print(_("ランプに油を注いだ。", "You fuel your lamp."));

	/* Comment */
	if ((o_ptr->name2 == EGO_LITE_DARKNESS) && (j_ptr->xtra4 > 0))
	{
		j_ptr->xtra4 = 0;
		msg_print(_("ランプが消えてしまった！", "Your lamp has gone out!"));
	}
	else if ((o_ptr->name2 == EGO_LITE_DARKNESS) || (j_ptr->name2 == EGO_LITE_DARKNESS))
	{
		j_ptr->xtra4 = 0;
		msg_print(_("しかしランプは全く光らない。", "Curiously, your lamp doesn't light."));
	}
	else if (j_ptr->xtra4 >= FUEL_LAMP)
	{
		j_ptr->xtra4 = FUEL_LAMP;
		msg_print(_("ランプの油は一杯だ。", "Your lamp is full."));
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


/*!
 * @brief オブジェクトが松明に束ねられるかどうかを判定する
 * An "item_tester_hook" for refilling torches
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトが松明に束ねられるならばTRUEを返す
 */
static bool item_tester_refill_torch(object_type *o_ptr)
{
	/* Torches are okay */
	if ((o_ptr->tval == TV_LITE) &&
	    (o_ptr->sval == SV_LITE_TORCH)) return (TRUE);

	/* Assume not okay */
	return (FALSE);
}


/*!
 * @brief 松明を束ねるコマンドのメインルーチン
 * Refuel the players torch (from the pack or floor)
 * @return なし
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
	msg_print(_("松明を結合した。", "You combine the torches."));

	/* Comment */
	if ((o_ptr->name2 == EGO_LITE_DARKNESS) && (j_ptr->xtra4 > 0))
	{
		j_ptr->xtra4 = 0;
		msg_print(_("松明が消えてしまった！", "Your torch has gone out!"));
	}
	else if ((o_ptr->name2 == EGO_LITE_DARKNESS) || (j_ptr->name2 == EGO_LITE_DARKNESS))
	{
		j_ptr->xtra4 = 0;
		msg_print(_("しかし松明は全く光らない。", "Curiously, your torche don't light."));
	}
	/* Over-fuel message */
	else if (j_ptr->xtra4 >= FUEL_TORCH)
	{
		j_ptr->xtra4 = FUEL_TORCH;
		msg_print(_("松明の寿命は十分だ。", "Your torch is fully fueled."));
	}

	/* Refuel message */
	else
	{
		msg_print(_("松明はいっそう明るく輝いた。", "Your torch glows more brightly."));
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


/*!
 * @brief 燃料を補充するコマンドのメインルーチン
 * Refill the players lamp, or restock his torches
 * @return なし
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
		msg_print(_("光源を装備していない。", "You are not wielding a light."));
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
		msg_print(_("この光源は寿命を延ばせない。", "Your light cannot be refilled."));
	}
}


/*!
 * @brief ターゲットを設定するコマンドのメインルーチン
 * Target command
 * @return なし
 */
void do_cmd_target(void)
{
	/* Target set */
	if (target_set(TARGET_KILL))
	{
		msg_print(_("ターゲット決定。", "Target Selected."));
	}

	/* Target aborted */
	else
	{
		msg_print(_("ターゲット解除。", "Target Aborted."));
	}
}



/*!
 * @brief 周囲を見渡すコマンドのメインルーチン
 * Look command
 * @return なし
 */
void do_cmd_look(void)
{
	/*TEST*/
	p_ptr->window |= PW_MONSTER_LIST;
	window_stuff();
	/*TEST*/

	/* Look around */
	if (target_set(TARGET_LOOK))
	{
		msg_print(_("ターゲット決定。", "Target Selected."));
	}
}


/*!
 * @brief 位置を確認するコマンドのメインルーチン
 * Allow the player to examine other sectors on the map
 * @return なし
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
		sprintf(out_val, _("マップ位置 [%d(%02d),%d(%02d)] (プレイヤーの%s)  方向?", 
					       "Map sector [%d(%02d),%d(%02d)], which is%s your sector.  Direction?"),
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



/*!
 * @brief モンスター種族情報を特定の基準によりソートするための比較処理
 * Sorting hook -- Comp function -- see below
 * @param u モンスター種族情報の入れるポインタ
 * @param v 条件基準ID
 * @param a 比較するモンスター種族のID1
 * @param b 比較するモンスター種族のID2
 * @return 2の方が大きければTRUEを返す
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


/*!
 * @brief モンスター種族情報を特定の基準によりソートするためのスワップ処理
 * Sorting hook -- Swap function -- see below
 * @param u モンスター種族情報の入れるポインタ
 * @param v 未使用
 * @param a スワップするモンスター種族のID1
 * @param b スワップするモンスター種族のID2
 * @return なし
 * @details
 * We use "u" to point to array of monster indexes,
 * and "v" to select the type of sorting to perform.
 */
void ang_sort_swap_hook(vptr u, vptr v, int a, int b)
{
	u16b *who = (u16b*)(u);

	u16b holder;

	/* Unused */
	(void)v;

	/* Swap */
	holder = who[a];
	who[a] = who[b];
	who[b] = holder;
}



/*!
 * @brief モンスターの思い出を見るコマンドのメインルーチン
 * Identify a character, allow recall of monsters
 * @return なし
 * @details
 * <pre>
 * Several "special" responses recall "multiple" monsters:
 *   ^A (all monsters)
 *   ^U (all unique monsters)
 *   ^N (all non-unique monsters)
 *
 * The responses may be sorted in several ways, see below.
 *
 * Note that the player ghosts are ignored. XXX XXX XXX
 * </pre>
 */
void do_cmd_query_symbol(void)
{
	int		i, n, r_idx;
	char	sym, query;
	char	buf[128];

	bool	all = FALSE;
	bool	uniq = FALSE;
	bool	norm = FALSE;
	bool	ride = FALSE;
	char    temp[80] = "";

	bool	recall = FALSE;

	u16b	why = 0;
	u16b	*who;

	/* Get a character, or abort */
	if (!get_com(_("知りたい文字を入力して下さい(記号 or ^A全,^Uユ,^N非ユ,^R乗馬,^M名前): ", 
				   "Enter character to be identified(^A:All,^U:Uniqs,^N:Non uniqs,^M:Name): "), &sym, FALSE)) return;

	/* Find that character info, and describe it */
	for (i = 0; ident_info[i]; ++i)
	{
		if (sym == ident_info[i][0]) break;
	}

	/* Describe */
	if (sym == KTRL('A'))
	{
		all = TRUE;
		strcpy(buf, _("全モンスターのリスト", "Full monster list."));
	}
	else if (sym == KTRL('U'))
	{
		all = uniq = TRUE;
		strcpy(buf, _("ユニーク・モンスターのリスト", "Unique monster list."));
	}
	else if (sym == KTRL('N'))
	{
		all = norm = TRUE;
		strcpy(buf, _("ユニーク外モンスターのリスト", "Non-unique monster list."));
	}
	else if (sym == KTRL('R'))
	{
		all = ride = TRUE;
		strcpy(buf, _("乗馬可能モンスターのリスト", "Ridable monster list."));
	}
	/* XTRA HACK WHATSEARCH */
	else if (sym == KTRL('M'))
	{
		all = TRUE;
		if (!get_string(_("名前(英語の場合小文字で可)", "Enter name:"),temp, 70))
		{
			temp[0]=0;
			return;
		}
		sprintf(buf, _("名前:%sにマッチ", "Monsters with a name \"%s\""),temp);
	}
	else if (ident_info[i])
	{
		sprintf(buf, "%c - %s.", sym, ident_info[i] + 2);
	}
	else
	{
		sprintf(buf, "%c - %s", sym, _("無効な文字", "Unknown Symbol"));
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

		/* Require ridable monsters if needed */
		if (ride && !(r_ptr->flags7 & (RF7_RIDING))) continue;

		/* XTRA HACK WHATSEARCH */
		if (temp[0])
		{
		  int xx;
		  char temp2[80];
  
		  for (xx=0; temp[xx] && xx<80; xx++)
		  {
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
		  if (my_strstr(temp2, temp) || my_strstr(r_name + r_ptr->name, temp) )
#else
		  if (my_strstr(temp2, temp))
#endif
			  who[n++]=i;
		}

		/* Collect "appropriate" monsters */
		else if (all || (r_ptr->d_char == sym)) who[n++] = i;
	}

	/* Nothing to recall */
	if (!n)
	{
		/* Free the "who" array */
		C_KILL(who, max_r_idx, u16b);

		return;
	}


	/* Prompt XXX XXX XXX */
	put_str(_("思い出を見ますか? (k:殺害順/y/n): ", "Recall details? (k/y/n): "), 0, _(36, 40));


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
		monster_race_track(r_idx);

		/* Hack -- Handle stuff */
		handle_stuff();

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
			}

			/* Hack -- Begin the prompt */
			roff_top(r_idx);

			/* Hack -- Complete the prompt */
			Term_addstr(-1, TERM_WHITE, _(" ['r'思い出, ESC]", " [(r)ecall, ESC]"));

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


