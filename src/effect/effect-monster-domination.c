#include "angband.h"
#include "effect/effect-monster-util.h"
#include "effect/effect-monster-domination.h"
#include "player-effects.h"
#include "spells-diceroll.h"

static void effect_monster_domination_corrupted_addition(player_type *caster_ptr, effect_monster_type *em_ptr)
{
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


// Powerful demons & undead can turn a mindcrafter's attacks back on them.
static void effect_monster_domination_corrupted(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	bool is_corrupted = ((em_ptr->r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) != 0) &&
		(em_ptr->r_ptr->level > caster_ptr->lev / 2) &&
		(one_in_(2));
	if (!is_corrupted)
	{
		em_ptr->note = _("には効果がなかった。", " is unaffected.");
		em_ptr->obvious = FALSE;
		return;
	}

	em_ptr->note = NULL;
	msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！",
		(em_ptr->seen ? "%^s's corrupted mind backlashes your attack!" :
			"%^ss corrupted mind backlashes your attack!")), em_ptr->m_name);
	if (randint0(100 + em_ptr->r_ptr->level / 2) < caster_ptr->skill_sav)
	{
		msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
		return;
	}

	effect_monster_domination_corrupted_addition(caster_ptr, em_ptr);
}


static void effect_monster_domination_addition(effect_monster_type *em_ptr)
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


gf_switch_result effect_monster_domination(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if (!is_hostile(em_ptr->m_ptr)) return GF_SWITCH_CONTINUE;

	if (em_ptr->seen) em_ptr->obvious = TRUE;

	if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) ||
		(em_ptr->r_ptr->flags3 & RF3_NO_CONF) ||
		(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
	{
		if (((em_ptr->r_ptr->flags3 & RF3_NO_CONF) != 0) && is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr))
			em_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);

		em_ptr->do_conf = 0;
		effect_monster_domination_corrupted(caster_ptr, em_ptr);
		em_ptr->dam = 0;
		return GF_SWITCH_CONTINUE;
	}

	if (!common_saving_throw_charm(caster_ptr, em_ptr->dam, em_ptr->m_ptr))
	{
		em_ptr->note = _("があなたに隷属した。", " is in your thrall!");
		set_pet(caster_ptr, em_ptr->m_ptr);
		em_ptr->dam = 0;
		return GF_SWITCH_CONTINUE;
	}

	effect_monster_domination_addition(em_ptr);
	em_ptr->dam = 0;
	return GF_SWITCH_CONTINUE;
}
