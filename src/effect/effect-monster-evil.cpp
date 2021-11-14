#include "effect/effect-monster-evil.h"
#include "effect/effect-monster-util.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-info.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"

static bool effect_monster_away_resist(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flagsr & RFR_RES_TELE) == 0) return false;

	if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (em_ptr->r_ptr->flagsr & RFR_RES_ALL))
	{
		if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
		em_ptr->note = _("には効果がなかった。", " is unaffected.");
		return true;
	}

	if (em_ptr->r_ptr->level > randint1(100))
	{
		if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
		em_ptr->note = _("には耐性がある！", " resists!");
		return true;
	}

	return false;
}


process_result effect_monster_away_undead(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flags3 & (RF3_UNDEAD)) == 0)
	{
		em_ptr->skipped = true;
		em_ptr->dam = 0;
		return PROCESS_CONTINUE;
	}

	bool resists_tele = effect_monster_away_resist(player_ptr, em_ptr);
	if (!resists_tele)
	{
		if (em_ptr->seen) em_ptr->obvious = true;
		if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
			em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);

		em_ptr->do_dist = em_ptr->dam;
	}

	em_ptr->dam = 0;
	return PROCESS_CONTINUE;
}


process_result effect_monster_away_evil(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flags3 & (RF3_EVIL)) == 0)
	{
		em_ptr->skipped = true;
		em_ptr->dam = 0;
		return PROCESS_CONTINUE;
	}

	bool resists_tele = effect_monster_away_resist(player_ptr, em_ptr);
	if (!resists_tele)
	{
		if (em_ptr->seen) em_ptr->obvious = true;
		if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
			em_ptr->r_ptr->r_flags3 |= (RF3_EVIL);

		em_ptr->do_dist = em_ptr->dam;
	}

	em_ptr->dam = 0;
	return PROCESS_CONTINUE;
}


process_result effect_monster_away_all(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
	bool resists_tele = effect_monster_away_resist(player_ptr, em_ptr);
	if (!resists_tele)
	{
		if (em_ptr->seen) em_ptr->obvious = true;

		em_ptr->do_dist = em_ptr->dam;
	}

	em_ptr->dam = 0;
	return PROCESS_CONTINUE;
}


process_result effect_monster_turn_undead(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flags3 & (RF3_UNDEAD)) == 0)
	{
		em_ptr->skipped = true;
		em_ptr->dam = 0;
		return PROCESS_CONTINUE;
	}

	if (em_ptr->seen) em_ptr->obvious = true;

	if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);

	em_ptr->do_fear = damroll(3, (em_ptr->dam / 2)) + 1;
	if (em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10)
	{
		em_ptr->note = _("には効果がなかった。", " is unaffected.");
		em_ptr->obvious = false;
		em_ptr->do_fear = 0;
	}

	em_ptr->dam = 0;
	return PROCESS_CONTINUE;
}


process_result effect_monster_turn_evil(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flags3 & (RF3_EVIL)) == 0)
	{
		em_ptr->skipped = true;
		em_ptr->dam = 0;
		return PROCESS_CONTINUE;
	}

	if (em_ptr->seen) em_ptr->obvious = true;

	if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_EVIL);

	em_ptr->do_fear = damroll(3, (em_ptr->dam / 2)) + 1;
	if (em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10)
	{
		em_ptr->note = _("には効果がなかった。", " is unaffected.");
		em_ptr->obvious = false;
		em_ptr->do_fear = 0;
	}

	em_ptr->dam = 0;
	return PROCESS_CONTINUE;
}


process_result effect_monster_turn_all(effect_monster_type *em_ptr)
{
	if (em_ptr->seen) em_ptr->obvious = true;

	em_ptr->do_fear = damroll(3, (em_ptr->dam / 2)) + 1;
	if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
		(em_ptr->r_ptr->flags3 & (RF3_NO_FEAR)) ||
		(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
	{
		em_ptr->note = _("には効果がなかった。", " is unaffected.");
		em_ptr->obvious = false;
		em_ptr->do_fear = 0;
	}

	em_ptr->dam = 0;
	return PROCESS_CONTINUE;
}


process_result effect_monster_disp_undead(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flags3 & (RF3_UNDEAD)) == 0)
	{
		em_ptr->skipped = true;
		em_ptr->dam = 0;
		return PROCESS_CONTINUE;
	}

	if (em_ptr->seen) em_ptr->obvious = true;

	if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
		em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);

	em_ptr->note = _("は身震いした。", " shudders.");
	em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
	return PROCESS_CONTINUE;
}


process_result effect_monster_disp_evil(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flags3 & (RF3_EVIL)) == 0)
	{
		em_ptr->skipped = true;
		em_ptr->dam = 0;
		return PROCESS_CONTINUE;
	}

	if (em_ptr->seen) em_ptr->obvious = true;

	if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_EVIL);

	em_ptr->note = _("は身震いした。", " shudders.");
	em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
	return PROCESS_CONTINUE;
}


process_result effect_monster_disp_good(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flags3 & (RF3_GOOD)) == 0)
	{
		em_ptr->skipped = true;
		em_ptr->dam = 0;
		return PROCESS_CONTINUE;
	}

	if (em_ptr->seen) em_ptr->obvious = true;

	if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_GOOD);

	em_ptr->note = _("は身震いした。", " shudders.");
	em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
	return PROCESS_CONTINUE;
}


process_result effect_monster_disp_living(effect_monster_type *em_ptr)
{
	if (!monster_living(em_ptr->m_ptr->r_idx))
	{
		em_ptr->skipped = true;
		em_ptr->dam = 0;
		return PROCESS_CONTINUE;
	}

	if (em_ptr->seen) em_ptr->obvious = true;

	em_ptr->note = _("は身震いした。", " shudders.");
	em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
	return PROCESS_CONTINUE;
}


process_result effect_monster_disp_demon(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
	if ((em_ptr->r_ptr->flags3 & (RF3_DEMON)) == 0)
	{
		em_ptr->skipped = true;
		em_ptr->dam = 0;
		return PROCESS_CONTINUE;
	}

	if (em_ptr->seen) em_ptr->obvious = true;

	if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_DEMON);

	em_ptr->note = _("は身震いした。", " shudders.");
	em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
	return PROCESS_CONTINUE;
}


process_result effect_monster_disp_all(effect_monster_type *em_ptr)
{
	if (em_ptr->seen) em_ptr->obvious = true;

	em_ptr->note = _("は身震いした。", " shudders.");
	em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
	return PROCESS_CONTINUE;
}
