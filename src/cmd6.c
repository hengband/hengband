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
#include "selfinfo.h"
#include "cmd-activate.h"
#include "cmd-eat.h"
#include "cmd-quaff.h"
#include "cmd-read.h"
#include "cmd-usestaff.h"



/*!
 * @brief 魔法棒の効果を発動する
 * @param sval オブジェクトのsval
 * @param dir 発動の方向ID
 * @param powerful 強力発動上の処理ならばTRUE
 * @param magic 魔道具術上の処理ならばTRUE
 * @return 発動により効果内容が確定したならばTRUEを返す
 */
static int wand_effect(OBJECT_SUBTYPE_VALUE sval, int dir, bool powerful, bool magic)
{
	int ident = FALSE;
	int lev = powerful ? p_ptr->lev * 2 : p_ptr->lev;
	int rad = powerful ? 3 : 2;

	/* XXX Hack -- Wand of wonder can do anything before it */
	if (sval == SV_WAND_WONDER)
	{
		int vir = virtue_number(V_CHANCE);
		sval = (OBJECT_SUBTYPE_VALUE)randint0(SV_WAND_WONDER);

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
			HIT_POINT dam = damroll((powerful ? 20 : 10), 10);
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
			HIT_POINT dam = powerful ? 40 + randint1(60) : 20 + randint1(30);
			if (wall_to_mud(dir, dam)) ident = TRUE;
			break;
		}

		case SV_WAND_LITE:
		{
			HIT_POINT dam = damroll((powerful ? 12 : 6), 8);
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

		case SV_WAND_HYPODYNAMIA:
		{
			if (hypodynamic_bolt(dir, 80 + lev)) ident = TRUE;
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
			fire_breath(GF_FIRE, dir, (powerful ? 300 : 200), 3);
			ident = TRUE;
			break;
		}

		case SV_WAND_DRAGON_COLD:
		{
			fire_breath(GF_COLD, dir, (powerful ? 270 : 180), 3);
			ident = TRUE;
			break;
		}

		case SV_WAND_DRAGON_BREATH:
		{
			HIT_POINT dam;
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
	p_ptr->energy_use = 100;

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
	OBJECT_IDX item;
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
static int rod_effect(OBJECT_SUBTYPE_VALUE sval, int dir, bool *use_charge, bool powerful, bool magic)
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
			HIT_POINT dam = damroll((powerful ? 12 : 6), 8);
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

		case SV_ROD_HYPODYNAMIA:
		{
			if (hypodynamic_bolt(dir, 70 + 3 * lev / 2)) ident = TRUE;
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
			HIT_POINT dam = powerful ? 40 + randint1(60) : 20 + randint1(30);
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
	p_ptr->energy_use = 100;

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
	OBJECT_IDX item;
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
	OBJECT_IDX item;
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
static OBJECT_SUBTYPE_VALUE select_magic_eater(bool only_browse)
{
	OBJECT_SUBTYPE_VALUE ext = 0;
	char choice;
	bool flag, request_list;
	OBJECT_TYPE_VALUE tval = 0;
	int             ask = TRUE;
	OBJECT_SUBTYPE_VALUE i = 0;
	char            out_val[160];

	int menu_line = (use_menu ? 1 : 0);

#ifdef ALLOW_REPEAT
	COMMAND_CODE sn;
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
			OBJECT_SUBTYPE_VALUE ctr;
			PERCENTAGE chance;
			IDX k_idx;
			char dummy[80];
			POSITION x1, y1;
			int level;
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
				if (ask) choice = (char)tolower(choice);

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
	OBJECT_SUBTYPE_VALUE item;
	PERCENTAGE chance;
	DEPTH level;
	IDX k_idx;
	OBJECT_TYPE_VALUE tval;
	OBJECT_SUBTYPE_VALUE sval;
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
		p_ptr->energy_use = 0;
		return FALSE;
	}
	if (item >= EATER_EXT*2) {tval = TV_ROD;sval = item - EATER_EXT*2;}
	else if (item >= EATER_EXT) {tval = TV_WAND;sval = item - EATER_EXT;}
	else {tval = TV_STAFF; sval = item;}
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
		p_ptr->energy_use = 100;

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
	p_ptr->energy_use = 100;
	if (tval == TV_ROD) p_ptr->magic_num1[item] += k_info[k_idx].pval * EATER_ROD_CHARGE;
	else p_ptr->magic_num1[item] -= EATER_CHARGE;

        return TRUE;
}
