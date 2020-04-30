/*!
 * todo 単体で1000行を超えている。要分割
 * @brief 魔法によるプレーヤーへの効果まとめ
 * @date 2020/04/29
 * @author Hourier
 */

#include "angband.h"
#include "effect/effect-player-util.h"
#include "effect/effect-player.h"
#include "main/sound-definitions-table.h"
#include "player-damage.h"
#include "world.h"
#include "object-broken.h"
#include "artifact.h"
#include "player/mimic-info-table.h"
#include "realm/realm-hex.h"
#include "effect/spells-effect-util.h"
#include "player-move.h"
#include "player-effects.h"
#include "spells-status.h"
#include "monster-spell.h"
#include "mutation.h"
#include "object-curse.h"
#include "spell/spells-type.h"

typedef enum effect_player_check_result
{
	EP_CHECK_FALSE = 0,
	EP_CHECK_TRUE = 1,
	EP_CHECK_CONTINUE = 2,
} ep_check_result;

/*!
 * @brief effect_player_type構造体を初期化する
 * @param ep_ptr 初期化前の構造体
 * @param who 魔法を唱えたモンスター (0ならプレーヤー自身)
 * @param dam 基本威力
 * @param effect_type 効果属性
 * @param flag 効果フラグ
 * @param monspell 効果元のモンスター魔法ID
 * @return 初期化後の構造体ポインタ
 */
static effect_player_type *initialize_effect_player(effect_player_type *ep_ptr, MONSTER_IDX who, HIT_POINT dam, EFFECT_ID effect_type, BIT_FLAGS flag, int monspell)
{
	ep_ptr->rlev = 0;
	ep_ptr->m_ptr = NULL;
	ep_ptr->get_damage = 0;
	ep_ptr->who = who;
	ep_ptr->dam = dam;
	ep_ptr->effect_type = effect_type;
	ep_ptr->flag = flag;
	ep_ptr->monspell = monspell;
	return ep_ptr;
}


/*!
 * @brief ボルト魔法を反射する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param ep_ptr プレーヤー効果構造体への参照ポインタ
 * @return 当たったらFALSE、反射したらTRUE
 */
static bool process_bolt_reflection(player_type *target_ptr, effect_player_type *ep_ptr)
{
	bool can_bolt_hit = target_ptr->reflect || (((target_ptr->special_defense & KATA_FUUJIN) != 0) && !target_ptr->blind);
	can_bolt_hit &= (ep_ptr->flag & PROJECT_REFLECTABLE) != 0;
	can_bolt_hit &= !one_in_(10);
	if (!can_bolt_hit) return FALSE;

	POSITION t_y, t_x;
	int max_attempts = 10;
	sound(SOUND_REFLECT);

	if (target_ptr->blind)
		msg_print(_("何かが跳ね返った！", "Something bounces!"));
	else if (target_ptr->special_defense & KATA_FUUJIN)
		msg_print(_("風の如く武器を振るって弾き返した！", "The attack bounces!"));
	else
		msg_print(_("攻撃が跳ね返った！", "The attack bounces!"));

	if (ep_ptr->who > 0)
	{
		floor_type *floor_ptr = target_ptr->current_floor_ptr;
		monster_type m_type = floor_ptr->m_list[ep_ptr->who];
		do
		{
			t_y = m_type.fy - 1 + randint1(3);
			t_x = m_type.fx - 1 + randint1(3);
			max_attempts--;
		} while (max_attempts && in_bounds2u(floor_ptr, t_y, t_x) && !projectable(target_ptr, target_ptr->y, target_ptr->x, t_y, t_x));

		if (max_attempts < 1)
		{
			t_y = m_type.fy;
			t_x = m_type.fx;
		}
	}
	else
	{
		t_y = target_ptr->y - 1 + randint1(3);
		t_x = target_ptr->x - 1 + randint1(3);
	}

	project(target_ptr, 0, 0, t_y, t_x, ep_ptr->dam, ep_ptr->effect_type, (PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE), ep_ptr->monspell);
	disturb(target_ptr, TRUE, TRUE);
	return TRUE;
}


/*!
 * @brief 反射・忍者の変わり身などでそもそも当たらない状況を判定する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param ep_ptr プレーヤー効果構造体への参照ポインタ
 * @param y 目標Y座標
 * @param x 目標X座標
 * @return 当たらなかったらFALSE、反射したらTRUE、当たったらCONTINUE
 */
static ep_check_result check_continue_player_effect(player_type *target_ptr, effect_player_type *ep_ptr, POSITION y, POSITION x)
{
	if (!player_bold(target_ptr, y, x))
		return EP_CHECK_FALSE;

	if (((target_ptr->special_defense & NINJA_KAWARIMI) != 0) &&
		(ep_ptr->dam > 0) &&
		(randint0(55) < (target_ptr->lev * 3 / 5 + 20)) &&
		(ep_ptr->who > 0) &&
		(ep_ptr->who != target_ptr->riding) &&
		kawarimi(target_ptr, TRUE))
		return EP_CHECK_FALSE;

	if ((ep_ptr->who == 0) || (ep_ptr->who == target_ptr->riding))
		return EP_CHECK_FALSE;

	if (process_bolt_reflection(target_ptr, ep_ptr))
		return EP_CHECK_TRUE;

	return EP_CHECK_CONTINUE;
}


/*!
 * @brief 魔法を発したモンスター名を記述する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param ep_ptr プレーヤー効果構造体への参照ポインタ
 * @param who_name モンスター名
 * @return なし
 */
static void describe_effect_source(player_type *target_ptr, effect_player_type *ep_ptr, concptr who_name)
{
	if (ep_ptr->who > 0)
	{
		ep_ptr->m_ptr = &target_ptr->current_floor_ptr->m_list[ep_ptr->who];
		ep_ptr->rlev = (&r_info[ep_ptr->m_ptr->r_idx])->level >= 1 ? (&r_info[ep_ptr->m_ptr->r_idx])->level : 1;
		monster_desc(target_ptr, ep_ptr->m_name, ep_ptr->m_ptr, 0);
		strcpy(ep_ptr->killer, who_name);
		return;
	}

	switch (ep_ptr->who)
	{
	case PROJECT_WHO_UNCTRL_POWER:
		strcpy(ep_ptr->killer, _("制御できない力の氾流", "uncontrollable power storm"));
		break;
	case PROJECT_WHO_GLASS_SHARDS:
		strcpy(ep_ptr->killer, _("ガラスの破片", "shards of glass"));
		break;
	default:
		strcpy(ep_ptr->killer, _("罠", "a trap"));
		break;
	}

	strcpy(ep_ptr->m_name, ep_ptr->killer);
}


/*!
 * @brief 魔法の効果によって様々なメッセーを出力したり与えるダメージの増減を行ったりする
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param em_ptr プレーヤー効果構造体への参照ポインタ
 * @return なし
 */
void switch_effects_player(player_type *target_ptr, effect_player_type *ep_ptr)
{
	switch (ep_ptr->effect_type)
	{
	case GF_ACID:
	{
		if (target_ptr->blind) msg_print(_("酸で攻撃された！", "You are hit by acid!"));

		ep_ptr->get_damage = acid_dam(target_ptr, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell, FALSE);
		break;
	}
	case GF_FIRE:
	{
		if (target_ptr->blind) msg_print(_("火炎で攻撃された！", "You are hit by fire!"));

		ep_ptr->get_damage = fire_dam(target_ptr, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell, FALSE);
		break;
	}
	case GF_COLD:
	{
		if (target_ptr->blind) msg_print(_("冷気で攻撃された！", "You are hit by cold!"));

		ep_ptr->get_damage = cold_dam(target_ptr, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell, FALSE);
		break;
	}
	case GF_ELEC:
	{
		if (target_ptr->blind) msg_print(_("電撃で攻撃された！", "You are hit by lightning!"));

		ep_ptr->get_damage = elec_dam(target_ptr, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell, FALSE);
		break;
	}
	case GF_POIS:
	{
		bool double_resist = is_oppose_pois(target_ptr);
		if (target_ptr->blind) msg_print(_("毒で攻撃された！", "You are hit by poison!"));

		if (target_ptr->resist_pois) ep_ptr->dam = (ep_ptr->dam + 2) / 3;
		if (double_resist) ep_ptr->dam = (ep_ptr->dam + 2) / 3;

		if ((!(double_resist || target_ptr->resist_pois)) && one_in_(HURT_CHANCE) && !CHECK_MULTISHADOW(target_ptr))
		{
			do_dec_stat(target_ptr, A_CON);
		}

		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);

		if (!(double_resist || target_ptr->resist_pois) && !CHECK_MULTISHADOW(target_ptr))
			set_poisoned(target_ptr, target_ptr->poisoned + randint0(ep_ptr->dam) + 10);

		break;
	}
	case GF_NUKE:
	{
		bool double_resist = is_oppose_pois(target_ptr);
		if (target_ptr->blind) msg_print(_("放射能で攻撃された！", "You are hit by radiation!"));

		if (target_ptr->resist_pois) ep_ptr->dam = (2 * ep_ptr->dam + 2) / 5;
		if (double_resist) ep_ptr->dam = (2 * ep_ptr->dam + 2) / 5;
		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		if ((double_resist || target_ptr->resist_pois) || CHECK_MULTISHADOW(target_ptr))
			break;

		set_poisoned(target_ptr, target_ptr->poisoned + randint0(ep_ptr->dam) + 10);

		if (one_in_(5)) /* 6 */
		{
			msg_print(_("奇形的な変身を遂げた！", "You undergo a freakish metamorphosis!"));
			if (one_in_(4)) /* 4 */
				do_poly_self(target_ptr);
			else
				status_shuffle(target_ptr);
		}

		if (one_in_(6))
		{
			inventory_damage(target_ptr, set_acid_destroy, 2);
		}

		break;
	}
	case GF_MISSILE:
	{
		if (target_ptr->blind) msg_print(_("何かで攻撃された！", "You are hit by something!"));
		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_HOLY_FIRE:
	{
		if (target_ptr->blind) msg_print(_("何かで攻撃された！", "You are hit by something!"));
		if (target_ptr->align > 10)
			ep_ptr->dam /= 2;
		else if (target_ptr->align < -10)
			ep_ptr->dam *= 2;
		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_HELL_FIRE:
	{
		if (target_ptr->blind) msg_print(_("何かで攻撃された！", "You are hit by something!"));
		if (target_ptr->align > 10)
			ep_ptr->dam *= 2;
		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_ARROW:
	{
		if (target_ptr->blind)
		{
			msg_print(_("何か鋭いもので攻撃された！", "You are hit by something sharp!"));
		}
		else if ((target_ptr->inventory_list[INVEN_RARM].name1 == ART_ZANTETSU) || (target_ptr->inventory_list[INVEN_LARM].name1 == ART_ZANTETSU))
		{
			msg_print(_("矢を斬り捨てた！", "You cut down the arrow!"));
			break;
		}

		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_PLASMA:
	{
		if (target_ptr->blind) msg_print(_("何かとても熱いもので攻撃された！", "You are hit by something *HOT*!"));
		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);

		if (!target_ptr->resist_sound && !CHECK_MULTISHADOW(target_ptr))
		{
			int plus_stun = (randint1((ep_ptr->dam > 40) ? 35 : (ep_ptr->dam * 3 / 4 + 5)));
			(void)set_stun(target_ptr, target_ptr->stun + plus_stun);
		}

		if (!(target_ptr->resist_fire || is_oppose_fire(target_ptr) || target_ptr->immune_fire))
		{
			inventory_damage(target_ptr, set_acid_destroy, 3);
		}

		break;
	}
	case GF_NETHER:
	{
		if (target_ptr->blind) msg_print(_("地獄の力で攻撃された！", "You are hit by nether forces!"));
		if (target_ptr->resist_neth)
		{
			if (!PRACE_IS_(target_ptr, RACE_SPECTRE))
			{
				ep_ptr->dam *= 6; ep_ptr->dam /= (randint1(4) + 7);
			}
		}
		else if (!CHECK_MULTISHADOW(target_ptr)) drain_exp(target_ptr, 200 + (target_ptr->exp / 100), 200 + (target_ptr->exp / 1000), 75);

		if (PRACE_IS_(target_ptr, RACE_SPECTRE) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("気分がよくなった。", "You feel invigorated!"));
			hp_player(target_ptr, ep_ptr->dam / 4);
			learn_spell(target_ptr, ep_ptr->monspell);
		}
		else
		{
			ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		}

		break;
	}
	case GF_WATER:
	{
		if (target_ptr->blind) msg_print(_("何か湿ったもので攻撃された！", "You are hit by something wet!"));
		if (CHECK_MULTISHADOW(target_ptr))
		{
			ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
			break;
		}

		if (!target_ptr->resist_sound && !target_ptr->resist_water)
		{
			set_stun(target_ptr, target_ptr->stun + randint1(40));
		}
		if (!target_ptr->resist_conf && !target_ptr->resist_water)
		{
			set_confused(target_ptr, target_ptr->confused + randint1(5) + 5);
		}

		if (one_in_(5) && !target_ptr->resist_water)
		{
			inventory_damage(target_ptr, set_cold_destroy, 3);
		}

		if (target_ptr->resist_water) ep_ptr->get_damage /= 4;

		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_CHAOS:
	{
		if (target_ptr->blind) msg_print(_("無秩序の波動で攻撃された！", "You are hit by a wave of anarchy!"));
		if (target_ptr->resist_chaos)
		{
			ep_ptr->dam *= 6; ep_ptr->dam /= (randint1(4) + 7);
		}

		if (CHECK_MULTISHADOW(target_ptr))
		{
			ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
			break;
		}

		if (!target_ptr->resist_conf)
		{
			(void)set_confused(target_ptr, target_ptr->confused + randint0(20) + 10);
		}
		if (!target_ptr->resist_chaos)
		{
			(void)set_image(target_ptr, target_ptr->image + randint1(10));
			if (one_in_(3))
			{
				msg_print(_("あなたの身体はカオスの力で捻じ曲げられた！", "Your body is twisted by chaos!"));
				(void)gain_mutation(target_ptr, 0);
			}
		}
		if (!target_ptr->resist_neth && !target_ptr->resist_chaos)
		{
			drain_exp(target_ptr, 5000 + (target_ptr->exp / 100), 500 + (target_ptr->exp / 1000), 75);
		}

		if (!target_ptr->resist_chaos || one_in_(9))
		{
			inventory_damage(target_ptr, set_elec_destroy, 2);
			inventory_damage(target_ptr, set_fire_destroy, 2);
		}

		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_SHARDS:
	{
		if (target_ptr->blind) msg_print(_("何か鋭いもので攻撃された！", "You are hit by something sharp!"));
		if (target_ptr->resist_shard)
		{
			ep_ptr->dam *= 6; ep_ptr->dam /= (randint1(4) + 7);
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_cut(target_ptr, target_ptr->cut + ep_ptr->dam);
		}

		if (!target_ptr->resist_shard || one_in_(13))
		{
			inventory_damage(target_ptr, set_cold_destroy, 2);
		}

		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_SOUND:
	{
		if (target_ptr->blind) msg_print(_("轟音で攻撃された！", "You are hit by a loud noise!"));
		if (target_ptr->resist_sound)
		{
			ep_ptr->dam *= 5; ep_ptr->dam /= (randint1(4) + 7);
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			int plus_stun = (randint1((ep_ptr->dam > 90) ? 35 : (ep_ptr->dam / 3 + 5)));
			(void)set_stun(target_ptr, target_ptr->stun + plus_stun);
		}

		if (!target_ptr->resist_sound || one_in_(13))
		{
			inventory_damage(target_ptr, set_cold_destroy, 2);
		}

		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_CONFUSION:
	{
		if (target_ptr->blind) msg_print(_("何か混乱するもので攻撃された！", "You are hit by something puzzling!"));
		if (target_ptr->resist_conf)
		{
			ep_ptr->dam *= 5; ep_ptr->dam /= (randint1(4) + 7);
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_confused(target_ptr, target_ptr->confused + randint1(20) + 10);
		}
		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_DISENCHANT:
	{
		if (target_ptr->blind) msg_print(_("何かさえないもので攻撃された！", "You are hit by something static!"));
		if (target_ptr->resist_disen)
		{
			ep_ptr->dam *= 6; ep_ptr->dam /= (randint1(4) + 7);
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			(void)apply_disenchant(target_ptr, 0);
		}
		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_NEXUS:
	{
		if (target_ptr->blind) msg_print(_("何か奇妙なもので攻撃された！", "You are hit by something strange!"));
		if (target_ptr->resist_nexus)
		{
			ep_ptr->dam *= 6; ep_ptr->dam /= (randint1(4) + 7);
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			apply_nexus(ep_ptr->m_ptr, target_ptr);
		}

		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_FORCE:
	{
		if (target_ptr->blind) msg_print(_("運動エネルギーで攻撃された！", "You are hit by kinetic force!"));
		if (!target_ptr->resist_sound && !CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_stun(target_ptr, target_ptr->stun + randint1(20));
		}

		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_ROCKET:
	{
		if (target_ptr->blind) msg_print(_("爆発があった！", "There is an explosion!"));
		if (!target_ptr->resist_sound && !CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_stun(target_ptr, target_ptr->stun + randint1(20));
		}

		if (target_ptr->resist_shard)
		{
			ep_ptr->dam /= 2;
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_cut(target_ptr, target_ptr->cut + (ep_ptr->dam / 2));
		}

		if (!target_ptr->resist_shard || one_in_(12))
		{
			inventory_damage(target_ptr, set_cold_destroy, 3);
		}

		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_INERTIAL:
	{
		if (target_ptr->blind) msg_print(_("何か遅いもので攻撃された！", "You are hit by something slow!"));
		if (!CHECK_MULTISHADOW(target_ptr)) (void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, FALSE);

		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_LITE:
	{
		if (target_ptr->blind) msg_print(_("何かで攻撃された！", "You are hit by something!"));
		if (target_ptr->resist_lite)
		{
			ep_ptr->dam *= 4; ep_ptr->dam /= (randint1(4) + 7);
		}
		else if (!target_ptr->blind && !target_ptr->resist_blind && !CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_blind(target_ptr, target_ptr->blind + randint1(5) + 2);
		}

		if (PRACE_IS_(target_ptr, RACE_VAMPIRE) || (target_ptr->mimic_form == MIMIC_VAMPIRE))
		{
			if (!CHECK_MULTISHADOW(target_ptr)) msg_print(_("光で肉体が焦がされた！", "The light scorches your flesh!"));
			ep_ptr->dam *= 2;
		}
		else if (PRACE_IS_(target_ptr, RACE_S_FAIRY))
		{
			ep_ptr->dam = ep_ptr->dam * 4 / 3;
		}

		if (target_ptr->wraith_form) ep_ptr->dam *= 2;
		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);

		if (!target_ptr->wraith_form || CHECK_MULTISHADOW(target_ptr))
			break;

		target_ptr->wraith_form = 0;
		msg_print(_("閃光のため非物質的な影の存在でいられなくなった。",
			"The light forces you out of your incorporeal shadow form."));

		target_ptr->redraw |= (PR_MAP | PR_STATUS);
		target_ptr->update |= (PU_MONSTERS);
		target_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
		break;
	}
	case GF_DARK:
	{
		if (target_ptr->blind) msg_print(_("何かで攻撃された！", "You are hit by something!"));
		if (target_ptr->resist_dark)
		{
			ep_ptr->dam *= 4; ep_ptr->dam /= (randint1(4) + 7);

			if (PRACE_IS_(target_ptr, RACE_VAMPIRE) || (target_ptr->mimic_form == MIMIC_VAMPIRE) || target_ptr->wraith_form) ep_ptr->dam = 0;
		}
		else if (!target_ptr->blind && !target_ptr->resist_blind && !CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_blind(target_ptr, target_ptr->blind + randint1(5) + 2);
		}

		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_TIME:
	{
		if (target_ptr->blind) msg_print(_("過去からの衝撃に攻撃された！", "You are hit by a blast from the past!"));

		if (target_ptr->resist_time)
		{
			ep_ptr->dam *= 4;
			ep_ptr->dam /= (randint1(4) + 7);
			msg_print(_("時間が通り過ぎていく気がする。", "You feel as if time is passing you by."));
			ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
			break;
		}

		if (CHECK_MULTISHADOW(target_ptr))
		{
			ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
			break;
		}

		switch (randint1(10))
		{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		{
			if (target_ptr->prace == RACE_ANDROID) break;

			msg_print(_("人生が逆戻りした気がする。", "You feel like a chunk of the past has been ripped away."));
			lose_exp(target_ptr, 100 + (target_ptr->exp / 100) * MON_DRAIN_LIFE);
			break;
		}
		case 6:
		case 7:
		case 8:
		case 9:
		{
			int k = 0;
			concptr act = NULL;
			switch (randint1(6))
			{
			case 1: k = A_STR; act = _("強く", "strong"); break;
			case 2: k = A_INT; act = _("聡明で", "bright"); break;
			case 3: k = A_WIS; act = _("賢明で", "wise"); break;
			case 4: k = A_DEX; act = _("器用で", "agile"); break;
			case 5: k = A_CON; act = _("健康で", "hale"); break;
			case 6: k = A_CHR; act = _("美しく", "beautiful"); break;
			}

			msg_format(_("あなたは以前ほど%sなくなってしまった...。", "You're not as %s as you used to be..."), act);
			target_ptr->stat_cur[k] = (target_ptr->stat_cur[k] * 3) / 4;
			if (target_ptr->stat_cur[k] < 3) target_ptr->stat_cur[k] = 3;

			target_ptr->update |= (PU_BONUS);
			break;
		}
		case 10:
		{
			msg_print(_("あなたは以前ほど力強くなくなってしまった...。", "You're not as powerful as you used to be..."));
			for (int k = 0; k < A_MAX; k++)
			{
				target_ptr->stat_cur[k] = (target_ptr->stat_cur[k] * 7) / 8;
				if (target_ptr->stat_cur[k] < 3) target_ptr->stat_cur[k] = 3;
			}

			target_ptr->update |= (PU_BONUS);
			break;
		}
		}

		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_GRAVITY:
	{
		if (target_ptr->blind) msg_print(_("何か重いもので攻撃された！", "You are hit by something heavy!"));
		msg_print(_("周辺の重力がゆがんだ。", "Gravity warps around you."));

		if (!CHECK_MULTISHADOW(target_ptr))
		{
			teleport_player(target_ptr, 5, TELEPORT_PASSIVE);
			if (!target_ptr->levitation)
				(void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, FALSE);
			if (!(target_ptr->resist_sound || target_ptr->levitation))
			{
				int plus_stun = (randint1((ep_ptr->dam > 90) ? 35 : (ep_ptr->dam / 3 + 5)));
				(void)set_stun(target_ptr, target_ptr->stun + plus_stun);
			}
		}

		if (target_ptr->levitation)
		{
			ep_ptr->dam = (ep_ptr->dam * 2) / 3;
		}

		if (!target_ptr->levitation || one_in_(13))
		{
			inventory_damage(target_ptr, set_cold_destroy, 2);
		}

		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_DISINTEGRATE:
	{
		if (target_ptr->blind) msg_print(_("純粋なエネルギーで攻撃された！", "You are hit by pure energy!"));

		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_OLD_HEAL:
	{
		if (target_ptr->blind) msg_print(_("何らかの攻撃によって気分がよくなった。", "You are hit by something invigorating!"));

		(void)hp_player(target_ptr, ep_ptr->dam);
		ep_ptr->dam = 0;
		break;
	}
	case GF_OLD_SPEED:
	{
		if (target_ptr->blind) msg_print(_("何かで攻撃された！", "You are hit by something!"));
		(void)set_fast(target_ptr, target_ptr->fast + randint1(5), FALSE);
		ep_ptr->dam = 0;
		break;
	}
	case GF_OLD_SLOW:
	{
		if (target_ptr->blind) msg_print(_("何か遅いもので攻撃された！", "You are hit by something slow!"));
		(void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, FALSE);
		break;
	}
	case GF_OLD_SLEEP:
	{
		if (target_ptr->free_act)  break;
		if (target_ptr->blind) msg_print(_("眠ってしまった！", "You fall asleep!"));

		if (ironman_nightmare)
		{
			msg_print(_("恐ろしい光景が頭に浮かんできた。", "A horrible vision enters your mind."));
			/* Have some nightmares */
			sanity_blast(target_ptr, NULL, FALSE);
		}

		set_paralyzed(target_ptr, target_ptr->paralyzed + ep_ptr->dam);
		ep_ptr->dam = 0;
		break;
	}
	case GF_MANA:
	case GF_SEEKER:
	case GF_SUPER_RAY:
	{
		if (target_ptr->blind) msg_print(_("魔法のオーラで攻撃された！", "You are hit by an aura of magic!"));

		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_PSY_SPEAR:
	{
		if (target_ptr->blind) msg_print(_("エネルギーの塊で攻撃された！", "You are hit by an energy!"));

		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_FORCE, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_METEOR:
	{
		if (target_ptr->blind) msg_print(_("何かが空からあなたの頭上に落ちてきた！", "Something falls from the sky on you!"));

		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		if (!target_ptr->resist_shard || one_in_(13))
		{
			if (!target_ptr->immune_fire) inventory_damage(target_ptr, set_fire_destroy, 2);
			inventory_damage(target_ptr, set_cold_destroy, 2);
		}

		break;
	}
	case GF_ICE:
	{
		if (target_ptr->blind) msg_print(_("何か鋭く冷たいもので攻撃された！", "You are hit by something sharp and cold!"));

		ep_ptr->get_damage = cold_dam(target_ptr, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell, FALSE);
		if (CHECK_MULTISHADOW(target_ptr)) break;

		if (!target_ptr->resist_shard)
		{
			(void)set_cut(target_ptr, target_ptr->cut + damroll(5, 8));
		}

		if (!target_ptr->resist_sound)
		{
			(void)set_stun(target_ptr, target_ptr->stun + randint1(15));
		}

		if ((!(target_ptr->resist_cold || is_oppose_cold(target_ptr))) || one_in_(12))
		{
			if (!target_ptr->immune_cold) inventory_damage(target_ptr, set_cold_destroy, 3);
		}

		break;
	}
	case GF_DEATH_RAY:
	{
		if (target_ptr->blind) msg_print(_("何か非常に冷たいもので攻撃された！", "You are hit by something extremely cold!"));

		if (target_ptr->mimic_form)
		{
			if (!(mimic_info[target_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_NONLIVING))
				ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);

			break;
		}

		switch (target_ptr->prace)
		{
		case RACE_GOLEM:
		case RACE_SKELETON:
		case RACE_ZOMBIE:
		case RACE_VAMPIRE:
		case RACE_DEMON:
		case RACE_SPECTRE:
		{
			ep_ptr->dam = 0;
			break;
		}
		default:
		{
			ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
			break;
		}
		}

		break;
	}
	case GF_DRAIN_MANA:
	{
		if (CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("攻撃は幻影に命中し、あなたには届かなかった。", "The attack hits Shadow, but you are unharmed!"));
			ep_ptr->dam = 0;
			break;
		}

		if (target_ptr->csp == 0)
		{
			ep_ptr->dam = 0;
			break;
		}

		if (ep_ptr->who > 0)
			msg_format(_("%^sに精神エネルギーを吸い取られてしまった！", "%^s draws psychic energy from you!"), ep_ptr->m_name);
		else
			msg_print(_("精神エネルギーを吸い取られてしまった！", "Your psychic energy is drawn!"));

		if (ep_ptr->dam >= target_ptr->csp)
		{
			ep_ptr->dam = target_ptr->csp;
			target_ptr->csp = 0;
			target_ptr->csp_frac = 0;
		}
		else
		{
			target_ptr->csp -= ep_ptr->dam;
		}

		learn_spell(target_ptr, ep_ptr->monspell);
		target_ptr->redraw |= (PR_MANA);
		target_ptr->window |= (PW_PLAYER | PW_SPELL);

		if ((ep_ptr->who <= 0) || (ep_ptr->m_ptr->hp >= ep_ptr->m_ptr->maxhp))
		{
			ep_ptr->dam = 0;
			break;
		}

		ep_ptr->m_ptr->hp += ep_ptr->dam;
		if (ep_ptr->m_ptr->hp > ep_ptr->m_ptr->maxhp) ep_ptr->m_ptr->hp = ep_ptr->m_ptr->maxhp;

		if (target_ptr->health_who == ep_ptr->who) target_ptr->redraw |= (PR_HEALTH);
		if (target_ptr->riding == ep_ptr->who) target_ptr->redraw |= (PR_UHEALTH);

		if (ep_ptr->m_ptr->ml)
		{
			msg_format(_("%^sは気分が良さそうだ。", "%^s appears healthier."), ep_ptr->m_name);
		}

		ep_ptr->dam = 0;
		break;
	}
	case GF_MIND_BLAST:
	{
		if ((randint0(100 + ep_ptr->rlev / 2) < MAX(5, target_ptr->skill_sav)) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			learn_spell(target_ptr, ep_ptr->monspell);
			break;
		}

		if (CHECK_MULTISHADOW(target_ptr))
		{
			ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
			break;
		}

		msg_print(_("霊的エネルギーで精神が攻撃された。", "Your mind is blasted by psionic energy."));
		if (!target_ptr->resist_conf)
		{
			(void)set_confused(target_ptr, target_ptr->confused + randint0(4) + 4);
		}

		if (!target_ptr->resist_chaos && one_in_(3))
		{
			(void)set_image(target_ptr, target_ptr->image + randint0(250) + 150);
		}

		target_ptr->csp -= 50;
		if (target_ptr->csp < 0)
		{
			target_ptr->csp = 0;
			target_ptr->csp_frac = 0;
		}

		target_ptr->redraw |= PR_MANA;
		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		break;
	}
	case GF_BRAIN_SMASH:
	{
		if ((randint0(100 + ep_ptr->rlev / 2) < MAX(5, target_ptr->skill_sav)) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			learn_spell(target_ptr, ep_ptr->monspell);
			break;
		}

		if (!CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("霊的エネルギーで精神が攻撃された。", "Your mind is blasted by psionic energy."));

			target_ptr->csp -= 100;
			if (target_ptr->csp < 0)
			{
				target_ptr->csp = 0;
				target_ptr->csp_frac = 0;
			}
			target_ptr->redraw |= PR_MANA;
		}

		ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		if (CHECK_MULTISHADOW(target_ptr)) break;

		if (!target_ptr->resist_blind)
		{
			(void)set_blind(target_ptr, target_ptr->blind + 8 + randint0(8));
		}

		if (!target_ptr->resist_conf)
		{
			(void)set_confused(target_ptr, target_ptr->confused + randint0(4) + 4);
		}

		if (!target_ptr->free_act)
		{
			(void)set_paralyzed(target_ptr, target_ptr->paralyzed + randint0(4) + 4);
		}

		(void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, FALSE);

		while (randint0(100 + ep_ptr->rlev / 2) > (MAX(5, target_ptr->skill_sav)))
			(void)do_dec_stat(target_ptr, A_INT);
		while (randint0(100 + ep_ptr->rlev / 2) > (MAX(5, target_ptr->skill_sav)))
			(void)do_dec_stat(target_ptr, A_WIS);

		if (!target_ptr->resist_chaos)
		{
			(void)set_image(target_ptr, target_ptr->image + randint0(250) + 150);
		}

		break;
	}
	case GF_CAUSE_1:
	{
		if ((randint0(100 + ep_ptr->rlev / 2) < target_ptr->skill_sav) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			learn_spell(target_ptr, ep_ptr->monspell);
		}
		else
		{
			if (!CHECK_MULTISHADOW(target_ptr)) curse_equipment(target_ptr, 15, 0);
			ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		}
		break;
	}
	case GF_CAUSE_2:
	{
		if ((randint0(100 + ep_ptr->rlev / 2) < target_ptr->skill_sav) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			learn_spell(target_ptr, ep_ptr->monspell);
		}
		else
		{
			if (!CHECK_MULTISHADOW(target_ptr)) curse_equipment(target_ptr, 25, MIN(ep_ptr->rlev / 2 - 15, 5));
			ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		}
		break;
	}
	case GF_CAUSE_3:
	{
		if ((randint0(100 + ep_ptr->rlev / 2) < target_ptr->skill_sav) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			learn_spell(target_ptr, ep_ptr->monspell);
		}
		else
		{
			if (!CHECK_MULTISHADOW(target_ptr)) curse_equipment(target_ptr, 33, MIN(ep_ptr->rlev / 2 - 15, 15));
			ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
		}
		break;
	}
	case GF_CAUSE_4:
	{
		if ((randint0(100 + ep_ptr->rlev / 2) < target_ptr->skill_sav) && !(ep_ptr->m_ptr->r_idx == MON_KENSHIROU) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし秘孔を跳ね返した！", "You resist the effects!"));
			learn_spell(target_ptr, ep_ptr->monspell);
		}
		else
		{
			ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
			if (!CHECK_MULTISHADOW(target_ptr)) (void)set_cut(target_ptr, target_ptr->cut + damroll(10, 10));
		}

		break;
	}
	case GF_HAND_DOOM:
	{
		if ((randint0(100 + ep_ptr->rlev / 2) < target_ptr->skill_sav) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			learn_spell(target_ptr, ep_ptr->monspell);
		}
		else
		{
			if (!CHECK_MULTISHADOW(target_ptr))
			{
				msg_print(_("あなたは命が薄まっていくように感じた！", "You feel your life fade away!"));
				curse_equipment(target_ptr, 40, 20);
			}

			ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->m_name, ep_ptr->monspell);

			if (target_ptr->chp < 1) target_ptr->chp = 1;
		}

		break;
	}
	default:
	{
		ep_ptr->dam = 0;
		break;
	}
	}
}


/*!
 * @brief 汎用的なビーム/ボルト/ボール系によるプレイヤーへの効果処理 / Helper function for "project()" below.
 * @param who 魔法を発動したモンスター(0ならばプレイヤー、負値ならば自然発生) / Index of "source" monster (zero for "player")
 * @param who_name 効果を起こしたモンスターの名前
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param effect_type 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flag 効果フラグ
 * @param monspell 効果元のモンスター魔法ID
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 */
bool affect_player(MONSTER_IDX who, player_type *target_ptr, concptr who_name, int r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID effect_type, BIT_FLAGS flag, int monspell)
{
	effect_player_type tmp_effect;
	effect_player_type *ep_ptr = initialize_effect_player(&tmp_effect, who, dam, effect_type, flag, monspell);
	ep_check_result check_result = check_continue_player_effect(target_ptr, ep_ptr, y, x);
	if (check_result != EP_CHECK_CONTINUE) return check_result;

	if (ep_ptr->dam > 1600) ep_ptr->dam = 1600;

	ep_ptr->dam = (ep_ptr->dam + r) / (r + 1);
	describe_effect_source(target_ptr, ep_ptr, who_name);
	switch_effects_player(target_ptr, ep_ptr);

	revenge_store(target_ptr, ep_ptr->get_damage);
	if ((target_ptr->tim_eyeeye || hex_spelling(target_ptr, HEX_EYE_FOR_EYE))
		&& (ep_ptr->get_damage > 0) && !target_ptr->is_dead && (ep_ptr->who > 0))
	{
		GAME_TEXT m_name_self[80];
		monster_desc(target_ptr, m_name_self, ep_ptr->m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);
		msg_format(_("攻撃が%s自身を傷つけた！", "The attack of %s has wounded %s!"), ep_ptr->m_name, m_name_self);
		project(target_ptr, 0, 0, ep_ptr->m_ptr->fy, ep_ptr->m_ptr->fx, ep_ptr->get_damage, GF_MISSILE, PROJECT_KILL, -1);
		if (target_ptr->tim_eyeeye) set_tim_eyeeye(target_ptr, target_ptr->tim_eyeeye - 5, TRUE);
	}

	if (target_ptr->riding && ep_ptr->dam > 0)
	{
		rakubadam_p = (ep_ptr->dam > 200) ? 200 : ep_ptr->dam;
	}

	disturb(target_ptr, TRUE, TRUE);
	if ((target_ptr->special_defense & NINJA_KAWARIMI) && ep_ptr->dam && ep_ptr->who && (ep_ptr->who != target_ptr->riding))
	{
		(void)kawarimi(target_ptr, FALSE);
	}

	return TRUE;
}
