/*!
 * @file cmd-quaff.c
 * @brief プレイヤーの飲むコマンド実装
 * @date 2018/09/07
 * @details
 * cmd6.cより分離。
 */

#include "angband.h"
#include "util.h"

#include "birth.h"
#include "selfinfo.h"
#include "object-hook.h"
#include "mutation.h"
#include "avatar.h"
#include "spells.h"
#include "spells-status.h"
#include "player-effects.h"
#include "player-status.h"
#include "player-damage.h"
#include "player-race.h"
#include "realm-hex.h"
#include "realm-song.h"
#include "spells-floor.h"
#include "object-broken.h"
#include "cmd-basic.h"
#include "floor.h"
#include "objectkind.h"
#include "view-mainwindow.h"
#include "player-class.h"

/*!
 * @brief 薬を飲むコマンドのサブルーチン /
 * Quaff a potion (from the pack or the floor)
 * @param item 飲む薬オブジェクトの所持品ID
 * @return なし
 */
void exe_quaff_potion(INVENTORY_IDX item)
{
	bool ident;
	DEPTH lev;
	object_type *o_ptr;
	object_type forge;
	object_type *q_ptr;

	take_turn(p_ptr, 100);

	if (p_ptr->timewalk)
	{
		if (flush_failure) flush();
		msg_print(_("瓶から水が流れ出てこない！", "The potion doesn't flow out from a bottle."));

		sound(SOUND_FAIL);
		return;
	}

	if (music_singing_any()) stop_singing(p_ptr);
	if (hex_spelling_any())
	{
		if (!hex_spelling(HEX_INHAIL)) stop_hex_spell_all();
	}

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
	q_ptr = &forge;
	object_copy(q_ptr, o_ptr);

	/* Single object */
	q_ptr->number = 1;

	/* Reduce and describe p_ptr->inventory_list */
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

			if (!(PRACE_IS_(p_ptr, RACE_GOLEM) ||
			      PRACE_IS_(p_ptr, RACE_ZOMBIE) ||
			      PRACE_IS_(p_ptr, RACE_DEMON) ||
			      PRACE_IS_(p_ptr, RACE_ANDROID) ||
			      PRACE_IS_(p_ptr, RACE_SPECTRE) ||
			      (mimic_info[p_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_NONLIVING)))
			{
				/* Only living creatures get thirsty */
				(void)set_food(PY_FOOD_STARVE - 1);
			}

			(void)set_poisoned(p_ptr, 0);
			(void)set_paralyzed(p_ptr->paralyzed + 4);
			ident = TRUE;
			break;

		case SV_POTION_POISON:
			if (!(p_ptr->resist_pois || IS_OPPOSE_POIS()))
			{
				if (set_poisoned(p_ptr, p_ptr->poisoned + randint0(15) + 10))
				{
					ident = TRUE;
				}
			}
			break;

		case SV_POTION_BLINDNESS:
			if (!p_ptr->resist_blind)
			{
				if (set_blind(p_ptr, p_ptr->blind + randint0(100) + 100))
				{
					ident = TRUE;
				}
			}
			break;

		case SV_POTION_BOOZE:
			ident = booze(p_ptr);
			break;

		case SV_POTION_SLEEP:
			if (!p_ptr->free_act)
			{
				msg_print(_("あなたは眠ってしまった。", "You fall asleep."));

				if (ironman_nightmare)
				{
					msg_print(_("恐ろしい光景が頭に浮かんできた。", "A horrible vision enters your mind."));

					/* Have some nightmares */
					sanity_blast(NULL, FALSE);
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

			(void)dec_stat(p_ptr, A_DEX, 25, TRUE);
			(void)dec_stat(p_ptr, A_WIS, 25, TRUE);
			(void)dec_stat(p_ptr, A_CON, 25, TRUE);
			(void)dec_stat(p_ptr, A_STR, 25, TRUE);
			(void)dec_stat(p_ptr, A_CHR, 25, TRUE);
			(void)dec_stat(p_ptr, A_INT, 25, TRUE);
			ident = TRUE;
			break;

		case SV_POTION_DEC_STR:
			if (do_dec_stat(p_ptr, A_STR)) ident = TRUE;
			break;

		case SV_POTION_DEC_INT:
			if (do_dec_stat(p_ptr, A_INT)) ident = TRUE;
			break;

		case SV_POTION_DEC_WIS:
			if (do_dec_stat(p_ptr, A_WIS)) ident = TRUE;
			break;

		case SV_POTION_DEC_DEX:
			if (do_dec_stat(p_ptr, A_DEX)) ident = TRUE;
			break;

		case SV_POTION_DEC_CON:
			if (do_dec_stat(p_ptr, A_CON)) ident = TRUE;
			break;

		case SV_POTION_DEC_CHR:
			if (do_dec_stat(p_ptr, A_CHR)) ident = TRUE;
			break;

		case SV_POTION_DETONATIONS:
			ident = detonation(p_ptr);
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
			if (set_poisoned(p_ptr, p_ptr->poisoned / 2)) ident = TRUE;
			break;

		case SV_POTION_CURE_POISON:
			if (set_poisoned(p_ptr, 0)) ident = TRUE;
			break;

		case SV_POTION_BOLDNESS:
			if (set_afraid(p_ptr, 0)) ident = TRUE;
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
			ident = heroism(25);
			break;

		case SV_POTION_BESERK_STRENGTH:
			ident = berserk(randint1(25) + 25);
			break;

		case SV_POTION_CURE_LIGHT:
			ident = cure_light_wounds(2, 8);
			break;

		case SV_POTION_CURE_SERIOUS:
			ident = cure_serious_wounds(4, 8);
			break;

		case SV_POTION_CURE_CRITICAL:
			ident = cure_critical_wounds(damroll(6, 8));
			break;

		case SV_POTION_HEALING:
			ident = cure_critical_wounds(300);
			break;

		case SV_POTION_STAR_HEALING:
			ident = cure_critical_wounds(1200);
			break;

		case SV_POTION_LIFE:
			ident = life_stream(TRUE, TRUE);
			break;

		case SV_POTION_RESTORE_MANA:
			ident = restore_mana(TRUE);
			break;

		case SV_POTION_RESTORE_EXP:
			if (restore_level()) ident = TRUE;
			break;

		case SV_POTION_RES_STR:
			if (do_res_stat(p_ptr, A_STR)) ident = TRUE;
			break;

		case SV_POTION_RES_INT:
			if (do_res_stat(p_ptr, A_INT)) ident = TRUE;
			break;

		case SV_POTION_RES_WIS:
			if (do_res_stat(p_ptr, A_WIS)) ident = TRUE;
			break;

		case SV_POTION_RES_DEX:
			if (do_res_stat(p_ptr, A_DEX)) ident = TRUE;
			break;

		case SV_POTION_RES_CON:
			if (do_res_stat(p_ptr, A_CON)) ident = TRUE;
			break;

		case SV_POTION_RES_CHR:
			if (do_res_stat(p_ptr, A_CHR)) ident = TRUE;
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
				EXP ee = (p_ptr->exp / 2) + 10;
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
			if (true_healing(50)) ident = TRUE;
			break;

		case SV_POTION_INVULNERABILITY:
			(void)set_invuln(p_ptr->invuln + randint1(4) + 4, FALSE);
			ident = TRUE;
			break;

		case SV_POTION_NEW_LIFE:
			roll_hitdice(p_ptr, 0L);
			get_max_stats();
			p_ptr->update |= PU_BONUS;
			lose_all_mutations();
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
				lose_all_mutations();
			}
			else
			{
				do
				{
					if (one_in_(2))
					{
						if(gain_mutation(p_ptr, 0)) ident = TRUE;
					}
					else if (lose_mutation(0)) ident = TRUE;
				} while(!ident || one_in_(2));
			}
			break;
		}
	}

	if (PRACE_IS_(p_ptr, RACE_SKELETON))
	{
		msg_print(_("液体の一部はあなたのアゴを素通りして落ちた！", "Some of the fluid falls through your jaws!"));
		(void)potion_smash_effect(0, p_ptr->y, p_ptr->x, q_ptr->k_idx);
	}
	p_ptr->update |= (PU_COMBINE | PU_REORDER);

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
 * @brief 薬を飲むコマンドのメインルーチン /
 * Quaff some potion (from the pack or floor)
 * @return なし
 */
void do_cmd_quaff_potion(void)
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

	/* Restrict choices to potions */
	item_tester_hook = item_tester_hook_quaff;

	q = _("どの薬を飲みますか? ", "Quaff which potion? ");
	s = _("飲める薬がない。", "You have no potions to quaff.");

	if (!choose_object(&item, q, s, (USE_INVEN | USE_FLOOR), 0)) return;

	/* Quaff the potion */
	exe_quaff_potion(item);
}
