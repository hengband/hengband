#include "effect/effect-monster-evil.h"
#include "effect/effect-monster-util.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-info.h"

static bool effect_monster_away_resist(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flagsr & RFR_RES_TELE) == 0) return FALSE;

	if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (em_ptr->r_ptr->flagsr & RFR_RES_ALL))
	{
		if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
		em_ptr->note = _("には効果がなかった。", " is unaffected.");
		return TRUE;
	}

	if (em_ptr->r_ptr->level > randint1(100))
	{
		if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
		em_ptr->note = _("には耐性がある！", " resists!");
		return TRUE;
	}

	return FALSE;
}


process_result effect_monster_away_undead(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flags3 & (RF3_UNDEAD)) == 0)
	{
		em_ptr->skipped = TRUE;
		em_ptr->dam = 0;
		return PROCESS_CONTINUE;
	}

	bool resists_tele = effect_monster_away_resist(caster_ptr, em_ptr);
	if (!resists_tele)
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr))
			em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);

		em_ptr->do_dist = em_ptr->dam;
	}

	em_ptr->dam = 0;
	return PROCESS_CONTINUE;
}


process_result effect_monster_away_evil(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flags3 & (RF3_EVIL)) == 0)
	{
		em_ptr->skipped = TRUE;
		em_ptr->dam = 0;
		return PROCESS_CONTINUE;
	}

	bool resists_tele = effect_monster_away_resist(caster_ptr, em_ptr);
	if (!resists_tele)
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr))
			em_ptr->r_ptr->r_flags3 |= (RF3_EVIL);

		em_ptr->do_dist = em_ptr->dam;
	}

	em_ptr->dam = 0;
	return PROCESS_CONTINUE;
}


process_result effect_monster_away_all(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	bool resists_tele = effect_monster_away_resist(caster_ptr, em_ptr);
	if (!resists_tele)
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		em_ptr->do_dist = em_ptr->dam;
	}

	em_ptr->dam = 0;
	return PROCESS_CONTINUE;
}


process_result effect_monster_turn_undead(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flags3 & (RF3_UNDEAD)) == 0)
	{
		em_ptr->skipped = TRUE;
		em_ptr->dam = 0;
		return PROCESS_CONTINUE;
	}

	if (em_ptr->seen) em_ptr->obvious = TRUE;

	if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);

	em_ptr->do_fear = damroll(3, (em_ptr->dam / 2)) + 1;
	if (em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10)
	{
		em_ptr->note = _("には効果がなかった。", " is unaffected.");
		em_ptr->obvious = FALSE;
		em_ptr->do_fear = 0;
	}

	em_ptr->dam = 0;
	return PROCESS_CONTINUE;
}


process_result effect_monster_turn_evil(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flags3 & (RF3_EVIL)) == 0)
	{
		em_ptr->skipped = TRUE;
		em_ptr->dam = 0;
		return PROCESS_CONTINUE;
	}

	if (em_ptr->seen) em_ptr->obvious = TRUE;

	if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_EVIL);

	em_ptr->do_fear = damroll(3, (em_ptr->dam / 2)) + 1;
	if (em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10)
	{
		em_ptr->note = _("には効果がなかった。", " is unaffected.");
		em_ptr->obvious = FALSE;
		em_ptr->do_fear = 0;
	}

	em_ptr->dam = 0;
	return PROCESS_CONTINUE;
}


process_result effect_monster_turn_all(effect_monster_type *em_ptr)
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
	return PROCESS_CONTINUE;
}


process_result effect_monster_disp_undead(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flags3 & (RF3_UNDEAD)) == 0)
	{
		em_ptr->skipped = TRUE;
		em_ptr->dam = 0;
		return PROCESS_CONTINUE;
	}

	if (em_ptr->seen) em_ptr->obvious = TRUE;

	if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr))
		em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);

	em_ptr->note = _("は身震いした。", " shudders.");
	em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
	return PROCESS_CONTINUE;
}


process_result effect_monster_disp_evil(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flags3 & (RF3_EVIL)) == 0)
	{
		em_ptr->skipped = TRUE;
		em_ptr->dam = 0;
		return PROCESS_CONTINUE;
	}

	if (em_ptr->seen) em_ptr->obvious = TRUE;

	if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_EVIL);

	em_ptr->note = _("は身震いした。", " shudders.");
	em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
	return PROCESS_CONTINUE;
}


process_result effect_monster_disp_good(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flags3 & (RF3_GOOD)) == 0)
	{
		em_ptr->skipped = TRUE;
		em_ptr->dam = 0;
		return PROCESS_CONTINUE;
	}

	if (em_ptr->seen) em_ptr->obvious = TRUE;

	if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_GOOD);

	em_ptr->note = _("は身震いした。", " shudders.");
	em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
	return PROCESS_CONTINUE;
}


process_result effect_monster_disp_living(effect_monster_type *em_ptr)
{
	if (!monster_living(em_ptr->m_ptr->r_idx))
	{
		em_ptr->skipped = TRUE;
		em_ptr->dam = 0;
		return PROCESS_CONTINUE;
	}

	if (em_ptr->seen) em_ptr->obvious = TRUE;

	em_ptr->note = _("は身震いした。", " shudders.");
	em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
	return PROCESS_CONTINUE;
}


process_result effect_monster_disp_demon(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flags3 & (RF3_DEMON)) == 0)
	{
		em_ptr->skipped = TRUE;
		em_ptr->dam = 0;
		return PROCESS_CONTINUE;
	}

	if (em_ptr->seen) em_ptr->obvious = TRUE;

	if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_DEMON);

	em_ptr->note = _("は身震いした。", " shudders.");
	em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
	return PROCESS_CONTINUE;
}


process_result effect_monster_disp_all(effect_monster_type *em_ptr)
{
	if (em_ptr->seen) em_ptr->obvious = TRUE;

	em_ptr->note = _("は身震いした。", " shudders.");
	em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
	return PROCESS_CONTINUE;
}
