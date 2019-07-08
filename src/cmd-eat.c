/*!
 * @file cmd-eat.c
 * @brief プレイヤーの食べるコマンド実装
 * @date 2018/09/07
 * @details
 * cmd6.cより分離。
 */


#include "angband.h"
#include "util.h"

#include "object-flavor.h"
#include "object-hook.h"
#include "avatar.h"
#include "spells-status.h"
#include "realm-hex.h"
#include "player-status.h"
#include "player-effects.h"
#include "player-damage.h"
#include "player-race.h"
#include "player-class.h"
#include "floor.h"
#include "objectkind.h"
#include "realm.h"
#include "realm-song.h"
#include "view-mainwindow.h"

/*!
 * @brief 食料を食べるコマンドのサブルーチン
 * @param item 食べるオブジェクトの所持品ID
 * @return なし
 */
void exe_eat_food(INVENTORY_IDX item)
{
	int ident, lev;
	object_type *o_ptr;

	if (music_singing_any()) stop_singing(p_ptr);
	if (hex_spelling_any()) stop_hex_spell_all();

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory_list[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &current_floor_ptr->o_list[0 - item];
	}

	sound(SOUND_EAT);

	take_turn(p_ptr, 100);

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
				if (set_poisoned(p_ptr, p_ptr->poisoned + randint0(10) + 10))
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
				if (set_blind(p_ptr, p_ptr->blind + randint0(200) + 200))
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
				if (set_afraid(p_ptr, p_ptr->afraid + randint0(10) + 10))
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
				if (set_confused(p_ptr, p_ptr->confused + randint0(10) + 10))
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
				if (set_image(p_ptr, p_ptr->image + randint0(250) + 250))
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
				if (set_paralyzed(p_ptr, p_ptr->paralyzed + randint0(10) + 10))
				{
					ident = TRUE;
				}
			}
			break;
		}

		case SV_FOOD_WEAKNESS:
		{
			take_hit(p_ptr, DAMAGE_NOESCAPE, damroll(6, 6), _("毒入り食料", "poisonous food"), -1);
			(void)do_dec_stat(p_ptr, A_STR);
			ident = TRUE;
			break;
		}

		case SV_FOOD_SICKNESS:
		{
			take_hit(p_ptr, DAMAGE_NOESCAPE, damroll(6, 6), _("毒入り食料", "poisonous food"), -1);
			(void)do_dec_stat(p_ptr, A_CON);
			ident = TRUE;
			break;
		}

		case SV_FOOD_STUPIDITY:
		{
			take_hit(p_ptr, DAMAGE_NOESCAPE, damroll(8, 8), _("毒入り食料", "poisonous food"), -1);
			(void)do_dec_stat(p_ptr, A_INT);
			ident = TRUE;
			break;
		}

		case SV_FOOD_NAIVETY:
		{
			take_hit(p_ptr, DAMAGE_NOESCAPE, damroll(8, 8), _("毒入り食料", "poisonous food"), -1);
			(void)do_dec_stat(p_ptr, A_WIS);
			ident = TRUE;
			break;
		}

		case SV_FOOD_UNHEALTH:
		{
			take_hit(p_ptr, DAMAGE_NOESCAPE, damroll(10, 10), _("毒入り食料", "poisonous food"), -1);
			(void)do_dec_stat(p_ptr, A_CON);
			ident = TRUE;
			break;
		}

		case SV_FOOD_DISEASE:
		{
			take_hit(p_ptr, DAMAGE_NOESCAPE, damroll(10, 10), _("毒入り食料", "poisonous food"), -1);
			(void)do_dec_stat(p_ptr, A_STR);
			ident = TRUE;
			break;
		}

		case SV_FOOD_CURE_POISON:
		{
			if (set_poisoned(p_ptr, 0)) ident = TRUE;
			break;
		}

		case SV_FOOD_CURE_BLINDNESS:
		{
			if (set_blind(p_ptr, 0)) ident = TRUE;
			break;
		}

		case SV_FOOD_CURE_PARANOIA:
		{
			if (set_afraid(p_ptr, 0)) ident = TRUE;
			break;
		}

		case SV_FOOD_CURE_CONFUSION:
		{
			if (set_confused(p_ptr, 0)) ident = TRUE;
			break;
		}

		case SV_FOOD_CURE_SERIOUS:
		{
			ident = cure_serious_wounds(4, 8);
			break;
		}

		case SV_FOOD_RESTORE_STR:
		{
			if (do_res_stat(p_ptr, A_STR)) ident = TRUE;
			break;
		}

		case SV_FOOD_RESTORE_CON:
		{
			if (do_res_stat(p_ptr, A_CON)) ident = TRUE;
			break;
		}

		case SV_FOOD_RESTORING:
		{
			ident = restore_all_status();
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
			(void)set_poisoned(p_ptr, 0);
			(void)hp_player(p_ptr, damroll(4, 8));
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
	p_ptr->update |= (PU_COMBINE | PU_REORDER);

	if (!(object_is_aware(o_ptr)))
	{
		chg_virtue(p_ptr, V_KNOWLEDGE, -1);
		chg_virtue(p_ptr, V_PATIENCE, -1);
		chg_virtue(p_ptr, V_CHANCE, 1);
	}

	/* We have tried it */
	if (o_ptr->tval == TV_FOOD) object_tried(o_ptr);

	/* The player is now aware of the object */
	if (ident && !object_is_aware(o_ptr))
	{
		object_aware(o_ptr);
		gain_exp(p_ptr, (lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);


	/* Food can feed the player */
	if (PRACE_IS_(p_ptr, RACE_VAMPIRE) || (p_ptr->mimic_form == MIMIC_VAMPIRE))
	{
		/* Reduced nutritional benefit */
		(void)set_food(p_ptr, p_ptr->food + (o_ptr->pval / 10));
		msg_print(_("あなたのような者にとって食糧など僅かな栄養にしかならない。",
			"Mere victuals hold scant sustenance for a being such as yourself."));

		if (p_ptr->food < PY_FOOD_ALERT)   /* Hungry */
			msg_print(_("あなたの飢えは新鮮な血によってのみ満たされる！",
				"Your hunger can only be satisfied with fresh blood!"));
	}
	else if ((PRACE_IS_(p_ptr, RACE_SKELETON) ||
		PRACE_IS_(p_ptr, RACE_GOLEM) ||
		PRACE_IS_(p_ptr, RACE_ZOMBIE) ||
		PRACE_IS_(p_ptr, RACE_SPECTRE)) &&
		(o_ptr->tval == TV_STAFF || o_ptr->tval == TV_WAND))
	{
		concptr staff;

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
			p_ptr->update |= (PU_COMBINE | PU_REORDER);
			p_ptr->window |= (PW_INVEN);

			return;
		}
		msg_format(_("あなたは%sの魔力をエネルギー源として吸収した。", "You absorb mana of the %s as your energy."), staff);

		/* Use a single charge */
		o_ptr->pval--;

		/* Eat a charge */
		set_food(p_ptr, p_ptr->food + 5000);

		/* XXX Hack -- unstack if necessary */
		if (o_ptr->tval == TV_STAFF &&
			(item >= 0) && (o_ptr->number > 1))
		{
			object_type forge;
			object_type *q_ptr;
			q_ptr = &forge;
			object_copy(q_ptr, o_ptr);

			/* Modify quantity */
			q_ptr->number = 1;

			/* Restore the charges */
			o_ptr->pval++;

			/* Unstack the used item */
			o_ptr->number--;
			p_ptr->total_weight -= q_ptr->weight;
			item = inven_carry(q_ptr);

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

		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Don't eat a staff/wand itself */
		return;
	}
	else if ((PRACE_IS_(p_ptr, RACE_DEMON) ||
		(mimic_info[p_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_DEMON)) &&
		(o_ptr->tval == TV_CORPSE && o_ptr->sval == SV_CORPSE &&
			my_strchr("pht", r_info[o_ptr->pval].d_char)))
	{
		/* Drain vitality of humanoids */
		GAME_TEXT o_name[MAX_NLEN];
		object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
		msg_format(_("%sは燃え上り灰になった。精力を吸収した気がする。", "%^s is burnt to ashes.  You absorb its vitality!"), o_name);
		(void)set_food(p_ptr, PY_FOOD_MAX - 1);
	}
	else if (PRACE_IS_(p_ptr, RACE_SKELETON))
	{
#if 0
		if (o_ptr->tval == TV_SKELETON ||
			(o_ptr->tval == TV_CORPSE && o_ptr->sval == SV_SKELETON))
		{
			msg_print(_("あなたは骨で自分の体を補った。", "Your body absorbs the bone."));
			set_food(p_ptr, p_ptr->food + 5000);
		}
		else
#endif

			if (!((o_ptr->sval == SV_FOOD_WAYBREAD) ||
				(o_ptr->sval < SV_FOOD_BISCUIT)))
			{
				object_type forge;
				object_type *q_ptr = &forge;

				msg_print(_("食べ物がアゴを素通りして落ちた！", "The food falls through your jaws!"));
				object_prep(q_ptr, lookup_kind(o_ptr->tval, o_ptr->sval));

				/* Drop the object from heaven */
				(void)drop_near(q_ptr, -1, p_ptr->y, p_ptr->x);
			}
			else
			{
				msg_print(_("食べ物がアゴを素通りして落ち、消えた！", "The food falls through your jaws and vanishes!"));
			}
	}
	else if (PRACE_IS_(p_ptr, RACE_GOLEM) ||
		PRACE_IS_(p_ptr, RACE_ZOMBIE) ||
		PRACE_IS_(p_ptr, RACE_ENT) ||
		PRACE_IS_(p_ptr, RACE_DEMON) ||
		PRACE_IS_(p_ptr, RACE_ANDROID) ||
		PRACE_IS_(p_ptr, RACE_SPECTRE) ||
		(mimic_info[p_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_NONLIVING))
	{
		msg_print(_("生者の食物はあなたにとってほとんど栄養にならない。", "The food of mortals is poor sustenance for you."));
		set_food(p_ptr, p_ptr->food + ((o_ptr->pval) / 20));
	}
	else if (o_ptr->tval == TV_FOOD && o_ptr->sval == SV_FOOD_WAYBREAD)
	{
		/* Waybread is always fully satisfying. */
		set_food(p_ptr, MAX(p_ptr->food, PY_FOOD_MAX - 1));
	}
	else
	{
		/* Food can feed the player */
		(void)set_food(p_ptr, p_ptr->food + o_ptr->pval);
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
 * @brief 食料を食べるコマンドのメインルーチン /
 * Eat some food (from the pack or floor)
 * @return なし
 */
void do_cmd_eat_food(void)
{
	OBJECT_IDX item;
	concptr        q, s;

	if (p_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
	{
		set_action(p_ptr, ACTION_NONE);
	}

	/* Restrict choices to food */
	item_tester_hook = item_tester_hook_eatable;

	q = _("どれを食べますか? ", "Eat which item? ");
	s = _("食べ物がない。", "You have nothing to eat.");

	if (!choose_object(&item, q, s, (USE_INVEN | USE_FLOOR), 0)) return;

	/* Eat the object */
	exe_eat_food(item);
}

