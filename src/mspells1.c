/* File: mspells1.c */

/* Purpose: Monster spells (attack player) */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"


#ifdef DRS_SMART_OPTIONS

/*
 * And now for Intelligent monster attacks (including spells).
 *
 * Original idea and code by "DRS" (David Reeves Sward).
 * Major modifications by "BEN" (Ben Harrison).
 *
 * Give monsters more intelligent attack/spell selection based on
 * observations of previous attacks on the player, and/or by allowing
 * the monster to "cheat" and know the player status.
 *
 * Maintain an idea of the player status, and use that information
 * to occasionally eliminate "ineffective" spell attacks.  We could
 * also eliminate ineffective normal attacks, but there is no reason
 * for the monster to do this, since he gains no benefit.
 * Note that MINDLESS monsters are not allowed to use this code.
 * And non-INTELLIGENT monsters only use it partially effectively.
 *
 * Actually learn what the player resists, and use that information
 * to remove attacks or spells before using them.  This will require
 * much less space, if I am not mistaken.  Thus, each monster gets a
 * set of 32 bit flags, "smart", build from the various "SM_*" flags.
 *
 * This has the added advantage that attacks and spells are related.
 * The "smart_learn" option means that the monster "learns" the flags
 * that should be set, and "smart_cheat" means that he "knows" them.
 * So "smart_cheat" means that the "smart" field is always up to date,
 * while "smart_learn" means that the "smart" field is slowly learned.
 * Both of them have the same effect on the "choose spell" routine.
 */



/*
 * Internal probability routine
 */
static bool int_outof(monster_race *r_ptr, int prob)
{
	/* Non-Smart monsters are half as "smart" */
	if (!(r_ptr->flags2 & RF2_SMART)) prob = prob / 2;

	/* Roll the dice */
	return (rand_int(100) < prob);
}



/*
 * Remove the "bad" spells from a spell list
 */
static void remove_bad_spells(int m_idx, u32b *f4p, u32b *f5p, u32b *f6p)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	u32b f4 = (*f4p);
	u32b f5 = (*f5p);
	u32b f6 = (*f6p);

	u32b smart = 0L;


	/* Too stupid to know anything */
	if (r_ptr->flags2 & RF2_STUPID) return;


	/* Must be cheating or learning */
	if (!smart_cheat && !smart_learn) return;


	/* Update acquired knowledge */
	if (smart_learn)
	{
		/* Hack -- Occasionally forget player status */
		if (m_ptr->smart && (rand_int(100) < 1)) m_ptr->smart = 0L;

		/* Use the memorized flags */
		smart = m_ptr->smart;
	}


	/* Cheat if requested */
	if (smart_cheat)
	{
		/* Know basic info */
		if (p_ptr->resist_acid) smart |= (SM_RES_ACID);
		if (p_ptr->oppose_acid) smart |= (SM_OPP_ACID);
		if (p_ptr->immune_acid) smart |= (SM_IMM_ACID);
		if (p_ptr->resist_elec) smart |= (SM_RES_ELEC);
		if (p_ptr->oppose_elec) smart |= (SM_OPP_ELEC);
		if (p_ptr->immune_elec) smart |= (SM_IMM_ELEC);
		if (p_ptr->resist_fire) smart |= (SM_RES_FIRE);
		if (p_ptr->oppose_fire) smart |= (SM_OPP_FIRE);
		if (p_ptr->immune_fire) smart |= (SM_IMM_FIRE);
		if (p_ptr->resist_cold) smart |= (SM_RES_COLD);
		if (p_ptr->oppose_cold) smart |= (SM_OPP_COLD);
		if (p_ptr->immune_cold) smart |= (SM_IMM_COLD);

		/* Know poison info */
		if (p_ptr->resist_pois) smart |= (SM_RES_POIS);
		if (p_ptr->oppose_pois) smart |= (SM_OPP_POIS);

		/* Know special resistances */
		if (p_ptr->resist_neth) smart |= (SM_RES_NETH);
		if (p_ptr->resist_lite) smart |= (SM_RES_LITE);
		if (p_ptr->resist_dark) smart |= (SM_RES_DARK);
		if (p_ptr->resist_fear) smart |= (SM_RES_FEAR);
		if (p_ptr->resist_conf) smart |= (SM_RES_CONF);
		if (p_ptr->resist_chaos) smart |= (SM_RES_CHAOS);
		if (p_ptr->resist_disen) smart |= (SM_RES_DISEN);
		if (p_ptr->resist_blind) smart |= (SM_RES_BLIND);
		if (p_ptr->resist_nexus) smart |= (SM_RES_NEXUS);
		if (p_ptr->resist_sound) smart |= (SM_RES_SOUND);
		if (p_ptr->resist_shard) smart |= (SM_RES_SHARD);
		if (p_ptr->reflect) smart |= (SM_IMM_REFLECT);

		/* Know bizarre "resistances" */
		if (p_ptr->free_act) smart |= (SM_IMM_FREE);
		if (!p_ptr->msp) smart |= (SM_IMM_MANA);
	}


	/* Nothing known */
	if (!smart) return;


	if (smart & SM_IMM_ACID)
	{
		if (int_outof(r_ptr, 200)) f4 &= ~(RF4_BR_ACID);
		if (int_outof(r_ptr, 200)) f5 &= ~(RF5_BA_ACID);
		if (int_outof(r_ptr, 200)) f5 &= ~(RF5_BO_ACID);
	}
	else if ((smart & (SM_OPP_ACID)) && (smart & (SM_RES_ACID)))
	{
		if (int_outof(r_ptr, 80)) f4 &= ~(RF4_BR_ACID);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BA_ACID);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BO_ACID);
	}
	else if ((smart & (SM_OPP_ACID)) || (smart & (SM_RES_ACID)))
	{
		if (int_outof(r_ptr, 30)) f4 &= ~(RF4_BR_ACID);
		if (int_outof(r_ptr, 30)) f5 &= ~(RF5_BA_ACID);
		if (int_outof(r_ptr, 30)) f5 &= ~(RF5_BO_ACID);
	}


	if (smart & (SM_IMM_ELEC))
	{
		if (int_outof(r_ptr, 200)) f4 &= ~(RF4_BR_ELEC);
		if (int_outof(r_ptr, 200)) f5 &= ~(RF5_BA_ELEC);
		if (int_outof(r_ptr, 200)) f5 &= ~(RF5_BO_ELEC);
	}
	else if ((smart & (SM_OPP_ELEC)) && (smart & (SM_RES_ELEC)))
	{
		if (int_outof(r_ptr, 80)) f4 &= ~(RF4_BR_ELEC);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BA_ELEC);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BO_ELEC);
	}
	else if ((smart & (SM_OPP_ELEC)) || (smart & (SM_RES_ELEC)))
	{
		if (int_outof(r_ptr, 30)) f4 &= ~(RF4_BR_ELEC);
		if (int_outof(r_ptr, 30)) f5 &= ~(RF5_BA_ELEC);
		if (int_outof(r_ptr, 30)) f5 &= ~(RF5_BO_ELEC);
	}


	if (smart & (SM_IMM_FIRE))
	{
		if (int_outof(r_ptr, 200)) f4 &= ~(RF4_BR_FIRE);
		if (int_outof(r_ptr, 200)) f5 &= ~(RF5_BA_FIRE);
		if (int_outof(r_ptr, 200)) f5 &= ~(RF5_BO_FIRE);
	}
	else if ((smart & (SM_OPP_FIRE)) && (smart & (SM_RES_FIRE)))
	{
		if (int_outof(r_ptr, 80)) f4 &= ~(RF4_BR_FIRE);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BA_FIRE);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BO_FIRE);
	}
	else if ((smart & (SM_OPP_FIRE)) || (smart & (SM_RES_FIRE)))
	{
		if (int_outof(r_ptr, 30)) f4 &= ~(RF4_BR_FIRE);
		if (int_outof(r_ptr, 30)) f5 &= ~(RF5_BA_FIRE);
		if (int_outof(r_ptr, 30)) f5 &= ~(RF5_BO_FIRE);
	}


	if (smart & (SM_IMM_COLD))
	{
		if (int_outof(r_ptr, 200)) f4 &= ~(RF4_BR_COLD);
		if (int_outof(r_ptr, 200)) f5 &= ~(RF5_BA_COLD);
		if (int_outof(r_ptr, 200)) f5 &= ~(RF5_BO_COLD);
		if (int_outof(r_ptr, 200)) f5 &= ~(RF5_BO_ICEE);
	}
	else if ((smart & (SM_OPP_COLD)) && (smart & (SM_RES_COLD)))
	{
		if (int_outof(r_ptr, 80)) f4 &= ~(RF4_BR_COLD);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BA_COLD);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BO_COLD);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BO_ICEE);
	}
	else if ((smart & (SM_OPP_COLD)) || (smart & (SM_RES_COLD)))
	{
		if (int_outof(r_ptr, 30)) f4 &= ~(RF4_BR_COLD);
		if (int_outof(r_ptr, 30)) f5 &= ~(RF5_BA_COLD);
		if (int_outof(r_ptr, 30)) f5 &= ~(RF5_BO_COLD);
		if (int_outof(r_ptr, 20)) f5 &= ~(RF5_BO_ICEE);
	}


	if ((smart & (SM_OPP_POIS)) && (smart & (SM_RES_POIS)))
	{
		if (int_outof(r_ptr, 80)) f4 &= ~(RF4_BR_POIS);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BA_POIS);
		if (int_outof(r_ptr, 60)) f4 &= ~(RF4_BA_NUKE);
		if (int_outof(r_ptr, 60)) f4 &= ~(RF4_BR_NUKE);
	}
	else if ((smart & (SM_OPP_POIS)) || (smart & (SM_RES_POIS)))
	{
		if (int_outof(r_ptr, 30)) f4 &= ~(RF4_BR_POIS);
		if (int_outof(r_ptr, 30)) f5 &= ~(RF5_BA_POIS);
	}


	if (smart & (SM_RES_NETH))
	{
		if (int_outof(r_ptr, 20)) f4 &= ~(RF4_BR_NETH);
		if (int_outof(r_ptr, 50)) f5 &= ~(RF5_BA_NETH);
		if (int_outof(r_ptr, 50)) f5 &= ~(RF5_BO_NETH);
	}

	if (smart & (SM_RES_LITE))
	{
		if (int_outof(r_ptr, 50)) f4 &= ~(RF4_BR_LITE);
		if (int_outof(r_ptr, 50)) f5 &= ~(RF5_BA_LITE);
	}

	if (smart & (SM_RES_DARK))
	{
		if (int_outof(r_ptr, 50)) f4 &= ~(RF4_BR_DARK);
		if (int_outof(r_ptr, 50)) f5 &= ~(RF5_BA_DARK);
	}

	if (smart & (SM_RES_FEAR))
	{
		if (int_outof(r_ptr, 200)) f5 &= ~(RF5_SCARE);
	}

	if (smart & (SM_RES_CONF))
	{
		if (int_outof(r_ptr, 200)) f5 &= ~(RF5_CONF);
		if (int_outof(r_ptr, 50)) f4 &= ~(RF4_BR_CONF);
	}

	if (smart & (SM_RES_CHAOS))
	{
		if (int_outof(r_ptr, 20)) f4 &= ~(RF4_BR_CHAO);
		if (int_outof(r_ptr, 50)) f4 &= ~(RF4_BA_CHAO);
	}

	if (smart & (SM_RES_DISEN))
	{
		if (int_outof(r_ptr, 40)) f4 &= ~(RF4_BR_DISE);
	}

	if (smart & (SM_RES_BLIND))
	{
		if (int_outof(r_ptr, 200)) f5 &= ~(RF5_BLIND);
	}

	if (smart & (SM_RES_NEXUS))
	{
		if (int_outof(r_ptr, 50)) f4 &= ~(RF4_BR_NEXU);
		if (int_outof(r_ptr, 200)) f6 &= ~(RF6_TELE_LEVEL);
	}

	if (smart & (SM_RES_SOUND))
	{
		if (int_outof(r_ptr, 50)) f4 &= ~(RF4_BR_SOUN);
	}

	if (smart & (SM_RES_SHARD))
	{
		if (int_outof(r_ptr, 40)) f4 &= ~(RF4_BR_SHAR);
	}

	if (smart & (SM_IMM_REFLECT))
	{
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_BO_COLD);
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_BO_FIRE);
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_BO_ACID);
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_BO_ELEC);
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_BO_NETH);
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_BO_WATE);
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_BO_MANA);
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_BO_PLAS);
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_BO_ICEE);
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_MISSILE);
		if (int_outof(r_ptr, 150)) f4 &= ~(RF4_ARROW_1);
		if (int_outof(r_ptr, 150)) f4 &= ~(RF4_ARROW_2);
		if (int_outof(r_ptr, 150)) f4 &= ~(RF4_ARROW_3);
		if (int_outof(r_ptr, 150)) f4 &= ~(RF4_ARROW_4);
	}

	if (smart & (SM_IMM_FREE))
	{
		if (int_outof(r_ptr, 200)) f5 &= ~(RF5_HOLD);
		if (int_outof(r_ptr, 200)) f5 &= ~(RF5_SLOW);
	}

	if (smart & (SM_IMM_MANA))
	{
		if (int_outof(r_ptr, 200)) f5 &= ~(RF5_DRAIN_MANA);
	}

	/* XXX XXX XXX No spells left? */
	/* if (!f4 && !f5 && !f6) ... */

	(*f4p) = f4;
	(*f5p) = f5;
	(*f6p) = f6;
}

#endif /* DRS_SMART_OPTIONS */


/*
 * Determine if there is a space near the player in which
 * a summoned creature can appear
 */
bool summon_possible(int y1, int x1)
{
	int y, x;

	/* Start at the player's location, and check 2 grids in each dir */
	for (y = y1 - 2; y <= y1 + 2; y++)
	{
		for (x = x1 - 2; x <= x1 + 2; x++)
		{
			/* Ignore illegal locations */
			if (!in_bounds(y, x)) continue;

			/* Only check a circular area */
			if (distance(y1, x1, y, x)>2) continue;

#if 0
			/* Hack: no summon on glyph of warding */
			if (cave[y][x].feat == FEAT_GLYPH) continue;
			if (cave[y][x].feat == FEAT_MINOR_GLYPH) continue;
#endif

			/* ...nor on the Pattern */
			if ((cave[y][x].feat >= FEAT_PATTERN_START)
				&& (cave[y][x].feat <= FEAT_PATTERN_XTRA2)) continue;

			/* Require empty floor grid in line of sight */
			if ((cave_empty_bold(y, x) || (cave[y][x].feat == FEAT_TREES)) && los(y1, x1, y, x) && los(y, x, y1, x1)) return (TRUE);
		}
	}

	return FALSE;
}


static bool raise_possible(int y, int x)
{
	int xx, yy;
	s16b this_o_idx, next_o_idx = 0;
	cave_type *c_ptr;

	for (xx = x - 5; xx <= x + 5; xx++)
	{
		for (yy = y - 5; yy <= y + 5; yy++)
		{
			if (distance(y, x, yy, xx) > 5) continue;
			if (!los(y, x, yy, xx)) continue;

			c_ptr = &cave[yy][xx];
			/* Scan the pile of objects */
			for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
			{
				/* Acquire object */
				object_type *o_ptr = &o_list[this_o_idx];

				/* Acquire next object */
				next_o_idx = o_ptr->next_o_idx;

				/* Known to be worthless? */
				if (o_ptr->tval == TV_CORPSE)
					return TRUE;
			}
		}
	}
	return FALSE;
}


/*
 * Originally, it was possible for a friendly to shoot another friendly.
 * Change it so a "clean shot" means no equally friendly monster is
 * between the attacker and target.
 */
/*
 * Determine if a bolt spell will hit the player.
 *
 * This is exactly like "projectable", but it will
 * return FALSE if a monster is in the way.
 * no equally friendly monster is
 * between the attacker and target.
 */
bool clean_shot(int y1, int x1, int y2, int x2, bool friend)
{
	/* Must be the same as projectable() */

	int i, y, x;

	int grid_n = 0;
	u16b grid_g[512];

	/* Check the projection path */
	grid_n = project_path(grid_g, MAX_RANGE, y1, x1, y2, x2, 0);

	/* No grid is ever projectable from itself */
	if (!grid_n) return (FALSE);

	/* Final grid */
	y = GRID_Y(grid_g[grid_n-1]);
	x = GRID_X(grid_g[grid_n-1]);

	/* May not end in an unrequested grid */
	if ((y != y2) || (x != x2)) return (FALSE);

	for (i = 0; i < grid_n; i++)
	{
		y = GRID_Y(grid_g[i]);
		x = GRID_X(grid_g[i]);

		if ((cave[y][x].m_idx > 0) && !((y == y2) && (x == x2)))
		{
			monster_type *m_ptr = &m_list[cave[y][x].m_idx];
			if (friend == is_pet(m_ptr))
			{
				return (FALSE);
			}
		}
		/* Pets may not shoot through the character - TNB */
		if ((y == py) && (x == px))
		{
			if (friend) return (FALSE);
		}
	}

	return (TRUE);
}

/*
 * Cast a bolt at the player
 * Stop if we hit a monster
 * Affect monsters and the player
 */
static void bolt(int m_idx, int typ, int dam_hp, int monspell, bool learnable)
{
	int flg = PROJECT_STOP | PROJECT_KILL | PROJECT_PLAYER;

	/* Target the player with a bolt attack */
	(void)project(m_idx, 0, py, px, dam_hp, typ, flg, (learnable ? monspell : -1));
}

static void beam(int m_idx, int typ, int dam_hp, int monspell, bool learnable)
{
	int flg = PROJECT_BEAM | PROJECT_KILL | PROJECT_THRU | PROJECT_PLAYER | PROJECT_NO_REF;

	/* Target the player with a bolt attack */
	(void)project(m_idx, 0, py, px, dam_hp, typ, flg, (learnable ? monspell : -1));
}


/*
 * Cast a breath (or ball) attack at the player
 * Pass over any monsters that may be in the way
 * Affect grids, objects, monsters, and the player
 */
static void breath(int y, int x, int m_idx, int typ, int dam_hp, int rad, bool breath, int monspell, bool learnable)
{
	int flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_PLAYER;

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Determine the radius of the blast */
	if ((rad < 1) && breath) rad = (r_ptr->flags2 & (RF2_POWERFUL)) ? 3 : 2;

	/* Handle breath attacks */
	if (breath) rad = 0 - rad;

	if (typ == GF_ROCKET) flg |= PROJECT_STOP;

	/* Target the player with a ball attack */
	(void)project(m_idx, rad, y, x, dam_hp, typ, flg, (learnable ? monspell : -1));
}


void curse_equipment(int chance, int heavy_chance)
{
	bool        changed = FALSE;
	u32b        o1, o2, o3;
	object_type *o_ptr = &inventory[INVEN_RARM + rand_int(12)];

	if (randint(100) > chance) return;

	if (!o_ptr->k_idx) return;

	object_flags(o_ptr, &o1, &o2, &o3);


	/* Extra, biased saving throw for blessed items */
	if ((o3 & TR3_BLESSED) && (randint(888) > chance))
	{
		char o_name[MAX_NLEN];
		object_desc(o_name, o_ptr, FALSE, 0);
#ifdef JP
msg_format("%sは呪いを跳ね返した！", o_name,
#else
		msg_format("Your %s resist%s cursing!", o_name,
#endif

			((o_ptr->number > 1) ? "" : "s"));
		/* Hmmm -- can we wear multiple items? If not, this is unnecessary */
		return;
	}

	if ((randint(100) <= heavy_chance) &&
		(o_ptr->name1 || o_ptr->name2 || o_ptr->art_name))
	{
		if (!(o3 & TR3_HEAVY_CURSE))
			changed = TRUE;
		o_ptr->art_flags3 |= TR3_HEAVY_CURSE;
		o_ptr->art_flags3 |= TR3_CURSED;
		o_ptr->ident |= IDENT_CURSED;
	}
	else
	{
		if (!(o_ptr->ident & IDENT_CURSED))
			changed = TRUE;
		o_ptr->art_flags3 |= TR3_CURSED;
		o_ptr->ident |= IDENT_CURSED;
	}

	if (changed)
	{
#ifdef JP
msg_print("悪意に満ちた黒いオーラがあなたをとりまいた...");
#else
		msg_print("There is a malignant black aura surrounding you...");
#endif

		o_ptr->feeling = FEEL_NONE;
	}
}


/*
 * Return TRUE if a spell is good for hurting the player (directly).
 */
static bool spell_attack(byte spell)
{
	/* All RF4 spells hurt (except for shriek and dispel) */
	if (spell < 128 && spell > 98) return (TRUE);

	/* Various "ball" spells */
	if (spell >= 128 && spell <= 128 + 8) return (TRUE);

	/* "Cause wounds" and "bolt" spells */
	if (spell >= 128 + 12 && spell < 128 + 27) return (TRUE);

	/* Hand of Doom */
	if (spell == 160 + 1) return (TRUE);

	/* Psycho-Spear */
	if (spell == 160 + 11) return (TRUE);

	/* Doesn't hurt */
	return (FALSE);
}


/*
 * Return TRUE if a spell is good for escaping.
 */
static bool spell_escape(byte spell)
{
	/* Blink or Teleport */
	if (spell == 160 + 4 || spell == 160 + 5) return (TRUE);

	/* Teleport the player away */
	if (spell == 160 + 9 || spell == 160 + 10) return (TRUE);

	/* Isn't good for escaping */
	return (FALSE);
}

/*
 * Return TRUE if a spell is good for annoying the player.
 */
static bool spell_annoy(byte spell)
{
	/* Shriek */
	if (spell == 96 + 0) return (TRUE);

	/* Brain smash, et al (added curses) */
	if (spell >= 128 + 9 && spell <= 128 + 14) return (TRUE);

	/* Scare, confuse, blind, slow, paralyze */
	if (spell >= 128 + 27 && spell <= 128 + 31) return (TRUE);

	/* Teleport to */
	if (spell == 160 + 8) return (TRUE);

	/* Teleport level */
	if (spell == 160 + 10) return (TRUE);

	/* Darkness, make traps, cause amnesia */
	if (spell >= 160 + 12 && spell <= 160 + 14) return (TRUE);

	/* Doesn't annoy */
	return (FALSE);
}

/*
 * Return TRUE if a spell summons help.
 */
static bool spell_summon(byte spell)
{
	/* All summon spells */
	if (spell >= 160 + 16) return (TRUE);

	/* Doesn't summon */
	return (FALSE);
}


/*
 * Return TRUE if a spell raise-dead.
 */
static bool spell_raise(byte spell)
{
	/* All raise-dead spells */
	if (spell == 160 + 15) return (TRUE);

	/* Doesn't summon */
	return (FALSE);
}


/*
 * Return TRUE if a spell is good in a tactical situation.
 */
static bool spell_tactic(byte spell)
{
	/* Blink */
	if (spell == 160 + 4) return (TRUE);

	/* Not good */
	return (FALSE);
}

/*
 * Return TRUE if a spell makes invulnerable.
 */
static bool spell_invulner(byte spell)
{
	/* Invulnerability */
	if (spell == 160 + 3) return (TRUE);

	/* No invulnerability */
	return (FALSE);
}

/*
 * Return TRUE if a spell hastes.
 */
static bool spell_haste(byte spell)
{
	/* Haste self */
	if (spell == 160 + 0) return (TRUE);

	/* Not a haste spell */
	return (FALSE);
}


/*
 * Return TRUE if a spell world.
 */
static bool spell_world(byte spell)
{
	/* world */
	if (spell == 160 + 6) return (TRUE);

	/* Not a haste spell */
	return (FALSE);
}


/*
 * Return TRUE if a spell special.
 */
static bool spell_special(byte spell)
{
	if (p_ptr->inside_battle) return FALSE;

	/* world */
	if (spell == 160 + 7) return (TRUE);

	/* Not a haste spell */
	return (FALSE);
}


/*
 * Return TRUE if a spell psycho-spear.
 */
static bool spell_psy_spe(byte spell)
{
	/* world */
	if (spell == 160 + 11) return (TRUE);

	/* Not a haste spell */
	return (FALSE);
}


/*
 * Return TRUE if a spell is good for healing.
 */
static bool spell_heal(byte spell)
{
	/* Heal */
	if (spell == 160 + 2) return (TRUE);

	/* No healing */
	return (FALSE);
}


/*
 * Return TRUE if a spell is good for dispel.
 */
static bool spell_dispel(byte spell)
{
	/* Dispel */
	if (spell == 96 + 2) return (TRUE);

	/* No dispel */
	return (FALSE);
}


/*
 * Check should monster cast dispel spell.
 */
static bool dispel_check(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Invulnabilty */
	if (p_ptr->invuln) return (TRUE);

	/* Wraith form */
	if (p_ptr->wraith_form) return (TRUE);

	/* Shield */
	if (p_ptr->shield) return (TRUE);

	/* Magic defence */
	if (p_ptr->magicdef) return (TRUE);

	/* Multi Shadow */
	if (p_ptr->multishadow) return (TRUE);

	/* Robe of dust */
	if (p_ptr->dustrobe) return (TRUE);

	/* Berserk Strength */
	if (p_ptr->shero && (p_ptr->pclass != CLASS_BERSERKER)) return (TRUE);

	/* Invulnability song */
	if (music_singing(MUSIC_INVULN)) return (TRUE);

	/* Demon Lord */
	if (p_ptr->mimic_form == MIMIC_DEMON_LORD) return (TRUE);

	/* Elemental resistances */
	if (r_ptr->flags4 & RF4_BR_ACID)
	{
		if (!p_ptr->immune_acid && p_ptr->oppose_acid) return (TRUE);

		if (p_ptr->special_defense & DEFENSE_ACID) return (TRUE);
	}

	if (r_ptr->flags4 & RF4_BR_FIRE)
	{
		if (!((p_ptr->prace == RACE_DEMON) && p_ptr->lev > 44))
		{
			if(!p_ptr->immune_fire && p_ptr->oppose_fire) return (TRUE);

			if(p_ptr->special_defense & DEFENSE_FIRE) return(TRUE);
		}
	}

	if (r_ptr->flags4 & RF4_BR_ELEC)
	{
		if (!p_ptr->immune_elec && p_ptr->oppose_elec) return (TRUE);

		if (p_ptr->special_defense & DEFENSE_ELEC) return (TRUE);
	}

	if (r_ptr->flags4 & RF4_BR_COLD)
	{
		if (!p_ptr->immune_cold && p_ptr->oppose_cold) return (TRUE);

		if (p_ptr->special_defense & DEFENSE_COLD) return (TRUE);
	}

	if (r_ptr->flags4 & (RF4_BR_POIS | RF4_BR_NUKE))
	{
		if (!((p_ptr->pclass == CLASS_NINJA) && p_ptr->lev > 44))
		{
			if (p_ptr->oppose_pois) return (TRUE);

			if (p_ptr->special_defense & DEFENSE_POIS) return (TRUE);
		}
	}

	/* Elemental resist music */
	if (music_singing(MUSIC_RESIST))
	{
		if (r_ptr->flags4 & (RF4_BR_ACID | RF4_BR_FIRE | RF4_BR_ELEC | RF4_BR_COLD | RF4_BR_POIS)) return (TRUE);
	}

	/* Ultimate resistance */
	if (p_ptr->ult_res) return (TRUE);

	/* Potion of Neo Tsuyosi special */
	if (p_ptr->tsuyoshi) return (TRUE);

	/* Elemental Brands */
	if ((p_ptr->special_attack & ATTACK_ACID) && !(r_ptr->flags3 & RF3_IM_ACID)) return (TRUE);
	if ((p_ptr->special_attack & ATTACK_FIRE) && !(r_ptr->flags3 & RF3_IM_FIRE)) return (TRUE);
	if ((p_ptr->special_attack & ATTACK_ELEC) && !(r_ptr->flags3 & RF3_IM_ELEC)) return (TRUE);
	if ((p_ptr->special_attack & ATTACK_COLD) && !(r_ptr->flags3 & RF3_IM_COLD)) return (TRUE);
	if ((p_ptr->special_attack & ATTACK_POIS) && !(r_ptr->flags3 & RF3_IM_POIS)) return (TRUE);

	/* Speed */
	if (p_ptr->pspeed < 145)
	{
		if (p_ptr->fast) return (TRUE);

		if (music_singing(MUSIC_SPEED)) return (TRUE);

		if (music_singing(MUSIC_SHERO)) return (TRUE);
	}

	/* Light speed */
	if (p_ptr->lightspeed && (m_ptr->mspeed < 136)) return (TRUE);

	if (p_ptr->riding && (m_list[p_ptr->riding].mspeed < 135))
	{
		if (m_list[p_ptr->riding].fast) return (TRUE);
	}

	/* No need to cast dispel spell */
	return (FALSE);
}


/*
 * Have a monster choose a spell from a list of "useful" spells.
 *
 * Note that this list does NOT include spells that will just hit
 * other monsters, and the list is restricted when the monster is
 * "desperate".  Should that be the job of this function instead?
 *
 * Stupid monsters will just pick a spell randomly.  Smart monsters
 * will choose more "intelligently".
 *
 * Use the helper functions above to put spells into categories.
 *
 * This function may well be an efficiency bottleneck.
 */
static int choose_attack_spell(int m_idx, byte spells[], byte num)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	byte escape[96], escape_num = 0;
	byte attack[96], attack_num = 0;
	byte summon[96], summon_num = 0;
	byte tactic[96], tactic_num = 0;
	byte annoy[96], annoy_num = 0;
        byte invul[96], invul_num = 0;
	byte haste[96], haste_num = 0;
	byte world[96], world_num = 0;
	byte special[96], special_num = 0;
	byte psy_spe[96], psy_spe_num = 0;
	byte raise[96], raise_num = 0;
	byte heal[96], heal_num = 0;
	byte dispel[96], dispel_num = 0;

	int i;

	/* Stupid monsters choose randomly */
	if (r_ptr->flags2 & (RF2_STUPID))
	{
		/* Pick at random */
		return (spells[rand_int(num)]);
	}

	/* Categorize spells */
	for (i = 0; i < num; i++)
	{
		/* Escape spell? */
		if (spell_escape(spells[i])) escape[escape_num++] = spells[i];

		/* Attack spell? */
		if (spell_attack(spells[i])) attack[attack_num++] = spells[i];

		/* Summon spell? */
		if (spell_summon(spells[i])) summon[summon_num++] = spells[i];

		/* Tactical spell? */
		if (spell_tactic(spells[i])) tactic[tactic_num++] = spells[i];

		/* Annoyance spell? */
		if (spell_annoy(spells[i])) annoy[annoy_num++] = spells[i];

		/* Invulnerability spell? */
		if (spell_invulner(spells[i])) invul[invul_num++] = spells[i];

		/* Haste spell? */
		if (spell_haste(spells[i])) haste[haste_num++] = spells[i];

		/* World spell? */
		if (spell_world(spells[i])) world[world_num++] = spells[i];

		/* Special spell? */
		if (spell_special(spells[i])) special[special_num++] = spells[i];

		/* Psycho-spear spell? */
		if (spell_psy_spe(spells[i])) psy_spe[psy_spe_num++] = spells[i];

		/* Raise-dead spell? */
		if (spell_raise(spells[i])) raise[raise_num++] = spells[i];

		/* Heal spell? */
		if (spell_heal(spells[i])) heal[heal_num++] = spells[i];

		/* Dispel spell? */
		if (spell_dispel(spells[i])) dispel[dispel_num++] = spells[i];
	}

	/*** Try to pick an appropriate spell type ***/

	/* world */
	if (world_num && (rand_int(100) < 15) && !world_monster)
	{
		/* Choose haste spell */
		return (world[rand_int(world_num)]);
	}

	/* special */
	if (special_num)
	{
		bool success = FALSE;
		switch(m_ptr->r_idx)
		{
			case MON_BANOR:
			case MON_LUPART:
				if ((m_ptr->hp < m_ptr->maxhp / 2) && r_info[MON_BANOR].max_num && r_info[MON_LUPART].max_num) success = TRUE;
				break;
			default: break;
		}
		if (success) return (special[rand_int(special_num)]);
	}

	/* Still hurt badly, couldn't flee, attempt to heal */
	if (m_ptr->hp < m_ptr->maxhp / 3 && one_in_(2))
	{
		/* Choose heal spell if possible */
		if (heal_num) return (heal[rand_int(heal_num)]);
	}

	/* Hurt badly or afraid, attempt to flee */
	if (((m_ptr->hp < m_ptr->maxhp / 3) || m_ptr->monfear) && one_in_(2))
	{
		/* Choose escape spell if possible */
		if (escape_num) return (escape[rand_int(escape_num)]);
	}

	/* special */
	if (special_num)
	{
		bool success = FALSE;
		switch(m_ptr->r_idx)
		{
			case MON_OHMU:
				if (rand_int(100) < 50) success = TRUE;
				break;
			case MON_BANORLUPART:
				if (rand_int(100) < 70) success = TRUE;
				break;
			default: break;
		}
		if (success) return (special[rand_int(special_num)]);
	}

	/* Player is close and we have attack spells, blink away */
	if ((distance(py, px, m_ptr->fy, m_ptr->fx) < 4) && (attack_num || (r_ptr->flags6 & RF6_TRAPS)) && (rand_int(100) < 75) && !world_monster)
	{
		/* Choose tactical spell */
		if (tactic_num) return (tactic[rand_int(tactic_num)]);
	}

	/* Summon if possible (sometimes) */
	if (summon_num && (rand_int(100) < 40))
	{
		/* Choose summon spell */
		return (summon[rand_int(summon_num)]);
	}

	/* dispel */
	if (dispel_num && one_in_(2))
	{
		/* Choose dispel spell if possible */
		if (dispel_check(m_idx))
		{
			return (dispel[rand_int(dispel_num)]);
		}
	}

	/* Raise-dead if possible (sometimes) */
	if (raise_num && (rand_int(100) < 40) && raise_possible(m_ptr->fy, m_ptr->fx))
	{
		/* Choose raise-dead spell */
		return (raise[rand_int(raise_num)]);
	}

	/* Attack spell (most of the time) */
	if (p_ptr->invuln)
	{
		if (psy_spe_num && (rand_int(100) < 50))
		{
			/* Choose attack spell */
			return (psy_spe[rand_int(psy_spe_num)]);
		}
		else if (attack_num && (rand_int(100) < 40))
		{
			/* Choose attack spell */
			return (attack[rand_int(attack_num)]);
		}
	}
	else if (attack_num && (rand_int(100) < 85))
	{
		/* Choose attack spell */
		return (attack[rand_int(attack_num)]);
	}

	/* Try another tactical spell (sometimes) */
	if (tactic_num && (rand_int(100) < 50) && !world_monster)
	{
		/* Choose tactic spell */
		return (tactic[rand_int(tactic_num)]);
	}

	/* Cast globe of invulnerability if not already in effect */
	if (invul_num && !(m_ptr->invulner) && (rand_int(100) < 50))
	{
		/* Choose Globe of Invulnerability */
		return (invul[rand_int(invul_num)]);
	}

	/* We're hurt (not badly), try to heal */
	if ((m_ptr->hp < m_ptr->maxhp * 3 / 4) && (rand_int(100) < 25))
	{
		/* Choose heal spell if possible */
		if (heal_num) return (heal[rand_int(heal_num)]);
	}

	/* Haste self if we aren't already somewhat hasted (rarely) */
	if (haste_num && (rand_int(100) < 20) && !(m_ptr->fast))
	{
		/* Choose haste spell */
		return (haste[rand_int(haste_num)]);
	}

	/* Annoy player (most of the time) */
	if (annoy_num && (rand_int(100) < 80))
	{
		/* Choose annoyance spell */
		return (annoy[rand_int(annoy_num)]);
	}

	/* Choose no spell */
	return (0);
}


/*
 * Creatures can cast spells, shoot missiles, and breathe.
 *
 * Returns "TRUE" if a spell (or whatever) was (successfully) cast.
 *
 * XXX XXX XXX This function could use some work, but remember to
 * keep it as optimized as possible, while retaining generic code.
 *
 * Verify the various "blind-ness" checks in the code.
 *
 * XXX XXX XXX Note that several effects should really not be "seen"
 * if the player is blind.  See also "effects.c" for other "mistakes".
 *
 * Perhaps monsters should breathe at locations *near* the player,
 * since this would allow them to inflict "partial" damage.
 *
 * Perhaps smart monsters should decline to use "bolt" spells if
 * there is a monster in the way, unless they wish to kill it.
 *
 * Note that, to allow the use of the "track_target" option at some
 * later time, certain non-optimal things are done in the code below,
 * including explicit checks against the "direct" variable, which is
 * currently always true by the time it is checked, but which should
 * really be set according to an explicit "projectable()" test, and
 * the use of generic "x,y" locations instead of the player location,
 * with those values being initialized with the player location.
 *
 * It will not be possible to "correctly" handle the case in which a
 * monster attempts to attack a location which is thought to contain
 * the player, but which in fact is nowhere near the player, since this
 * might induce all sorts of messages about the attack itself, and about
 * the effects of the attack, which the player might or might not be in
 * a position to observe.  Thus, for simplicity, it is probably best to
 * only allow "faulty" attacks by a monster if one of the important grids
 * (probably the initial or final grid) is in fact in view of the player.
 * It may be necessary to actually prevent spell attacks except when the
 * monster actually has line of sight to the player.  Note that a monster
 * could be left in a bizarre situation after the player ducked behind a
 * pillar and then teleported away, for example.
 *
 * Note that certain spell attacks do not use the "project()" function
 * but "simulate" it via the "direct" variable, which is always at least
 * as restrictive as the "project()" function.  This is necessary to
 * prevent "blindness" attacks and such from bending around walls, etc,
 * and to allow the use of the "track_target" option in the future.
 *
 * Note that this function attempts to optimize the use of spells for the
 * cases in which the monster has no spells, or has spells but cannot use
 * them, or has spells but they will have no "useful" effect.  Note that
 * this function has been an efficiency bottleneck in the past.
 *
 * Note the special "MFLAG_NICE" flag, which prevents a monster from using
 * any spell attacks until the player has had a single chance to move.
 */
bool make_attack_spell(int m_idx)
{
	int             k, chance, thrown_spell = 0, rlev, failrate;
	byte            spell[96], num = 0;
	u32b            f4, f5, f6;
	monster_type    *m_ptr = &m_list[m_idx];
	monster_race    *r_ptr = &r_info[m_ptr->r_idx];
	char            m_name[80];
	char            m_poss[80];
	char            ddesc[80];
	bool            no_inate = FALSE;
	bool            do_disi = FALSE;
	int             dam = 0;
	int s_num_6 = (easy_band ? 2 : 6);
	int s_num_4 = (easy_band ? 1 : 4);

	/* Target location */
	int x = px;
	int y = py;

	/* Summon count */
	int count = 0;

	/* Extract the blind-ness */
	bool blind = (p_ptr->blind ? TRUE : FALSE);

	/* Extract the "see-able-ness" */
	bool seen = (!blind && m_ptr->ml);

	bool maneable = player_has_los_bold(m_ptr->fy, m_ptr->fx);
	bool learnable = (seen && maneable && !world_monster);

	/* Assume "normal" target */
	bool normal = TRUE;

	/* Assume "projectable" */
	bool direct = TRUE;

	/* Cannot cast spells when confused */
	if (m_ptr->confused)
	{
		m_ptr->target_y = 0;
		m_ptr->target_x = 0;
		return (FALSE);
	}

	/* Cannot cast spells when nice */
	if (m_ptr->mflag & MFLAG_NICE) return (FALSE);
	if (!is_hostile(m_ptr)) return (FALSE);

	/* Hack -- Extract the spell probability */
	chance = (r_ptr->freq_inate + r_ptr->freq_spell) / 2;

	/* Not allowed to cast spells */
	if (!chance) return (FALSE);


	if (stupid_monsters)
	{
		/* Only do spells occasionally */
		if (rand_int(100) >= chance) return (FALSE);
	}
	else
	{
		if (rand_int(100) >=  chance) return (FALSE);

		/* Sometimes forbid inate attacks (breaths) */
		if (rand_int(100) >= (chance * 2)) no_inate = TRUE;
	}

	/* XXX XXX XXX Handle "track_target" option (?) */


	/* Extract the racial spell flags */
	f4 = r_ptr->flags4;
	f5 = r_ptr->flags5;
	f6 = r_ptr->flags6;

	/* Hack -- require projectable player */
	if (normal)
	{
		/* Check range */
		if ((m_ptr->cdis > MAX_RANGE) && !m_ptr->target_y) return (FALSE);

		/* Check path */
		if (projectable(m_ptr->fy, m_ptr->fx, y, x))
		{
			/* Breath disintegration to the glyph */
			if ((!cave_floor_bold(y,x)) && (r_ptr->flags4 & RF4_BR_DISI) && one_in_(2)) do_disi = TRUE;
		}

		/* Check path to next grid */
		else
		{
			bool success = FALSE;

			if ((r_ptr->flags4 & RF4_BR_DISI) &&
			    (m_ptr->cdis < MAX_RANGE/2) &&
			    in_disintegration_range(m_ptr->fy, m_ptr->fx, y, x) &&
			    (one_in_(10) || (projectable(y, x, m_ptr->fy, m_ptr->fx) && one_in_(2))))
			{
				do_disi = TRUE;
				success = TRUE;
			}
			else
			{
				int i;
				int tonari;
				int tonari_y[4][8] = {{-1,-1,-1,0,0,1,1,1},
						      {-1,-1,-1,0,0,1,1,1},
						      {1,1,1,0,0,-1,-1,-1},
						      {1,1,1,0,0,-1,-1,-1}};
				int tonari_x[4][8] = {{-1,0,1,-1,1,-1,0,1},
						      {1,0,-1,1,-1,1,0,-1},
						      {-1,0,1,-1,1,-1,0,1},
						      {1,0,-1,1,-1,1,0,-1}};

				if (m_ptr->fy < py && m_ptr->fx < px) tonari = 0;
				else if (m_ptr->fy < py) tonari = 1;
				else if (m_ptr->fx < px) tonari = 2;
				else tonari = 3;

				for (i = 0; i < 8; i++)
				{
					int next_x = x + tonari_x[tonari][i];
					int next_y = y + tonari_y[tonari][i];
					cave_type *c_ptr;

					/* Access the next grid */
					c_ptr = &cave[next_y][next_x];

					/* Skip door, rubble, wall */
					if ((c_ptr->feat >= FEAT_DOOR_HEAD) && (c_ptr->feat <= FEAT_PERM_SOLID)) continue;

					/* Skip tree */
					if (c_ptr->feat == FEAT_TREES) continue;

					/* Skip mountain */
					if (c_ptr->feat == FEAT_MOUNTAIN) continue;

					if (projectable(m_ptr->fy, m_ptr->fx, next_y, next_x))
					{
						y = next_y;
						x = next_x;
						success = TRUE;
						break;
					}
				}
			}

			if (!success)
			{
				if (m_ptr->target_y && m_ptr->target_x)
				{
					y = m_ptr->target_y;
					x = m_ptr->target_x;
					f4 &= (RF4_INDIRECT_MASK);
					f5 &= (RF5_INDIRECT_MASK);
					f6 &= (RF6_INDIRECT_MASK);
					success = TRUE;
				}
			}

			/* No spells */
			if (!success) return FALSE;
		}
	}

	m_ptr->target_y = 0;
	m_ptr->target_x = 0;

	/* Extract the monster level */
	rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);

	if (!stupid_monsters)
	{
		/* Forbid inate attacks sometimes */
		if (no_inate) f4 &= 0x500000FF;
	}

	if (!p_ptr->csp)
	{
		f5 &= ~(RF5_DRAIN_MANA);
	}
	if ((p_ptr->pclass == CLASS_NINJA) && (r_ptr->flags3 & (RF3_UNDEAD | RF3_HURT_LITE)))
	{
		f6 &= ~(RF6_DARKNESS);
	}

	if (dun_level && (!p_ptr->inside_quest || (p_ptr->inside_quest < MIN_RANDOM_QUEST)) && (d_info[dungeon_type].flags1 & DF1_NO_MAGIC))
	{
		f4 &= (RF4_NOMAGIC_MASK);
		f5 &= (RF5_NOMAGIC_MASK);
		f6 &= (RF6_NOMAGIC_MASK);
	}

	/* Hack -- allow "desperate" spells */
	if ((r_ptr->flags2 & (RF2_SMART)) &&
		(m_ptr->hp < m_ptr->maxhp / 10) &&
		(rand_int(100) < 50))
	{
		/* Require intelligent spells */
		f4 &= (RF4_INT_MASK);
		f5 &= (RF5_INT_MASK);
		f6 &= (RF6_INT_MASK);

		/* No spells left */
		if (!f4 && !f5 && !f6) return (FALSE);
	}

	/* Remove the "ineffective" spells */
	remove_bad_spells(m_idx, &f4, &f5, &f6);

	if (p_ptr->inside_arena)
	{
		f4 &= ~(RF4_SUMMON_MASK);
		f5 &= ~(RF5_SUMMON_MASK);
		f6 &= ~(RF6_SUMMON_MASK);
	}

	/* No spells left */
	if (!f4 && !f5 && !f6) return (FALSE);

	if (!stupid_monsters)
	{
		/* Check for a clean bolt shot */
		if (((f4 & RF4_BOLT_MASK) ||
		     (f5 & RF5_BOLT_MASK) ||
		     (f6 & RF6_BOLT_MASK)) &&
		     !(r_ptr->flags2 & RF2_STUPID) &&
		     !clean_shot(m_ptr->fy, m_ptr->fx, py, px, FALSE))
		{
			/* Remove spells that will only hurt friends */
			f4 &= ~(RF4_BOLT_MASK);
			f5 &= ~(RF5_BOLT_MASK);
			f6 &= ~(RF6_BOLT_MASK);
		}

		/* Check for a possible summon */
		if (((f4 & RF4_SUMMON_MASK) ||
		     (f5 & RF5_SUMMON_MASK) ||
		     (f6 & RF6_SUMMON_MASK)) &&
		     !(r_ptr->flags2 & RF2_STUPID) &&
		     !(summon_possible(y, x)))
		{
			/* Remove summoning spells */
			f4 &= ~(RF4_SUMMON_MASK);
			f5 &= ~(RF5_SUMMON_MASK);
			f6 &= ~(RF6_SUMMON_MASK);
		}

		/* No spells left */
		if (!f4 && !f5 && !f6) return (FALSE);
	}

	/* Extract the "inate" spells */
	for (k = 0; k < 32; k++)
	{
		if (f4 & (1L << k)) spell[num++] = k + 32 * 3;
	}

	/* Extract the "normal" spells */
	for (k = 0; k < 32; k++)
	{
		if (f5 & (1L << k)) spell[num++] = k + 32 * 4;
	}

	/* Extract the "bizarre" spells */
	for (k = 0; k < 32; k++)
	{
		if (f6 & (1L << k)) spell[num++] = k + 32 * 5;
	}

	/* No spells left */
	if (!num) return (FALSE);

	/* Stop if player is dead or gone */
	if (!alive || death) return (FALSE);

	/* Stop if player is leaving */
	if (p_ptr->leaving) return (FALSE);

	/* Get the monster name (or "it") */
	monster_desc(m_name, m_ptr, 0x00);

	/* Get the monster possessive ("his"/"her"/"its") */
	monster_desc(m_poss, m_ptr, 0x22);

	/* Hack -- Get the "died from" name */
	monster_desc(ddesc, m_ptr, 0x88);

	if (stupid_monsters)
	{
		/* Choose a spell to cast */
		thrown_spell = spell[rand_int(num)];
	}
	else
	{
		int attempt = 10;
		if (do_disi) thrown_spell = 96+31;
		else
		{
			while(attempt--)
			{
				thrown_spell = choose_attack_spell(m_idx, spell, num);
				if (thrown_spell) break;
			}
		}

		/* Abort if no spell was chosen */
		if (!thrown_spell) return (FALSE);

		/* Calculate spell failure rate */
		failrate = 25 - (rlev + 3) / 4;

		/* Hack -- Stupid monsters will never fail (for jellies and such) */
		if (r_ptr->flags2 & RF2_STUPID) failrate = 0;

		/* Check for spell failure (inate attacks never fail) */
		if ((thrown_spell >= 128) && ((m_ptr->stunned && one_in_(2)) || (rand_int(100) < failrate)))
		{
			disturb(1, 0);
			/* Message */
			if (thrown_spell != (160+7)) /* Not RF6_SPECIAL */
			{
#ifdef JP
msg_format("%^sは呪文を唱えようとしたが失敗した。", m_name);
#else
				msg_format("%^s tries to cast a spell, but fails.", m_name);
#endif
			}

			return (TRUE);
		}
	}

	/* Cast the spell. */
	switch (thrown_spell)
	{
		/* RF4_SHRIEK */
		case 96+0:
		{
			if (!direct) break;
			disturb(1, 0);
#ifdef JP
msg_format("%^sがかん高い金切り声をあげた。", m_name);
#else
			msg_format("%^s makes a high pitched shriek.", m_name);
#endif

			aggravate_monsters(m_idx);
			break;
		}

		/* RF4_XXX1 */
		case 96+1:
		{
			/* XXX XXX XXX */
			break;
		}

		/* RF4_DISPEL */
		case 96+2:
		{
			if (x!=px || y!=py) return (FALSE);
			disturb(1, 0);
#ifdef JP
			if (blind) msg_format("%^sが何かを力強くつぶやいた。", m_name);
			else msg_format("%^sが魔力消去の呪文を念じた。", m_name);
#else
			if (blind) msg_format("%^s mumbles powerfully.", m_name);
			else msg_format("%^s invokes a dispel magic.", m_name);
#endif
			set_fast(0, TRUE);
			set_lightspeed(0, TRUE);
			set_slow(0, TRUE);
			set_shield(0, TRUE);
			set_blessed(0, TRUE);
			set_tsuyoshi(0, TRUE);
			set_hero(0, TRUE);
			set_shero(0, TRUE);
			set_protevil(0, TRUE);
			set_invuln(0, TRUE);
			set_wraith_form(0, TRUE);
			set_kabenuke(0, TRUE);
			set_tim_res_nether(0, TRUE);
			set_tim_res_time(0, TRUE);
			/* by henkma */
			set_tim_reflect(0,TRUE);
			set_multishadow(0,TRUE);
			set_dustrobe(0,TRUE);

			set_tim_invis(0, TRUE);
			set_tim_infra(0, TRUE);
			set_tim_esp(0, TRUE);
			set_tim_regen(0, TRUE);
			set_tim_stealth(0, TRUE);
			set_tim_ffall(0, TRUE);
			set_tim_sh_touki(0, TRUE);
			set_tim_sh_fire(0, TRUE);
			set_magicdef(0, TRUE);
			set_resist_magic(0, TRUE);
			set_oppose_acid(0, TRUE);
			set_oppose_elec(0, TRUE);
			set_oppose_fire(0, TRUE);
			set_oppose_cold(0, TRUE);
			set_oppose_pois(0, TRUE);
			set_ultimate_res(0, TRUE);
			set_mimic(0, 0, TRUE);
			set_ele_attack(0, 0);
			set_ele_immune(0, 0);
			/* Cancel glowing hands */
			if (p_ptr->special_attack & ATTACK_CONFUSE)
			{
				p_ptr->special_attack &= ~(ATTACK_CONFUSE);
#ifdef JP
				msg_print("手の輝きがなくなった。");
#else
				msg_print("Your hands stop glowing.");
#endif

			}
			if ((p_ptr->pclass == CLASS_BARD) && (p_ptr->magic_num1[0]))
			{
				p_ptr->magic_num1[1] = p_ptr->magic_num1[0];
				p_ptr->magic_num1[0] = 0;
#ifdef JP
				msg_print("歌が途切れた。");
#else
				msg_print("Your singing is interrupted.");
#endif
				p_ptr->action = ACTION_NONE;

				/* Recalculate bonuses */
				p_ptr->update |= (PU_BONUS | PU_HP);

				/* Redraw map */
				p_ptr->redraw |= (PR_MAP | PR_STATUS | PR_STATE);

				/* Update monsters */
				p_ptr->update |= (PU_MONSTERS);

				/* Window stuff */
				p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

				p_ptr->energy -= 100;
			}
			if (p_ptr->riding)
			{
				m_list[p_ptr->riding].invulner = 0;
				m_list[p_ptr->riding].fast = 0;
				m_list[p_ptr->riding].slow = 0;
				p_ptr->update |= PU_BONUS;
				if (p_ptr->health_who == p_ptr->riding) p_ptr->redraw |= PR_HEALTH;
				p_ptr->redraw |= (PR_UHEALTH);
			}

#ifdef JP
			if ((p_ptr->pseikaku == SEIKAKU_COMBAT) || (inventory[INVEN_BOW].name1 == ART_CRIMSON))
				msg_print("やりやがったな！");
#endif
			learn_spell(MS_DISPEL);
			break;
		}

		/* RF4_XXX4X4 */
		case 96+3:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かを射った。", m_name);
#else
			if (blind) msg_format("%^s shoots something.", m_name);
#endif

#ifdef JP
else msg_format("%^sがロケットを発射した。", m_name);
#else
			else msg_format("%^s fires a rocket.", m_name);
#endif

			dam = ((m_ptr->hp / 4) > 800 ? 800 : (m_ptr->hp / 4));
			breath(y, x, m_idx, GF_ROCKET,
				dam, 2, FALSE, MS_ROCKET, learnable);
			update_smart_learn(m_idx, DRS_SHARD);
			break;
		}

		/* RF4_ARROW_1 */
		case 96+4:
		{
			if (x!=px || y!=py) return (FALSE);
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが奇妙な音を発した。", m_name);
#else
			if (blind) msg_format("%^s makes a strange noise.", m_name);
#endif

#ifdef JP
else msg_format("%^sが矢を放った。", m_name);
#else
			else msg_format("%^s fires an arrow.", m_name);
#endif

			dam = damroll(2, 5);
			bolt(m_idx, GF_ARROW, dam, MS_ARROW_1, learnable);
			update_smart_learn(m_idx, DRS_REFLECT);
			break;
		}

		/* RF4_ARROW_2 */
		case 96+5:
		{
			if (x!=px || y!=py) return (FALSE);
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが奇妙な音を発した。", m_name);
#else
			if (blind) msg_format("%^s makes a strange noise.", m_name);
#endif

#ifdef JP
else msg_format("%^sが矢を放った。", m_name);
#else
			else msg_format("%^s fires an arrow!", m_name);
#endif

			dam = damroll(3, 6);
			bolt(m_idx, GF_ARROW, dam, MS_ARROW_2, learnable);
			update_smart_learn(m_idx, DRS_REFLECT);
			break;
		}

		/* RF4_ARROW_3 */
		case 96+6:
		{
			if (x!=px || y!=py) return (FALSE);
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが奇妙な音を発した。", m_name);
#else
			if (blind) msg_format("%^s makes a strange noise.", m_name);
#endif

#ifdef JP
			else msg_format("%sがボルトを撃った。", m_name);
#else
			else msg_format("%^s fires a bolt.", m_name);
#endif
			dam = damroll(5, 6);
			bolt(m_idx, GF_ARROW, dam, MS_ARROW_3, learnable);
			update_smart_learn(m_idx, DRS_REFLECT);
			break;
		}

		/* RF4_ARROW_4 */
		case 96+7:
		{
			if (x!=px || y!=py) return (FALSE);
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが奇妙な音を発した。", m_name);
#else
			if (blind) msg_format("%^s makes a strange noise.", m_name);
#endif

#ifdef JP
			else msg_format("%sがボルトを撃った。", m_name);
#else
			else msg_format("%^s fires a bolt.", m_name);
#endif
			dam = damroll(7, 6);
			bolt(m_idx, GF_ARROW, dam, MS_ARROW_4, learnable);
			update_smart_learn(m_idx, DRS_REFLECT);
			break;
		}

		/* RF4_BR_ACID */
		case 96+8:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sが酸のブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes acid.", m_name);
#endif

			dam = ((m_ptr->hp / 3) > 1600 ? 1600 : (m_ptr->hp / 3));
			breath(y, x, m_idx, GF_ACID, dam, 0, TRUE, MS_BR_ACID, learnable);
			update_smart_learn(m_idx, DRS_ACID);
			break;
		}

		/* RF4_BR_ELEC */
		case 96+9:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sが稲妻のブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes lightning.", m_name);
#endif

			dam = ((m_ptr->hp / 3) > 1600 ? 1600 : (m_ptr->hp / 3));
			breath(y, x, m_idx, GF_ELEC, dam,0, TRUE, MS_BR_ELEC, learnable);
			update_smart_learn(m_idx, DRS_ELEC);
			break;
		}

		/* RF4_BR_FIRE */
		case 96+10:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sが火炎のブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes fire.", m_name);
#endif

			dam = ((m_ptr->hp / 3) > 1600 ? 1600 : (m_ptr->hp / 3));
			breath(y, x, m_idx, GF_FIRE, dam,0, TRUE, MS_BR_FIRE, learnable);
			update_smart_learn(m_idx, DRS_FIRE);
			break;
		}

		/* RF4_BR_COLD */
		case 96+11:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sが冷気のブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes frost.", m_name);
#endif

			dam = ((m_ptr->hp / 3) > 1600 ? 1600 : (m_ptr->hp / 3));
			breath(y, x, m_idx, GF_COLD, dam,0, TRUE, MS_BR_COLD, learnable);
			update_smart_learn(m_idx, DRS_COLD);
			break;
		}

		/* RF4_BR_POIS */
		case 96+12:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sがガスのブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes gas.", m_name);
#endif

			dam = ((m_ptr->hp / 3) > 800 ? 800 : (m_ptr->hp / 3));
			breath(y, x, m_idx, GF_POIS, dam, 0, TRUE, MS_BR_POIS, learnable);
			update_smart_learn(m_idx, DRS_POIS);
			break;
		}


		/* RF4_BR_NETH */
		case 96+13:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sが地獄のブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes nether.", m_name);
#endif

			dam = ((m_ptr->hp / 6) > 550 ? 550 : (m_ptr->hp / 6));
			breath(y, x, m_idx, GF_NETHER, dam,0, TRUE, MS_BR_NETHER, learnable);
			update_smart_learn(m_idx, DRS_NETH);
			break;
		}

		/* RF4_BR_LITE */
		case 96+14:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sが閃光のブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes light.", m_name);
#endif

			dam = ((m_ptr->hp / 6) > 400 ? 400 : (m_ptr->hp / 6));
			breath(y, x, m_idx, GF_LITE, dam,0, TRUE, MS_BR_LITE, learnable);
			update_smart_learn(m_idx, DRS_LITE);
			break;
		}

		/* RF4_BR_DARK */
		case 96+15:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sが暗黒のブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes darkness.", m_name);
#endif

			dam = ((m_ptr->hp / 6) > 400 ? 400 : (m_ptr->hp / 6));
			breath(y, x, m_idx, GF_DARK, dam,0, TRUE, MS_BR_DARK, learnable);
			update_smart_learn(m_idx, DRS_DARK);
			break;
		}

		/* RF4_BR_CONF */
		case 96+16:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sが混乱のブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes confusion.", m_name);
#endif

			dam = ((m_ptr->hp / 6) > 450 ? 450 : (m_ptr->hp / 6));
			breath(y, x, m_idx, GF_CONFUSION, dam,0, TRUE, MS_BR_CONF, learnable);
			update_smart_learn(m_idx, DRS_CONF);
			break;
		}

		/* RF4_BR_SOUN */
		case 96+17:
		{
			disturb(1, 0);
			if (m_ptr->r_idx == MON_JAIAN)
#ifdef JP
				msg_format("「ボォエ〜〜〜〜〜〜」");
#else
				msg_format("'Booooeeeeee'");
#endif
#ifdef JP
else if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			else if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sが轟音のブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes sound.", m_name);
#endif

			dam = ((m_ptr->hp / 6) > 450 ? 450 : (m_ptr->hp / 6));
			breath(y, x, m_idx, GF_SOUND, dam,0, TRUE, MS_BR_SOUND, learnable);
			update_smart_learn(m_idx, DRS_SOUND);
			break;
		}

		/* RF4_BR_CHAO */
		case 96+18:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sがカオスのブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes chaos.", m_name);
#endif

			dam = ((m_ptr->hp / 6) > 600 ? 600 : (m_ptr->hp / 6));
			breath(y, x, m_idx, GF_CHAOS, dam,0, TRUE, MS_BR_CHAOS, learnable);
			update_smart_learn(m_idx, DRS_CHAOS);
			break;
		}

		/* RF4_BR_DISE */
		case 96+19:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sが劣化のブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes disenchantment.", m_name);
#endif

			dam = ((m_ptr->hp / 6) > 500 ? 500 : (m_ptr->hp / 6));
			breath(y, x, m_idx, GF_DISENCHANT, dam,0, TRUE, MS_BR_DISEN, learnable);
			update_smart_learn(m_idx, DRS_DISEN);
			break;
		}

		/* RF4_BR_NEXU */
		case 96+20:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sが因果混乱のブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes nexus.", m_name);
#endif

			dam = ((m_ptr->hp / 3) > 250 ? 250 : (m_ptr->hp / 3));
			breath(y, x, m_idx, GF_NEXUS, dam,0, TRUE, MS_BR_NEXUS, learnable);
			update_smart_learn(m_idx, DRS_NEXUS);
			break;
		}

		/* RF4_BR_TIME */
		case 96+21:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sが時間逆転のブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes time.", m_name);
#endif

			dam = ((m_ptr->hp / 3) > 150 ? 150 : (m_ptr->hp / 3));
			breath(y, x, m_idx, GF_TIME, dam,0, TRUE, MS_BR_TIME, learnable);
			break;
		}

		/* RF4_BR_INER */
		case 96+22:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sが遅鈍のブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes inertia.", m_name);
#endif

			dam = ((m_ptr->hp / 6) > 200 ? 200 : (m_ptr->hp / 6));
			breath(y, x, m_idx, GF_INERTIA, dam,0, TRUE, MS_BR_INERTIA, learnable);
			break;
		}

		/* RF4_BR_GRAV */
		case 96+23:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sが重力のブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes gravity.", m_name);
#endif

			dam = ((m_ptr->hp / 3) > 200 ? 200 : (m_ptr->hp / 3));
			breath(y, x, m_idx, GF_GRAVITY, dam,0, TRUE, MS_BR_GRAVITY, learnable);
			break;
		}

		/* RF4_BR_SHAR */
		case 96+24:
		{
			disturb(1, 0);
			if (m_ptr->r_idx == MON_BOTEI)
#ifdef JP
				msg_format("「ボ帝ビルカッター！！！」");
#else
				msg_format("'Boty-Build cutter!!!'");
#endif
#ifdef JP
else if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			else if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sが破片のブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes shards.", m_name);
#endif

			dam = ((m_ptr->hp / 6) > 500 ? 500 : (m_ptr->hp / 6));
			breath(y, x, m_idx, GF_SHARDS, dam,0, TRUE, MS_BR_SHARDS, learnable);
			update_smart_learn(m_idx, DRS_SHARD);
			break;
		}

		/* RF4_BR_PLAS */
		case 96+25:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sがプラズマのブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes plasma.", m_name);
#endif

			dam = ((m_ptr->hp / 6) > 150 ? 150 : (m_ptr->hp / 6));
			breath(y, x, m_idx, GF_PLASMA, dam,0, TRUE, MS_BR_PLASMA, learnable);
			break;
		}

		/* RF4_BR_WALL */
		case 96+26:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sがフォースのブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes force.", m_name);
#endif

			dam = ((m_ptr->hp / 6) > 200 ? 200 : (m_ptr->hp / 6));
			breath(y, x, m_idx, GF_FORCE, dam,0, TRUE, MS_BR_FORCE, learnable);
			break;
		}

		/* RF4_BR_MANA */
		case 96+27:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sが魔力のブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes mana.", m_name);
#endif
			dam = ((m_ptr->hp / 3) > 250 ? 250 : (m_ptr->hp / 3));
			breath(y, x, m_idx, GF_MANA, dam,0, TRUE, MS_BR_MANA, learnable);
			break;
		}

		/* RF4_BA_NUKE */
		case 96+28:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが放射能球を放った。", m_name);
#else
			else msg_format("%^s casts a ball of radiation.", m_name);
#endif

			dam = (rlev + damroll(10, 6)) * ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 1);
			breath(y, x, m_idx, GF_NUKE, dam, 2, FALSE, MS_BALL_NUKE, learnable);
			update_smart_learn(m_idx, DRS_POIS);
			break;
		}

		/* RF4_BR_NUKE */
		case 96+29:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sが放射性廃棄物のブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes toxic waste.", m_name);
#endif

			dam = ((m_ptr->hp / 3) > 800 ? 800 : (m_ptr->hp / 3));
			breath(y, x, m_idx, GF_NUKE, dam,0, TRUE, MS_BR_NUKE, learnable);
			update_smart_learn(m_idx, DRS_POIS);
			break;
		}

		/* RF4_BA_CHAO */
		case 96+30:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが恐ろしげにつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles frighteningly.", m_name);
#endif

#ifdef JP
else msg_format("%^sが純ログルスを放った。", m_name);/*nuke me*/
#else
			else msg_format("%^s invokes a raw Logrus.", m_name);
#endif

			dam = ((r_ptr->flags2 & RF2_POWERFUL) ? (rlev * 3) : (rlev * 2))+ damroll(10, 10);
			breath(y, x, m_idx, GF_CHAOS, dam, 4, FALSE, MS_BALL_CHAOS, learnable);
			update_smart_learn(m_idx, DRS_CHAOS);
			break;
		}

		/* RF4_BR_DISI */
		case 96+31:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かのブレスを吐いた。", m_name);
#else
			if (blind) msg_format("%^s breathes.", m_name);
#endif

#ifdef JP
else msg_format("%^sが分解のブレスを吐いた。", m_name);
#else
			else msg_format("%^s breathes disintegration.", m_name);
#endif

			dam = ((m_ptr->hp / 6) > 150 ? 150 : (m_ptr->hp / 6));
			breath(y, x, m_idx, GF_DISINTEGRATE, dam,0, TRUE, MS_BR_DISI, learnable);
			break;
		}



		/* RF5_BA_ACID */
		case 128+0:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sがアシッド・ボールの呪文を唱えた。", m_name);
#else
			else msg_format("%^s casts an acid ball.", m_name);
#endif

			dam = (randint(rlev * 3) + 15) * ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 1);
			breath(y, x, m_idx, GF_ACID, dam, 2, FALSE, MS_BALL_ACID, learnable);
			update_smart_learn(m_idx, DRS_ACID);
			break;
		}

		/* RF5_BA_ELEC */
		case 128+1:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sがサンダー・ボールの呪文を唱えた。", m_name);
#else
			else msg_format("%^s casts a lightning ball.", m_name);
#endif

			dam = (randint(rlev * 3 / 2) + 8) * ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 1);
			breath(y, x, m_idx, GF_ELEC, dam, 2, FALSE, MS_BALL_ELEC, learnable);
			update_smart_learn(m_idx, DRS_ELEC);
			break;
		}

		/* RF5_BA_FIRE */
		case 128+2:
		{
			disturb(1, 0);

			if (m_ptr->r_idx == MON_ROLENTO)
			{
#ifdef JP
				if (blind)
					msg_format("%sが何かを投げた。", m_name);
				else 
					msg_format("%sは手榴弾を投げた。", m_name);
#else
				if (blind)
					msg_format("%^s throws something.", m_name);
				else
					msg_format("%^s throws a hand grenade.", m_name);
#endif
			}
			else
			{
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
				if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sがファイア・ボールの呪文を唱えた。", m_name);
#else
				else msg_format("%^s casts a fire ball.", m_name);
#endif
			}

			dam = (randint(rlev * 7 / 2) + 10) * ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 1);
			breath(y, x, m_idx, GF_FIRE, dam, 2, FALSE, MS_BALL_FIRE, learnable);
			update_smart_learn(m_idx, DRS_FIRE);
			break;
		}

		/* RF5_BA_COLD */
		case 128+3:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sがアイス・ボールの呪文を唱えた。", m_name);
#else
			else msg_format("%^s casts a frost ball.", m_name);
#endif

			dam = (randint(rlev * 3 / 2) + 10) * ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 1);
			breath(y, x, m_idx, GF_COLD, dam, 2, FALSE, MS_BALL_COLD, learnable);
			update_smart_learn(m_idx, DRS_COLD);
			break;
		}

		/* RF5_BA_POIS */
		case 128+4:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが悪臭雲の呪文を唱えた。", m_name);
#else
			else msg_format("%^s casts a stinking cloud.", m_name);
#endif

			dam = damroll(12, 2) * ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 1);
			breath(y, x, m_idx, GF_POIS, dam, 2, FALSE, MS_BALL_POIS, learnable);
			update_smart_learn(m_idx, DRS_POIS);
			break;
		}

		/* RF5_BA_NETH */
		case 128+5:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが地獄球の呪文を唱えた。", m_name);
#else
			else msg_format("%^s casts a nether ball.", m_name);
#endif

			dam = 50 + damroll(10, 10) + (rlev * ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 1));
			breath(y, x, m_idx, GF_NETHER, dam, 2, FALSE, MS_BALL_NETHER, learnable);
			update_smart_learn(m_idx, DRS_NETH);
			break;
		}

		/* RF5_BA_WATE */
		case 128+6:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが流れるような身振りをした。", m_name);
#else
			else msg_format("%^s gestures fluidly.", m_name);
#endif

#ifdef JP
msg_print("あなたは渦巻きに飲み込まれた。");
#else
			msg_print("You are engulfed in a whirlpool.");
#endif

			dam = ((r_ptr->flags2 & RF2_POWERFUL) ? randint(rlev * 3) : randint(rlev * 2)) + 50;
			breath(y, x, m_idx, GF_WATER, dam, 4, FALSE, MS_BALL_WATER, learnable);
			break;
		}

		/* RF5_BA_MANA */
		case 128+7:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かを力強くつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles powerfully.", m_name);
#endif

#ifdef JP
else msg_format("%^sが魔力の嵐の呪文を念じた。", m_name);
#else
			else msg_format("%^s invokes a mana storm.", m_name);
#endif

			dam = (rlev * 4) + 50 + damroll(10, 10);
			breath(y, x, m_idx, GF_MANA, dam, 4, FALSE, MS_BALL_MANA, learnable);
			break;
		}

		/* RF5_BA_DARK */
		case 128+8:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かを力強くつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles powerfully.", m_name);
#endif

#ifdef JP
else msg_format("%^sが暗黒の嵐の呪文を念じた。", m_name);
#else
			else msg_format("%^s invokes a darkness storm.", m_name);
#endif

			dam = (rlev * 4) + 50 + damroll(10, 10);
			breath(y, x, m_idx, GF_DARK, dam, 4, FALSE, MS_BALL_DARK, learnable);
			update_smart_learn(m_idx, DRS_DARK);
			break;
		}

		/* RF5_DRAIN_MANA */
		case 128+9:
		{
			if (x!=px || y!=py) return (FALSE);
			if (!direct) break;
			disturb(1, 0);
			if (p_ptr->csp)
			{
				int r1;

				/* Basic message */
#ifdef JP
msg_format("%^sに精神エネルギーを吸い取られてしまった！", m_name);
#else
				msg_format("%^s draws psychic energy from you!", m_name);
#endif


				/* Attack power */
				r1 = (randint(rlev) / 2) + 1;

				/* Full drain */
				if (r1 >= p_ptr->csp)
				{
					r1 = p_ptr->csp;
					p_ptr->csp = 0;
					p_ptr->csp_frac = 0;
				}

				/* Partial drain */
				else
				{
					p_ptr->csp -= r1;
				}

				learn_spell(MS_DRAIN_MANA);

				/* Redraw mana */
				p_ptr->redraw |= (PR_MANA);

				/* Window stuff */
				p_ptr->window |= (PW_PLAYER);
				p_ptr->window |= (PW_SPELL);

				/* Heal the monster */
				if (m_ptr->hp < m_ptr->maxhp)
				{
					/* Heal */
					m_ptr->hp += (6 * r1);
					if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

					/* Redraw (later) if needed */
					if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
					if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);

					/* Special message */
					if (seen)
					{
#ifdef JP
msg_format("%^sは気分が良さそうだ。", m_name);
#else
						msg_format("%^s appears healthier.", m_name);
#endif

					}
				}
			}
			update_smart_learn(m_idx, DRS_MANA);
			break;
		}

		/* RF5_MIND_BLAST */
		case 128+10:
		{
			if (x!=px || y!=py) return (FALSE);
			if (!direct) break;
			disturb(1, 0);
			if (!seen)
			{
#ifdef JP
msg_print("何かがあなたの精神に念を放っているようだ。");
#else
				msg_print("You feel something focusing on your mind.");
#endif

			}
			else
			{
#ifdef JP
msg_format("%^sがあなたの瞳をじっとにらんでいる。", m_name);
#else
				msg_format("%^s gazes deep into your eyes.", m_name);
#endif

			}

			dam = damroll(7, 7);
			if (rand_int(100 + rlev/2) < (MAX(5, p_ptr->skill_sav)))
			{
#ifdef JP
msg_print("しかし効力を跳ね返した！");
#else
				msg_print("You resist the effects!");
#endif
				learn_spell(MS_MIND_BLAST);
			}
			else
			{
#ifdef JP
msg_print("霊的エネルギーで精神が攻撃された。");
#else
				msg_print("Your mind is blasted by psyonic energy.");
#endif

				if (!p_ptr->resist_conf)
				{
					(void)set_confused(p_ptr->confused + rand_int(4) + 4);
				}

				if (!p_ptr->resist_chaos && one_in_(3))
				{
					(void)set_image(p_ptr->image + rand_int(250) + 150);
				}

				p_ptr->csp -= 50;
				if (p_ptr->csp < 0)
				{
					p_ptr->csp = 0;
					p_ptr->csp_frac = 0;
				}
				p_ptr->redraw |= PR_MANA;

				take_hit(DAMAGE_ATTACK, dam, ddesc, MS_MIND_BLAST);
			}
			break;
		}

		/* RF5_BRAIN_SMASH */
		case 128+11:
		{
			if (x!=px || y!=py) return (FALSE);
			if (!direct) break;
			disturb(1, 0);
			if (!seen)
			{
#ifdef JP
msg_print("何かがあなたの精神に念を放っているようだ。");
#else
				msg_print("You feel something focusing on your mind.");
#endif

			}
			else
			{
#ifdef JP
msg_format("%^sがあなたの瞳をじっと見ている。", m_name);
#else
				msg_format("%^s looks deep into your eyes.", m_name);
#endif

			}

			dam = damroll(12, 12);
			if (rand_int(100 + rlev/2) < (MAX(5, p_ptr->skill_sav)))
			{
#ifdef JP
msg_print("しかし効力を跳ね返した！");
#else
				msg_print("You resist the effects!");
#endif
				learn_spell(MS_BRAIN_SMASH);
			}
			else
			{
#ifdef JP
msg_print("霊的エネルギーで精神が攻撃された。");
#else
				msg_print("Your mind is blasted by psionic energy.");
#endif

				p_ptr->csp -= 100;
				if (p_ptr->csp < 0)
				{
					p_ptr->csp = 0;
					p_ptr->csp_frac = 0;
				}
				p_ptr->redraw |= PR_MANA;

				take_hit(DAMAGE_ATTACK, dam, ddesc, MS_BRAIN_SMASH);
				if (!p_ptr->resist_blind)
				{
					(void)set_blind(p_ptr->blind + 8 + rand_int(8));
				}
				if (!p_ptr->resist_conf)
				{
					(void)set_confused(p_ptr->confused + rand_int(4) + 4);
				}
				if (!p_ptr->free_act)
				{
					(void)set_paralyzed(p_ptr->paralyzed + rand_int(4) + 4);
				}
				(void)set_slow(p_ptr->slow + rand_int(4) + 4, FALSE);

				while (rand_int(100 + rlev/2) > (MAX(5, p_ptr->skill_sav)))
					(void)do_dec_stat(A_INT);
				while (rand_int(100 + rlev/2) > (MAX(5, p_ptr->skill_sav)))
					(void)do_dec_stat(A_WIS);

				if (!p_ptr->resist_chaos)
				{
					(void)set_image(p_ptr->image + rand_int(250) + 150);
				}
			}
			break;
		}

		/* RF5_CAUSE_1 */
		case 128+12:
		{
			if (x!=px || y!=py) return (FALSE);
			if (!direct) break;
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sがあなたを指さして呪った。", m_name);
#else
			else msg_format("%^s points at you and curses.", m_name);
#endif

			dam = damroll(3, 8);
			if (rand_int(100 + rlev/2) < p_ptr->skill_sav)
			{
#ifdef JP
msg_print("しかし効力を跳ね返した！");
#else
				msg_print("You resist the effects!");
#endif
				learn_spell(MS_CAUSE_1);
			}
			else
			{
				curse_equipment(33, 0);
				take_hit(DAMAGE_ATTACK, dam, ddesc, MS_CAUSE_1);
			}
			break;
		}

		/* RF5_CAUSE_2 */
		case 128+13:
		{
			if (x!=px || y!=py) return (FALSE);
			if (!direct) break;
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sがあなたを指さして恐ろしげに呪った。", m_name);
#else
			else msg_format("%^s points at you and curses horribly.", m_name);
#endif

			dam = damroll(8, 8);
			if (rand_int(100 + rlev/2) < p_ptr->skill_sav)
			{
#ifdef JP
msg_print("しかし効力を跳ね返した！");
#else
				msg_print("You resist the effects!");
#endif
				learn_spell(MS_CAUSE_2);
			}
			else
			{
				curse_equipment(50, 5);
				take_hit(DAMAGE_ATTACK, dam, ddesc, MS_CAUSE_2);
			}
			break;
		}

		/* RF5_CAUSE_3 */
		case 128+14:
		{
			if (x!=px || y!=py) return (FALSE);
			if (!direct) break;
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かを大声で叫んだ。", m_name);
#else
			if (blind) msg_format("%^s mumbles loudly.", m_name);
#endif

#ifdef JP
else msg_format("%^sがあなたを指さして恐ろしげに呪文を唱えた！", m_name);
#else
			else msg_format("%^s points at you, incanting terribly!", m_name);
#endif

			dam = damroll(10, 15);
			if (rand_int(100 + rlev/2) < p_ptr->skill_sav)
			{
#ifdef JP
msg_print("しかし効力を跳ね返した！");
#else
				msg_print("You resist the effects!");
#endif
				learn_spell(MS_CAUSE_3);
			}
			else
			{
				curse_equipment(80, 15);
				take_hit(DAMAGE_ATTACK, dam, ddesc, MS_CAUSE_3);
			}
			break;
		}

		/* RF5_CAUSE_4 */
		case 128+15:
		{
			if (x!=px || y!=py) return (FALSE);
			if (!direct) break;
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが「お前は既に死んでいる」と叫んだ。", m_name);
#else
			if (blind) msg_format("%^s screams the word 'DIE!'", m_name);
#endif

#ifdef JP
else msg_format("%^sがあなたの秘孔を突いて「お前は既に死んでいる」と叫んだ。", m_name);
#else
			else msg_format("%^s points at you, screaming the word DIE!", m_name);
#endif

			dam = damroll(15, 15);
			if ((rand_int(100 + rlev/2) < p_ptr->skill_sav) && !(m_ptr->r_idx == MON_KENSHIROU))
			{
#ifdef JP
msg_print("しかし秘孔を跳ね返した！");
#else
				msg_print("You resist the effects!");
#endif
				learn_spell(MS_CAUSE_4);
			}
			else
			{
				take_hit(DAMAGE_ATTACK, dam, ddesc, MS_CAUSE_4);
				(void)set_cut(p_ptr->cut + damroll(10, 10));
			}
			break;
		}

		/* RF5_BO_ACID */
		case 128+16:
		{
			if (x!=px || y!=py) return (FALSE);
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sがアシッド・ボルトの呪文を唱えた。", m_name);
#else
			else msg_format("%^s casts a acid bolt.", m_name);
#endif

			dam = (damroll(7, 8) + (rlev / 3)) * ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 1);
			bolt(m_idx, GF_ACID, dam, MS_BOLT_ACID, learnable);
			update_smart_learn(m_idx, DRS_ACID);
			update_smart_learn(m_idx, DRS_REFLECT);
			break;
		}

		/* RF5_BO_ELEC */
		case 128+17:
		{
			if (x!=px || y!=py) return (FALSE);
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sがサンダー・ボルトの呪文を唱えた。", m_name);
#else
			else msg_format("%^s casts a lightning bolt.", m_name);
#endif

			dam = (damroll(4, 8) + (rlev / 3)) * ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 1);
			bolt(m_idx, GF_ELEC, dam, MS_BOLT_ELEC, learnable);
			update_smart_learn(m_idx, DRS_ELEC);
			update_smart_learn(m_idx, DRS_REFLECT);
			break;
		}

		/* RF5_BO_FIRE */
		case 128+18:
		{
			if (x!=px || y!=py) return (FALSE);
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sがファイア・ボルトの呪文を唱えた。", m_name);
#else
			else msg_format("%^s casts a fire bolt.", m_name);
#endif

			dam = (damroll(9, 8) + (rlev / 3)) * ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 1);
			bolt(m_idx, GF_FIRE, dam, MS_BOLT_FIRE, learnable);
			update_smart_learn(m_idx, DRS_FIRE);
			update_smart_learn(m_idx, DRS_REFLECT);
			break;
		}

		/* RF5_BO_COLD */
		case 128+19:
		{
			if (x!=px || y!=py) return (FALSE);
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sがアイス・ボルトの呪文を唱えた。", m_name);
#else
			else msg_format("%^s casts a frost bolt.", m_name);
#endif

			dam = (damroll(6, 8) + (rlev / 3)) * ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 1);
			bolt(m_idx, GF_COLD, dam, MS_BOLT_COLD, learnable);
			update_smart_learn(m_idx, DRS_COLD);
			update_smart_learn(m_idx, DRS_REFLECT);
			break;
		}

		/* RF5_BA_LITE */
		case 128+20:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かを力強くつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles powerfully.", m_name);
#endif

#ifdef JP
else msg_format("%^sがスターバーストの呪文を念じた。", m_name);
#else
			else msg_format("%^s invokes a starburst.", m_name);
#endif

			dam = (rlev * 4) + 50 + damroll(10, 10);
			breath(y, x, m_idx, GF_LITE, dam, 4, FALSE, MS_STARBURST, learnable);
			update_smart_learn(m_idx, DRS_LITE);
			break;
		}

		/* RF5_BO_NETH */
		case 128+21:
		{
			if (x!=px || y!=py) return (FALSE);
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが地獄の矢の呪文を唱えた。", m_name);
#else
			else msg_format("%^s casts a nether bolt.", m_name);
#endif

			dam = 30 + damroll(5, 5) + (rlev * 4) / ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 3);
			bolt(m_idx, GF_NETHER, dam, MS_BOLT_NETHER, learnable);
			update_smart_learn(m_idx, DRS_NETH);
			update_smart_learn(m_idx, DRS_REFLECT);
			break;
		}

		/* RF5_BO_WATE */
		case 128+22:
		{
			if (x!=px || y!=py) return (FALSE);
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sがウォーター・ボルトの呪文を唱えた。", m_name);
#else
			else msg_format("%^s casts a water bolt.", m_name);
#endif

			dam = damroll(10, 10) + (rlev * 3 / ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 3));
			bolt(m_idx, GF_WATER, dam, MS_BOLT_WATER, learnable);
			update_smart_learn(m_idx, DRS_REFLECT);
			break;
		}

		/* RF5_BO_MANA */
		case 128+23:
		{
			if (x!=px || y!=py) return (FALSE);
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが魔力の矢の呪文を唱えた。", m_name);
#else
			else msg_format("%^s casts a mana bolt.", m_name);
#endif

			dam = randint(rlev * 7 / 2) + 50;
			bolt(m_idx, GF_MANA, dam, MS_BOLT_MANA, learnable);
			update_smart_learn(m_idx, DRS_REFLECT);
			break;
		}

		/* RF5_BO_PLAS */
		case 128+24:
		{
			if (x!=px || y!=py) return (FALSE);
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sがプラズマ・ボルトの呪文を唱えた。", m_name);
#else
			else msg_format("%^s casts a plasma bolt.", m_name);
#endif

			dam = 10 + damroll(8, 7) + (rlev * 3 / ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 3));
			bolt(m_idx, GF_PLASMA, dam, MS_BOLT_PLASMA, learnable);
			update_smart_learn(m_idx, DRS_REFLECT);
			break;
		}

		/* RF5_BO_ICEE */
		case 128+25:
		{
			if (x!=px || y!=py) return (FALSE);
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが極寒の矢の呪文を唱えた。", m_name);
#else
			else msg_format("%^s casts an ice bolt.", m_name);
#endif

			dam = damroll(6, 6) + (rlev * 3 / ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 3));
			bolt(m_idx, GF_ICE, dam, MS_BOLT_ICE, learnable);
			update_smart_learn(m_idx, DRS_COLD);
			update_smart_learn(m_idx, DRS_REFLECT);
			break;
		}

		/* RF5_MISSILE */
		case 128+26:
		{
			if (x!=px || y!=py) return (FALSE);
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sがマジック・ミサイルの呪文を唱えた。", m_name);
#else
			else msg_format("%^s casts a magic missile.", m_name);
#endif

			dam = damroll(2, 6) + (rlev / 3);
			bolt(m_idx, GF_MISSILE, dam, MS_MAGIC_MISSILE, learnable);
			update_smart_learn(m_idx, DRS_REFLECT);
			break;
		}

		/* RF5_SCARE */
		case 128+27:
		{
			if (x!=px || y!=py) return (FALSE);
			if (!direct) break;
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやくと、恐ろしげな音が聞こえた。", m_name);
#else
			if (blind) msg_format("%^s mumbles, and you hear scary noises.", m_name);
#endif

#ifdef JP
else msg_format("%^sが恐ろしげな幻覚を作り出した。", m_name);
#else
			else msg_format("%^s casts a fearful illusion.", m_name);
#endif

			if (p_ptr->resist_fear)
			{
#ifdef JP
msg_print("しかし恐怖に侵されなかった。");
#else
				msg_print("You refuse to be frightened.");
#endif

			}
			else if (rand_int(100 + rlev/2) < p_ptr->skill_sav)
			{
#ifdef JP
msg_print("しかし恐怖に侵されなかった。");
#else
				msg_print("You refuse to be frightened.");
#endif

			}
			else
			{
				(void)set_afraid(p_ptr->afraid + rand_int(4) + 4);
			}
			learn_spell(MS_SCARE);
			update_smart_learn(m_idx, DRS_FEAR);
			break;
		}

		/* RF5_BLIND */
		case 128+28:
		{
			if (x!=px || y!=py) return (FALSE);
			if (!direct) break;
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが呪文を唱えてあなたの目をくらました！", m_name);
#else
			else msg_format("%^s casts a spell, burning your eyes!", m_name);
#endif

			if (p_ptr->resist_blind)
			{
#ifdef JP
msg_print("しかし効果がなかった！");
#else
				msg_print("You are unaffected!");
#endif

			}
			else if (rand_int(100 + rlev/2) < p_ptr->skill_sav)
			{
#ifdef JP
msg_print("しかし効力を跳ね返した！");
#else
				msg_print("You resist the effects!");
#endif

			}
			else
			{
				(void)set_blind(12 + rand_int(4));
			}
			learn_spell(MS_BLIND);
			update_smart_learn(m_idx, DRS_BLIND);
			break;
		}

		/* RF5_CONF */
		case 128+29:
		{
			if (x!=px || y!=py) return (FALSE);
			if (!direct) break;
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやくと、頭を悩ます音がした。", m_name);
#else
			if (blind) msg_format("%^s mumbles, and you hear puzzling noises.", m_name);
#endif

#ifdef JP
else msg_format("%^sが誘惑的な幻覚を作り出した。", m_name);
#else
			else msg_format("%^s creates a mesmerising illusion.", m_name);
#endif

			if (p_ptr->resist_conf)
			{
#ifdef JP
msg_print("しかし幻覚にはだまされなかった。");
#else
				msg_print("You disbelieve the feeble spell.");
#endif

			}
			else if (rand_int(100 + rlev/2) < p_ptr->skill_sav)
			{
#ifdef JP
msg_print("しかし幻覚にはだまされなかった。");
#else
				msg_print("You disbelieve the feeble spell.");
#endif

			}
			else
			{
				(void)set_confused(p_ptr->confused + rand_int(4) + 4);
			}
			learn_spell(MS_CONF);
			update_smart_learn(m_idx, DRS_CONF);
			break;
		}

		/* RF5_SLOW */
		case 128+30:
		{
			if (x!=px || y!=py) return (FALSE);
			if (!direct) break;
			disturb(1, 0);
#ifdef JP
msg_format("%^sがあなたの筋力を吸い取ろうとした！", m_name);
#else
			msg_format("%^s drains power from your muscles!", m_name);
#endif

			if (p_ptr->free_act)
			{
#ifdef JP
msg_print("しかし効果がなかった！");
#else
				msg_print("You are unaffected!");
#endif

			}
			else if (rand_int(100 + rlev/2) < p_ptr->skill_sav)
			{
#ifdef JP
msg_print("しかし効力を跳ね返した！");
#else
				msg_print("You resist the effects!");
#endif

			}
			else
			{
				(void)set_slow(p_ptr->slow + rand_int(4) + 4, FALSE);
			}
			learn_spell(MS_SLOW);
			update_smart_learn(m_idx, DRS_FREE);
			break;
		}

		/* RF5_HOLD */
		case 128+31:
		{
			if (x!=px || y!=py) return (FALSE);
			if (!direct) break;
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sがあなたの目をじっと見つめた！", m_name);
#else
			else msg_format("%^s stares deep into your eyes!", m_name);
#endif

			if (p_ptr->free_act)
			{
#ifdef JP
msg_print("しかし効果がなかった！");
#else
				msg_print("You are unaffected!");
#endif

			}
			else if (rand_int(100 + rlev/2) < p_ptr->skill_sav)
			{
#ifdef JP
msg_format("しかし効力を跳ね返した！");
#else
				msg_format("You resist the effects!");
#endif

			}
			else
			{
				(void)set_paralyzed(p_ptr->paralyzed + rand_int(4) + 4);
			}
			learn_spell(MS_SLEEP);
			update_smart_learn(m_idx, DRS_FREE);
			break;
		}

		/* RF6_HASTE */
		case 160+0:
		{
			disturb(1, 0);
			if (blind)
			{
#ifdef JP
msg_format("%^sが何かをつぶやいた。", m_name);
#else
				msg_format("%^s mumbles.", m_name);
#endif

			}
			else
			{
#ifdef JP
msg_format("%^sが自分の体に念を送った。", m_name, m_poss);
#else
				msg_format("%^s concentrates on %s body.", m_name, m_poss);
#endif

			}

			/* Allow quick speed increases to base+10 */
			if (!m_ptr->fast)
			{
#ifdef JP
msg_format("%^sの動きが速くなった。", m_name);
#else
				msg_format("%^s starts moving faster.", m_name);
#endif
			}
			m_ptr->fast = MIN(200, m_ptr->fast + 100);
			if (p_ptr->riding == m_idx) p_ptr->update |= PU_BONUS;
			break;
		}

		/* RF6_HAND_DOOM */
		case 160+1:
		{
			if (x!=px || y!=py) return (FALSE);
			disturb(1, 0);
#ifdef JP
msg_format("%^sが破滅の手を放った！", m_name);
#else
			msg_format("%^s invokes the Hand of Doom!", m_name);
#endif

			if (rand_int(100 + rlev/2) < p_ptr->skill_sav)
			{
#ifdef JP
msg_format("しかし効力を跳ね返した！");
#else
				msg_format("You resist the effects!");
#endif
				learn_spell(MS_HAND_DOOM);

			}
			else
			{
				int dummy = (((s32b) ((40 + randint(20)) * (p_ptr->chp))) / 100);
#ifdef JP
msg_print("あなたは命が薄まっていくように感じた！");
#else
				msg_print("Your feel your life fade away!");
#endif

				take_hit(DAMAGE_ATTACK, dummy, m_name, MS_HAND_DOOM);
				curse_equipment(100, 20);

				if (p_ptr->chp < 1) p_ptr->chp = 1;
			}
			break;
		}

		/* RF6_HEAL */
		case 160+2:
		{
			disturb(1, 0);

			/* Message */
			if (blind)
			{
#ifdef JP
msg_format("%^sが何かをつぶやいた。", m_name);
#else
				msg_format("%^s mumbles.", m_name);
#endif

			}
			else
			{
#ifdef JP
msg_format("%^sが自分の傷に集中した。", m_name);
#else
				msg_format("%^s concentrates on %s wounds.", m_name, m_poss);
#endif

			}

			/* Heal some */
			m_ptr->hp += (rlev * 6);

			/* Fully healed */
			if (m_ptr->hp >= m_ptr->maxhp)
			{
				/* Fully healed */
				m_ptr->hp = m_ptr->maxhp;

				/* Message */
				if (seen)
				{
#ifdef JP
msg_format("%^sは完全に治った！", m_name);
#else
					msg_format("%^s looks completely healed!", m_name);
#endif

				}
				else
				{
#ifdef JP
msg_format("%^sは完全に治ったようだ！", m_name);
#else
					msg_format("%^s sounds completely healed!", m_name);
#endif

				}
			}

			/* Partially healed */
			else
			{
				/* Message */
				if (seen)
				{
#ifdef JP
msg_format("%^sは体力を回復したようだ。", m_name);
#else
					msg_format("%^s looks healthier.", m_name);
#endif

				}
				else
				{
#ifdef JP
msg_format("%^sは体力を回復したようだ。", m_name);
#else
					msg_format("%^s sounds healthier.", m_name);
#endif

				}
			}

			/* Redraw (later) if needed */
			if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
			if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);

			/* Cancel fear */
			if (m_ptr->monfear)
			{
				/* Cancel fear */
				m_ptr->monfear = 0;

				/* Message */
#ifdef JP
msg_format("%^sは勇気を取り戻した。", m_name, m_poss);
#else
				msg_format("%^s recovers %s courage.", m_name, m_poss);
#endif

			}
			break;
		}

		/* RF6_INVULNER */
		case 160+3:
		{
			disturb(1, 0);

			/* Message */
			if (!seen)
			{
#ifdef JP
msg_format("%^sが何かを力強くつぶやいた。", m_name);
#else
				msg_format("%^s mumbles powerfully.", m_name);
#endif

			}
			else
			{
#ifdef JP
msg_format("%sは無傷の球の呪文を唱えた。", m_name);
#else
				msg_format("%^s casts a Globe of Invulnerability.", m_name);
#endif

			}

			if (!(m_ptr->invulner))
				m_ptr->invulner = randint(4) + 4;

			if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
			if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);
			break;
		}

		/* RF6_BLINK */
		case 160+4:
		{
			disturb(1, 0);
#ifdef JP
msg_format("%^sが瞬時に消えた。", m_name);
#else
			msg_format("%^s blinks away.", m_name);
#endif

			teleport_away(m_idx, 10, FALSE);
			p_ptr->update |= (PU_MONSTERS | PU_MON_LITE);
			break;
		}

		/* RF6_TPORT */
		case 160+5:
		{
			int i, oldfy, oldfx;
			u32b f1 = 0 , f2 = 0 , f3 = 0;
			object_type *o_ptr;

			oldfy = m_ptr->fy;
			oldfx = m_ptr->fx;

			disturb(1, 0);
#ifdef JP
msg_format("%^sがテレポートした。", m_name);
#else
			msg_format("%^s teleports away.", m_name);
#endif

			teleport_away(m_idx, MAX_SIGHT * 2 + 5, FALSE);

			if (los(py, px, oldfy, oldfx) && !world_monster)
			{
				for (i=INVEN_RARM;i<INVEN_TOTAL;i++)
				{
					o_ptr = &inventory[i];
					if(!(o_ptr->ident & IDENT_CURSED))
					{
						object_flags(o_ptr, &f1, &f2, &f3);

						if((f3 & TR3_TELEPORT) || (p_ptr->muta1 & MUT1_VTELEPORT) || (p_ptr->pclass == CLASS_IMITATOR))
						{
#ifdef JP
							if(get_check_strict("ついていきますか？", 1))
#else
							if(get_check_strict("Do you follow it? ", 1))
#endif
							{
								if (randint(3) == 1)
								{
									teleport_player(200);
#ifdef JP
									msg_print("失敗！");
#else
									msg_print("Failed!");
#endif
								}
								else teleport_player_to(m_ptr->fy, m_ptr->fx, TRUE);
								p_ptr->energy -= 100;
							}
							break;
						}
					}
				}
			}
			break;
		}

		/* RF6_WORLD */
		case 160+6:
		{
			int who = 0;
			disturb(1, 0);
			if(m_ptr->r_idx == MON_DIO) who = 1;
			else if(m_ptr->r_idx == MON_WONG) who = 3;
			dam = who;
			if (!process_the_world(randint(2)+2, who, TRUE)) return (FALSE);
			break;
		}

		/* RF6_SPECIAL */
		case 160+7:
		{
			int k;

			disturb(1, 0);
			switch(m_ptr->r_idx)
			{
			case MON_OHMU:
				if (p_ptr->inside_arena || p_ptr->inside_battle) return FALSE;
				for (k = 0; k < 6; k++)
				{
					count += summon_specific(m_idx, m_ptr->fy, m_ptr->fx, rlev, SUMMON_BIZARRE1, TRUE, FALSE, FALSE, FALSE, FALSE);
				}
				return FALSE;
				
			case MON_BANORLUPART:
				{
					int dummy_hp = (m_ptr->hp + 1) / 2;
					int dummy_maxhp = m_ptr->maxhp/2;
					int dummy_y = m_ptr->fy;
					int dummy_x = m_ptr->fx;

					if (p_ptr->inside_arena || p_ptr->inside_battle || !summon_possible(m_ptr->fy, m_ptr->fx)) return FALSE;
					delete_monster_idx(cave[m_ptr->fy][m_ptr->fx].m_idx);
					summon_named_creature(dummy_y, dummy_x, MON_BANOR, FALSE, FALSE, is_friendly(m_ptr), FALSE);
					m_list[hack_m_idx_ii].hp = dummy_hp;
					m_list[hack_m_idx_ii].maxhp = dummy_maxhp;
					summon_named_creature(dummy_y, dummy_x, MON_LUPART, FALSE, FALSE, is_friendly(m_ptr), FALSE);
					m_list[hack_m_idx_ii].hp = dummy_hp;
					m_list[hack_m_idx_ii].maxhp = dummy_maxhp;

#ifdef JP
					msg_print("『バーノール・ルパート』が分裂した！");
#else
					msg_print("Banor=Rupart splits in two person!");
#endif

					break;
				}
				case MON_BANOR:
				case MON_LUPART:
				{
					int dummy_hp = 0;
					int dummy_maxhp = 0;
					int dummy_y = m_ptr->fy;
					int dummy_x = m_ptr->fx;

					if (!r_info[MON_BANOR].cur_num || !r_info[MON_LUPART].cur_num) return (FALSE);
					for (k = 1; k < m_max; k++)
					{
						if (m_list[k].r_idx == MON_BANOR || m_list[k].r_idx == MON_LUPART)
						{
							dummy_hp += m_list[k].hp;
							dummy_maxhp += m_list[k].maxhp;
							if (m_list[k].r_idx != m_ptr->r_idx)
							{
								dummy_y = m_list[k].fy;
								dummy_x = m_list[k].fx;
							}
							delete_monster_idx(k);
						}
					}
					summon_named_creature(dummy_y, dummy_x, MON_BANORLUPART, FALSE, FALSE, is_friendly(m_ptr), FALSE);
					m_list[hack_m_idx_ii].hp = dummy_hp;
					m_list[hack_m_idx_ii].maxhp = dummy_maxhp;

#ifdef JP
					msg_print("『バーノール』と『ルパート』が合体した！");
#else
					msg_print("Banor and Rupart combine into one!");
#endif

					break;
				}
				default: return FALSE;
			}
			break;
		}

		/* RF6_TELE_TO */
		case 160+8:
		{
			if (x!=px || y!=py) return (FALSE);
			if (!direct) break;
			disturb(1, 0);
#ifdef JP
msg_format("%^sがあなたを引き戻した。", m_name);
#else
			msg_format("%^s commands you to return.", m_name);
#endif

			teleport_player_to(m_ptr->fy, m_ptr->fx, TRUE);
			learn_spell(MS_TELE_TO);
			break;
		}

		/* RF6_TELE_AWAY */
		case 160+9:
		{
			if (x!=px || y!=py) return (FALSE);
			if (!direct) break;
			disturb(1, 0);
#ifdef JP
msg_format("%^sにテレポートさせられた。", m_name);
			if ((p_ptr->pseikaku == SEIKAKU_COMBAT) || (inventory[INVEN_BOW].name1 == ART_CRIMSON))
				msg_print("くっそ〜");
#else
			msg_format("%^s teleports you away.", m_name);
#endif

			learn_spell(MS_TELE_AWAY);
			teleport_player(100);
			break;
		}

		/* RF6_TELE_LEVEL */
		case 160+10:
		{
			if (x!=px || y!=py) return (FALSE);
			if (!direct) break;
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何か奇妙な言葉をつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles strangely.", m_name);
#endif

#ifdef JP
else msg_format("%^sがあなたの足を指さした。", m_name);
#else
			else msg_format("%^s gestures at your feet.", m_name);
#endif

			if (p_ptr->resist_nexus)
			{
#ifdef JP
msg_print("しかし効果がなかった！");
#else
				msg_print("You are unaffected!");
#endif

			}
			else if (rand_int(100 + rlev/2) < p_ptr->skill_sav)
			{
#ifdef JP
msg_print("しかし効力を跳ね返した！");
#else
				msg_print("You resist the effects!");
#endif

			}
			else
			{
				teleport_player_level();
			}
			learn_spell(MS_TELE_LEVEL);
			update_smart_learn(m_idx, DRS_NEXUS);
			break;
		}

		/* RF6_PSY_SPEAR */
		case 160+11:
		{
			if (x!=px || y!=py) return (FALSE);
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが光の剣を放った。", m_name);
#else
			else msg_format("%^s throw a Psycho-Spear.", m_name);
#endif

			dam = (r_ptr->flags2 & RF2_POWERFUL) ? (randint(rlev * 2) + 150) : (randint(rlev * 3 / 2) + 100);
			beam(m_idx, GF_PSY_SPEAR, dam, MS_PSY_SPEAR, learnable);
			break;
		}

		/* RF6_DARKNESS */
		case 160+12:
		{
			if (x!=px || y!=py) return (FALSE);
			if (!direct) break;
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else if (p_ptr->pclass == CLASS_NINJA) msg_format("%^sが辺りを明るく照らした。", m_name);
else msg_format("%^sが暗闇の中で手を振った。", m_name);
#else
			else if (p_ptr->pclass == CLASS_NINJA)
				msg_format("%^s cast a spell to light up.", m_name);
			else msg_format("%^s gestures in shadow.", m_name);
#endif

			learn_spell(MS_DARKNESS);
			if (p_ptr->pclass == CLASS_NINJA)
				(void)lite_area(0, 3);
			else
				(void)unlite_area(0, 3);
			break;
		}

		/* RF6_TRAPS */
		case 160+13:
		{
			if (!direct) break;
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいて邪悪に微笑んだ。", m_name);
#else
			if (blind) msg_format("%^s mumbles, and then cackles evilly.", m_name);
#endif

#ifdef JP
else msg_format("%^sが呪文を唱えて邪悪に微笑んだ。", m_name);
#else
			else msg_format("%^s casts a spell and cackles evilly.", m_name);
#endif

			learn_spell(MS_MAKE_TRAP);
			(void)trap_creation(y, x);
			break;
		}

		/* RF6_FORGET */
		case 160+14:
		{
			if (x!=px || y!=py) return (FALSE);
			if (!direct) break;
			disturb(1, 0);
#ifdef JP
msg_format("%^sがあなたの記憶を消去しようとしている。", m_name);
#else
			msg_format("%^s tries to blank your mind.", m_name);
#endif


			if (rand_int(100 + rlev/2) < p_ptr->skill_sav)
			{
#ifdef JP
msg_print("しかし効力を跳ね返した！");
#else
				msg_print("You resist the effects!");
#endif

			}
			else if (lose_all_info())
			{
#ifdef JP
msg_print("記憶が薄れてしまった。");
#else
				msg_print("Your memories fade away.");
#endif

			}
			learn_spell(MS_FORGET);
			break;
		}

		/* RF6_RAISE_DEAD */
		case 160+15:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが死者復活の呪文を唱えた。", m_name);
#else
			else msg_format("%^s casts a spell to revive corpses.", m_name);
#endif
			animate_dead(m_idx, m_ptr->fy, m_ptr->fx);
			break;
		}

		/* RF6_SUMMON_KIN */
		case 160+16:
		{
			disturb(1, 0);
			if (m_ptr->r_idx == MON_ROLENTO)
			{
#ifdef JP
				if (blind)
					msg_format("%^sが何か大量に投げた。", m_name);
				else 
					msg_format("%^sは手榴弾をばらまいた。", m_name);
#else
				if (blind)
					msg_format("%^s spreads something.", m_name);
				else
					msg_format("%^s throws some hand grenades.", m_name);
#endif
			}
			else if (m_ptr->r_idx == MON_SERPENT)
			{
#ifdef JP
				if (blind)
					msg_format("%^sが何かをつぶやいた。", m_name);
				else
					msg_format("%^sがダンジョンの主を召喚した。", m_name);
#else
				if (blind)
					msg_format("%^s mumbles.", m_name);
				else
					msg_format("%^s magically summons guardians of dungeons.", m_name);
#endif
			}
			else
			{
#ifdef JP
				if (blind)
					msg_format("%^sが何かをつぶやいた。", m_name);
				else
					msg_format("%^sは魔法で%sを召喚した。",
					m_name,
					((r_ptr->flags1) & RF1_UNIQUE ?
					"手下" : "仲間"));
#else
				if (blind)
					msg_format("%^s mumbles.", m_name);
				else
					msg_format("%^s magically summons %s %s.",
					m_name, m_poss,
					((r_ptr->flags1) & RF1_UNIQUE ?
					"minions" : "kin"));
#endif
			}

			if(m_ptr->r_idx == MON_ROLENTO)
			{
				int num = 1 + randint(3);
				for (k = 0; k < num; k++)
				{
					count += summon_named_creature(y, x, MON_SHURYUUDAN, FALSE, FALSE, is_friendly(m_ptr), is_pet(m_ptr));
				}
			}
			else if(m_ptr->r_idx == MON_LOUSY)
			{
				int num = 2 + randint(3);
				for (k = 0; k < num; k++)
				{
					count += summon_specific(m_idx, y, x, rlev, SUMMON_LOUSE, TRUE, FALSE, FALSE, FALSE, FALSE);
				}
			}
			else if(m_ptr->r_idx == MON_BULLGATES)
			{
				int num = 2 + randint(3);
				for (k = 0; k < num; k++)
				{
					count += summon_named_creature(y, x, 921, FALSE, FALSE, is_friendly(m_ptr), is_pet(m_ptr));
				}
			}
			else if (m_ptr->r_idx == MON_CALDARM)
			{
				int num = randint(3);
				for (k = 0; k < num; k++)
				{
					count += summon_named_creature(y, x, 930, FALSE, FALSE, is_friendly(m_ptr), is_pet(m_ptr));
				}
			}
			else if (m_ptr->r_idx == MON_SERPENT)
			{
				int num = 2 + randint(3);
				for (k = 0; k < num; k++)
				{
					count += summon_specific(m_idx, y, x, rlev, SUMMON_GUARDIANS, TRUE, FALSE, FALSE, TRUE, FALSE);
				}
			}
			else
			{

				summon_kin_type = r_ptr->d_char; /* Big hack */

				for (k = 0; k < 4; k++)
				{
					count += summon_specific(m_idx, y, x, rlev, SUMMON_KIN, TRUE, FALSE, FALSE, FALSE, FALSE);
				}
			}
#ifdef JP
if (blind && count) msg_print("多くのものが間近に現れた音がする。");
#else
			if (blind && count) msg_print("You hear many things appear nearby.");
#endif


			break;
		}

		/* RF6_S_CYBER */
		case 160+17:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sがサイバーデーモンを召喚した！", m_name);
#else
			else msg_format("%^s magically summons Cyberdemons!", m_name);
#endif

#ifdef JP
if (blind && count) msg_print("重厚な足音が近くで聞こえる。");
#else
			if (blind && count) msg_print("You hear heavy steps nearby.");
#endif

			summon_cyber(m_idx, y, x);
			break;
		}

		/* RF6_S_MONSTER */
		case 160+18:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが魔法で仲間を召喚した！", m_name);
#else
			else msg_format("%^s magically summons help!", m_name);
#endif

			for (k = 0; k < 1; k++)
			{
				count += summon_specific(m_idx, y, x, rlev, 0, TRUE, FALSE, FALSE, TRUE, FALSE);
			}
#ifdef JP
if (blind && count) msg_print("何かが間近に現れた音がする。");
#else
			if (blind && count) msg_print("You hear something appear nearby.");
#endif

			break;
		}

		/* RF6_S_MONSTERS */
		case 160+19:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが魔法でモンスターを召喚した！", m_name);
#else
			else msg_format("%^s magically summons monsters!", m_name);
#endif

			for (k = 0; k < s_num_6; k++)
			{
				count += summon_specific(m_idx, y, x, rlev, 0, TRUE, FALSE, FALSE, TRUE, FALSE);
			}
#ifdef JP
if (blind && count) msg_print("多くのものが間近に現れた音がする。");
#else
			if (blind && count) msg_print("You hear many things appear nearby.");
#endif

			break;
		}

		/* RF6_S_ANT */
		case 160+20:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが魔法でアリを召喚した。", m_name);
#else
			else msg_format("%^s magically summons ants.", m_name);
#endif

			for (k = 0; k < s_num_6; k++)
			{
				count += summon_specific(m_idx, y, x, rlev, SUMMON_ANT, TRUE, FALSE, FALSE, FALSE, FALSE);
			}
#ifdef JP
if (blind && count) msg_print("多くのものが間近に現れた音がする。");
#else
			if (blind && count) msg_print("You hear many things appear nearby.");
#endif

			break;
		}

		/* RF6_S_SPIDER */
		case 160+21:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが魔法でクモを召喚した。", m_name);
#else
			else msg_format("%^s magically summons spiders.", m_name);
#endif

			for (k = 0; k < s_num_6; k++)
			{
				count += summon_specific(m_idx, y, x, rlev, SUMMON_SPIDER, TRUE, FALSE, FALSE, FALSE, FALSE);
			}
#ifdef JP
if (blind && count) msg_print("多くのものが間近に現れた音がする。");
#else
			if (blind && count) msg_print("You hear many things appear nearby.");
#endif

			break;
		}

		/* RF6_S_HOUND */
		case 160+22:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが魔法でハウンドを召喚した。", m_name);
#else
			else msg_format("%^s magically summons hounds.", m_name);
#endif

			for (k = 0; k < s_num_4; k++)
			{
				count += summon_specific(m_idx, y, x, rlev, SUMMON_HOUND, TRUE, FALSE, FALSE, FALSE, FALSE);
			}
#ifdef JP
if (blind && count) msg_print("多くのものが間近に現れた音がする。");
#else
			if (blind && count) msg_print("You hear many things appear nearby.");
#endif

			break;
		}

		/* RF6_S_HYDRA */
		case 160+23:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが魔法でヒドラを召喚した。", m_name);
#else
			else msg_format("%^s magically summons hydras.", m_name);
#endif

			for (k = 0; k < s_num_4; k++)
			{
				count += summon_specific(m_idx, y, x, rlev, SUMMON_HYDRA, TRUE, FALSE, FALSE, FALSE, FALSE);
			}
#ifdef JP
if (blind && count) msg_print("多くのものが間近に現れた音がする。");
#else
			if (blind && count) msg_print("You hear many things appear nearby.");
#endif

			break;
		}

		/* RF6_S_ANGEL */
		case 160+24:
		{
			int num = 1;

			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが魔法で天使を召喚した！", m_name);
#else
			else msg_format("%^s magically summons an angel!", m_name);
#endif

			if ((r_ptr->flags1 & RF1_UNIQUE) && !easy_band)
			{
				num += r_ptr->level/40;
			}

			for (k = 0; k < num; k++)
			{
				count += summon_specific(m_idx, y, x, rlev, SUMMON_ANGEL, TRUE, FALSE, FALSE, FALSE, FALSE);
			}

			if (count < 2)
			{
#ifdef JP
if (blind && count) msg_print("何かが間近に現れた音がする。");
#else
				if (blind && count) msg_print("You hear something appear nearby.");
#endif
			}
			else
			{
#ifdef JP
if (blind) msg_print("多くのものが間近に現れた音がする。");
#else
				if (blind) msg_print("You hear many things appear nearby.");
#endif
			}

			break;
		}

		/* RF6_S_DEMON */
		case 160+25:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sは魔法で混沌の宮廷から悪魔を召喚した！", m_name);
#else
			else msg_format("%^s magically summons a demon from the Courts of Chaos!", m_name);
#endif

			for (k = 0; k < 1; k++)
			{
				count += summon_specific(m_idx, y, x, rlev, SUMMON_DEMON, TRUE, FALSE, FALSE, FALSE, FALSE);
			}
#ifdef JP
if (blind && count) msg_print("何かが間近に現れた音がする。");
#else
			if (blind && count) msg_print("You hear something appear nearby.");
#endif

			break;
		}

		/* RF6_S_UNDEAD */
		case 160+26:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが魔法でアンデッドの強敵を召喚した！", m_name);
#else
			else msg_format("%^s magically summons an undead adversary!", m_name);
#endif

			for (k = 0; k < 1; k++)
			{
				count += summon_specific(m_idx, y, x, rlev, SUMMON_UNDEAD, TRUE, FALSE, FALSE, FALSE, FALSE);
			}
#ifdef JP
if (blind && count) msg_print("何かが間近に現れた音がする。");
#else
			if (blind && count) msg_print("You hear something appear nearby.");
#endif

			break;
		}

		/* RF6_S_DRAGON */
		case 160+27:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが魔法でドラゴンを召喚した！", m_name);
#else
			else msg_format("%^s magically summons a dragon!", m_name);
#endif

			for (k = 0; k < 1; k++)
			{
				count += summon_specific(m_idx, y, x, rlev, SUMMON_DRAGON, TRUE, FALSE, FALSE, FALSE, FALSE);
			}
#ifdef JP
if (blind && count) msg_print("何かが間近に現れた音がする。");
#else
			if (blind && count) msg_print("You hear something appear nearby.");
#endif

			break;
		}

		/* RF6_S_HI_UNDEAD */
		case 160+28:
		{
			disturb(1, 0);

			if (((m_ptr->r_idx == MON_MORGOTH) || (m_ptr->r_idx == MON_SAURON) || (m_ptr->r_idx == MON_ANGMAR)) && ((r_info[MON_NAZGUL].cur_num+2) < r_info[MON_NAZGUL].max_num))
			{
				int cy = y;
				int cx = x;

#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
				if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが魔法で幽鬼戦隊を召喚した！", m_name);
#else
				else msg_format("%^s magically summons rangers of Nazgul!", m_name);
#endif
				msg_print(NULL);

				for (k = 0; k < 30; k++)
				{
					if (!summon_possible(cy, cx) || !cave_floor_bold(cy, cx))
					{
						int j;
						for (j = 100; j > 0; j--)
						{
							scatter(&cy, &cx, y, x, 2, 0);
							if (cave_floor_bold(cy, cx)) break;
						}
						if (!j) break;
					}
					if (!cave_floor_bold(cy, cx)) continue;

					if (summon_named_creature(cy, cx, MON_NAZGUL, FALSE, FALSE, is_friendly(m_ptr), is_pet(m_ptr)))
					{
						y = cy;
						x = cx;
						count++;
						if (count == 1)
#ifdef JP
msg_format("「幽鬼戦隊%d号、ナズグル・ブラック！」", count);
#else
							msg_format("A Nazgul says 'Nazgul-Rangers Number %d, Nazgul-Black!'",count);
#endif
						else
#ifdef JP
msg_format("「同じく%d号、ナズグル・ブラック！」", count);
#else
							msg_format("Another one says 'Number %d, Nazgul-Black!'",count);
#endif
						msg_print(NULL);
					}
				}
#ifdef JP
msg_format("「%d匹そろって、リングレンジャー！」", count);
#else
msg_format("They say 'The %d meets! We are the Ring-Ranger!'.", count);
#endif
				msg_print(NULL);
			}
			else
			{
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
				if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが魔法で強力なアンデッドを召喚した！", m_name);
#else
				else msg_format("%^s magically summons greater undead!", m_name);
#endif

				for (k = 0; k < s_num_6; k++)
				{
					count += summon_specific(m_idx, y, x, rlev, SUMMON_HI_UNDEAD, TRUE, FALSE, FALSE, TRUE, FALSE);
				}
			}
			if (blind && count)
			{
#ifdef JP
msg_print("間近で何か多くのものが這い回る音が聞こえる。");
#else
				msg_print("You hear many creepy things appear nearby.");
#endif

			}
			break;
		}

		/* RF6_S_HI_DRAGON */
		case 160+29:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが魔法で古代ドラゴンを召喚した！", m_name);
#else
			else msg_format("%^s magically summons ancient dragons!", m_name);
#endif

			for (k = 0; k < s_num_4; k++)
			{
				count += summon_specific(m_idx, y, x, rlev, SUMMON_HI_DRAGON, TRUE, FALSE, FALSE, TRUE, FALSE);
			}
			if (blind && count)
			{
#ifdef JP
msg_print("多くの力強いものが間近に現れた音が聞こえる。");
#else
				msg_print("You hear many powerful things appear nearby.");
#endif

			}
			break;
		}

		/* RF6_S_AMBERITES */
		case 160+30:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sがアンバーの王を召喚した！", m_name);
#else
			else msg_format("%^s magically summons Lords of Amber!", m_name);
#endif



			for (k = 0; k < s_num_4; k++)
			{
				count += summon_specific(m_idx, y, x, rlev, SUMMON_AMBERITES, TRUE, FALSE, FALSE, TRUE, FALSE);
			}
			if (blind && count)
			{
#ifdef JP
msg_print("不死の者が近くに現れるのが聞こえた。");
#else
				msg_print("You hear immortal beings appear nearby.");
#endif

			}
			break;
		}

		/* RF6_S_UNIQUE */
		case 160+31:
		{
			disturb(1, 0);
#ifdef JP
if (blind) msg_format("%^sが何かをつぶやいた。", m_name);
#else
			if (blind) msg_format("%^s mumbles.", m_name);
#endif

#ifdef JP
else msg_format("%^sが魔法で特別な強敵を召喚した！", m_name);
#else
			else msg_format("%^s magically summons special opponents!", m_name);
#endif

			for (k = 0; k < s_num_4; k++)
			{
				count += summon_specific(m_idx, y, x, rlev, SUMMON_UNIQUE, TRUE, FALSE, FALSE, TRUE, FALSE);
			}
			if (r_ptr->flags3 & RF3_GOOD)
			{
				for (k = count; k < s_num_4; k++)
				{
					count += summon_specific(m_idx, y, x, rlev, SUMMON_ANGEL, TRUE, FALSE, FALSE, TRUE, FALSE);
				}
			}
			else
			{
				for (k = count; k < s_num_4; k++)
				{
					count += summon_specific(m_idx, y, x, rlev, SUMMON_HI_UNDEAD, TRUE, FALSE, FALSE, TRUE, FALSE);
				}
			}
			if (blind && count)
			{
#ifdef JP
msg_print("多くの力強いものが間近に現れた音が聞こえる。");
#else
				msg_print("You hear many powerful things appear nearby.");
#endif

			}
			break;
		}
	}

	if ((p_ptr->action == ACTION_LEARN) && thrown_spell > 175)
	{
		learn_spell(thrown_spell - 96);
	}

	if (seen && maneable && !world_monster && (p_ptr->pclass == CLASS_IMITATOR))
	{
		if (thrown_spell != 167)
		{
			if (mane_num == MAX_MANE)
			{
				int i;
				mane_num--;
				for (i = 0;i < mane_num;i++)
				{
					mane_spell[i] = mane_spell[i+1];
					mane_dam[i] = mane_dam[i+1];
				}
			}
			mane_spell[mane_num] = thrown_spell - 96;
			mane_dam[mane_num] = dam;
			mane_num++;
			new_mane = TRUE;

			p_ptr->redraw |= (PR_MANE);
		}
	}

	/* Remember what the monster did to us */
	if (seen)
	{
		/* Inate spell */
		if (thrown_spell < 32 * 4)
		{
			r_ptr->r_flags4 |= (1L << (thrown_spell - 32 * 3));
			if (r_ptr->r_cast_inate < MAX_UCHAR) r_ptr->r_cast_inate++;
		}

		/* Bolt or Ball */
		else if (thrown_spell < 32 * 5)
		{
			r_ptr->r_flags5 |= (1L << (thrown_spell - 32 * 4));
			if (r_ptr->r_cast_spell < MAX_UCHAR) r_ptr->r_cast_spell++;
		}

		/* Special spell */
		else if (thrown_spell < 32 * 6)
		{
			r_ptr->r_flags6 |= (1L << (thrown_spell - 32 * 5));
			if (r_ptr->r_cast_spell < MAX_UCHAR) r_ptr->r_cast_spell++;
		}
	}


	/* Always take note of monsters that kill you */
	if (death && (r_ptr->r_deaths < MAX_SHORT))
	{
		r_ptr->r_deaths++;
	}

	/* A spell was cast */
	return (TRUE);
}
