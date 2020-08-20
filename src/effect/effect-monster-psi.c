#include "effect/effect-monster-psi.h"
#include "core/player-redraw-types.h"
#include "core/window-redrawer.h"
#include "effect/effect-monster-util.h"
#include "floor/line-of-sight.h"
#include "mind/mind-mirror-master.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "status/bad-status-setter.h"
#include "player/player-damage.h"
#include "view/display-messages.h"
#include "world/world.h"

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


static bool effect_monster_psi_corrupted(player_type *caster_ptr, effect_monster_type *em_ptr)
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
	if (!effect_monster_psi_corrupted(caster_ptr, em_ptr)) return;

	if ((randint0(100 + em_ptr->r_ptr->level / 2) < caster_ptr->skill_sav) && !check_multishadow(caster_ptr))
	{
		msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
		em_ptr->dam = 0;
		return;
	}

	/* Injure +/- confusion */
	monster_desc(caster_ptr, em_ptr->killer, em_ptr->m_ptr, MD_WRONGDOER_NAME);
	take_hit(caster_ptr, DAMAGE_ATTACK, em_ptr->dam, em_ptr->killer, -1);
	if (!one_in_(4) || check_multishadow(caster_ptr))
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


process_result effect_monster_psi(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if (em_ptr->seen) em_ptr->obvious = TRUE;
	if (!(los(caster_ptr, em_ptr->m_ptr->fy, em_ptr->m_ptr->fx, caster_ptr->y, caster_ptr->x)))
	{
		if (em_ptr->seen_msg)
			msg_format(_("%sはあなたが見えないので影響されない！", "%^s can't see you, and isn't affected!"), em_ptr->m_name);

		em_ptr->skipped = TRUE;
		return PROCESS_CONTINUE;
	}

	effect_monster_psi_resist(caster_ptr, em_ptr);
	effect_monster_psi_addition(em_ptr);
	em_ptr->note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");
	return PROCESS_CONTINUE;
}


static bool effect_monster_psi_drain_corrupted(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	bool is_corrupted = ((em_ptr->r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) != 0) &&
		(em_ptr->r_ptr->level > caster_ptr->lev / 2) &&
		(one_in_(2));
	if (!is_corrupted) return FALSE;

	em_ptr->note = NULL;
	msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！",
		(em_ptr->seen ? "%^s's corrupted mind backlashes your attack!" :
			"%^ss corrupted mind backlashes your attack!")), em_ptr->m_name);
	return TRUE;
}


// Powerful demons & undead can turn a mindcrafter's attacks back on them.
static void effect_monster_psi_drain_resist(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	em_ptr->note = _("には耐性がある！", " resists!");
	em_ptr->dam /= 3;
	if (effect_monster_psi_drain_corrupted(caster_ptr, em_ptr)) return;

	if ((randint0(100 + em_ptr->r_ptr->level / 2) < caster_ptr->skill_sav) && !check_multishadow(caster_ptr))
	{
		msg_print(_("あなたは効力を跳ね返した！", "You resist the effects!"));
		em_ptr->dam = 0;
		return;
	}

	monster_desc(caster_ptr, em_ptr->killer, em_ptr->m_ptr, MD_WRONGDOER_NAME);
	if (check_multishadow(caster_ptr))
	{
		take_hit(caster_ptr, DAMAGE_ATTACK, em_ptr->dam, em_ptr->killer, -1);
		em_ptr->dam = 0;
		return;
	}

	msg_print(_("超能力パワーを吸いとられた！", "Your psychic energy is drained!"));
	caster_ptr->csp -= damroll(5, em_ptr->dam) / 2;
	if (caster_ptr->csp < 0) caster_ptr->csp = 0;

	caster_ptr->redraw |= PR_MANA;
	caster_ptr->window |= (PW_SPELL);
	take_hit(caster_ptr, DAMAGE_ATTACK, em_ptr->dam, em_ptr->killer, -1);
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


process_result effect_monster_psi_drain(player_type *caster_ptr, effect_monster_type *em_ptr)
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
	return PROCESS_CONTINUE;
}


process_result effect_monster_telekinesis(player_type *caster_ptr, effect_monster_type *em_ptr)
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

	return PROCESS_CONTINUE;
}
