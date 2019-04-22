#include "angband.h"
#include "util.h"

#include "spells-summon.h"
#include "avatar.h"
#include "player-status.h"
#include "player-effects.h"
#include "spells.h"
#include "spells-status.h"
#include "spells-floor.h"
#include "object-hook.h"
#include "cmd-basic.h"



/*!
* @brief 杖の効果を発動する
* @param sval オブジェクトのsval
* @param use_charge 使用回数を消費したかどうかを返す参照ポインタ
* @param powerful 強力発動上の処理ならばTRUE
* @param magic 魔道具術上の処理ならばTRUE
* @param known 判明済ならばTRUE
* @return 発動により効果内容が確定したならばTRUEを返す
*/
int staff_effect(OBJECT_SUBTYPE_VALUE sval, bool *use_charge, bool powerful, bool magic, bool known)
{
	int k;
	int ident = FALSE;
	PLAYER_LEVEL lev = powerful ? p_ptr->lev * 2 : p_ptr->lev;
	POSITION detect_rad = powerful ? DETECT_RAD_DEFAULT * 3 / 2 : DETECT_RAD_DEFAULT;

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
				if (summon_specific(0, p_ptr->y, p_ptr->x, current_floor_ptr->dun_level, 0, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET), '\0'))
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
			}
			else {
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
				ident = TRUE;
			}
			break;
		}

		case SV_STAFF_STARLITE:
			ident = starlight(magic);
			break;

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
			ident = cure_light_wounds((powerful ? 4 : 2), 8);
			break;
		}

		case SV_STAFF_CURING:
		{
			ident = true_healing(0);
			if (set_shero(0, TRUE)) ident = TRUE;
			break;
		}

		case SV_STAFF_HEALING:
		{
			if (cure_critical_wounds(powerful ? 500 : 300)) ident = TRUE;
			break;
		}

		case SV_STAFF_THE_MAGI:
		{
			if (do_res_stat(A_INT)) ident = TRUE;
			ident |= restore_mana(FALSE);
			if (set_shero(0, TRUE)) ident = TRUE;
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
			ident = probing();
			break;
		}

		case SV_STAFF_DISPEL_EVIL:
		{
			ident = dispel_evil(powerful ? 120 : 80);
			break;
		}

		case SV_STAFF_POWER:
		{
			ident = dispel_monsters(powerful ? 225 : 150) ;
			break;
		}

		case SV_STAFF_HOLINESS:
		{
			ident = cleansing_nova(p_ptr, magic, powerful);
			break;
		}

		case SV_STAFF_GENOCIDE:
		{
			ident = symbol_genocide((magic ? lev + 50 : 200), TRUE);
			break;
		}

		case SV_STAFF_EARTHQUAKES:
		{
			if (earthquake(p_ptr->y, p_ptr->x, (powerful ? 15 : 10)))
				ident = TRUE;
			else
				msg_print(_("ダンジョンが揺れた。", "The dungeon trembles."));

			break;
		}

		case SV_STAFF_DESTRUCTION:
		{
			ident = destroy_area(p_ptr->y, p_ptr->x, (powerful ? 18 : 13) + randint0(5), FALSE);
			break;
		}

		case SV_STAFF_ANIMATE_DEAD:
		{
			ident = animate_dead(0, p_ptr->y, p_ptr->x);
			break;
		}

		case SV_STAFF_MSTORM:
		{
			ident = unleash_mana_storm(p_ptr, powerful);
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
void do_cmd_use_staff_aux(INVENTORY_IDX item)
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
		o_ptr = &current_floor_ptr->o_list[0 - item];
	}


	/* Mega-Hack -- refuse to use a pile from the ground */
	if ((item < 0) && (o_ptr->number > 1))
	{
		msg_print(_("まずは杖を拾わなければ。", "You must first pick up the staffs."));
		return;
	}


	take_turn(p_ptr, 100);

	/* Extract the item level */
	lev = k_info[o_ptr->k_idx].level;
	if (lev > 50) lev = 50 + (lev - 50) / 2;

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

	if (cmd_limit_time_walk(p_ptr)) return;

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
		p_ptr->update |= (PU_COMBINE | PU_REORDER);
		p_ptr->window |= (PW_INVEN);

		return;
	}


	sound(SOUND_ZAP);

	ident = staff_effect(o_ptr->sval, &use_charge, FALSE, FALSE, object_is_aware(o_ptr));

	if (!(object_is_aware(o_ptr)))
	{
		chg_virtue(V_PATIENCE, -1);
		chg_virtue(V_CHANCE, 1);
		chg_virtue(V_KNOWLEDGE, -1);
	}
	p_ptr->update |= (PU_COMBINE | PU_REORDER);

	/* Tried the item */
	object_tried(o_ptr);

	/* An identification was made */
	if (ident && !object_is_aware(o_ptr))
	{
		object_aware(o_ptr);
		gain_exp((lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

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
	OBJECT_IDX item;
	concptr q, s;

	if (p_ptr->wild_mode)
	{
		return;
	}

	if (cmd_limit_arena(p_ptr)) return;

	if (p_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
	{
		set_action(ACTION_NONE);
	}

	/* Restrict choices to wands */
	item_tester_tval = TV_STAFF;

	q = _("どの杖を使いますか? ", "Use which staff? ");
	s = _("使える杖がない。", "You have no staff to use.");

	if (!choose_object(&item, q, s, (USE_INVEN | USE_FLOOR))) return;

	do_cmd_use_staff_aux(item);
}
