#include "angband.h"
#include "effect-player-util.h"
#include "effect/effect-player-switcher.h"
#include "spell/spells-type.h"
#include "player-damage.h"
#include "world.h"
#include "object-broken.h"
#include "player-effects.h"
#include "player/mimic-info-table.h"
#include "monster-spell.h"
#include "object-curse.h"
#include "effect/effect-player-resist-hurt.h"

void effect_player_old_heal(player_type *target_ptr,
                            effect_player_type *ep_ptr) {
  if (target_ptr->blind)
    msg_print(_("何らかの攻撃によって気分がよくなった。",
                "You are hit by something invigorating!"));

  (void)hp_player(target_ptr, ep_ptr->dam);
  ep_ptr->dam = 0;
}

void effect_player_old_speed(player_type *target_ptr,
                             effect_player_type *ep_ptr) {
  if (target_ptr->blind)
    msg_print(_("何かで攻撃された！", "You are hit by something!"));

  (void)set_fast(target_ptr, target_ptr->fast + randint1(5), FALSE);
  ep_ptr->dam = 0;
}

void effect_player_old_slow(player_type *target_ptr) {
  if (target_ptr->blind)
    msg_print(
        _("何か遅いもので攻撃された！", "You are hit by something slow!"));

  (void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, FALSE);
}

void effect_player_old_sleep(player_type *target_ptr,
                             effect_player_type *ep_ptr) {
  if (target_ptr->free_act)
    return;

  if (target_ptr->blind)
    msg_print(_("眠ってしまった！", "You fall asleep!"));

  if (ironman_nightmare) {
    msg_print(_("恐ろしい光景が頭に浮かんできた。",
                "A horrible vision enters your mind."));

    /* Have some nightmares */
    sanity_blast(target_ptr, NULL, FALSE);
  }

  set_paralyzed(target_ptr, target_ptr->paralyzed + ep_ptr->dam);
  ep_ptr->dam = 0;
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
		effect_player_elements(target_ptr, ep_ptr, _("酸で攻撃された！", "You are hit by acid!"), acid_dam);
		return;
	case GF_FIRE:
		effect_player_elements(target_ptr, ep_ptr, _("火炎で攻撃された！", "You are hit by fire!"), fire_dam);
		return;
	case GF_COLD:
		effect_player_elements(target_ptr, ep_ptr, _("冷気で攻撃された！", "You are hit by cold!"), cold_dam);
		return;
	case GF_ELEC:
		effect_player_elements(target_ptr, ep_ptr, _("電撃で攻撃された！", "You are hit by lightning!"), elec_dam);
		return;
	case GF_POIS:
		effect_player_poison(target_ptr, ep_ptr);
		return;
	case GF_NUKE:
		effect_player_nuke(target_ptr, ep_ptr);
		return;
	case GF_MISSILE:
		effect_player_missile(target_ptr, ep_ptr);
		return;
	case GF_HOLY_FIRE:
		effect_player_holy_fire(target_ptr, ep_ptr);
		return;
	case GF_HELL_FIRE:
		effect_player_hell_fire(target_ptr, ep_ptr);
		return;
	case GF_ARROW:
		effect_player_arrow(target_ptr, ep_ptr);
		return;
	case GF_PLASMA:
		effect_player_plasma(target_ptr, ep_ptr);
		return;
	case GF_NETHER:
		effect_player_nether(target_ptr, ep_ptr);
		return;
	case GF_WATER:
		effect_player_water(target_ptr, ep_ptr);
		return;
	case GF_CHAOS:
		effect_player_chaos(target_ptr, ep_ptr);
		return;
	case GF_SHARDS:
		effect_player_shards(target_ptr, ep_ptr);
		return;
	case GF_SOUND:
		effect_player_sound(target_ptr, ep_ptr);
		return;
	case GF_CONFUSION:
		effect_player_confusion(target_ptr, ep_ptr);
		return;
	case GF_DISENCHANT:
		effect_player_disenchant(target_ptr, ep_ptr);
		return;
	case GF_NEXUS:
		effect_player_nexus(target_ptr, ep_ptr);
		return;
	case GF_FORCE:
		effect_player_force(target_ptr, ep_ptr);
		return;
	case GF_ROCKET:
		effect_player_rocket(target_ptr, ep_ptr);
		return;
	case GF_INERTIAL:
		effect_player_inertial(target_ptr, ep_ptr);
		return;
	case GF_LITE:
		effect_player_lite(target_ptr, ep_ptr);
		return;
	case GF_DARK:
		effect_player_dark(target_ptr, ep_ptr);
		return;
	case GF_TIME:
		effect_player_time(target_ptr, ep_ptr);
		return;
	case GF_GRAVITY:
		effect_player_gravity(target_ptr, ep_ptr);
		return;
	case GF_DISINTEGRATE:
		effect_player_disintegration(target_ptr, ep_ptr);
		return;
	case GF_OLD_HEAL:
        effect_player_old_heal(target_ptr, ep_ptr);
        return;
	case GF_OLD_SPEED:
		effect_player_old_speed(target_ptr, ep_ptr);
        return;
	case GF_OLD_SLOW:
		effect_player_old_slow(target_ptr);
		return;
	case GF_OLD_SLEEP:
		effect_plyaer_old_sleep(target_ptr, ep_ptr);
		return;
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
