/*!
 *  @file cmd-item.c
 *  @brief プレイヤーのアイテムに関するコマンドの実装1 / Inventory and equipment commands
 *  @date 2014/01/02
 *  @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */


#include "angband.h"
#include "core.h"
#include "util.h"
#include "autopick.h"
#include "term.h"

#include "selfinfo.h"
#include "cmd-activate.h"
#include "cmd-eat.h"
#include "cmd-quaff.h"
#include "cmd-read.h"
#include "cmd-usestaff.h"
#include "cmd-zaprod.h"
#include "cmd-zapwand.h"
#include "cmd-pet.h"
#include "cmd-basic.h"

#include "object-flavor.h"
#include "object-hook.h"
#include "object-ego.h"
#include "sort.h"
#include "quest.h"
#include "artifact.h"
#include "avatar.h"
#include "player-status.h"
#include "player-effects.h"
#include "player-class.h"
#include "player-personality.h"
#include "monster.h"
#include "view-mainwindow.h"
#include "spells.h"
#include "objectkind.h"
#include "autopick.h"
#include "targeting.h"
#include "snipe.h"
#include "player-race.h"
#include "view-mainwindow.h"
#include "player-inventory.h"

/*!
 * @brief 持ち物一覧を表示するコマンドのメインルーチン / Display p_ptr->inventory_list
 * @return なし 
 */
void do_cmd_inven(void)
{
	char out_val[160];
	command_wrk = FALSE;
	if (easy_floor) command_wrk = (USE_INVEN);
	screen_save();
	(void)show_inven(0, USE_FULL, 0);

#ifdef JP
	sprintf(out_val, "持ち物： 合計 %3d.%1d kg (限界の%ld%%) コマンド: ",
		(int)lbtokg1(p_ptr->total_weight) , (int)lbtokg2(p_ptr->total_weight) ,
		(long int)((p_ptr->total_weight * 100) / weight_limit(p_ptr)));
#else
	sprintf(out_val, "Inventory: carrying %d.%d pounds (%ld%% of capacity). Command: ",
		(int)(p_ptr->total_weight / 10), (int)(p_ptr->total_weight % 10),
		(p_ptr->total_weight * 100) / weight_limit(p_ptr));
#endif

	prt(out_val, 0, 0);
	command_new = inkey();
	screen_load();

	if (command_new == ESCAPE)
	{
		TERM_LEN wid, hgt;
		Term_get_size(&wid, &hgt);
		command_new = 0;
		command_gap = wid - 30;
	}
	else
	{
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
	command_wrk = TRUE;
	if (easy_floor) command_wrk = (USE_EQUIP);
	screen_save();

	(void)show_equip(0, USE_FULL, 0);

#ifdef JP
	sprintf(out_val, "装備： 合計 %3d.%1d kg (限界の%ld%%) コマンド: ",
	    (int)lbtokg1(p_ptr->total_weight) , (int)lbtokg2(p_ptr->total_weight) ,
	    (long int)((p_ptr->total_weight * 100) / weight_limit(p_ptr)));
#else
	sprintf(out_val, "Equipment: carrying %d.%d pounds (%ld%% of capacity). Command: ",
	    (int)(p_ptr->total_weight / 10), (int)(p_ptr->total_weight % 10),
	    (long int)((p_ptr->total_weight * 100) / weight_limit(p_ptr)));
#endif

	prt(out_val, 0, 0);
	command_new = inkey();
	screen_load();

	if (command_new == ESCAPE)
	{
		TERM_LEN wid, hgt;
		Term_get_size(&wid, &hgt);
		command_new = 0;
		command_gap = wid - 30;
	}

	else
	{
		command_see = TRUE;
	}
}

bool select_ring_slot = FALSE;

/*!
 * @brief 装備するコマンドのメインルーチン / Wield or wear a single item from the pack or floor
 * @return なし 
 */
void do_cmd_wield(void)
{
	OBJECT_IDX item, slot;
	object_type forge;
	object_type *q_ptr;
	object_type *o_ptr;

	concptr act;
	concptr q, s;

	GAME_TEXT o_name[MAX_NLEN];


	OBJECT_IDX need_switch_wielding = 0;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(p_ptr, ACTION_NONE);
	}

	/* Restrict the choices */
	item_tester_hook = item_tester_hook_wear;

	q = _("どれを装備しますか? ", "Wear/Wield which item? ");
	s = _("装備可能なアイテムがない。", "You have nothing you can wear or wield.");

	o_ptr = choose_object(&item, q, s, (USE_INVEN | USE_FLOOR), 0);
	if (!o_ptr) return;

	/* Check the slot */
	slot = wield_slot(o_ptr);

	switch (o_ptr->tval)
	{
	/* Shields and some misc. items */
	case TV_CAPTURE:
	case TV_SHIELD:
	case TV_CARD:
		/* Dual wielding */
		if (has_melee_weapon(p_ptr, INVEN_RARM) && has_melee_weapon(p_ptr, INVEN_LARM))
		{
			/* Restrict the choices */
			item_tester_hook = item_tester_hook_melee_weapon;

			/* Choose a weapon from the equipment only */
			q = _("どちらの武器と取り替えますか?", "Replace which weapon? ");
			s = _("おっと。", "Oops.");
			if (!choose_object(&slot, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT), 0)) return;
			if (slot == INVEN_RARM) need_switch_wielding = INVEN_LARM;
		}

		else if (has_melee_weapon(p_ptr, INVEN_LARM)) slot = INVEN_RARM;

		/* Both arms are already used by non-weapon */
		else if (p_ptr->inventory_list[INVEN_RARM].k_idx && !object_is_melee_weapon(&p_ptr->inventory_list[INVEN_RARM]) &&
		         p_ptr->inventory_list[INVEN_LARM].k_idx && !object_is_melee_weapon(&p_ptr->inventory_list[INVEN_LARM]))
		{
			/* Restrict the choices */
			item_tester_hook = item_tester_hook_mochikae;

			/* Choose a hand */
			q = _("どちらの手に装備しますか?", "Equip which hand? ");
			s = _("おっと。", "Oops.");
			if (!choose_object(&slot, q, s, (USE_EQUIP), 0)) return;
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

		else if (!p_ptr->inventory_list[INVEN_RARM].k_idx && has_melee_weapon(p_ptr, INVEN_LARM))
		{
			if (!get_check(_("二刀流で戦いますか？", "Dual wielding? "))) slot = INVEN_LARM;
		}

		/* Both arms are already used */
		else if (p_ptr->inventory_list[INVEN_LARM].k_idx && p_ptr->inventory_list[INVEN_RARM].k_idx)
		{
			/* Restrict the choices */
			item_tester_hook = item_tester_hook_mochikae;

			/* Choose a hand */
			q = _("どちらの手に装備しますか?", "Equip which hand? ");
			s = _("おっと。", "Oops.");
			
			if (!choose_object(&slot, q, s, (USE_EQUIP), 0)) return;
			if ((slot == INVEN_LARM) && !has_melee_weapon(p_ptr, INVEN_RARM))
				need_switch_wielding = INVEN_RARM;
		}
		break;

	/* Rings */
	case TV_RING:
		/* Choose a ring slot */
		if (p_ptr->inventory_list[INVEN_LEFT].k_idx && p_ptr->inventory_list[INVEN_RIGHT].k_idx)
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

		if (!choose_object(&slot, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT), 0))
		{
			select_ring_slot = FALSE;
			return;
		}
		select_ring_slot = FALSE;
		break;
	}

	/* Prevent wielding into a cursed slot */
	if (object_is_cursed(&p_ptr->inventory_list[slot]))
	{
		object_desc(o_name, &p_ptr->inventory_list[slot], (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
		msg_format("%s%sは呪われているようだ。", describe_use(slot) , o_name );
#else
		msg_format("The %s you are %s appears to be cursed.", o_name, describe_use(slot));
#endif
		return;
	}

	if (confirm_wear &&
		((object_is_cursed(o_ptr) && object_is_known(o_ptr)) ||
		((o_ptr->ident & IDENT_SENSE) &&
			(FEEL_BROKEN <= o_ptr->feeling) && (o_ptr->feeling <= FEEL_CURSED))))
	{
		char dummy[MAX_NLEN+80];

		object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
		sprintf(dummy, _("本当に%s{呪われている}を使いますか？", "Really use the %s {cursed}? "), o_name);

		if (!get_check(dummy)) return;
	}

	if ((o_ptr->name1 == ART_STONEMASK) && object_is_known(o_ptr) && (p_ptr->prace != RACE_VAMPIRE) && (p_ptr->prace != RACE_ANDROID))
	{
		char dummy[MAX_NLEN+100];

		object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

		sprintf(dummy, _("%sを装備すると吸血鬼になります。よろしいですか？",
			"%s will transforms you into a vampire permanently when equiped. Do you become a vampire?"), o_name);

		if (!get_check(dummy)) return;
	}

	if (need_switch_wielding && !object_is_cursed(&p_ptr->inventory_list[need_switch_wielding]))
	{
		object_type *slot_o_ptr = &p_ptr->inventory_list[slot];
		object_type *switch_o_ptr = &p_ptr->inventory_list[need_switch_wielding];
		object_type object_tmp;
		object_type *otmp_ptr = &object_tmp;
		GAME_TEXT switch_name[MAX_NLEN];

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

	take_turn(p_ptr, 100);
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
	o_ptr = &p_ptr->inventory_list[slot];

	/* Take off existing item */
	if (o_ptr->k_idx)
	{
		/* Take off existing item */
		(void)inven_takeoff(slot, 255);
	}

	/* Wear the new stuff */
	object_copy(o_ptr, q_ptr);

	o_ptr->marked |= OM_TOUCHED;

	p_ptr->total_weight += q_ptr->weight;

	/* Increment the equip counter by hand */
	p_ptr->equip_cnt++;

#define STR_WIELD_RARM _("%s(%c)を右手に装備した。", "You are wielding %s (%c) in your right hand.")
#define STR_WIELD_LARM _("%s(%c)を左手に装備した。", "You are wielding %s (%c) in your left hand.")
#define STR_WIELD_ARMS _("%s(%c)を両手で構えた。", "You are wielding %s (%c) with both hands.")

	/* Where is the item now */
	switch (slot)
	{
	case INVEN_RARM:
		if (object_allow_two_hands_wielding(o_ptr) && (empty_hands(p_ptr, FALSE) == EMPTY_HAND_LARM) && CAN_TWO_HANDS_WIELDING())
			act = STR_WIELD_ARMS;
		else
			act = (left_hander ? STR_WIELD_LARM : STR_WIELD_RARM);
		break;

	case INVEN_LARM:
		if (object_allow_two_hands_wielding(o_ptr) && (empty_hands(p_ptr, FALSE) == EMPTY_HAND_RARM) && CAN_TWO_HANDS_WIELDING())
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

	object_desc(o_name, o_ptr, 0);
	msg_format(act, o_name, index_to_label(slot));

	/* Cursed! */
	if (object_is_cursed(o_ptr))
	{
		msg_print(_("うわ！ すさまじく冷たい！", "Oops! It feels deathly cold!"));
		chg_virtue(p_ptr, V_HARMONY, -1);

		/* Note the curse */
		o_ptr->ident |= (IDENT_SENSE);
	}

	/* The Stone Mask make the player current_world_ptr->game_turn into a vampire! */
	if ((o_ptr->name1 == ART_STONEMASK) && (p_ptr->prace != RACE_VAMPIRE) && (p_ptr->prace != RACE_ANDROID))
	{
		/* Turn into a vampire */
		change_race(p_ptr, RACE_VAMPIRE, "");
	}

	p_ptr->update |= (PU_BONUS | PU_TORCH | PU_MANA);
	p_ptr->redraw |= (PR_EQUIPPY);
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	calc_android_exp(p_ptr);
}

/*!
 * @brief 持ち替え処理
 * @param item 持ち替えを行いたい装備部位ID
 * @return なし
 */
void kamaenaoshi(INVENTORY_IDX item)
{
	object_type *o_ptr, *new_o_ptr;
	GAME_TEXT o_name[MAX_NLEN];

	if (item == INVEN_RARM)
	{
		if (has_melee_weapon(p_ptr, INVEN_LARM))
		{
			o_ptr = &p_ptr->inventory_list[INVEN_LARM];
			object_desc(o_name, o_ptr, 0);

			if (!object_is_cursed(o_ptr))
			{
				new_o_ptr = &p_ptr->inventory_list[INVEN_RARM];
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
		o_ptr = &p_ptr->inventory_list[INVEN_RARM];
		if (o_ptr->k_idx) object_desc(o_name, o_ptr, 0);

		if (has_melee_weapon(p_ptr, INVEN_RARM))
		{
			if (object_allow_two_hands_wielding(o_ptr) && CAN_TWO_HANDS_WIELDING())
				msg_format(_("%sを両手で構えた。", "You are wielding %s with both hands."), o_name);
		}
		else if (!(empty_hands(p_ptr, FALSE) & EMPTY_HAND_RARM) && !object_is_cursed(o_ptr))
		{
			new_o_ptr = &p_ptr->inventory_list[INVEN_LARM];
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
	OBJECT_IDX item;
	object_type *o_ptr;
	concptr q, s;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(p_ptr, ACTION_NONE);
	}

	q = _("どれを装備からはずしますか? ", "Take off which item? ");
	s = _("はずせる装備がない。", "You are not wearing anything to take off.");

	o_ptr = choose_object(&item, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT), 0);
	if (!o_ptr) return;

	/* Item is cursed */
	if (object_is_cursed(o_ptr))
	{
		if ((o_ptr->curse_flags & TRC_PERMA_CURSE) || (p_ptr->pclass != CLASS_BERSERKER))
		{
			msg_print(_("ふーむ、どうやら呪われているようだ。", "Hmmm, it seems to be cursed."));

			return;
		}

		if (((o_ptr->curse_flags & TRC_HEAVY_CURSE) && one_in_(7)) || one_in_(4))
		{
			msg_print(_("呪われた装備を力づくで剥がした！", "You teared a cursed equipment off by sheer strength!"));

			o_ptr->ident |= (IDENT_SENSE);
			o_ptr->curse_flags = 0L;
			o_ptr->feeling = FEEL_NONE;

			p_ptr->update |= (PU_BONUS);
			p_ptr->window |= (PW_EQUIP);

			msg_print(_("呪いを打ち破った。", "You break the curse."));
		}
		else
		{
			msg_print(_("装備を外せなかった。", "You couldn't remove the equipment."));
			take_turn(p_ptr, 50);
			return;
		}
	}

	take_turn(p_ptr, 50);

	/* Take off the item */
	(void)inven_takeoff(item, 255);
	kamaenaoshi(item);
	calc_android_exp(p_ptr);
	p_ptr->redraw |= (PR_EQUIPPY);
}


/*!
 * @brief アイテムを落とすコマンドのメインルーチン / Drop an item
 * @return なし
 */
void do_cmd_drop(void)
{
	OBJECT_IDX item;
	int amt = 1;

	object_type *o_ptr;

	concptr q, s;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(p_ptr, ACTION_NONE);
	}

	q = _("どのアイテムを落としますか? ", "Drop which item? ");
	s = _("落とせるアイテムを持っていない。", "You have nothing to drop.");

	o_ptr = choose_object(&item, q, s, (USE_EQUIP | USE_INVEN | IGNORE_BOTHHAND_SLOT), 0);
	if (!o_ptr) return;

	/* Hack -- Cannot remove cursed items */
	if ((item >= INVEN_RARM) && object_is_cursed(o_ptr))
	{
		msg_print(_("ふーむ、どうやら呪われているようだ。", "Hmmm, it seems to be cursed."));
		return;
	}

	if (o_ptr->number > 1)
	{
		amt = get_quantity(NULL, o_ptr->number);
		if (amt <= 0) return;
	}

	take_turn(p_ptr, 50);

	/* Drop (some of) the item */
	inven_drop(item, amt);

	if (item >= INVEN_RARM)
	{
		kamaenaoshi(item);
		calc_android_exp(p_ptr);
	}

	p_ptr->redraw |= (PR_EQUIPPY);
}


/*!
 * @brief アイテムを破壊するコマンドのメインルーチン / Destroy an item
 * @return なし
 */
void do_cmd_destroy(void)
{
	OBJECT_IDX item;
	QUANTITY amt = 1;
	QUANTITY old_number;

	bool force = FALSE;

	object_type *o_ptr;
	object_type forge;
	object_type *q_ptr = &forge;

	GAME_TEXT o_name[MAX_NLEN];
	char out_val[MAX_NLEN+40];

	concptr q, s;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(p_ptr, ACTION_NONE);
	}

	/* Hack -- force destruction */
	if (command_arg > 0) force = TRUE;

	q = _("どのアイテムを壊しますか? ", "Destroy which item? ");
	s = _("壊せるアイテムを持っていない。", "You have nothing to destroy.");

	o_ptr = choose_object(&item, q, s, (USE_INVEN | USE_FLOOR), 0);
	if (!o_ptr) return;

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
		handle_stuff();

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

	if (o_ptr->number > 1)
	{
		amt = get_quantity(NULL, o_ptr->number);
		if (amt <= 0) return;
	}


	old_number = o_ptr->number;
	o_ptr->number = amt;
	object_desc(o_name, o_ptr, 0);
	o_ptr->number = old_number;

	take_turn(p_ptr, 100);

	/* Artifacts cannot be destroyed */
	if (!can_player_destroy_object(o_ptr))
	{
		free_turn(p_ptr);

		msg_format(_("%sは破壊不可能だ。", "You cannot destroy %s."), o_name);
		return;
	}

	object_copy(q_ptr, o_ptr);

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

	if (item_tester_high_level_book(q_ptr))
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
			gain_exp(p_ptr, tester_exp * amt);
		}
		if (item_tester_high_level_book(q_ptr) && q_ptr->tval == TV_LIFE_BOOK)
		{
			chg_virtue(p_ptr, V_UNLIFE, 1);
			chg_virtue(p_ptr, V_VITALITY, -1);
		}
		else if (item_tester_high_level_book(q_ptr) && q_ptr->tval == TV_DEATH_BOOK)
		{
			chg_virtue(p_ptr, V_UNLIFE, -1);
			chg_virtue(p_ptr, V_VITALITY, 1);
		}
	
		if (q_ptr->to_a || q_ptr->to_h || q_ptr->to_d)
			chg_virtue(p_ptr, V_ENCHANT, -1);
	
		if (object_value_real(q_ptr) > 30000)
			chg_virtue(p_ptr, V_SACRIFICE, 2);
	
		else if (object_value_real(q_ptr) > 10000)
			chg_virtue(p_ptr, V_SACRIFICE, 1);
	}

	if (q_ptr->to_a != 0 || q_ptr->to_d != 0 || q_ptr->to_h != 0)
		chg_virtue(p_ptr, V_HARMONY, 1);

	if (item >= INVEN_RARM) calc_android_exp(p_ptr);
}


/*!
 * @brief アイテムを調査するコマンドのメインルーチン / Observe an item which has been *identify*-ed
 * @return なし
 */
void do_cmd_observe(void)
{
	OBJECT_IDX item;
	object_type *o_ptr;
	GAME_TEXT o_name[MAX_NLEN];
	concptr q, s;

	q = _("どのアイテムを調べますか? ", "Examine which item? ");
	s = _("調べられるアイテムがない。", "You have nothing to examine.");

	o_ptr = choose_object(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
	if (!o_ptr) return;

	/* Require full knowledge */
	if (!(o_ptr->ident & IDENT_MENTAL))
	{
		msg_print(_("このアイテムについて特に知っていることはない。", "You have no special knowledge about that item."));
		return;
	}

	object_desc(o_name, o_ptr, 0);
	msg_format(_("%sを調べている...", "Examining %s..."), o_name);
	if (!screen_object(o_ptr, SCROBJ_FORCE_DETAIL)) msg_print(_("特に変わったところはないようだ。", "You see nothing special."));
}



/*!
 * @brief アイテムの銘を消すコマンドのメインルーチン
 * Remove the inscription from an object XXX Mention item (when done)?
 * @return なし
 */
void do_cmd_uninscribe(void)
{
	OBJECT_IDX item;
	object_type *o_ptr;
	concptr q, s;

	q = _("どのアイテムの銘を消しますか? ", "Un-inscribe which item? ");
	s = _("銘を消せるアイテムがない。", "You have nothing to un-inscribe.");

	o_ptr = choose_object(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
	if (!o_ptr) return;

	/* Nothing to remove */
	if (!o_ptr->inscription)
	{
		msg_print(_("このアイテムには消すべき銘がない。", "That item had no inscription to remove."));
		return;
	}

	msg_print(_("銘を消した。", "Inscription removed."));

	/* Remove the incription */
	o_ptr->inscription = 0;
	p_ptr->update |= (PU_COMBINE);
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
	OBJECT_IDX item;
	object_type *o_ptr;
	GAME_TEXT o_name[MAX_NLEN];
	char out_val[80];
	concptr q, s;

	q = _("どのアイテムに銘を刻みますか? ", "Inscribe which item? ");
	s = _("銘を刻めるアイテムがない。", "You have nothing to inscribe.");

	o_ptr = choose_object(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
	if (!o_ptr) return;

	/* Describe the activity */
	object_desc(o_name, o_ptr, OD_OMIT_INSCRIPTION);

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
		p_ptr->update |= (PU_COMBINE);
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* .や$の関係で, 再計算が必要なはず -- henkma */
		p_ptr->update |= (PU_BONUS);
	}
}


/*!
 * @brief ランタンに燃料を加えるコマンドのメインルーチン
 * Refill the players lamp (from the pack or floor)
 * @return なし
 */
static void do_cmd_refill_lamp(void)
{
	OBJECT_IDX item;
	object_type *o_ptr;
	object_type *j_ptr;
	concptr q, s;

	/* Restrict the choices */
	item_tester_hook = item_tester_refill_lantern;

	q = _("どの油つぼから注ぎますか? ", "Refill with which flask? ");
	s = _("油つぼがない。", "You have no flasks of oil.");

	o_ptr = choose_object(&item, q, s, (USE_INVEN | USE_FLOOR), 0);
	if (!o_ptr) return;

	take_turn(p_ptr, 50);

	/* Access the lantern */
	j_ptr = &p_ptr->inventory_list[INVEN_LITE];

	/* Refuel */
	j_ptr->xtra4 += o_ptr->xtra4;
	msg_print(_("ランプに油を注いだ。", "You fuel your lamp."));

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

	p_ptr->update |= (PU_TORCH);
}

/*!
 * @brief 松明を束ねるコマンドのメインルーチン
 * Refuel the players torch (from the pack or floor)
 * @return なし
 */
static void do_cmd_refill_torch(void)
{
	OBJECT_IDX item;

	object_type *o_ptr;
	object_type *j_ptr;

	concptr q, s;

	/* Restrict the choices */
	item_tester_hook = object_can_refill_torch;

	q = _("どの松明で明かりを強めますか? ", "Refuel with which torch? ");
	s = _("他に松明がない。", "You have no extra torches.");

	o_ptr = choose_object(&item, q, s, (USE_INVEN | USE_FLOOR), 0);
	if (!o_ptr) return;

	take_turn(p_ptr, 50);

	/* Access the primary torch */
	j_ptr = &p_ptr->inventory_list[INVEN_LITE];

	/* Refuel */
	j_ptr->xtra4 += o_ptr->xtra4 + 5;

	msg_print(_("松明を結合した。", "You combine the torches."));

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
	o_ptr = &p_ptr->inventory_list[INVEN_LITE];

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(p_ptr, ACTION_NONE);
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
	if (p_ptr->wild_mode) return;

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
	p_ptr->window |= PW_MONSTER_LIST;
	handle_stuff();

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
	DIRECTION dir;
	POSITION y1, x1, y2, x2;
	GAME_TEXT tmp_val[80];
	GAME_TEXT out_val[160];
	TERM_LEN wid, hgt;

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
			strcpy(tmp_val, _("真上", "\0"));
		}
		else
		{
			sprintf(tmp_val, "%s%s",
				((y2 < y1) ? _("北", " North") : (y2 > y1) ? _("南", " South") : ""),
				((x2 < x1) ? _("西", " West") : (x2 > x1) ? _("東", " East") : ""));
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

	p_ptr->update |= (PU_MONSTERS);
	p_ptr->redraw |= (PR_MAP);
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
	handle_stuff();
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
 * Note that the player ghosts are ignored. 
 * </pre>
 */
void do_cmd_query_symbol(void)
{
	IDX i;
	int n;
	MONRACE_IDX r_idx;
	char sym, query;
	char buf[128];

	bool all = FALSE;
	bool uniq = FALSE;
	bool norm = FALSE;
	bool ride = FALSE;
	char temp[80] = "";

	bool recall = FALSE;

	u16b why = 0;
	MONRACE_IDX *who;

	/* Get a character, or abort */
	if (!get_com(_("知りたい文字を入力して下さい(記号 or ^A全,^Uユ,^N非ユ,^R乗馬,^M名前): ", 
				   "Enter character to be identified(^A:All,^U:Uniqs,^N:Non uniqs,^M:Name): "), &sym, FALSE)) return;

	/* Find that character info, and describe it */
	for (i = 0; ident_info[i]; ++i)
	{
		if (sym == ident_info[i][0]) break;
	}
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
	C_MAKE(who, max_r_idx, MONRACE_IDX);

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
			TERM_LEN xx;
			char temp2[80];

			for (xx = 0; temp[xx] && xx < 80; xx++)
			{
#ifdef JP
				if (iskanji(temp[xx])) { xx++; continue; }
#endif
				if (isupper(temp[xx])) temp[xx] = (char)tolower(temp[xx]);
			}

#ifdef JP
			strcpy(temp2, r_name + r_ptr->E_name);
#else
			strcpy(temp2, r_name + r_ptr->name);
#endif
			for (xx = 0; temp2[xx] && xx < 80; xx++)
				if (isupper(temp2[xx])) temp2[xx] = (char)tolower(temp2[xx]);

#ifdef JP
			if (my_strstr(temp2, temp) || my_strstr(r_name + r_ptr->name, temp))
#else
			if (my_strstr(temp2, temp))
#endif
				who[n++] = i;
		}

		/* Collect "appropriate" monsters */
		else if (all || (r_ptr->d_char == sym)) who[n++] = i;
	}

	/* Nothing to recall */
	if (!n)
	{
		C_KILL(who, max_r_idx, MONRACE_IDX);
		return;
	}

	/* Prompt */
	put_str(_("思い出を見ますか? (k:殺害順/y/n): ", "Recall details? (k/y/n): "), 0, _(36, 40));

	/* Query */
	query = inkey();
	prt(buf, 0, 0);

	why = 2;

	/* Sort the array */
	ang_sort(who, &why, n, ang_sort_comp_hook, ang_sort_swap_hook);

	/* Sort by kills (and level) */
	if (query == 'k')
	{
		why = 4;
		query = 'y';
	}

	/* Catch "escape" */
	if (query != 'y')
	{
		C_KILL(who, max_r_idx, MONRACE_IDX);
		return;
	}

	/* Sort if needed */
	if (why == 4)
	{
		ang_sort(who, &why, n, ang_sort_comp_hook, ang_sort_swap_hook);
	}


	/* Start at the end */
	i = n - 1;

	/* Scan the monster memory */
	while (1)
	{
		r_idx = who[i];

		/* Hack -- Auto-recall */
		monster_race_track(r_idx);
		handle_stuff();

		/* Interact */
		while (1)
		{
			if (recall)
			{
				screen_save();

				/* Recall on screen */
				screen_roff(who[i], 0);
			}

			/* Hack -- Begin the prompt */
			roff_top(r_idx);

			/* Hack -- Complete the prompt */
			Term_addstr(-1, TERM_WHITE, _(" ['r'思い出, ESC]", " [(r)ecall, ESC]"));

			query = inkey();

			/* Unrecall */
			if (recall)
			{
				screen_load();
			}

			/* Normal commands */
			if (query != 'r') break;

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
	C_KILL(who, max_r_idx, IDX);

	/* Re-display the identity */
	prt(buf, 0, 0);
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
	OBJECT_IDX item;
	object_type *o_ptr;
	concptr q, s;

	if (p_ptr->wild_mode)
	{
		return;
	}

	if (cmd_limit_arena(p_ptr)) return;

	if (p_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
	{
		set_action(p_ptr, ACTION_NONE);
	}

	item_tester_hook = item_tester_hook_use;

	q = _("どれを使いますか？", "Use which item? ");
	s = _("使えるものがありません。", "You have nothing to use.");

	o_ptr = choose_object(&item, q, s, (USE_INVEN | USE_EQUIP | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
	if (!o_ptr) return;

	switch (o_ptr->tval)
	{
		case TV_SPIKE:
			do_cmd_spike(p_ptr);
			break;

		case TV_FOOD:
			exe_eat_food(p_ptr, item);
			break;

		case TV_WAND:
			exe_aim_wand(p_ptr, item);
			break;

		case TV_STAFF:
			exe_use_staff(p_ptr, item);
			break;

		case TV_ROD:
			exe_zap_rod(p_ptr, item);
			break;

		case TV_POTION:
			exe_quaff_potion(p_ptr, item);
			break;

		case TV_SCROLL:
			if (cmd_limit_blind(p_ptr)) return;
			if (cmd_limit_confused(p_ptr)) return;
			exe_read(p_ptr, item, TRUE);
			break;

		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
			exe_fire(item, &p_ptr->inventory_list[INVEN_BOW], SP_NONE);
			break;

		default:
			exe_activate(p_ptr, item);
			break;
	}
}

