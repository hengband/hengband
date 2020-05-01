/*!
 * @brief 魔法種別による各種処理切り替え
 * @date 2020/04/29
 * @author Hourier
 */

#include "angband.h"
#include "effect-monster-util.h"
#include "effect/effect-monster-switcher.h"
#include "player-damage.h"
#include "world.h"
#include "avatar.h"
#include "monster-spell.h"
#include "quest.h"
#include "monster-status.h"
#include "effect/spells-effect-util.h"
#include "player-effects.h"
#include "spells-diceroll.h"
#include "monsterrace-hook.h"
#include "combat/melee.h"
#include "cmd/cmd-pet.h" // 暫定、後で消すかも.
#include "spell/spells-type.h"
#include "effect/effect-monster-resist-hurt.h"

static bool effect_monster_psi_empty_mind(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flags2 & RF2_EMPTY_MIND) == 0) return FALSE;

	em_ptr->dam = 0;
	em_ptr->note = _("には完全な耐性がある！", " is immune.");
	if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr))
		em_ptr->r_ptr->r_flags2 |= (RF2_EMPTY_MIND);

	return TRUE;
}


static bool effect_monster_psi_weird_mind(effect_monster_type *em_ptr)
{
	bool has_resistance = ((em_ptr->r_ptr->flags2 & (RF2_STUPID | RF2_WEIRD_MIND)) != 0) ||
		((em_ptr->r_ptr->flags3 & RF3_ANIMAL) != 0) ||
		(em_ptr->r_ptr->level > randint1(3 * em_ptr->dam));
	if (!has_resistance) return FALSE;

	em_ptr->note = _("には耐性がある！", " resists!");
	em_ptr->dam /= 3;
	return TRUE;
}


static bool effect_monster_psi_demon(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	bool is_powerful = ((em_ptr->r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) != 0) &&
		(em_ptr->r_ptr->level > caster_ptr->lev / 2) &&
		one_in_(2);
	if (!is_powerful) return FALSE;

	em_ptr->note = NULL;
	msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！",
		(em_ptr->seen ? "%^s's corrupted mind backlashes your attack!" :
			"%^ss corrupted mind backlashes your attack!")), em_ptr->m_name);
	return TRUE;
}


static void effect_monster_psi_resist_addition(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	switch (randint1(4))
	{
	case 1:
		set_confused(caster_ptr, caster_ptr->confused + 3 + randint1(em_ptr->dam));
		break;
	case 2:
		set_stun(caster_ptr, caster_ptr->stun + randint1(em_ptr->dam));
		break;
	case 3:
	{
		if (em_ptr->r_ptr->flags3 & RF3_NO_FEAR)
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
		else
			set_afraid(caster_ptr, caster_ptr->afraid + 3 + randint1(em_ptr->dam));

		break;
	}
	default:
		if (!caster_ptr->free_act)
			(void)set_paralyzed(caster_ptr, caster_ptr->paralyzed + randint1(em_ptr->dam));

		break;
	}
}


// Powerful demons & undead can turn a mindcrafter's attacks back on them.
static void effect_monster_psi_resist(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if (effect_monster_psi_empty_mind(caster_ptr, em_ptr)) return;
	if (effect_monster_psi_weird_mind(em_ptr)) return;
	if (!effect_monster_psi_demon(caster_ptr, em_ptr)) return;

	if ((randint0(100 + em_ptr->r_ptr->level / 2) < caster_ptr->skill_sav) && !CHECK_MULTISHADOW(caster_ptr))
	{
		msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
		em_ptr->dam = 0;
		return;
	}

	/* Injure +/- confusion */
	monster_desc(caster_ptr, em_ptr->killer, em_ptr->m_ptr, MD_WRONGDOER_NAME);
	take_hit(caster_ptr, DAMAGE_ATTACK, em_ptr->dam, em_ptr->killer, -1);
	if (!one_in_(4) || CHECK_MULTISHADOW(caster_ptr))
	{
		em_ptr->dam = 0;
		return;
	}

	effect_monster_psi_resist_addition(caster_ptr, em_ptr);
	em_ptr->dam = 0;
}


static void effect_monster_psi_addition(effect_monster_type *em_ptr)
{
	if ((em_ptr->dam <= 0) || !one_in_(4)) return;

	switch (randint1(4))
	{
	case 1:
		em_ptr->do_conf = 3 + randint1(em_ptr->dam);
		break;
	case 2:
		em_ptr->do_stun = 3 + randint1(em_ptr->dam);
		break;
	case 3:
		em_ptr->do_fear = 3 + randint1(em_ptr->dam);
		break;
	default:
		em_ptr->note = _("は眠り込んでしまった！", " falls asleep!");
		em_ptr->do_sleep = 3 + randint1(em_ptr->dam);
		break;
	}
}


gf_switch_result effect_monster_psi(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if (em_ptr->seen) em_ptr->obvious = TRUE;
	if (!(los(caster_ptr, em_ptr->m_ptr->fy, em_ptr->m_ptr->fx, caster_ptr->y, caster_ptr->x)))
	{
		if (em_ptr->seen_msg)
			msg_format(_("%sはあなたが見えないので影響されない！", "%^s can't see you, and isn't affected!"), em_ptr->m_name);

		em_ptr->skipped = TRUE;
		return GF_SWITCH_CONTINUE;
	}

	effect_monster_psi_resist(caster_ptr, em_ptr);
	effect_monster_psi_addition(em_ptr);
	em_ptr->note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");
	return GF_SWITCH_CONTINUE;
}


// Powerful demons & undead can turn a mindcrafter's attacks back on them.
static void effect_monster_psi_drain_resist(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	em_ptr->note = _("には耐性がある！", " resists!");
	em_ptr->dam /= 3;
	bool is_corrupted = ((em_ptr->r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) != 0) &&
		(em_ptr->r_ptr->level > caster_ptr->lev / 2) &&
		(one_in_(2));
	if (!is_corrupted) return;

	em_ptr->note = NULL;
	msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！",
		(em_ptr->seen ? "%^s's corrupted mind backlashes your attack!" :
			"%^ss corrupted mind backlashes your attack!")), em_ptr->m_name);
	if ((randint0(100 + em_ptr->r_ptr->level / 2) < caster_ptr->skill_sav) && !CHECK_MULTISHADOW(caster_ptr))
	{
		msg_print(_("あなたは効力を跳ね返した！", "You resist the effects!"));
		em_ptr->dam = 0;
		return;
	}

	monster_desc(caster_ptr, em_ptr->killer, em_ptr->m_ptr, MD_WRONGDOER_NAME);
	if (CHECK_MULTISHADOW(caster_ptr))
	{
		take_hit(caster_ptr, DAMAGE_ATTACK, em_ptr->dam, em_ptr->killer, -1);  /* has already been /3 */
		em_ptr->dam = 0;
		return;
	}

	msg_print(_("超能力パワーを吸いとられた！", "Your psychic energy is drained!"));
	caster_ptr->csp -= damroll(5, em_ptr->dam) / 2;
	if (caster_ptr->csp < 0) caster_ptr->csp = 0;

	caster_ptr->redraw |= PR_MANA;
	caster_ptr->window |= (PW_SPELL);
	take_hit(caster_ptr, DAMAGE_ATTACK, em_ptr->dam, em_ptr->killer, -1);  /* has already been /3 */
	em_ptr->dam = 0;
}


static void effect_monster_psi_drain_change_power(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	int b = damroll(5, em_ptr->dam) / 4;
	concptr str = (caster_ptr->pclass == CLASS_MINDCRAFTER) ? _("超能力パワー", "psychic energy") : _("魔力", "mana");
	concptr msg = _("あなたは%sの苦痛を%sに変換した！",
		(em_ptr->seen ? "You convert %s's pain into %s!" : "You convert %ss pain into %s!"));
	msg_format(msg, em_ptr->m_name, str);

	b = MIN(caster_ptr->msp, caster_ptr->csp + b);
	caster_ptr->csp = b;
	caster_ptr->redraw |= PR_MANA;
	caster_ptr->window |= (PW_SPELL);
}


gf_switch_result effect_monster_psi_drain(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if (em_ptr->seen) em_ptr->obvious = TRUE;

	if (em_ptr->r_ptr->flags2 & RF2_EMPTY_MIND)
	{
		em_ptr->dam = 0;
		em_ptr->note = _("には完全な耐性がある！", " is immune.");
	}
	else if ((em_ptr->r_ptr->flags2 & (RF2_STUPID | RF2_WEIRD_MIND)) ||
		(em_ptr->r_ptr->flags3 & RF3_ANIMAL) ||
		(em_ptr->r_ptr->level > randint1(3 * em_ptr->dam)))
	{
		effect_monster_psi_drain_resist(caster_ptr, em_ptr);
	}
	else if (em_ptr->dam > 0)
	{
		effect_monster_psi_drain_change_power(caster_ptr, em_ptr);
	}

	em_ptr->note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");
}


/*!
 * @brief 魔法の効果によって様々なメッセーを出力したり与えるダメージの増減を行ったりする
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return ここのスイッチングで終るならTRUEかFALSE、後続処理を実行するならCONTINUE
 */
gf_switch_result switch_effects_monster(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	floor_type *floor_ptr = caster_ptr->current_floor_ptr;
	switch (em_ptr->effect_type)
	{
	case GF_PSY_SPEAR:
	case GF_MISSILE:
	case GF_ARROW:
	case GF_MANA:
	case GF_METEOR:
	case GF_BLOOD_CURSE:
	case GF_SEEKER:
	case GF_SUPER_RAY:
		return effect_monster_void(em_ptr);
	case GF_ACID:
		return effect_monster_acid(caster_ptr, em_ptr);
	case GF_ELEC:
		return effect_monster_elec(caster_ptr, em_ptr);
	case GF_FIRE:
		return effect_monster_fire(caster_ptr, em_ptr);
	case GF_COLD:
		return effect_monster_cold(caster_ptr, em_ptr);
	case GF_POIS:
		return effect_monster_pois(caster_ptr, em_ptr);
	case GF_NUKE:
		return effect_monster_nuke(caster_ptr, em_ptr);
	case GF_HELL_FIRE:
		return effect_monster_hell_fire(caster_ptr, em_ptr);
	case GF_HOLY_FIRE:
		return effect_monster_holy_fire(caster_ptr, em_ptr);
	case GF_PLASMA:
		return effect_monster_plasma(caster_ptr, em_ptr);
	case GF_NETHER:
		return effect_monster_nether(caster_ptr, em_ptr);
	case GF_WATER:
		return effect_monster_water(caster_ptr, em_ptr);
	case GF_CHAOS:
		return effect_monster_chaos(caster_ptr, em_ptr);
	case GF_SHARDS:
		return effect_monster_shards(caster_ptr, em_ptr);
	case GF_ROCKET:
		return effect_monster_rocket(caster_ptr, em_ptr);
	case GF_SOUND:
		return effect_monster_sound(caster_ptr, em_ptr);
	case GF_CONFUSION:
		return effect_monster_confusion(caster_ptr, em_ptr);
	case GF_DISENCHANT:
		return effect_monster_disenchant(caster_ptr, em_ptr);
	case GF_NEXUS:
		return effect_monster_nexus(caster_ptr, em_ptr);
	case GF_FORCE:
		return effect_monster_force(caster_ptr, em_ptr);
	case GF_INERTIAL:
		return effect_monster_inertial(caster_ptr, em_ptr);
	case GF_TIME:
		return effect_monster_time(caster_ptr, em_ptr);
	case GF_GRAVITY:
		return effect_monster_gravity(caster_ptr, em_ptr);
	case GF_DISINTEGRATE:
		return effect_monster_disintegration(caster_ptr, em_ptr);
	case GF_PSI:
		return effect_monster_psi(caster_ptr, em_ptr);
	case GF_PSI_DRAIN:
		return effect_monster_psi_drain(caster_ptr, em_ptr);
	case GF_TELEKINESIS:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (one_in_(4))
		{
			if (caster_ptr->riding && (em_ptr->g_ptr->m_idx == caster_ptr->riding)) em_ptr->do_dist = 0;
			else em_ptr->do_dist = 7;
		}

		em_ptr->do_stun = damroll((em_ptr->caster_lev / 20) + 3, em_ptr->dam) + 1;
		if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
			(em_ptr->r_ptr->level > 5 + randint1(em_ptr->dam)))
		{
			em_ptr->do_stun = 0;
			em_ptr->obvious = FALSE;
		}

		break;
	}
	case GF_DOMINATION:
	{
		if (!is_hostile(em_ptr->m_ptr)) break;
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) ||
			(em_ptr->r_ptr->flags3 & RF3_NO_CONF) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
		{
			if (em_ptr->r_ptr->flags3 & RF3_NO_CONF)
			{
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);
			}

			em_ptr->do_conf = 0;

			/*
			 * Powerful demons & undead can turn a mindcrafter's
			 * attacks back on them
			 */
			if ((em_ptr->r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) &&
				(em_ptr->r_ptr->level > caster_ptr->lev / 2) &&
				(one_in_(2)))
			{
				em_ptr->note = NULL;
				msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！",
					(em_ptr->seen ? "%^s's corrupted mind backlashes your attack!" :
						"%^ss corrupted mind backlashes your attack!")), em_ptr->m_name);

				/* Saving throw */
				if (randint0(100 + em_ptr->r_ptr->level / 2) < caster_ptr->skill_sav)
				{
					msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
				}
				else
				{
					/* Confuse, stun, terrify */
					switch (randint1(4))
					{
					case 1:
						set_stun(caster_ptr, caster_ptr->stun + em_ptr->dam / 2);
						break;
					case 2:
						set_confused(caster_ptr, caster_ptr->confused + em_ptr->dam / 2);
						break;
					default:
					{
						if (em_ptr->r_ptr->flags3 & RF3_NO_FEAR)
							em_ptr->note = _("には効果がなかった。", " is unaffected.");
						else
							set_afraid(caster_ptr, caster_ptr->afraid + em_ptr->dam);
					}
					}
				}
			}
			else
			{
				em_ptr->note = _("には効果がなかった。", " is unaffected.");
				em_ptr->obvious = FALSE;
			}
		}
		else
		{
			if (!common_saving_throw_charm(caster_ptr, em_ptr->dam, em_ptr->m_ptr))
			{
				em_ptr->note = _("があなたに隷属した。", " is in your thrall!");
				set_pet(caster_ptr, em_ptr->m_ptr);
			}
			else
			{
				switch (randint1(4))
				{
				case 1:
					em_ptr->do_stun = em_ptr->dam / 2;
					break;
				case 2:
					em_ptr->do_conf = em_ptr->dam / 2;
					break;
				default:
					em_ptr->do_fear = em_ptr->dam;
				}
			}
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_ICE:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		em_ptr->do_stun = (randint1(15) + 1) / (em_ptr->r + 1);
		if (em_ptr->r_ptr->flagsr & RFR_IM_COLD)
		{
			em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
			em_ptr->dam /= 9;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_IM_COLD);
		}
		else if (em_ptr->r_ptr->flags3 & (RF3_HURT_COLD))
		{
			em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
			em_ptr->dam *= 2;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_HURT_COLD);
		}

		break;
	}
	case GF_HYPODYNAMIA:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (!monster_living(em_ptr->m_ptr->r_idx))
		{
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr))
			{
				if (em_ptr->r_ptr->flags3 & RF3_DEMON) em_ptr->r_ptr->r_flags3 |= (RF3_DEMON);
				if (em_ptr->r_ptr->flags3 & RF3_UNDEAD) em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);
				if (em_ptr->r_ptr->flags3 & RF3_NONLIVING) em_ptr->r_ptr->r_flags3 |= (RF3_NONLIVING);
			}
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
			em_ptr->dam = 0;
		}
		else
			em_ptr->do_time = (em_ptr->dam + 7) / 8;

		break;
	}
	case GF_DEATH_RAY:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (!monster_living(em_ptr->m_ptr->r_idx))
		{
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr))
			{
				if (em_ptr->r_ptr->flags3 & RF3_DEMON) em_ptr->r_ptr->r_flags3 |= (RF3_DEMON);
				if (em_ptr->r_ptr->flags3 & RF3_UNDEAD) em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);
				if (em_ptr->r_ptr->flags3 & RF3_NONLIVING) em_ptr->r_ptr->r_flags3 |= (RF3_NONLIVING);
			}
			em_ptr->note = _("には完全な耐性がある！", " is immune.");
			em_ptr->obvious = FALSE;
			em_ptr->dam = 0;
		}
		else if (((em_ptr->r_ptr->flags1 & RF1_UNIQUE) &&
			(randint1(888) != 666)) ||
			(((em_ptr->r_ptr->level + randint1(20)) > randint1((em_ptr->caster_lev / 2) + randint1(10))) &&
				randint1(100) != 66))
		{
			em_ptr->note = _("には耐性がある！", " resists!");
			em_ptr->obvious = FALSE;
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_OLD_POLY:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		em_ptr->do_polymorph = TRUE;

		/* Powerful monsters can resist */
		if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
			(em_ptr->r_ptr->flags1 & RF1_QUESTOR) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->do_polymorph = FALSE;
			em_ptr->obvious = FALSE;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_OLD_CLONE:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if ((floor_ptr->inside_arena) || is_pet(em_ptr->m_ptr) || (em_ptr->r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) || (em_ptr->r_ptr->flags7 & (RF7_NAZGUL | RF7_UNIQUE2)))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
		}
		else
		{
			em_ptr->m_ptr->hp = em_ptr->m_ptr->maxhp;
			if (multiply_monster(caster_ptr, em_ptr->g_ptr->m_idx, TRUE, 0L))
			{
				em_ptr->note = _("が分裂した！", " spawns!");
			}
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_STAR_HEAL:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		(void)set_monster_csleep(caster_ptr, em_ptr->g_ptr->m_idx, 0);

		if (em_ptr->m_ptr->maxhp < em_ptr->m_ptr->max_maxhp)
		{
			if (em_ptr->seen_msg) msg_format(_("%^sの強さが戻った。", "%^s recovers %s vitality."), em_ptr->m_name, em_ptr->m_poss);
			em_ptr->m_ptr->maxhp = em_ptr->m_ptr->max_maxhp;
		}

		if (!em_ptr->dam)
		{
			if (caster_ptr->health_who == em_ptr->g_ptr->m_idx) caster_ptr->redraw |= (PR_HEALTH);
			if (caster_ptr->riding == em_ptr->g_ptr->m_idx) caster_ptr->redraw |= (PR_UHEALTH);
			break;
		}
	}
	/* Fall through */
	case GF_OLD_HEAL:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		/* Wake up */
		(void)set_monster_csleep(caster_ptr, em_ptr->g_ptr->m_idx, 0);
		if (MON_STUNNED(em_ptr->m_ptr))
		{
			if (em_ptr->seen_msg) msg_format(_("%^sは朦朧状態から立ち直った。", "%^s is no longer stunned."), em_ptr->m_name);
			(void)set_monster_stunned(caster_ptr, em_ptr->g_ptr->m_idx, 0);
		}
		if (MON_CONFUSED(em_ptr->m_ptr))
		{
			if (em_ptr->seen_msg) msg_format(_("%^sは混乱から立ち直った。", "%^s is no longer confused."), em_ptr->m_name);
			(void)set_monster_confused(caster_ptr, em_ptr->g_ptr->m_idx, 0);
		}
		if (MON_MONFEAR(em_ptr->m_ptr))
		{
			if (em_ptr->seen_msg) msg_format(_("%^sは勇気を取り戻した。", "%^s recovers %s courage."), em_ptr->m_name, em_ptr->m_poss);
			(void)set_monster_monfear(caster_ptr, em_ptr->g_ptr->m_idx, 0);
		}

		if (em_ptr->m_ptr->hp < 30000) em_ptr->m_ptr->hp += em_ptr->dam;
		if (em_ptr->m_ptr->hp > em_ptr->m_ptr->maxhp) em_ptr->m_ptr->hp = em_ptr->m_ptr->maxhp;

		if (!em_ptr->who)
		{
			chg_virtue(caster_ptr, V_VITALITY, 1);

			if (em_ptr->r_ptr->flags1 & RF1_UNIQUE)
				chg_virtue(caster_ptr, V_INDIVIDUALISM, 1);

			if (is_friendly(em_ptr->m_ptr))
				chg_virtue(caster_ptr, V_HONOUR, 1);
			else if (!(em_ptr->r_ptr->flags3 & RF3_EVIL))
			{
				if (em_ptr->r_ptr->flags3 & RF3_GOOD)
					chg_virtue(caster_ptr, V_COMPASSION, 2);
				else
					chg_virtue(caster_ptr, V_COMPASSION, 1);
			}

			if (em_ptr->r_ptr->flags3 & RF3_ANIMAL)
				chg_virtue(caster_ptr, V_NATURE, 1);
		}

		if (em_ptr->m_ptr->r_idx == MON_LEPER)
		{
			em_ptr->heal_leper = TRUE;
			if (!em_ptr->who) chg_virtue(caster_ptr, V_COMPASSION, 5);
		}

		if (caster_ptr->health_who == em_ptr->g_ptr->m_idx) caster_ptr->redraw |= (PR_HEALTH);
		if (caster_ptr->riding == em_ptr->g_ptr->m_idx) caster_ptr->redraw |= (PR_UHEALTH);

		em_ptr->note = _("は体力を回復したようだ。", " looks healthier.");

		em_ptr->dam = 0;
		break;
	}
	case GF_OLD_SPEED:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if (set_monster_fast(caster_ptr, em_ptr->g_ptr->m_idx, MON_FAST(em_ptr->m_ptr) + 100))
		{
			em_ptr->note = _("の動きが速くなった。", " starts moving faster.");
		}

		if (!em_ptr->who)
		{
			if (em_ptr->r_ptr->flags1 & RF1_UNIQUE)
				chg_virtue(caster_ptr, V_INDIVIDUALISM, 1);
			if (is_friendly(em_ptr->m_ptr))
				chg_virtue(caster_ptr, V_HONOUR, 1);
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_OLD_SLOW:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		/* Powerful monsters can resist */
		if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
		}
		else
		{
			if (set_monster_slow(caster_ptr, em_ptr->g_ptr->m_idx, MON_SLOW(em_ptr->m_ptr) + 50))
			{
				em_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
			}
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_OLD_SLEEP:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
			(em_ptr->r_ptr->flags3 & RF3_NO_SLEEP) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
		{
			if (em_ptr->r_ptr->flags3 & RF3_NO_SLEEP)
			{
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_NO_SLEEP);
			}

			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
		}
		else
		{
			em_ptr->note = _("は眠り込んでしまった！", " falls asleep!");
			em_ptr->do_sleep = 500;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_STASIS_EVIL:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
			!(em_ptr->r_ptr->flags3 & RF3_EVIL) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
		}
		else
		{
			em_ptr->note = _("は動けなくなった！", " is suspended!");
			em_ptr->do_sleep = 500;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_STASIS:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
		}
		else
		{
			em_ptr->note = _("は動けなくなった！", " is suspended!");
			em_ptr->do_sleep = 500;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_CHARM:
	{
		int vir = virtue_number(caster_ptr, V_HARMONY);
		if (vir)
		{
			em_ptr->dam += caster_ptr->virtues[vir - 1] / 10;
		}

		vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
		if (vir)
		{
			em_ptr->dam -= caster_ptr->virtues[vir - 1] / 20;
		}

		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if (common_saving_throw_charm(caster_ptr, em_ptr->dam, em_ptr->m_ptr))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;

			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else if (caster_ptr->cursed & TRC_AGGRAVATE)
		{
			em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else
		{
			em_ptr->note = _("は突然友好的になったようだ！", " suddenly seems friendly!");
			set_pet(caster_ptr, em_ptr->m_ptr);

			chg_virtue(caster_ptr, V_INDIVIDUALISM, -1);
			if (em_ptr->r_ptr->flags3 & RF3_ANIMAL)
				chg_virtue(caster_ptr, V_NATURE, 1);
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_CONTROL_UNDEAD:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		int vir = virtue_number(caster_ptr, V_UNLIFE);
		if (vir)
		{
			em_ptr->dam += caster_ptr->virtues[vir - 1] / 10;
		}

		vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
		if (vir)
		{
			em_ptr->dam -= caster_ptr->virtues[vir - 1] / 20;
		}

		if (common_saving_throw_control(caster_ptr, em_ptr->dam, em_ptr->m_ptr) ||
			!(em_ptr->r_ptr->flags3 & RF3_UNDEAD))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else if (caster_ptr->cursed & TRC_AGGRAVATE)
		{
			em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else
		{
			em_ptr->note = _("は既にあなたの奴隷だ！", " is in your thrall!");
			set_pet(caster_ptr, em_ptr->m_ptr);
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_CONTROL_DEMON:
	{
		int vir;
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		vir = virtue_number(caster_ptr, V_UNLIFE);
		if (vir)
		{
			em_ptr->dam += caster_ptr->virtues[vir - 1] / 10;
		}

		vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
		if (vir)
		{
			em_ptr->dam -= caster_ptr->virtues[vir - 1] / 20;
		}

		if (common_saving_throw_control(caster_ptr, em_ptr->dam, em_ptr->m_ptr) ||
			!(em_ptr->r_ptr->flags3 & RF3_DEMON))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else if (caster_ptr->cursed & TRC_AGGRAVATE)
		{
			em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else
		{
			em_ptr->note = _("は既にあなたの奴隷だ！", " is in your thrall!");
			set_pet(caster_ptr, em_ptr->m_ptr);
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_CONTROL_ANIMAL:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		int vir = virtue_number(caster_ptr, V_NATURE);
		if (vir)
		{
			em_ptr->dam += caster_ptr->virtues[vir - 1] / 10;
		}

		vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
		if (vir)
		{
			em_ptr->dam -= caster_ptr->virtues[vir - 1] / 20;
		}

		if (common_saving_throw_control(caster_ptr, em_ptr->dam, em_ptr->m_ptr) ||
			!(em_ptr->r_ptr->flags3 & RF3_ANIMAL))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else if (caster_ptr->cursed & TRC_AGGRAVATE)
		{
			em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else
		{
			em_ptr->note = _("はなついた。", " is tamed!");
			set_pet(caster_ptr, em_ptr->m_ptr);
			if (em_ptr->r_ptr->flags3 & RF3_ANIMAL)
				chg_virtue(caster_ptr, V_NATURE, 1);
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_CHARM_LIVING:
	{
		int vir = virtue_number(caster_ptr, V_UNLIFE);
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		vir = virtue_number(caster_ptr, V_UNLIFE);
		if (vir)
		{
			em_ptr->dam -= caster_ptr->virtues[vir - 1] / 10;
		}

		vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
		if (vir)
		{
			em_ptr->dam -= caster_ptr->virtues[vir - 1] / 20;
		}

		msg_format(_("%sを見つめた。", "You stare into %s."), em_ptr->m_name);

		if (common_saving_throw_charm(caster_ptr, em_ptr->dam, em_ptr->m_ptr) ||
			!monster_living(em_ptr->m_ptr->r_idx))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else if (caster_ptr->cursed & TRC_AGGRAVATE)
		{
			em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else
		{
			em_ptr->note = _("を支配した。", " is tamed!");
			set_pet(caster_ptr, em_ptr->m_ptr);
			if (em_ptr->r_ptr->flags3 & RF3_ANIMAL)
				chg_virtue(caster_ptr, V_NATURE, 1);
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_OLD_CONF:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		em_ptr->do_conf = damroll(3, (em_ptr->dam / 2)) + 1;
		if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
			(em_ptr->r_ptr->flags3 & (RF3_NO_CONF)) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
		{
			if (em_ptr->r_ptr->flags3 & (RF3_NO_CONF))
			{
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);
			}

			em_ptr->do_conf = 0;
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_STUN:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		em_ptr->do_stun = damroll((em_ptr->caster_lev / 20) + 3, (em_ptr->dam)) + 1;
		if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
		{
			em_ptr->do_stun = 0;
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_LITE_WEAK:
	{
		if (!em_ptr->dam)
		{
			em_ptr->skipped = TRUE;
			break;
		}

		if (em_ptr->r_ptr->flags3 & (RF3_HURT_LITE))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_HURT_LITE);

			em_ptr->note = _("は光に身をすくめた！", " cringes from the light!");
			em_ptr->note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
		}
		else
		{
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_LITE:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if (em_ptr->r_ptr->flagsr & RFR_RES_LITE)
		{
			em_ptr->note = _("には耐性がある！", " resists!");
			em_ptr->dam *= 2; em_ptr->dam /= (randint1(6) + 6);
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_RES_LITE);
		}
		else if (em_ptr->r_ptr->flags3 & (RF3_HURT_LITE))
		{
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_HURT_LITE);
			em_ptr->note = _("は光に身をすくめた！", " cringes from the light!");
			em_ptr->note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
			em_ptr->dam *= 2;
		}
		break;
	}
	case GF_DARK:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if (em_ptr->r_ptr->flagsr & RFR_RES_DARK)
		{
			em_ptr->note = _("には耐性がある！", " resists!");
			em_ptr->dam *= 2; em_ptr->dam /= (randint1(6) + 6);
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_RES_DARK);
		}

		break;
	}
	case GF_KILL_WALL:
	{
		if (em_ptr->r_ptr->flags3 & (RF3_HURT_ROCK))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_HURT_ROCK);

			em_ptr->note = _("の皮膚がただれた！", " loses some skin!");
			em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
		}
		else
		{
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_AWAY_UNDEAD:
	{
		if (em_ptr->r_ptr->flags3 & (RF3_UNDEAD))
		{
			bool resists_tele = FALSE;

			if (em_ptr->r_ptr->flagsr & RFR_RES_TELE)
			{
				if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (em_ptr->r_ptr->flagsr & RFR_RES_ALL))
				{
					if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
					em_ptr->note = _("には効果がなかった。", " is unaffected.");
					resists_tele = TRUE;
				}
				else if (em_ptr->r_ptr->level > randint1(100))
				{
					if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
					em_ptr->note = _("には耐性がある！", " resists!");
					resists_tele = TRUE;
				}
			}

			if (!resists_tele)
			{
				if (em_ptr->seen) em_ptr->obvious = TRUE;
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);
				em_ptr->do_dist = em_ptr->dam;
			}
		}
		else
		{
			em_ptr->skipped = TRUE;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_AWAY_EVIL:
	{
		if (em_ptr->r_ptr->flags3 & (RF3_EVIL))
		{
			bool resists_tele = FALSE;

			if (em_ptr->r_ptr->flagsr & RFR_RES_TELE)
			{
				if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (em_ptr->r_ptr->flagsr & RFR_RES_ALL))
				{
					if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
					em_ptr->note = _("には効果がなかった。", " is unaffected.");
					resists_tele = TRUE;
				}
				else if (em_ptr->r_ptr->level > randint1(100))
				{
					if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
					em_ptr->note = _("には耐性がある！", " resists!");
					resists_tele = TRUE;
				}
			}

			if (!resists_tele)
			{
				if (em_ptr->seen) em_ptr->obvious = TRUE;
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_EVIL);
				em_ptr->do_dist = em_ptr->dam;
			}
		}
		else
		{
			em_ptr->skipped = TRUE;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_AWAY_ALL:
	{
		bool resists_tele = FALSE;
		if (em_ptr->r_ptr->flagsr & RFR_RES_TELE)
		{
			if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (em_ptr->r_ptr->flagsr & RFR_RES_ALL))
			{
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
				em_ptr->note = _("には効果がなかった。", " is unaffected.");
				resists_tele = TRUE;
			}
			else if (em_ptr->r_ptr->level > randint1(100))
			{
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
				em_ptr->note = _("には耐性がある！", " resists!");
				resists_tele = TRUE;
			}
		}

		if (!resists_tele)
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			em_ptr->do_dist = em_ptr->dam;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_TURN_UNDEAD:
	{
		if (em_ptr->r_ptr->flags3 & (RF3_UNDEAD))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);

			em_ptr->do_fear = damroll(3, (em_ptr->dam / 2)) + 1;
			if (em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10)
			{
				em_ptr->note = _("には効果がなかった。", " is unaffected.");
				em_ptr->obvious = FALSE;
				em_ptr->do_fear = 0;
			}
		}
		else
		{
			em_ptr->skipped = TRUE;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_TURN_EVIL:
	{
		if (em_ptr->r_ptr->flags3 & (RF3_EVIL))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_EVIL);

			em_ptr->do_fear = damroll(3, (em_ptr->dam / 2)) + 1;
			if (em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10)
			{
				em_ptr->note = _("には効果がなかった。", " is unaffected.");
				em_ptr->obvious = FALSE;
				em_ptr->do_fear = 0;
			}
		}
		else
		{
			em_ptr->skipped = TRUE;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_TURN_ALL:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		em_ptr->do_fear = damroll(3, (em_ptr->dam / 2)) + 1;
		if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
			(em_ptr->r_ptr->flags3 & (RF3_NO_FEAR)) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
			em_ptr->do_fear = 0;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_DISP_UNDEAD:
	{
		if (em_ptr->r_ptr->flags3 & (RF3_UNDEAD))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			/* Learn about em_ptr->effect_type */
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);

			em_ptr->note = _("は身震いした。", " shudders.");
			em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
		}
		else
		{
			em_ptr->skipped = TRUE;
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_DISP_EVIL:
	{
		if (em_ptr->r_ptr->flags3 & (RF3_EVIL))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_EVIL);

			em_ptr->note = _("は身震いした。", " shudders.");
			em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
		}
		else
		{
			em_ptr->skipped = TRUE;
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_DISP_GOOD:
	{
		if (em_ptr->r_ptr->flags3 & (RF3_GOOD))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_GOOD);

			em_ptr->note = _("は身震いした。", " shudders.");
			em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
		}
		else
		{
			em_ptr->skipped = TRUE;
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_DISP_LIVING:
	{
		if (monster_living(em_ptr->m_ptr->r_idx))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			em_ptr->note = _("は身震いした。", " shudders.");
			em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
		}
		else
		{
			em_ptr->skipped = TRUE;
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_DISP_DEMON:
	{
		if (em_ptr->r_ptr->flags3 & (RF3_DEMON))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_DEMON);

			em_ptr->note = _("は身震いした。", " shudders.");
			em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
		}
		else
		{
			em_ptr->skipped = TRUE;
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_DISP_ALL:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		em_ptr->note = _("は身震いした。", " shudders.");
		em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
		break;
	}
	case GF_DRAIN_MANA:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if ((em_ptr->r_ptr->flags4 & ~(RF4_NOMAGIC_MASK)) || (em_ptr->r_ptr->a_ability_flags1 & ~(RF5_NOMAGIC_MASK)) || (em_ptr->r_ptr->a_ability_flags2 & ~(RF6_NOMAGIC_MASK)))
		{
			if (em_ptr->who > 0)
			{
				if (em_ptr->m_caster_ptr->hp < em_ptr->m_caster_ptr->maxhp)
				{
					em_ptr->m_caster_ptr->hp += em_ptr->dam;
					if (em_ptr->m_caster_ptr->hp > em_ptr->m_caster_ptr->maxhp) em_ptr->m_caster_ptr->hp = em_ptr->m_caster_ptr->maxhp;
					if (caster_ptr->health_who == em_ptr->who) caster_ptr->redraw |= (PR_HEALTH);
					if (caster_ptr->riding == em_ptr->who) caster_ptr->redraw |= (PR_UHEALTH);

					if (em_ptr->see_s_msg)
					{
						monster_desc(caster_ptr, em_ptr->killer, em_ptr->m_caster_ptr, 0);
						msg_format(_("%^sは気分が良さそうだ。", "%^s appears healthier."), em_ptr->killer);
					}
				}
			}
			else
			{
				msg_format(_("%sから精神エネルギーを吸いとった。", "You draw psychic energy from %s."), em_ptr->m_name);
				(void)hp_player(caster_ptr, em_ptr->dam);
			}
		}
		else
		{
			if (em_ptr->see_s_msg) msg_format(_("%sには効果がなかった。", "%s is unaffected."), em_ptr->m_name);
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_MIND_BLAST:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (!em_ptr->who) msg_format(_("%sをじっと睨んだ。", "You gaze intently at %s."), em_ptr->m_name);

		if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
			(em_ptr->r_ptr->flags3 & RF3_NO_CONF) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->caster_lev - 10) < 1 ? 1 : (em_ptr->caster_lev - 10)) + 10))
		{
			if (em_ptr->r_ptr->flags3 & (RF3_NO_CONF))
			{
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);
			}

			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
		}
		else if (em_ptr->r_ptr->flags2 & RF2_EMPTY_MIND)
		{
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags2 |= (RF2_EMPTY_MIND);
			em_ptr->note = _("には完全な耐性がある！", " is immune.");
			em_ptr->dam = 0;
		}
		else if (em_ptr->r_ptr->flags2 & RF2_WEIRD_MIND)
		{
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags2 |= (RF2_WEIRD_MIND);
			em_ptr->note = _("には耐性がある。", " resists.");
			em_ptr->dam /= 3;
		}
		else
		{
			em_ptr->note = _("は精神攻撃を食らった。", " is blasted by psionic energy.");
			em_ptr->note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");

			if (em_ptr->who > 0) em_ptr->do_conf = randint0(4) + 4;
			else em_ptr->do_conf = randint0(8) + 8;
		}

		break;
	}
	case GF_BRAIN_SMASH:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (!em_ptr->who) msg_format(_("%sをじっと睨んだ。", "You gaze intently at %s."), em_ptr->m_name);

		if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
			(em_ptr->r_ptr->flags3 & RF3_NO_CONF) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->caster_lev - 10) < 1 ? 1 : (em_ptr->caster_lev - 10)) + 10))
		{
			if (em_ptr->r_ptr->flags3 & (RF3_NO_CONF))
			{
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);
			}

			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
		}
		else if (em_ptr->r_ptr->flags2 & RF2_EMPTY_MIND)
		{
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags2 |= (RF2_EMPTY_MIND);
			em_ptr->note = _("には完全な耐性がある！", " is immune.");
			em_ptr->dam = 0;
		}
		else if (em_ptr->r_ptr->flags2 & RF2_WEIRD_MIND)
		{
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags2 |= (RF2_WEIRD_MIND);
			em_ptr->note = _("には耐性がある！", " resists!");
			em_ptr->dam /= 3;
		}
		else
		{
			em_ptr->note = _("は精神攻撃を食らった。", " is blasted by psionic energy.");
			em_ptr->note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");
			if (em_ptr->who > 0)
			{
				em_ptr->do_conf = randint0(4) + 4;
				em_ptr->do_stun = randint0(4) + 4;
			}
			else
			{
				em_ptr->do_conf = randint0(8) + 8;
				em_ptr->do_stun = randint0(8) + 8;
			}
			(void)set_monster_slow(caster_ptr, em_ptr->g_ptr->m_idx, MON_SLOW(em_ptr->m_ptr) + 10);
		}

		break;
	}
	case GF_CAUSE_1:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (!em_ptr->who) msg_format(_("%sを指差して呪いをかけた。", "You point at %s and curse."), em_ptr->m_name);
		if (randint0(100 + (em_ptr->caster_lev / 2)) < (em_ptr->r_ptr->level + 35))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_CAUSE_2:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (!em_ptr->who) msg_format(_("%sを指差して恐ろしげに呪いをかけた。", "You point at %s and curse horribly."), em_ptr->m_name);

		if (randint0(100 + (em_ptr->caster_lev / 2)) < (em_ptr->r_ptr->level + 35))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_CAUSE_3:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (!em_ptr->who) msg_format(_("%sを指差し、恐ろしげに呪文を唱えた！", "You point at %s, incanting terribly!"), em_ptr->m_name);

		if (randint0(100 + (em_ptr->caster_lev / 2)) < (em_ptr->r_ptr->level + 35))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_CAUSE_4:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (!em_ptr->who)
			msg_format(_("%sの秘孔を突いて、「お前は既に死んでいる」と叫んだ。",
				"You point at %s, screaming the word, 'DIE!'."), em_ptr->m_name);

		if ((randint0(100 + (em_ptr->caster_lev / 2)) < (em_ptr->r_ptr->level + 35)) && ((em_ptr->who <= 0) || (em_ptr->m_caster_ptr->r_idx != MON_KENSHIROU)))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
		}
		break;
	}
	case GF_HAND_DOOM:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flags1 & RF1_UNIQUE)
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
		}
		else
		{
			if ((em_ptr->who > 0) ? ((em_ptr->caster_lev + randint1(em_ptr->dam)) > (em_ptr->r_ptr->level + 10 + randint1(20))) :
				(((em_ptr->caster_lev / 2) + randint1(em_ptr->dam)) > (em_ptr->r_ptr->level + randint1(200))))
			{
				em_ptr->dam = ((40 + randint1(20)) * em_ptr->m_ptr->hp) / 100;

				if (em_ptr->m_ptr->hp < em_ptr->dam) em_ptr->dam = em_ptr->m_ptr->hp - 1;
			}
			else
			{
				/* todo 乱数で破滅のを弾いた結果が「耐性を持っている」ことになるのはおかしい */
				em_ptr->note = _("は耐性を持っている！", "resists!");
				em_ptr->dam = 0;
			}
		}

		break;
	}
	case GF_CAPTURE:
	{
		int nokori_hp;
		if ((floor_ptr->inside_quest && (quest[floor_ptr->inside_quest].type == QUEST_TYPE_KILL_ALL) && !is_pet(em_ptr->m_ptr)) ||
			(em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (em_ptr->r_ptr->flags7 & (RF7_NAZGUL)) || (em_ptr->r_ptr->flags7 & (RF7_UNIQUE2)) || (em_ptr->r_ptr->flags1 & RF1_QUESTOR) || em_ptr->m_ptr->parent_m_idx)
		{
			msg_format(_("%sには効果がなかった。", "%s is unaffected."), em_ptr->m_name);
			em_ptr->skipped = TRUE;
			break;
		}

		if (is_pet(em_ptr->m_ptr)) nokori_hp = em_ptr->m_ptr->maxhp * 4L;
		else if ((caster_ptr->pclass == CLASS_BEASTMASTER) && monster_living(em_ptr->m_ptr->r_idx))
			nokori_hp = em_ptr->m_ptr->maxhp * 3 / 10;
		else
			nokori_hp = em_ptr->m_ptr->maxhp * 3 / 20;

		if (em_ptr->m_ptr->hp >= nokori_hp)
		{
			msg_format(_("もっと弱らせないと。", "You need to weaken %s more."), em_ptr->m_name);
			em_ptr->skipped = TRUE;
		}
		else if (em_ptr->m_ptr->hp < randint0(nokori_hp))
		{
			if (em_ptr->m_ptr->mflag2 & MFLAG2_CHAMELEON) choose_new_monster(caster_ptr, em_ptr->g_ptr->m_idx, FALSE, MON_CHAMELEON);
			msg_format(_("%sを捕えた！", "You capture %^s!"), em_ptr->m_name);
			cap_mon = em_ptr->m_ptr->r_idx;
			cap_mspeed = em_ptr->m_ptr->mspeed;
			cap_hp = em_ptr->m_ptr->hp;
			cap_maxhp = em_ptr->m_ptr->max_maxhp;
			cap_nickname = em_ptr->m_ptr->nickname;
			if (em_ptr->g_ptr->m_idx == caster_ptr->riding)
			{
				if (rakuba(caster_ptr, -1, FALSE))
				{
					msg_format(_("地面に落とされた。", "You have fallen from %s."), em_ptr->m_name);
				}
			}

			delete_monster_idx(caster_ptr, em_ptr->g_ptr->m_idx);

			return GF_SWITCH_TRUE;
		}
		else
		{
			msg_format(_("うまく捕まえられなかった。", "You failed to capture %s."), em_ptr->m_name);
			em_ptr->skipped = TRUE;
		}

		break;
	}
	case GF_ATTACK:
	{
		return (gf_switch_result)py_attack(caster_ptr, em_ptr->y, em_ptr->x, em_ptr->dam);
	}
	case GF_ENGETSU:
	{
		int effect = 0;
		bool done = TRUE;

		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flags2 & RF2_EMPTY_MIND)
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
			em_ptr->skipped = TRUE;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags2 |= (RF2_EMPTY_MIND);
			break;
		}
		if (MON_CSLEEP(em_ptr->m_ptr))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
			em_ptr->skipped = TRUE;
			break;
		}

		if (one_in_(5)) effect = 1;
		else if (one_in_(4)) effect = 2;
		else if (one_in_(3)) effect = 3;
		else done = FALSE;

		if (effect == 1)
		{
			if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
				(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
			{
				em_ptr->note = _("には効果がなかった。", " is unaffected.");
				em_ptr->obvious = FALSE;
			}
			else
			{
				if (set_monster_slow(caster_ptr, em_ptr->g_ptr->m_idx, MON_SLOW(em_ptr->m_ptr) + 50))
				{
					em_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
				}
			}
		}
		else if (effect == 2)
		{
			em_ptr->do_stun = damroll((caster_ptr->lev / 10) + 3, (em_ptr->dam)) + 1;
			if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
				(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
			{
				em_ptr->do_stun = 0;
				em_ptr->note = _("には効果がなかった。", " is unaffected.");
				em_ptr->obvious = FALSE;
			}
		}
		else if (effect == 3)
		{
			if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
				(em_ptr->r_ptr->flags3 & RF3_NO_SLEEP) ||
				(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
			{
				if (em_ptr->r_ptr->flags3 & RF3_NO_SLEEP)
				{
					if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_NO_SLEEP);
				}

				em_ptr->note = _("には効果がなかった。", " is unaffected.");
				em_ptr->obvious = FALSE;
			}
			else
			{
				/* Go to sleep (much) later */
				em_ptr->note = _("は眠り込んでしまった！", " falls asleep!");
				em_ptr->do_sleep = 500;
			}
		}

		if (!done)
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_GENOCIDE:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (genocide_aux(caster_ptr, em_ptr->g_ptr->m_idx, em_ptr->dam, !em_ptr->who, (em_ptr->r_ptr->level + 1) / 2, _("モンスター消滅", "Genocide One")))
		{
			if (em_ptr->seen_msg) msg_format(_("%sは消滅した！", "%^s disappeared!"), em_ptr->m_name);
			chg_virtue(caster_ptr, V_VITALITY, -1);
			return GF_SWITCH_TRUE;
		}

		em_ptr->skipped = TRUE;
		break;
	}
	case GF_PHOTO:
	{
		if (!em_ptr->who)
			msg_format(_("%sを写真に撮った。", "You take a photograph of %s."), em_ptr->m_name);

		if (em_ptr->r_ptr->flags3 & (RF3_HURT_LITE))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_HURT_LITE);

			em_ptr->note = _("は光に身をすくめた！", " cringes from the light!");
			em_ptr->note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
		}
		else
		{
			em_ptr->dam = 0;
		}

		em_ptr->photo = em_ptr->m_ptr->r_idx;
		break;
	}
	case GF_CRUSADE:
	{
		bool success = FALSE;
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if ((em_ptr->r_ptr->flags3 & (RF3_GOOD)) && !floor_ptr->inside_arena)
		{
			if (em_ptr->r_ptr->flags3 & (RF3_NO_CONF)) em_ptr->dam -= 50;
			if (em_ptr->dam < 1) em_ptr->dam = 1;

			if (is_pet(em_ptr->m_ptr))
			{
				em_ptr->note = _("の動きが速くなった。", " starts moving faster.");
				(void)set_monster_fast(caster_ptr, em_ptr->g_ptr->m_idx, MON_FAST(em_ptr->m_ptr) + 100);
				success = TRUE;
			}
			else if ((em_ptr->r_ptr->flags1 & (RF1_QUESTOR)) ||
				(em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
				(em_ptr->m_ptr->mflag2 & MFLAG2_NOPET) ||
				(caster_ptr->cursed & TRC_AGGRAVATE) ||
				((em_ptr->r_ptr->level + 10) > randint1(em_ptr->dam)))
			{
				if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else
			{
				em_ptr->note = _("を支配した。", " is tamed!");
				set_pet(caster_ptr, em_ptr->m_ptr);
				(void)set_monster_fast(caster_ptr, em_ptr->g_ptr->m_idx, MON_FAST(em_ptr->m_ptr) + 100);

				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_GOOD);
				success = TRUE;
			}
		}

		if (!success)
		{
			if (!(em_ptr->r_ptr->flags3 & RF3_NO_FEAR))
			{
				em_ptr->do_fear = randint1(90) + 10;
			}
			else if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr))
				em_ptr->r_ptr->r_flags3 |= (RF3_NO_FEAR);
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_WOUNDS:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if (randint0(100 + em_ptr->dam) < (em_ptr->r_ptr->level + 50))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
		}
		break;
	}
	default:
	{
		em_ptr->skipped = TRUE;
		em_ptr->dam = 0;
		break;
	}
	}

	return GF_SWITCH_CONTINUE;
}
