/*!
 * todo 単体で1000行を超えている。要分割
 * @brief 魔法によるプレーヤーへの効果まとめ
 * @date 2020/04/29
 * @author Hourier
 */

#include "angband.h"
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

/*!
 * @brief 汎用的なビーム/ボルト/ボール系によるプレイヤーへの効果処理 / Helper function for "project()" below.
 * @param who 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source" monster (zero for "player")
 * @param who_name 効果を起こしたモンスターの名前
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param typ 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flag 効果フラグ
 * @param monspell 効果元のモンスター魔法ID
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 */
bool affect_player(MONSTER_IDX who, player_type *target_ptr, concptr who_name, int r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ, BIT_FLAGS flag, int monspell)
{
	int k = 0;
	DEPTH rlev = 0;
	bool obvious = TRUE;
	bool blind = (target_ptr->blind ? TRUE : FALSE);
	bool fuzzy = FALSE;
	monster_type *m_ptr = NULL;
	GAME_TEXT m_name[MAX_NLEN];
	char killer[80];
	concptr act = NULL;
	int get_damage = 0;
	if (!player_bold(target_ptr, y, x)) return FALSE;

	if ((target_ptr->special_defense & NINJA_KAWARIMI) && dam && (randint0(55) < (target_ptr->lev * 3 / 5 + 20)) && who && (who != target_ptr->riding))
	{
		if (kawarimi(target_ptr, TRUE)) return FALSE;
	}

	if (!who) return FALSE;
	if (who == target_ptr->riding) return FALSE;

	if ((target_ptr->reflect || ((target_ptr->special_defense & KATA_FUUJIN) && !target_ptr->blind)) && (flag & PROJECT_REFLECTABLE) && !one_in_(10))
	{
		POSITION t_y, t_x;
		int max_attempts = 10;
		sound(SOUND_REFLECT);

		if (blind)
			msg_print(_("何かが跳ね返った！", "Something bounces!"));
		else if (target_ptr->special_defense & KATA_FUUJIN)
			msg_print(_("風の如く武器を振るって弾き返した！", "The attack bounces!"));
		else
			msg_print(_("攻撃が跳ね返った！", "The attack bounces!"));

		if (who > 0)
		{
			do
			{
				t_y = target_ptr->current_floor_ptr->m_list[who].fy - 1 + randint1(3);
				t_x = target_ptr->current_floor_ptr->m_list[who].fx - 1 + randint1(3);
				max_attempts--;
			} while (max_attempts && in_bounds2u(target_ptr->current_floor_ptr, t_y, t_x) && !projectable(target_ptr, target_ptr->y, target_ptr->x, t_y, t_x));

			if (max_attempts < 1)
			{
				t_y = target_ptr->current_floor_ptr->m_list[who].fy;
				t_x = target_ptr->current_floor_ptr->m_list[who].fx;
			}
		}
		else
		{
			t_y = target_ptr->y - 1 + randint1(3);
			t_x = target_ptr->x - 1 + randint1(3);
		}

		project(target_ptr, 0, 0, t_y, t_x, dam, typ, (PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE), monspell);
		disturb(target_ptr, TRUE, TRUE);
		return TRUE;
	}

	if (dam > 1600) dam = 1600;

	dam = (dam + r) / (r + 1);
	if (blind) fuzzy = TRUE;

	if (who > 0)
	{
		m_ptr = &target_ptr->current_floor_ptr->m_list[who];
		rlev = (((&r_info[m_ptr->r_idx])->level >= 1) ? (&r_info[m_ptr->r_idx])->level : 1);
		monster_desc(target_ptr, m_name, m_ptr, 0);
		strcpy(killer, who_name);
	}
	else
	{
		switch (who)
		{
		case PROJECT_WHO_UNCTRL_POWER:
			strcpy(killer, _("制御できない力の氾流", "uncontrollable power storm"));
			break;

		case PROJECT_WHO_GLASS_SHARDS:
			strcpy(killer, _("ガラスの破片", "shards of glass"));
			break;

		default:
			strcpy(killer, _("罠", "a trap"));
			break;
		}
		strcpy(m_name, killer);
	}

	switch (typ)
	{
	case GF_ACID:
	{
		if (fuzzy) msg_print(_("酸で攻撃された！", "You are hit by acid!"));

		get_damage = acid_dam(target_ptr, dam, killer, monspell, FALSE);
		break;
	}
	case GF_FIRE:
	{
		if (fuzzy) msg_print(_("火炎で攻撃された！", "You are hit by fire!"));

		get_damage = fire_dam(target_ptr, dam, killer, monspell, FALSE);
		break;
	}
	case GF_COLD:
	{
		if (fuzzy) msg_print(_("冷気で攻撃された！", "You are hit by cold!"));

		get_damage = cold_dam(target_ptr, dam, killer, monspell, FALSE);
		break;
	}
	case GF_ELEC:
	{
		if (fuzzy) msg_print(_("電撃で攻撃された！", "You are hit by lightning!"));

		get_damage = elec_dam(target_ptr, dam, killer, monspell, FALSE);
		break;
	}
	case GF_POIS:
	{
		bool double_resist = is_oppose_pois(target_ptr);
		if (fuzzy) msg_print(_("毒で攻撃された！", "You are hit by poison!"));

		if (target_ptr->resist_pois) dam = (dam + 2) / 3;
		if (double_resist) dam = (dam + 2) / 3;

		if ((!(double_resist || target_ptr->resist_pois)) && one_in_(HURT_CHANCE) && !CHECK_MULTISHADOW(target_ptr))
		{
			do_dec_stat(target_ptr, A_CON);
		}

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);

		if (!(double_resist || target_ptr->resist_pois) && !CHECK_MULTISHADOW(target_ptr))
			set_poisoned(target_ptr, target_ptr->poisoned + randint0(dam) + 10);

		break;
	}
	case GF_NUKE:
	{
		bool double_resist = is_oppose_pois(target_ptr);
		if (fuzzy) msg_print(_("放射能で攻撃された！", "You are hit by radiation!"));

		if (target_ptr->resist_pois) dam = (2 * dam + 2) / 5;
		if (double_resist) dam = (2 * dam + 2) / 5;
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		if ((double_resist || target_ptr->resist_pois) || CHECK_MULTISHADOW(target_ptr))
			break;

		set_poisoned(target_ptr, target_ptr->poisoned + randint0(dam) + 10);

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
		if (fuzzy) msg_print(_("何かで攻撃された！", "You are hit by something!"));
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_HOLY_FIRE:
	{
		if (fuzzy) msg_print(_("何かで攻撃された！", "You are hit by something!"));
		if (target_ptr->align > 10)
			dam /= 2;
		else if (target_ptr->align < -10)
			dam *= 2;
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_HELL_FIRE:
	{
		if (fuzzy) msg_print(_("何かで攻撃された！", "You are hit by something!"));
		if (target_ptr->align > 10)
			dam *= 2;
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_ARROW:
	{
		if (fuzzy)
		{
			msg_print(_("何か鋭いもので攻撃された！", "You are hit by something sharp!"));
		}
		else if ((target_ptr->inventory_list[INVEN_RARM].name1 == ART_ZANTETSU) || (target_ptr->inventory_list[INVEN_LARM].name1 == ART_ZANTETSU))
		{
			msg_print(_("矢を斬り捨てた！", "You cut down the arrow!"));
			break;
		}

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_PLASMA:
	{
		if (fuzzy) msg_print(_("何かとても熱いもので攻撃された！", "You are hit by something *HOT*!"));
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);

		if (!target_ptr->resist_sound && !CHECK_MULTISHADOW(target_ptr))
		{
			int plus_stun = (randint1((dam > 40) ? 35 : (dam * 3 / 4 + 5)));
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
		if (fuzzy) msg_print(_("地獄の力で攻撃された！", "You are hit by nether forces!"));
		if (target_ptr->resist_neth)
		{
			if (!PRACE_IS_(target_ptr, RACE_SPECTRE))
			{
				dam *= 6; dam /= (randint1(4) + 7);
			}
		}
		else if (!CHECK_MULTISHADOW(target_ptr)) drain_exp(target_ptr, 200 + (target_ptr->exp / 100), 200 + (target_ptr->exp / 1000), 75);

		if (PRACE_IS_(target_ptr, RACE_SPECTRE) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("気分がよくなった。", "You feel invigorated!"));
			hp_player(target_ptr, dam / 4);
			learn_spell(target_ptr, monspell);
		}
		else
		{
			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		}

		break;
	}
	case GF_WATER:
	{
		if (fuzzy) msg_print(_("何か湿ったもので攻撃された！", "You are hit by something wet!"));
		if (CHECK_MULTISHADOW(target_ptr))
		{
			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
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

		if (target_ptr->resist_water) get_damage /= 4;

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_CHAOS:
	{
		if (fuzzy) msg_print(_("無秩序の波動で攻撃された！", "You are hit by a wave of anarchy!"));
		if (target_ptr->resist_chaos)
		{
			dam *= 6; dam /= (randint1(4) + 7);
		}

		if (CHECK_MULTISHADOW(target_ptr))
		{
			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
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

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_SHARDS:
	{
		if (fuzzy) msg_print(_("何か鋭いもので攻撃された！", "You are hit by something sharp!"));
		if (target_ptr->resist_shard)
		{
			dam *= 6; dam /= (randint1(4) + 7);
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_cut(target_ptr, target_ptr->cut + dam);
		}

		if (!target_ptr->resist_shard || one_in_(13))
		{
			inventory_damage(target_ptr, set_cold_destroy, 2);
		}

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_SOUND:
	{
		if (fuzzy) msg_print(_("轟音で攻撃された！", "You are hit by a loud noise!"));
		if (target_ptr->resist_sound)
		{
			dam *= 5; dam /= (randint1(4) + 7);
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			int plus_stun = (randint1((dam > 90) ? 35 : (dam / 3 + 5)));
			(void)set_stun(target_ptr, target_ptr->stun + plus_stun);
		}

		if (!target_ptr->resist_sound || one_in_(13))
		{
			inventory_damage(target_ptr, set_cold_destroy, 2);
		}

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_CONFUSION:
	{
		if (fuzzy) msg_print(_("何か混乱するもので攻撃された！", "You are hit by something puzzling!"));
		if (target_ptr->resist_conf)
		{
			dam *= 5; dam /= (randint1(4) + 7);
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_confused(target_ptr, target_ptr->confused + randint1(20) + 10);
		}
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_DISENCHANT:
	{
		if (fuzzy) msg_print(_("何かさえないもので攻撃された！", "You are hit by something static!"));
		if (target_ptr->resist_disen)
		{
			dam *= 6; dam /= (randint1(4) + 7);
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			(void)apply_disenchant(target_ptr, 0);
		}
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_NEXUS:
	{
		if (fuzzy) msg_print(_("何か奇妙なもので攻撃された！", "You are hit by something strange!"));
		if (target_ptr->resist_nexus)
		{
			dam *= 6; dam /= (randint1(4) + 7);
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			apply_nexus(m_ptr, target_ptr);
		}

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_FORCE:
	{
		if (fuzzy) msg_print(_("運動エネルギーで攻撃された！", "You are hit by kinetic force!"));
		if (!target_ptr->resist_sound && !CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_stun(target_ptr, target_ptr->stun + randint1(20));
		}

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_ROCKET:
	{
		if (fuzzy) msg_print(_("爆発があった！", "There is an explosion!"));
		if (!target_ptr->resist_sound && !CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_stun(target_ptr, target_ptr->stun + randint1(20));
		}

		if (target_ptr->resist_shard)
		{
			dam /= 2;
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_cut(target_ptr, target_ptr->cut + (dam / 2));
		}

		if (!target_ptr->resist_shard || one_in_(12))
		{
			inventory_damage(target_ptr, set_cold_destroy, 3);
		}

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_INERTIAL:
	{
		if (fuzzy) msg_print(_("何か遅いもので攻撃された！", "You are hit by something slow!"));
		if (!CHECK_MULTISHADOW(target_ptr)) (void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, FALSE);

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_LITE:
	{
		if (fuzzy) msg_print(_("何かで攻撃された！", "You are hit by something!"));
		if (target_ptr->resist_lite)
		{
			dam *= 4; dam /= (randint1(4) + 7);
		}
		else if (!blind && !target_ptr->resist_blind && !CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_blind(target_ptr, target_ptr->blind + randint1(5) + 2);
		}

		if (PRACE_IS_(target_ptr, RACE_VAMPIRE) || (target_ptr->mimic_form == MIMIC_VAMPIRE))
		{
			if (!CHECK_MULTISHADOW(target_ptr)) msg_print(_("光で肉体が焦がされた！", "The light scorches your flesh!"));
			dam *= 2;
		}
		else if (PRACE_IS_(target_ptr, RACE_S_FAIRY))
		{
			dam = dam * 4 / 3;
		}

		if (target_ptr->wraith_form) dam *= 2;
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);

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
		if (fuzzy) msg_print(_("何かで攻撃された！", "You are hit by something!"));
		if (target_ptr->resist_dark)
		{
			dam *= 4; dam /= (randint1(4) + 7);

			if (PRACE_IS_(target_ptr, RACE_VAMPIRE) || (target_ptr->mimic_form == MIMIC_VAMPIRE) || target_ptr->wraith_form) dam = 0;
		}
		else if (!blind && !target_ptr->resist_blind && !CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_blind(target_ptr, target_ptr->blind + randint1(5) + 2);
		}

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_TIME:
	{
		if (fuzzy) msg_print(_("過去からの衝撃に攻撃された！", "You are hit by a blast from the past!"));

		if (target_ptr->resist_time)
		{
			dam *= 4;
			dam /= (randint1(4) + 7);
			msg_print(_("時間が通り過ぎていく気がする。", "You feel as if time is passing you by."));
			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
			break;
		}

		if (CHECK_MULTISHADOW(target_ptr))
		{
			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
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
			for (k = 0; k < A_MAX; k++)
			{
				target_ptr->stat_cur[k] = (target_ptr->stat_cur[k] * 7) / 8;
				if (target_ptr->stat_cur[k] < 3) target_ptr->stat_cur[k] = 3;
			}

			target_ptr->update |= (PU_BONUS);
			break;
		}
		}

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_GRAVITY:
	{
		if (fuzzy) msg_print(_("何か重いもので攻撃された！", "You are hit by something heavy!"));
		msg_print(_("周辺の重力がゆがんだ。", "Gravity warps around you."));

		if (!CHECK_MULTISHADOW(target_ptr))
		{
			teleport_player(target_ptr, 5, TELEPORT_PASSIVE);
			if (!target_ptr->levitation)
				(void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, FALSE);
			if (!(target_ptr->resist_sound || target_ptr->levitation))
			{
				int plus_stun = (randint1((dam > 90) ? 35 : (dam / 3 + 5)));
				(void)set_stun(target_ptr, target_ptr->stun + plus_stun);
			}
		}

		if (target_ptr->levitation)
		{
			dam = (dam * 2) / 3;
		}

		if (!target_ptr->levitation || one_in_(13))
		{
			inventory_damage(target_ptr, set_cold_destroy, 2);
		}

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_DISINTEGRATE:
	{
		if (fuzzy) msg_print(_("純粋なエネルギーで攻撃された！", "You are hit by pure energy!"));

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_OLD_HEAL:
	{
		if (fuzzy) msg_print(_("何らかの攻撃によって気分がよくなった。", "You are hit by something invigorating!"));

		(void)hp_player(target_ptr, dam);
		dam = 0;
		break;
	}
	case GF_OLD_SPEED:
	{
		if (fuzzy) msg_print(_("何かで攻撃された！", "You are hit by something!"));
		(void)set_fast(target_ptr, target_ptr->fast + randint1(5), FALSE);
		dam = 0;
		break;
	}
	case GF_OLD_SLOW:
	{
		if (fuzzy) msg_print(_("何か遅いもので攻撃された！", "You are hit by something slow!"));
		(void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, FALSE);
		break;
	}
	case GF_OLD_SLEEP:
	{
		if (target_ptr->free_act)  break;
		if (fuzzy) msg_print(_("眠ってしまった！", "You fall asleep!"));

		if (ironman_nightmare)
		{
			msg_print(_("恐ろしい光景が頭に浮かんできた。", "A horrible vision enters your mind."));
			/* Have some nightmares */
			sanity_blast(target_ptr, NULL, FALSE);
		}

		set_paralyzed(target_ptr, target_ptr->paralyzed + dam);
		dam = 0;
		break;
	}
	case GF_MANA:
	case GF_SEEKER:
	case GF_SUPER_RAY:
	{
		if (fuzzy) msg_print(_("魔法のオーラで攻撃された！", "You are hit by an aura of magic!"));

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_PSY_SPEAR:
	{
		if (fuzzy) msg_print(_("エネルギーの塊で攻撃された！", "You are hit by an energy!"));

		get_damage = take_hit(target_ptr, DAMAGE_FORCE, dam, killer, monspell);
		break;
	}
	case GF_METEOR:
	{
		if (fuzzy) msg_print(_("何かが空からあなたの頭上に落ちてきた！", "Something falls from the sky on you!"));

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		if (!target_ptr->resist_shard || one_in_(13))
		{
			if (!target_ptr->immune_fire) inventory_damage(target_ptr, set_fire_destroy, 2);
			inventory_damage(target_ptr, set_cold_destroy, 2);
		}

		break;
	}
	case GF_ICE:
	{
		if (fuzzy) msg_print(_("何か鋭く冷たいもので攻撃された！", "You are hit by something sharp and cold!"));

		get_damage = cold_dam(target_ptr, dam, killer, monspell, FALSE);
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
		if (fuzzy) msg_print(_("何か非常に冷たいもので攻撃された！", "You are hit by something extremely cold!"));

		if (target_ptr->mimic_form)
		{
			if (!(mimic_info[target_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_NONLIVING))
				get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);

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
			dam = 0;
			break;
		}
		default:
		{
			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
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
			dam = 0;
			break;
		}

		if (target_ptr->csp == 0)
		{
			dam = 0;
			break;
		}

		if (who > 0)
			msg_format(_("%^sに精神エネルギーを吸い取られてしまった！", "%^s draws psychic energy from you!"), m_name);
		else
			msg_print(_("精神エネルギーを吸い取られてしまった！", "Your psychic energy is drawn!"));

		if (dam >= target_ptr->csp)
		{
			dam = target_ptr->csp;
			target_ptr->csp = 0;
			target_ptr->csp_frac = 0;
		}
		else
		{
			target_ptr->csp -= dam;
		}

		learn_spell(target_ptr, monspell);
		target_ptr->redraw |= (PR_MANA);
		target_ptr->window |= (PW_PLAYER | PW_SPELL);

		if ((who <= 0) || (m_ptr->hp >= m_ptr->maxhp))
		{
			dam = 0;
			break;
		}

		m_ptr->hp += dam;
		if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

		if (target_ptr->health_who == who) target_ptr->redraw |= (PR_HEALTH);
		if (target_ptr->riding == who) target_ptr->redraw |= (PR_UHEALTH);

		if (m_ptr->ml)
		{
			msg_format(_("%^sは気分が良さそうだ。", "%^s appears healthier."), m_name);
		}

		dam = 0;
		break;
	}
	case GF_MIND_BLAST:
	{
		if ((randint0(100 + rlev / 2) < MAX(5, target_ptr->skill_sav)) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			learn_spell(target_ptr, monspell);
			break;
		}

		if (CHECK_MULTISHADOW(target_ptr))
		{
			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
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
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}
	case GF_BRAIN_SMASH:
	{
		if ((randint0(100 + rlev / 2) < MAX(5, target_ptr->skill_sav)) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			learn_spell(target_ptr, monspell);
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

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
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

		while (randint0(100 + rlev / 2) > (MAX(5, target_ptr->skill_sav)))
			(void)do_dec_stat(target_ptr, A_INT);
		while (randint0(100 + rlev / 2) > (MAX(5, target_ptr->skill_sav)))
			(void)do_dec_stat(target_ptr, A_WIS);

		if (!target_ptr->resist_chaos)
		{
			(void)set_image(target_ptr, target_ptr->image + randint0(250) + 150);
		}

		break;
	}
	case GF_CAUSE_1:
	{
		if ((randint0(100 + rlev / 2) < target_ptr->skill_sav) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			learn_spell(target_ptr, monspell);
		}
		else
		{
			if (!CHECK_MULTISHADOW(target_ptr)) curse_equipment(target_ptr, 15, 0);
			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		}
		break;
	}
	case GF_CAUSE_2:
	{
		if ((randint0(100 + rlev / 2) < target_ptr->skill_sav) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			learn_spell(target_ptr, monspell);
		}
		else
		{
			if (!CHECK_MULTISHADOW(target_ptr)) curse_equipment(target_ptr, 25, MIN(rlev / 2 - 15, 5));
			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		}
		break;
	}
	case GF_CAUSE_3:
	{
		if ((randint0(100 + rlev / 2) < target_ptr->skill_sav) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			learn_spell(target_ptr, monspell);
		}
		else
		{
			if (!CHECK_MULTISHADOW(target_ptr)) curse_equipment(target_ptr, 33, MIN(rlev / 2 - 15, 15));
			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		}
		break;
	}
	case GF_CAUSE_4:
	{
		if ((randint0(100 + rlev / 2) < target_ptr->skill_sav) && !(m_ptr->r_idx == MON_KENSHIROU) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし秘孔を跳ね返した！", "You resist the effects!"));
			learn_spell(target_ptr, monspell);
		}
		else
		{
			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
			if (!CHECK_MULTISHADOW(target_ptr)) (void)set_cut(target_ptr, target_ptr->cut + damroll(10, 10));
		}

		break;
	}
	case GF_HAND_DOOM:
	{
		if ((randint0(100 + rlev / 2) < target_ptr->skill_sav) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			learn_spell(target_ptr, monspell);
		}
		else
		{
			if (!CHECK_MULTISHADOW(target_ptr))
			{
				msg_print(_("あなたは命が薄まっていくように感じた！", "You feel your life fade away!"));
				curse_equipment(target_ptr, 40, 20);
			}

			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, m_name, monspell);

			if (target_ptr->chp < 1) target_ptr->chp = 1;
		}

		break;
	}
	default:
	{
		dam = 0;
		break;
	}
	}

	revenge_store(target_ptr, get_damage);
	if ((target_ptr->tim_eyeeye || hex_spelling(target_ptr, HEX_EYE_FOR_EYE))
		&& (get_damage > 0) && !target_ptr->is_dead && (who > 0))
	{
		GAME_TEXT m_name_self[80];
		monster_desc(target_ptr, m_name_self, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);
		msg_format(_("攻撃が%s自身を傷つけた！", "The attack of %s has wounded %s!"), m_name, m_name_self);
		project(target_ptr, 0, 0, m_ptr->fy, m_ptr->fx, get_damage, GF_MISSILE, PROJECT_KILL, -1);
		if (target_ptr->tim_eyeeye) set_tim_eyeeye(target_ptr, target_ptr->tim_eyeeye - 5, TRUE);
	}

	if (target_ptr->riding && dam > 0)
	{
		rakubadam_p = (dam > 200) ? 200 : dam;
	}

	disturb(target_ptr, TRUE, TRUE);
	if ((target_ptr->special_defense & NINJA_KAWARIMI) && dam && who && (who != target_ptr->riding))
	{
		(void)kawarimi(target_ptr, FALSE);
	}

	return (obvious);
}
