/*!
 * @brief 魔法種別による各種処理切り替え
 * @date 2020/04/29
 * @author Hourier
 */

#include "angband.h"
#include "effect-monster-util.h"
#include "effect/effect-monster-switcher.h"
#include "player-damage.h"
#include "avatar.h"
#include "quest.h"
#include "monster-status.h"
#include "effect/spells-effect-util.h"
#include "player-effects.h"
#include "monsterrace-hook.h"
#include "combat/melee.h"
#include "cmd/cmd-pet.h" // 暫定、後で消すかも.
#include "spell/spells-type.h"
#include "effect/effect-monster-resist-hurt.h"
#include "effect/effect-monster-psi.h"
#include "effect/effect-monster-domination.h"
#include "effect/effect-monster-oldies.h"
#include "effect/effect-monster-charm.h"
#include "effect/effect-monster-lite-dark.h"
#include "effect/effect-monster-evil.h"
#include "effect/effect-monster-spirit.h"

gf_switch_result effect_monster_hypodynamia(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if (em_ptr->seen) em_ptr->obvious = TRUE;
	if (monster_living(em_ptr->m_ptr->r_idx))
	{
		em_ptr->do_time = (em_ptr->dam + 7) / 8;
		return GF_SWITCH_CONTINUE;
	}

	if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr))
	{
		if (em_ptr->r_ptr->flags3 & RF3_DEMON) em_ptr->r_ptr->r_flags3 |= (RF3_DEMON);
		if (em_ptr->r_ptr->flags3 & RF3_UNDEAD) em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);
		if (em_ptr->r_ptr->flags3 & RF3_NONLIVING) em_ptr->r_ptr->r_flags3 |= (RF3_NONLIVING);
	}

	em_ptr->note = _("には効果がなかった。", " is unaffected.");
	em_ptr->obvious = FALSE;
	em_ptr->dam = 0;
	return GF_SWITCH_CONTINUE;
}


// todo リファクタリング前のコード時点で、単に耐性があるだけでもダメージ0だった.
gf_switch_result effect_monster_death_ray(player_type *caster_ptr, effect_monster_type *em_ptr)
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
		return GF_SWITCH_CONTINUE;
	}

	if (((em_ptr->r_ptr->flags1 & RF1_UNIQUE) &&
		(randint1(888) != 666)) ||
		(((em_ptr->r_ptr->level + randint1(20)) > randint1((em_ptr->caster_lev / 2) + randint1(10))) &&
			randint1(100) != 66))
	{
		em_ptr->note = _("には耐性がある！", " resists!");
		em_ptr->obvious = FALSE;
		em_ptr->dam = 0;
	}

	return GF_SWITCH_CONTINUE;
}


gf_switch_result effect_monster_stasis(effect_monster_type *em_ptr, bool to_evil)
{
	if (em_ptr->seen) em_ptr->obvious = TRUE;

	int stasis_damage = (em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10);
	bool has_resistance = (em_ptr->r_ptr->flags1 & RF1_UNIQUE) != 0;
	has_resistance |= em_ptr->r_ptr->level > randint1(stasis_damage) + 10;
	if (to_evil) has_resistance |= (em_ptr->r_ptr->flags3 & RF3_EVIL) == 0;

	if (has_resistance)
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
	return GF_SWITCH_CONTINUE;
}


gf_switch_result effect_monster_stun(effect_monster_type *em_ptr)
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
	return GF_SWITCH_CONTINUE;
}


gf_switch_result effect_monster_kill_wall(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flags3 & (RF3_HURT_ROCK)) == 0)
	{
		em_ptr->dam = 0;
		return GF_SWITCH_CONTINUE;
	}

	if (em_ptr->seen) em_ptr->obvious = TRUE;

	if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_HURT_ROCK);

	em_ptr->note = _("の皮膚がただれた！", " loses some skin!");
	em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
	return GF_SWITCH_CONTINUE;
}


gf_switch_result effect_monster_curse_1(effect_monster_type *em_ptr)
{
	if (em_ptr->seen) em_ptr->obvious = TRUE;
	if (!em_ptr->who) msg_format(_("%sを指差して呪いをかけた。", "You point at %s and curse."), em_ptr->m_name);
	if (randint0(100 + (em_ptr->caster_lev / 2)) < (em_ptr->r_ptr->level + 35))
	{
		em_ptr->note = _("には効果がなかった。", " is unaffected.");
		em_ptr->dam = 0;
	}

	return GF_SWITCH_CONTINUE;
}


gf_switch_result effect_monster_curse_2(effect_monster_type *em_ptr)
{
	if (em_ptr->seen) em_ptr->obvious = TRUE;
	if (!em_ptr->who) msg_format(_("%sを指差して恐ろしげに呪いをかけた。", "You point at %s and curse horribly."), em_ptr->m_name);

	if (randint0(100 + (em_ptr->caster_lev / 2)) < (em_ptr->r_ptr->level + 35))
	{
		em_ptr->note = _("には効果がなかった。", " is unaffected.");
		em_ptr->dam = 0;
	}

	return GF_SWITCH_CONTINUE;
}


gf_switch_result effect_monster_curse_3(effect_monster_type *em_ptr)
{
	if (em_ptr->seen) em_ptr->obvious = TRUE;
	if (!em_ptr->who) msg_format(_("%sを指差し、恐ろしげに呪文を唱えた！", "You point at %s, incanting terribly!"), em_ptr->m_name);

	if (randint0(100 + (em_ptr->caster_lev / 2)) < (em_ptr->r_ptr->level + 35))
	{
		em_ptr->note = _("には効果がなかった。", " is unaffected.");
		em_ptr->dam = 0;
	}

	return GF_SWITCH_CONTINUE;
}


gf_switch_result effect_monster_curse_4(effect_monster_type *em_ptr)
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

	return GF_SWITCH_CONTINUE;
}


gf_switch_result effect_monster_hand_doom(effect_monster_type *em_ptr)
{
	if (em_ptr->seen) em_ptr->obvious = TRUE;

	if (em_ptr->r_ptr->flags1 & RF1_UNIQUE)
	{
		em_ptr->note = _("には効果がなかった。", " is unaffected.");
		em_ptr->dam = 0;
		return GF_SWITCH_CONTINUE;
	}

	if ((em_ptr->who > 0) ? ((em_ptr->caster_lev + randint1(em_ptr->dam)) > (em_ptr->r_ptr->level + 10 + randint1(20))) :
		(((em_ptr->caster_lev / 2) + randint1(em_ptr->dam)) > (em_ptr->r_ptr->level + randint1(200))))
	{
		em_ptr->dam = ((40 + randint1(20)) * em_ptr->m_ptr->hp) / 100;
		if (em_ptr->m_ptr->hp < em_ptr->dam)
			em_ptr->dam = em_ptr->m_ptr->hp - 1;
	}
	else
	{
		em_ptr->note = _("は破滅の手に耐え切った！", "resists!");
		em_ptr->dam = 0;
	}

	return GF_SWITCH_CONTINUE;
}


gf_switch_result effect_monster_capture(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	floor_type *floor_ptr = caster_ptr->current_floor_ptr;
	int nokori_hp;
	if ((floor_ptr->inside_quest && (quest[floor_ptr->inside_quest].type == QUEST_TYPE_KILL_ALL) && !is_pet(em_ptr->m_ptr)) ||
		(em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (em_ptr->r_ptr->flags7 & (RF7_NAZGUL)) || (em_ptr->r_ptr->flags7 & (RF7_UNIQUE2)) || (em_ptr->r_ptr->flags1 & RF1_QUESTOR) || em_ptr->m_ptr->parent_m_idx)
	{
		msg_format(_("%sには効果がなかった。", "%s is unaffected."), em_ptr->m_name);
		em_ptr->skipped = TRUE;
		return GF_SWITCH_CONTINUE;
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

	return GF_SWITCH_CONTINUE;
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
		return effect_monster_telekinesis(caster_ptr, em_ptr);
	case GF_DOMINATION:
		return effect_monster_domination(caster_ptr, em_ptr);
	case GF_ICE:
		return effect_monster_icee_bolt(caster_ptr, em_ptr);
	case GF_HYPODYNAMIA:
		return effect_monster_hypodynamia(caster_ptr, em_ptr);
	case GF_DEATH_RAY:
		return effect_monster_death_ray(caster_ptr, em_ptr);
	case GF_OLD_POLY:
		return effect_monster_old_poly(em_ptr);
	case GF_OLD_CLONE:
		return effect_monster_old_clone(caster_ptr, em_ptr);
	case GF_STAR_HEAL:
		if (effect_monster_old_clone(caster_ptr, em_ptr) == GF_SWITCH_TRUE)
			return GF_SWITCH_CONTINUE;
	/* Fall through */
	case GF_OLD_HEAL:
		return effect_monster_old_heal(caster_ptr, em_ptr);
	case GF_OLD_SPEED:
		return effect_monster_old_speed(caster_ptr, em_ptr);
	case GF_OLD_SLOW:
		return effect_monster_old_slow(caster_ptr, em_ptr);
	case GF_OLD_SLEEP:
		return effect_monster_old_sleep(caster_ptr, em_ptr);
	case GF_STASIS_EVIL:
		return effect_monster_stasis(em_ptr, TRUE);
	case GF_STASIS:
		return effect_monster_stasis(em_ptr, FALSE);
	case GF_CHARM:
		return effect_monster_charm(caster_ptr, em_ptr);
	case GF_CONTROL_UNDEAD:
		return effect_monster_control_undead(caster_ptr, em_ptr);
	case GF_CONTROL_DEMON:
		return effect_monster_control_demon(caster_ptr, em_ptr);
	case GF_CONTROL_ANIMAL:
		return effect_monster_control_animal(caster_ptr, em_ptr);
	case GF_CHARM_LIVING:
		return effect_monster_charm_living(caster_ptr, em_ptr);
	case GF_OLD_CONF:
		return effect_monster_old_conf(caster_ptr, em_ptr);
	case GF_STUN:
		return effect_monster_stun(em_ptr);
	case GF_LITE_WEAK:
		return effect_monster_lite_weak(caster_ptr, em_ptr);
	case GF_LITE:
		return effect_monster_lite(caster_ptr, em_ptr);
	case GF_DARK:
		return effect_monster_dark(caster_ptr, em_ptr);
	case GF_KILL_WALL:
		return effect_monster_kill_wall(caster_ptr, em_ptr);
	case GF_AWAY_UNDEAD:
		return effect_monster_away_undead(caster_ptr, em_ptr);
	case GF_AWAY_EVIL:
		return effect_monster_away_evil(caster_ptr, em_ptr);
	case GF_AWAY_ALL:
		return effect_monster_away_all(caster_ptr, em_ptr);
	case GF_TURN_UNDEAD:
		return effect_monster_turn_undead(caster_ptr, em_ptr);
	case GF_TURN_EVIL:
		return effect_monster_turn_evil(caster_ptr, em_ptr);
	case GF_TURN_ALL:
		return effect_monster_turn_all(em_ptr);
	case GF_DISP_UNDEAD:
		return effect_monster_disp_undead(caster_ptr, em_ptr);
	case GF_DISP_EVIL:
		return effect_monster_disp_evil(caster_ptr, em_ptr);
	case GF_DISP_GOOD:
		return effect_monster_disp_good(caster_ptr, em_ptr);
	case GF_DISP_LIVING:
		return effect_monster_disp_living(em_ptr);
	case GF_DISP_DEMON:
		return effect_monster_disp_demon(caster_ptr, em_ptr);
	case GF_DISP_ALL:
		return effect_monster_disp_all(em_ptr);
	case GF_DRAIN_MANA:
		return effect_monster_drain_mana(caster_ptr, em_ptr);
	case GF_MIND_BLAST:
		return effect_monster_mind_blast(caster_ptr, em_ptr);
	case GF_BRAIN_SMASH:
		return effect_monster_brain_smash(caster_ptr, em_ptr);
	case GF_CAUSE_1:
		return effect_monster_curse_1(em_ptr);
	case GF_CAUSE_2:
		return effect_monster_curse_2(em_ptr);
	case GF_CAUSE_3:
		return effect_monster_curse_3(em_ptr);
	case GF_CAUSE_4:
		return effect_monster_curse_4(em_ptr);
	case GF_HAND_DOOM:
		return effect_monster_hand_doom(em_ptr);
	case GF_CAPTURE:
		return effect_monster_capture(caster_ptr, em_ptr);
	case GF_ATTACK:
		return (gf_switch_result)py_attack(caster_ptr, em_ptr->y, em_ptr->x, em_ptr->dam);
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
