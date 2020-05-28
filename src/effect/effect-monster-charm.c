#include "system/angband.h"
#include "effect/effect-monster-util.h"
#include "effect/effect-monster-charm.h"
#include "player/avatar.h"
#include "spell/spells-diceroll.h"
#include "monster/monster-race-hook.h"
#include "object/trc-types.h"

static void effect_monster_charm_resist(player_type *caster_ptr, effect_monster_type *em_ptr)
{
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
}


gf_switch_result effect_monster_charm(player_type *caster_ptr, effect_monster_type *em_ptr)
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

	effect_monster_charm_resist(caster_ptr, em_ptr);
	em_ptr->dam = 0;
	return GF_SWITCH_CONTINUE;
}


gf_switch_result effect_monster_control_undead(player_type *caster_ptr, effect_monster_type *em_ptr)
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
	return GF_SWITCH_CONTINUE;
}


gf_switch_result effect_monster_control_demon(player_type *caster_ptr, effect_monster_type *em_ptr)
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
	return GF_SWITCH_CONTINUE;
}


gf_switch_result effect_monster_control_animal(player_type *caster_ptr, effect_monster_type *em_ptr)
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
	return GF_SWITCH_CONTINUE;
}


gf_switch_result effect_monster_charm_living(player_type *caster_ptr, effect_monster_type *em_ptr)
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
	return GF_SWITCH_CONTINUE;
}
