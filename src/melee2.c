/* File: melee2.c */

/* Purpose: Monster spells and movement */

/*
* Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
*
* This software may be copied and distributed for educational, research, and
* not for profit purposes provided that this copyright and statement are
* included in all such copies.
*/

/*
* This file has several additions to it by Keldon Jones (keldon@umr.edu)
* to improve the general quality of the AI (version 0.1.1).
*/

#include "angband.h"

#define SPEAK_CHANCE 8
#define GRINDNOISE 20
#define CYBERNOISE 20

/*
 * Calculate the direction to the next enemy
 */
static bool get_enemy_dir(int m_idx, int *mm)
{
	int i;
	int x = 0, y = 0;
	int t_idx;
	int start;
	int plus = 1;

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	monster_type *t_ptr;

	if (riding_t_m_idx && (m_ptr->fx == px) && (m_ptr->fy == py))
	{
		y = m_list[riding_t_m_idx].fy;
		x = m_list[riding_t_m_idx].fx;
	}
	else if (is_pet(m_ptr) && pet_t_m_idx)
	{
		y = m_list[pet_t_m_idx].fy;
		x = m_list[pet_t_m_idx].fx;
	}
	else
	{
		if (p_ptr->inside_battle)
		{
			start = randint1(m_max-1)+m_max;
			if(randint0(2)) plus = -1;
		}
		else start = m_max + 1;

		/* Scan thru all monsters */
		for (i = start; ((i < start + m_max) && (i > start - m_max)); i+=plus)
		{
			int dummy = (i % m_max);
			bool can_pass_wall;

			if (!dummy) continue;

			t_idx = dummy;
			t_ptr = &m_list[t_idx];

			/* The monster itself isn't a target */
			if (t_ptr == m_ptr) continue;

			/* Paranoia -- Skip dead monsters */
			if (!t_ptr->r_idx) continue;

			if (is_pet(m_ptr))
			{
				/* Hack -- only fight away from player */
				if (p_ptr->pet_follow_distance < 0)
				{
					/* No fighting near player */
					if (t_ptr->cdis <= (0 - p_ptr->pet_follow_distance))
					{
						continue;
					}
				}
				/* Hack -- no fighting away from player */
				else if ((m_ptr->cdis < t_ptr->cdis) &&
							(t_ptr->cdis > p_ptr->pet_follow_distance))
				{
					continue;
				}

				if (r_ptr->aaf < t_ptr->cdis) continue;
			}

			/* Monster must be 'an enemy' */
			if (!are_enemies(m_ptr, t_ptr)) continue;

			can_pass_wall = (((r_ptr->flags2 & RF2_PASS_WALL) && ((m_idx != p_ptr->riding) || (p_ptr->pass_wall))) || ((r_ptr->flags2 & RF2_KILL_WALL) && (m_idx != p_ptr->riding)));

			/* Monster must be projectable if we can't pass through walls */
			if (!can_pass_wall &&
				!projectable(m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx))
			{
				continue;
			}
			if (can_pass_wall &&
				!in_disintegration_range(m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx))
			{
				continue;
			}

			/* OK -- we've got a target */
			y = t_ptr->fy;
			x = t_ptr->fx;

			break;
		}
		if (!x && !y) return FALSE;
	}

	/* Extract the direction */
	x -= m_ptr->fx;
	y -= m_ptr->fy;

	/* North */
	if ((y < 0) && (x == 0))
	{
		mm[0] = 8;
		mm[1] = 7;
		mm[2] = 9;
	}
	/* South */
	else if ((y > 0) && (x == 0))
	{
		mm[0] = 2;
		mm[1] = 1;
		mm[2] = 3;
	}
	/* East */
	else if ((x > 0) && (y == 0))
	{
		mm[0] = 6;
		mm[1] = 9;
		mm[2] = 3;
	}
	/* West */
	else if ((x < 0) && (y == 0))
	{
		mm[0] = 4;
		mm[1] = 7;
		mm[2] = 1;
	}
	/* North-West */
	else if ((y < 0) && (x < 0))
	{
		mm[0] = 7;
		mm[1] = 4;
		mm[2] = 8;
	}
	/* North-East */
	else if ((y < 0) && (x > 0))
	{
		mm[0] = 9;
		mm[1] = 6;
		mm[2] = 8;
	}
	/* South-West */
	else if ((y > 0) && (x < 0))
	{
		mm[0] = 1;
		mm[1] = 4;
		mm[2] = 2;
	}
	/* South-East */
	else if ((y > 0) && (x > 0))
	{
		mm[0] = 3;
		mm[1] = 6;
		mm[2] = 2;
	}

	/* Found a monster */
	return TRUE;
}


/*
 * Hack, based on mon_take_hit... perhaps all monster attacks on
 * other monsters should use this?
 */
void mon_take_hit_mon(bool is_psy_spear, int m_idx, int dam, bool *fear, cptr note, int who)
{
	monster_type	*m_ptr = &m_list[m_idx];

	monster_race	*r_ptr = &r_info[m_ptr->r_idx];

	char m_name[160];

	bool seen = m_ptr->ml;

	/* Can the player be aware of this attack? */
	bool known = (m_ptr->cdis <= MAX_SIGHT);

	/* Extract monster name */
	monster_desc(m_name, m_ptr, 0);

	/* Redraw (later) if needed */
	if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
	if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);

	/* Wake it up */
	m_ptr->csleep = 0;

	if (p_ptr->riding && (m_idx == p_ptr->riding)) disturb(1, 0);

	if (m_ptr->invulner && randint0(PENETRATE_INVULNERABILITY))
	{
		if (seen)
		{
#ifdef JP
msg_format("%^sはダメージを受けない。", m_name);
#else
			msg_format("%^s is unharmed.", m_name);
#endif

		}

		return;
	}

	if (r_ptr->flags3 & RF3_RES_ALL)
	{
		if(dam > 0)
		{
			dam /= 100;
			if((dam == 0) && one_in_(3)) dam = 1;
		}
		if (dam==0)
		{
			if (seen)
			{
#ifdef JP
msg_format("%^sはダメージを受けない。", m_name);
#else
				msg_format("%^s is unharmed.", m_name);
#endif

			}
			return;
		}
	}

	/* Hurt it */
	m_ptr->hp -= dam;

	/* It is dead now... or is it? */
	if (m_ptr->hp < 0)
	{
		if (((r_ptr->flags1 & RF1_UNIQUE) ||
			(r_ptr->flags7 & RF7_UNIQUE_7) ||
			(r_ptr->flags1 & RF1_QUESTOR)) &&
		    !p_ptr->inside_battle)
		{
			m_ptr->hp = 1;
		}
		else
		{
			/* Make a sound */
			if (!monster_living(r_ptr))
			{
				sound(SOUND_N_KILL);
			}
			else
			{
				sound(SOUND_KILL);
			}

			if (known)
			{
				monster_desc(m_name, m_ptr, 0x100);
				/* Unseen death by normal attack */
				if (!seen)
				{
					mon_fight = TRUE;
				}
				/* Death by special attack */
				else if (note)
				{
#ifdef JP
msg_format("%^s%s", m_name, note);
#else
					msg_format("%^s%s", m_name, note);
#endif

				}
				/* Death by normal attack -- nonliving monster */
				else if (!monster_living(r_ptr))
				{
#ifdef JP
msg_format("%^sは破壊された。", m_name);
#else
					msg_format("%^s is destroyed.", m_name);
#endif

				}
				/* Death by normal attack -- living monster */
				else
				{
#ifdef JP
msg_format("%^sは殺された。", m_name);
#else
					msg_format("%^s is killed.", m_name);
#endif

				}
			}

			monster_gain_exp(who, m_ptr->r_idx);

			/* Generate treasure */
			monster_death(m_idx, FALSE);

			/* Delete the monster */
			delete_monster_idx(m_idx);

			/* Not afraid */
			(*fear) = FALSE;

			/* Monster is dead */
			return;
		}
	}

#ifdef ALLOW_FEAR

	/* Mega-Hack -- Pain cancels fear */
	if (m_ptr->monfear && (dam > 0))
	{
		int tmp = randint1(dam / 4);

		/* Cure a little fear */
		if (tmp < m_ptr->monfear)
		{
			/* Reduce fear */
			m_ptr->monfear -= tmp;
		}

		/* Cure all the fear */
		else
		{
			/* Cure fear */
			m_ptr->monfear = 0;

			/* No more fear */
			(*fear) = FALSE;
		}
	}

	/* Sometimes a monster gets scared by damage */
	if (!m_ptr->monfear && !(r_ptr->flags3 & RF3_NO_FEAR))
	{
		int		percentage;

		/* Percentage of fully healthy */
		percentage = (100L * m_ptr->hp) / m_ptr->maxhp;

		/*
		* Run (sometimes) if at 10% or less of max hit points,
		* or (usually) when hit for half its current hit points
		 */
		if (((percentage <= 10) && (randint0(10) < percentage)) ||
			((dam >= m_ptr->hp) && (randint0(100) < 80)))
		{
			/* Hack -- note fear */
			(*fear) = TRUE;

			/* XXX XXX XXX Hack -- Add some timed fear */
			m_ptr->monfear += (randint1(10) +
				(((dam >= m_ptr->hp) && (percentage > 7)) ?
				20 : ((11 - percentage) * 5)));
		}
	}

#endif /* ALLOW_FEAR */

	if ((dam > 0) && !is_pet(m_ptr) && !is_friendly(m_ptr) && (who != m_idx))
	{
		if (is_pet(&m_list[who]) && (m_ptr->target_y != py) && (m_ptr->target_x != px))
		{
			set_target(m_ptr, m_list[who].fy, m_list[who].fx);
		}
	}

	if (p_ptr->riding && (p_ptr->riding == m_idx) && (dam > 0))
	{
		char m_name[80];

		/* Extract monster name */
		monster_desc(m_name, m_ptr, 0);

		if (m_ptr->hp > m_ptr->maxhp/3) dam = (dam + 1) / 2;
		if (rakuba((dam > 200) ? 200 : dam, FALSE))
		{
#ifdef JP
msg_format("%^sに振り落とされた！", m_name);
#else
				msg_format("You have thrown off from %s!", m_name);
#endif
		}
	}

	/* Not dead yet */
	return;
}


/*
 * Returns whether a given monster will try to run from the player.
 *
 * Monsters will attempt to avoid very powerful players.  See below.
 *
 * Because this function is called so often, little details are important
 * for efficiency.  Like not using "mod" or "div" when possible.  And
 * attempting to check the conditions in an optimal order.  Note that
 * "(x << 2) == (x * 4)" if "x" has enough bits to hold the result.
 *
 * Note that this function is responsible for about one to five percent
 * of the processor use in normal conditions...
 */
static int mon_will_run(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];

#ifdef ALLOW_TERROR

	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	u16b p_lev, m_lev;
	u16b p_chp, p_mhp;
	u16b m_chp, m_mhp;
	u32b p_val, m_val;

#endif

	/* Friends can be commanded to avoid the player */
	if (is_pet(m_ptr))
	{
		/* Are we trying to avoid the player? */
		return ((p_ptr->pet_follow_distance < 0) &&
				  (m_ptr->cdis <= (0 - p_ptr->pet_follow_distance)));
	}

	/* Keep monsters from running too far away */
	if (m_ptr->cdis > MAX_SIGHT + 5) return (FALSE);

	/* All "afraid" monsters will run away */
	if (m_ptr->monfear) return (TRUE);

#ifdef ALLOW_TERROR

	/* Nearby monsters will not become terrified */
	if (m_ptr->cdis <= 5) return (FALSE);

	/* Examine player power (level) */
	p_lev = p_ptr->lev;

	/* Examine monster power (level plus morale) */
	m_lev = r_ptr->level + (m_idx & 0x08) + 25;

	/* Optimize extreme cases below */
	if (m_lev > p_lev + 4) return (FALSE);
	if (m_lev + 4 <= p_lev) return (TRUE);

	/* Examine player health */
	p_chp = p_ptr->chp;
	p_mhp = p_ptr->mhp;

	/* Examine monster health */
	m_chp = m_ptr->hp;
	m_mhp = m_ptr->maxhp;

	/* Prepare to optimize the calculation */
	p_val = (p_lev * p_mhp) + (p_chp << 2); /* div p_mhp */
	m_val = (m_lev * m_mhp) + (m_chp << 2); /* div m_mhp */

	/* Strong players scare strong monsters */
	if (p_val * m_mhp > m_val * p_mhp) return (TRUE);

#endif

	/* Assume no terror */
	return (FALSE);
}




/*
 * Search spell castable grid
 */
static bool get_moves_aux2(int m_idx, int *yp, int *xp)
{
	int i, y, x, y1, x1, best = 999;

	cave_type *c_ptr;
	bool can_open_door = FALSE;
	int now_cost;

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Monster location */
	y1 = m_ptr->fy;
	x1 = m_ptr->fx;

	/* Monster can already cast spell to player */
	if (projectable(y1, x1, py, px)) return (FALSE);

	/* Set current grid cost */
	now_cost = cave[y1][x1].cost;
	if (now_cost == 0) now_cost = 999;

	/* Can monster bash or open doors? */
	if (r_ptr->flags2 & (RF2_BASH_DOOR | RF2_OPEN_DOOR))
	{
		can_open_door = TRUE;
	}

	/* Check nearby grids, diagonals first */
	for (i = 7; i >= 0; i--)
	{
		int cost;

		/* Get the location */
		y = y1 + ddy_ddd[i];
		x = x1 + ddx_ddd[i];

		/* Ignore locations off of edge */
		if (!in_bounds2(y, x)) continue;

		/* Simply move to player */
		if ((y == py) && (x == px)) return (FALSE);

		c_ptr = &cave[y][x];

		cost = c_ptr->cost;

		/* Monster cannot kill or pass walls */
		if (!(((r_ptr->flags2 & RF2_PASS_WALL) && ((m_idx != p_ptr->riding) || p_ptr->pass_wall)) || (r_ptr->flags2 & RF2_KILL_WALL)))
		{
			if (cost == 0) continue;
			if (!can_open_door && (c_ptr->feat >= FEAT_DOOR_HEAD && c_ptr->feat <= FEAT_SECRET)) continue;
		}

		/* Hack -- for kill or pass wall monster.. */
		if (cost == 0) cost = 998;

		if (now_cost < cost) continue;

		if (!projectable(y, x, py, px)) continue;

		/* Accept louder sounds */
		if (best < cost) continue;
		best = cost;

		(*yp) = y1 + ddy_ddd[i];
		(*xp) = x1 + ddx_ddd[i];
	}

	/* No legal move (?) */
	if (best == 999) return (FALSE);

	/* Success */
	return (TRUE);
}


/*
 * Choose the "best" direction for "flowing"
 *
 * Note that ghosts and rock-eaters are never allowed to "flow",
 * since they should move directly towards the player.
 *
 * Prefer "non-diagonal" directions, but twiddle them a little
 * to angle slightly towards the player's actual location.
 *
 * Allow very perceptive monsters to track old "spoor" left by
 * previous locations occupied by the player.  This will tend
 * to have monsters end up either near the player or on a grid
 * recently occupied by the player (and left via "teleport").
 *
 * Note that if "smell" is turned on, all monsters get vicious.
 *
 * Also note that teleporting away from a location will cause
 * the monsters who were chasing you to converge on that location
 * as long as you are still near enough to "annoy" them without
 * being close enough to chase directly.  I have no idea what will
 * happen if you combine "smell" with low "aaf" values.
 */
static bool get_moves_aux(int m_idx, int *yp, int *xp, bool no_flow)
{
	int i, y, x, y1, x1, best;

	cave_type *c_ptr;
	bool use_sound = FALSE;
	bool use_scent = FALSE;

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Monster flowing disabled */
	if (stupid_monsters) return (FALSE);

	/* Can monster cast attack spell? */
	if (r_ptr->flags4 & (RF4_ATTACK_MASK) ||
	    r_ptr->flags5 & (RF5_ATTACK_MASK) ||
	    r_ptr->flags6 & (RF6_ATTACK_MASK))
	{
		/* Can move spell castable grid? */
		if (get_moves_aux2(m_idx, yp, xp)) return (TRUE);
	}

	/* Monster can't flow */
	if (no_flow) return (FALSE);

	/* Monster can go through rocks */
	if ((r_ptr->flags2 & RF2_PASS_WALL) && ((m_idx != p_ptr->riding) || (p_ptr->pass_wall))) return (FALSE);
	if (r_ptr->flags2 & RF2_KILL_WALL) return (FALSE);
	if (!cave_floor_bold(py, px) && (cave[py][px].feat != FEAT_TREES)) return (FALSE);

	/* Monster location */
	y1 = m_ptr->fy;
	x1 = m_ptr->fx;

	/* Hack -- Player can see us, run towards him */
	if (player_has_los_bold(y1, x1)) return (FALSE);

	/* Monster grid */
	c_ptr = &cave[y1][x1];

	/* If we can hear noises, advance towards them */
	if (c_ptr->cost)
	{
		use_sound = TRUE;
		best = 999;
	}

	/* Otherwise, try to follow a scent trail */
	else if (c_ptr->when)
	{
		/* Too old smell */
		if (cave[py][px].when - c_ptr->when > 127) return (FALSE);

		use_scent = TRUE;
		best = 0;
	}

	/* Otherwise, advance blindly */
	else
	{
		return (FALSE);
	}

	/* Check nearby grids, diagonals first */
	for (i = 7; i >= 0; i--)
	{
		/* Get the location */
		y = y1 + ddy_ddd[i];
		x = x1 + ddx_ddd[i];

		/* Ignore locations off of edge */
		if (!in_bounds2(y, x)) continue;

		c_ptr = &cave[y][x];

		/* We're following a scent trail */
		if (use_scent)
		{
			int when = c_ptr->when;

			/* Accept younger scent */
			if (best > when) continue;
			best = when;
		}

		/* We're using sound */
		else
		{
			int cost;

			if (r_ptr->flags2 & (RF2_BASH_DOOR | RF2_OPEN_DOOR))
				cost = c_ptr->dist;
			else cost = c_ptr->cost;

			/* Accept louder sounds */
			if ((cost == 0) || (best < cost)) continue;
			best = cost;
		}

		/* Hack -- Save the "twiddled" location */
		(*yp) = py + 16 * ddy_ddd[i];
		(*xp) = px + 16 * ddx_ddd[i];
	}

	/* No legal move (?) */
	if (best == 999 || best == 0) return (FALSE);

	/* Success */
	return (TRUE);
}


/*
* Provide a location to flee to, but give the player a wide berth.
*
* A monster may wish to flee to a location that is behind the player,
* but instead of heading directly for it, the monster should "swerve"
* around the player so that he has a smaller chance of getting hit.
*/
static bool get_fear_moves_aux(int m_idx, int *yp, int *xp)
{
	int y, x, y1, x1, fy, fx, gy = 0, gx = 0;
	int dist = 0, score = -1;
	int i;

	monster_type *m_ptr = &m_list[m_idx];

	/* Monster flowing disabled */
	if (stupid_monsters) return (FALSE);

	/* Monster location */
	fy = m_ptr->fy;
	fx = m_ptr->fx;

	/* Desired destination */
	y1 = fy - (*yp);
	x1 = fx - (*xp);

	/* Check nearby grids, diagonals first */
	for (i = 7; i >= 0; i--)
	{
		int dis, s;

		/* Get the location */
		y = fy + ddy_ddd[i];
		x = fx + ddx_ddd[i];

		/* Ignore locations off of edge */
		if (!in_bounds2(y, x)) continue;

		if (cave[y][x].dist < dist) continue;

		/* Calculate distance of this grid from our destination */
		dis = distance(y, x, y1, x1);

		/* Score this grid */
		s = 5000 / (dis + 3) - 500 / (cave[y][x].dist + 1);

		/* No negative scores */
		if (s < 0) s = 0;

		/* Ignore lower scores */
		if (s < score) continue;

		/* Save the score and time */
		dist = cave[y][x].dist;
		score = s;

		/* Save the location */
		gy = y;
		gx = x;
	}

	/* No legal move (?) */
	if (score == -1) return (FALSE);

	/* Find deltas */
	(*yp) = fy - gy;
	(*xp) = fx - gx;

	/* Success */
	return (TRUE);
}

/*
 * Hack -- Precompute a bunch of calls to distance() in find_safety() and
 * find_hiding().
 *
 * The pair of arrays dist_offsets_y[n] and dist_offsets_x[n] contain the
 * offsets of all the locations with a distance of n from a central point,
 * with an offset of (0,0) indicating no more offsets at this distance.
 *
 * This is, of course, fairly unreadable, but it eliminates multiple loops
 * from the previous version.
 *
 * It is probably better to replace these arrays with code to compute
 * the relevant arrays, even if the storage is pre-allocated in hard
 * coded sizes.  At the very least, code should be included which is
 * able to generate and dump these arrays (ala "los()").  XXX XXX XXX
 *
 * Also, the storage needs could be halved by using bytes.  XXX XXX XXX
 *
 * These arrays could be combined into two big arrays, using sub-arrays
 * to hold the offsets and lengths of each portion of the sub-arrays, and
 * this could perhaps also be used somehow in the "look" code.  XXX XXX XXX
 */


static sint d_off_y_0[] =
{ 0 };

static sint d_off_x_0[] =
{ 0 };


static sint d_off_y_1[] =
{ -1, -1, -1, 0, 0, 1, 1, 1, 0 };

static sint d_off_x_1[] =
{ -1, 0, 1, -1, 1, -1, 0, 1, 0 };


static sint d_off_y_2[] =
{ -1, -1, -2, -2, -2, 0, 0, 1, 1, 2, 2, 2, 0 };

static sint d_off_x_2[] =
{ -2, 2, -1, 0, 1, -2, 2, -2, 2, -1, 0, 1, 0 };


static sint d_off_y_3[] =
{ -1, -1, -2, -2, -3, -3, -3, 0, 0, 1, 1, 2, 2,
  3, 3, 3, 0 };

static sint d_off_x_3[] =
{ -3, 3, -2, 2, -1, 0, 1, -3, 3, -3, 3, -2, 2,
  -1, 0, 1, 0 };


static sint d_off_y_4[] =
{ -1, -1, -2, -2, -3, -3, -3, -3, -4, -4, -4, 0,
  0, 1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 0 };

static sint d_off_x_4[] =
{ -4, 4, -3, 3, -2, -3, 2, 3, -1, 0, 1, -4, 4,
  -4, 4, -3, 3, -2, -3, 2, 3, -1, 0, 1, 0 };


static sint d_off_y_5[] =
{ -1, -1, -2, -2, -3, -3, -4, -4, -4, -4, -5, -5,
  -5, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 4, 5, 5,
  5, 0 };

static sint d_off_x_5[] =
{ -5, 5, -4, 4, -4, 4, -2, -3, 2, 3, -1, 0, 1,
  -5, 5, -5, 5, -4, 4, -4, 4, -2, -3, 2, 3, -1,
  0, 1, 0 };


static sint d_off_y_6[] =
{ -1, -1, -2, -2, -3, -3, -4, -4, -5, -5, -5, -5,
  -6, -6, -6, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5,
  5, 5, 6, 6, 6, 0 };

static sint d_off_x_6[] =
{ -6, 6, -5, 5, -5, 5, -4, 4, -2, -3, 2, 3, -1,
  0, 1, -6, 6, -6, 6, -5, 5, -5, 5, -4, 4, -2,
  -3, 2, 3, -1, 0, 1, 0 };


static sint d_off_y_7[] =
{ -1, -1, -2, -2, -3, -3, -4, -4, -5, -5, -5, -5,
  -6, -6, -6, -6, -7, -7, -7, 0, 0, 1, 1, 2, 2, 3,
  3, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 0 };

static sint d_off_x_7[] =
{ -7, 7, -6, 6, -6, 6, -5, 5, -4, -5, 4, 5, -2,
  -3, 2, 3, -1, 0, 1, -7, 7, -7, 7, -6, 6, -6,
  6, -5, 5, -4, -5, 4, 5, -2, -3, 2, 3, -1, 0,
  1, 0 };


static sint d_off_y_8[] =
{ -1, -1, -2, -2, -3, -3, -4, -4, -5, -5, -6, -6,
  -6, -6, -7, -7, -7, -7, -8, -8, -8, 0, 0, 1, 1,
  2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7,
  8, 8, 8, 0 };

static sint d_off_x_8[] =
{ -8, 8, -7, 7, -7, 7, -6, 6, -6, 6, -4, -5, 4,
  5, -2, -3, 2, 3, -1, 0, 1, -8, 8, -8, 8, -7,
  7, -7, 7, -6, 6, -6, 6, -4, -5, 4, 5, -2, -3,
  2, 3, -1, 0, 1, 0 };


static sint d_off_y_9[] =
{ -1, -1, -2, -2, -3, -3, -4, -4, -5, -5, -6, -6,
  -7, -7, -7, -7, -8, -8, -8, -8, -9, -9, -9, 0,
  0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 7,
  7, 8, 8, 8, 8, 9, 9, 9, 0 };

static sint d_off_x_9[] =
{ -9, 9, -8, 8, -8, 8, -7, 7, -7, 7, -6, 6, -4,
  -5, 4, 5, -2, -3, 2, 3, -1, 0, 1, -9, 9, -9,
  9, -8, 8, -8, 8, -7, 7, -7, 7, -6, 6, -4, -5,
  4, 5, -2, -3, 2, 3, -1, 0, 1, 0 };


static sint *dist_offsets_y[10] =
{
	d_off_y_0, d_off_y_1, d_off_y_2, d_off_y_3, d_off_y_4,
	d_off_y_5, d_off_y_6, d_off_y_7, d_off_y_8, d_off_y_9
};

static sint *dist_offsets_x[10] =
{
	d_off_x_0, d_off_x_1, d_off_x_2, d_off_x_3, d_off_x_4,
	d_off_x_5, d_off_x_6, d_off_x_7, d_off_x_8, d_off_x_9
};

/*
* Choose a "safe" location near a monster for it to run toward.
*
* A location is "safe" if it can be reached quickly and the player
* is not able to fire into it (it isn't a "clean shot").  So, this will
* cause monsters to "duck" behind walls.  Hopefully, monsters will also
* try to run towards corridor openings if they are in a room.
*
* This function may take lots of CPU time if lots of monsters are
* fleeing.
*
* Return TRUE if a safe location is available.
*/
static bool find_safety(int m_idx, int *yp, int *xp)
{
	monster_type *m_ptr = &m_list[m_idx];

	int fy = m_ptr->fy;
	int fx = m_ptr->fx;

	int y, x, dy, dx, d, dis, i;
	int gy = 0, gx = 0, gdis = 0;

	sint *y_offsets;
	sint *x_offsets;
	
	cave_type *c_ptr;

	/* Start with adjacent locations, spread further */
	for (d = 1; d < 10; d++)
	{
		/* Get the lists of points with a distance d from (fx, fy) */
		y_offsets = dist_offsets_y[d];
		x_offsets = dist_offsets_x[d];
		
		/* Check the locations */
		for (i = 0, dx = x_offsets[0], dy = y_offsets[0];
		     dx != 0 || dy != 0;
		     i++, dx = x_offsets[i], dy = y_offsets[i])
		{
			y = fy + dy;
			x = fx + dx;
			
			/* Skip illegal locations */
			if (!in_bounds(y, x)) continue;

			c_ptr = &cave[y][x];

			/* Skip locations in a wall */
			if (!cave_floor_grid(c_ptr)) continue;

			/* Check for "availability" (if monsters can flow) */
			if (!stupid_monsters && !(m_ptr->mflag2 & MFLAG_NOFLOW))
			{
				/* Ignore grids very far from the player */
				if (c_ptr->when < cave[py][px].when) continue;

				/* Ignore too-distant grids */
				if (c_ptr->dist > cave[fy][fx].dist + 2 * d) continue;
			}
			
			/* Check for absence of shot (more or less) */
			if (clean_shot(fy, fx, y, x, FALSE))
			{
							
				/* Calculate distance from player */
				dis = distance(y, x, py, px);

				/* Remember if further than previous */
				if (dis > gdis)
				{
					gy = y;
					gx = x;
					if (!player_has_los_grid(c_ptr))
					{
						gdis = dis * 5;
					}
					else
					{
						gdis = dis;
					}
				}
			}
		}

		/* Check for success */
		if (gdis > d + m_ptr->cdis)
		{
			/* Good location */
			(*yp) = fy - gy;
			(*xp) = fx - gx;

			/* Found safe place */
			return (TRUE);
		}
	}

	/* No safe place */
	
	/* Save farthest location from player in LOS of monster */
	(*yp) = fy - gy;
	(*xp) = fx - gx;
	
	/* Hack - return TRUE anyway. */
	return (TRUE);
}


/*
 * Choose a good hiding place near a monster for it to run toward.
 *
 * Pack monsters will use this to "ambush" the player and lure him out
 * of corridors into open space so they can swarm him.
 *
 * Return TRUE if a good location is available.
 */
static bool find_hiding(int m_idx, int *yp, int *xp)
{
	monster_type *m_ptr = &m_list[m_idx];

	int fy = m_ptr->fy;
	int fx = m_ptr->fx;

	int y, x, dy, dx, d, dis, i;
	int gy = 0, gx = 0, gdis = 999;

	sint *y_offsets, *x_offsets;
	
	cave_type *c_ptr;

	/* Start with adjacent locations, spread further */
	for (d = 1; d < 10; d++)
	{
		/* Get the lists of points with a distance d from (fx, fy) */
		y_offsets = dist_offsets_y[d];
		x_offsets = dist_offsets_x[d];

		/* Check the locations */
		for (i = 0, dx = x_offsets[0], dy = y_offsets[0];
		     dx != 0 || dy != 0;
		     i++, dx = x_offsets[i], dy = y_offsets[i])
		{
			y = fy + dy;
			x = fx + dx;
			
			/* Skip illegal locations */
			if (!in_bounds(y, x)) continue;

			c_ptr = &cave[y][x];

			/* Skip occupied locations */
			if (!cave_empty_grid(c_ptr)) continue;

			/* Check for hidden, available grid */
			if (!player_has_los_grid(c_ptr) && clean_shot(fy, fx, y, x, FALSE))
			{
				/* Calculate distance from player */
				dis = distance(y, x, py, px);

				/* Remember if closer than previous */
				if (dis < gdis && dis >= 2)
				{
					gy = y;
					gx = x;
					gdis = dis;
				}
			}
		}

		/* Check for success */
		if (gdis < 999)
		{
			/* Good location */
			(*yp) = fy - gy;
			(*xp) = fx - gx;

			/* Found good place */
			return (TRUE);
		}
	}

	/* No good place */
	return (FALSE);
}


/*
 * Choose "logical" directions for monster movement
 */
static bool get_moves(int m_idx, int *mm)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	int          y, ay, x, ax;
	int          move_val = 0;
	int          y2 = py;
	int          x2 = px;
	bool         done = FALSE;
	bool         will_run = mon_will_run(m_idx);
	cave_type	*c_ptr;
	bool         no_flow = ((m_ptr->mflag2 & MFLAG_NOFLOW) && (cave[m_ptr->fy][m_ptr->fx].cost > 2));
	bool         can_pass_wall;

	/* Flow towards the player */
	if (!stupid_monsters)
	{
		/* Flow towards the player */
		(void)get_moves_aux(m_idx, &y2, &x2, no_flow);
	}

	can_pass_wall = ((r_ptr->flags2 & RF2_PASS_WALL) && ((m_idx != p_ptr->riding) || (p_ptr->pass_wall)));

	/* Extract the "pseudo-direction" */
	y = m_ptr->fy - y2;
	x = m_ptr->fx - x2;

	if (!stupid_monsters && !will_run && is_hostile(m_ptr) &&
	    (r_ptr->flags1 & RF1_FRIENDS) &&
	    (los(m_ptr->fy, m_ptr->fx, py, px) ||
	    (cave[m_ptr->fy][m_ptr->fx].dist < MAX_SIGHT / 2)))
	{
	/*
	 * Animal packs try to get the player out of corridors
	 * (...unless they can move through walls -- TY)
	 */
		if ((r_ptr->flags3 & RF3_ANIMAL) && !can_pass_wall &&
			 !(r_ptr->flags2 & RF2_KILL_WALL))
		{
			int i, room = 0;

			/* Count room grids next to player */
			for (i = 0; i < 8; i++)
			{
				int x = px + ddx_ddd[i];
				int y = py + ddy_ddd[i];
				
				cave_type *c_ptr;

				if (!in_bounds2(y, x)) continue;
				
				c_ptr = &cave[y][x];

				/* Check grid */
				if (((cave_floor_grid(c_ptr)) || ((c_ptr->feat & 0x60) == 0x60)) &&
					 monster_can_cross_terrain(c_ptr->feat, r_ptr))
				{
					/* One more room grid */
					room++;
				}
			}
			if (cave[py][px].info & CAVE_ROOM) room -= 2;
			if (!r_ptr->flags4 && !r_ptr->flags5 && !r_ptr->flags6) room -= 2;

			/* Not in a room and strong player */
			if (room < (8 * (p_ptr->chp + p_ptr->csp)) /
			    (p_ptr->mhp + p_ptr->msp))
			{
				/* Find hiding place */
				if (find_hiding(m_idx, &y, &x)) done = TRUE;
			}
		}

		/* Monster groups try to surround the player */
		if (!done && (cave[m_ptr->fy][m_ptr->fx].dist < 3))
		{
			int i;

			/* Find an empty square near the player to fill */
			for (i = 0; i < 8; i++)
			{
				/* Pick squares near player (semi-randomly) */
				y2 = py + ddy_ddd[(m_idx + i) & 7];
				x2 = px + ddx_ddd[(m_idx + i) & 7];

				/* Already there? */
				if ((m_ptr->fy == y2) && (m_ptr->fx == x2))
				{
					/* Attack the player */
					y2 = py;
					x2 = px;

					break;
				}

				if (!in_bounds2(y2, x2)) continue;

				/* Ignore filled grids */
				c_ptr = &cave[y2][x2];
				if (!cave_empty_grid(c_ptr)) continue;

				/* Try to fill this hole */
				break;
			}

			/* Extract the new "pseudo-direction" */
			y = m_ptr->fy - y2;
			x = m_ptr->fx - x2;

			/* Done */
			done = TRUE;
		}
	}

	/* Apply fear if possible and necessary */
	if ((stupid_monsters || is_pet(m_ptr)) && will_run)
	{
		/* XXX XXX Not very "smart" */
		y = (-y), x = (-x);
	}
	else
	{
		if (!done && will_run)
		{
			/* Try to find safe place */
			if (!find_safety(m_idx, &y, &x))
			{
				/* This is not a very "smart" method XXX XXX */
				y = (-y);
				x = (-x);
			}
			else
			{
				/* Attempt to avoid the player */
				if (!stupid_monsters && !no_flow)
				{
					/* Adjust movement */
					(void)get_fear_moves_aux(m_idx, &y, &x);
				}
			}
		}
	}


	if (!stupid_monsters)
	{
		/* Check for no move */
		if (!x && !y) return (FALSE);
	}


	/* Extract the "absolute distances" */
	ax = ABS(x);
	ay = ABS(y);

	/* Do something weird */
	if (y < 0) move_val += 8;
	if (x > 0) move_val += 4;

	/* Prevent the diamond maneuvre */
	if (ay > (ax << 1))
	{
		move_val++;
		move_val++;
	}
	else if (ax > (ay << 1))
	{
		move_val++;
	}

	/* Extract some directions */
	switch (move_val)
	{
	case 0:
		mm[0] = 9;
		if (ay > ax)
		{
			mm[1] = 8;
			mm[2] = 6;
			mm[3] = 7;
			mm[4] = 3;
		}
		else
		{
			mm[1] = 6;
			mm[2] = 8;
			mm[3] = 3;
			mm[4] = 7;
		}
		break;
	case 1:
	case 9:
		mm[0] = 6;
		if (y < 0)
		{
			mm[1] = 3;
			mm[2] = 9;
			mm[3] = 2;
			mm[4] = 8;
		}
		else
		{
			mm[1] = 9;
			mm[2] = 3;
			mm[3] = 8;
			mm[4] = 2;
		}
		break;
	case 2:
	case 6:
		mm[0] = 8;
		if (x < 0)
		{
			mm[1] = 9;
			mm[2] = 7;
			mm[3] = 6;
			mm[4] = 4;
		}
		else
		{
			mm[1] = 7;
			mm[2] = 9;
			mm[3] = 4;
			mm[4] = 6;
		}
		break;
	case 4:
		mm[0] = 7;
		if (ay > ax)
		{
			mm[1] = 8;
			mm[2] = 4;
			mm[3] = 9;
			mm[4] = 1;
		}
		else
		{
			mm[1] = 4;
			mm[2] = 8;
			mm[3] = 1;
			mm[4] = 9;
		}
		break;
	case 5:
	case 13:
		mm[0] = 4;
		if (y < 0)
		{
			mm[1] = 1;
			mm[2] = 7;
			mm[3] = 2;
			mm[4] = 8;
		}
		else
		{
			mm[1] = 7;
			mm[2] = 1;
			mm[3] = 8;
			mm[4] = 2;
		}
		break;
	case 8:
		mm[0] = 3;
		if (ay > ax)
		{
			mm[1] = 2;
			mm[2] = 6;
			mm[3] = 1;
			mm[4] = 9;
		}
		else
		{
			mm[1] = 6;
			mm[2] = 2;
			mm[3] = 9;
			mm[4] = 1;
		}
		break;
	case 10:
	case 14:
		mm[0] = 2;
		if (x < 0)
		{
			mm[1] = 3;
			mm[2] = 1;
			mm[3] = 6;
			mm[4] = 4;
		}
		else
		{
			mm[1] = 1;
			mm[2] = 3;
			mm[3] = 4;
			mm[4] = 6;
		}
		break;
	case 12:
		mm[0] = 1;
		if (ay > ax)
		{
			mm[1] = 2;
			mm[2] = 4;
			mm[3] = 3;
			mm[4] = 7;
		}
		else
		{
			mm[1] = 4;
			mm[2] = 2;
			mm[3] = 7;
			mm[4] = 3;
		}
		break;
	}

	/* Wants to move... */
	return (TRUE);
}


static int check_hit2(int power, int level, int ac, int stun)
{
	int i, k;

	/* Percentile dice */
	k = randint0(100);

	if (stun && one_in_(2)) return FALSE;

	/* Hack -- Always miss or hit */
	if (k < 10) return (k < 5);

	/* Calculate the "attack quality" */
	i = (power + (level * 3));

	/* Power and Level compete against Armor */
	if ((i > 0) && (randint1(i) > ((ac * 3) / 4))) return (TRUE);

	/* Assume miss */
	return (FALSE);
}


/* Monster attacks monster */
static bool monst_attack_monst(int m_idx, int t_idx)
{
	monster_type    *m_ptr = &m_list[m_idx];
	monster_type    *t_ptr = &m_list[t_idx];

	monster_race    *r_ptr = &r_info[m_ptr->r_idx];
	monster_race    *tr_ptr = &r_info[t_ptr->r_idx];

	int             ap_cnt;
	int             ac, rlev, pt;
	char            m_name[80], t_name[80];
	char            temp[80];
	bool            blinked, heal_effect;
	bool            explode = FALSE, touched = FALSE, fear = FALSE;
	int             y_saver = t_ptr->fy;
	int             x_saver = t_ptr->fx;

	bool see_m = m_ptr->ml;
	bool see_t = t_ptr->ml;
	bool see_either = see_m || see_t;

	/* Can the player be aware of this attack? */
	bool known = (m_ptr->cdis <= MAX_SIGHT) || (t_ptr->cdis <= MAX_SIGHT);
	bool do_silly_attack = (one_in_(2) && p_ptr->image);

	/* Cannot attack self */
	if (m_idx == t_idx) return FALSE;

	/* Not allowed to attack */
	if (r_ptr->flags1 & RF1_NEVER_BLOW) return FALSE;

	if (d_info[dungeon_type].flags1 & DF1_NO_MELEE) return (FALSE);

	/* Wake it up */
	t_ptr->csleep = 0;

	/* Total armor */
	ac = tr_ptr->ac;

	/* Extract the effective monster level */
	rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);

	/* Get the monster name (or "it") */
	monster_desc(m_name, m_ptr, 0);

	/* Get the monster name (or "it") */
	monster_desc(t_name, t_ptr, 0);

	/* Assume no blink */
	blinked = FALSE;

	if (!see_either && known)
	{
		mon_fight = TRUE;
	}

	if (p_ptr->riding && (m_idx == p_ptr->riding)) disturb(1, 0);

	/* Scan through all four blows */
	for (ap_cnt = 0; ap_cnt < 4; ap_cnt++)
	{
		bool obvious = FALSE;

		int power = 0;
		int damage = 0;

		cptr act = NULL;

		/* Extract the attack infomation */
		int effect = r_ptr->blow[ap_cnt].effect;
		int method = r_ptr->blow[ap_cnt].method;
		int d_dice = r_ptr->blow[ap_cnt].d_dice;
		int d_side = r_ptr->blow[ap_cnt].d_side;

		if (!m_ptr->r_idx) break;

		/* Stop attacking if the target dies! */
		if (t_ptr->fx != x_saver || t_ptr->fy != y_saver)
			break;

		/* Hack -- no more attacks */
		if (!method) break;

		if (blinked) /* Stop! */
		{
			/* break; */
		}

		if (method == RBM_SHOOT) continue;

		/* Extract the attack "power" */
		switch (effect)
		{
		case RBE_HURT:          power = 60; break;
		case RBE_POISON:        power =  5; break;
		case RBE_UN_BONUS:      power = 20; break;
		case RBE_UN_POWER:      power = 15; break;
		case RBE_EAT_GOLD:      power =  5; break;
		case RBE_EAT_ITEM:      power =  5; break;
		case RBE_EAT_FOOD:      power =  5; break;
		case RBE_EAT_LITE:      power =  5; break;
		case RBE_ACID:          power =  0; break;
		case RBE_ELEC:          power = 10; break;
		case RBE_FIRE:          power = 10; break;
		case RBE_COLD:          power = 10; break;
		case RBE_BLIND:         power =  2; break;
		case RBE_CONFUSE:       power = 10; break;
		case RBE_TERRIFY:       power = 10; break;
		case RBE_PARALYZE:      power =  2; break;
		case RBE_LOSE_STR:      power =  0; break;
		case RBE_LOSE_DEX:      power =  0; break;
		case RBE_LOSE_CON:      power =  0; break;
		case RBE_LOSE_INT:      power =  0; break;
		case RBE_LOSE_WIS:      power =  0; break;
		case RBE_LOSE_CHR:      power =  0; break;
		case RBE_LOSE_ALL:      power =  2; break;
		case RBE_SHATTER:       power = 60; break;
		case RBE_EXP_10:        power =  5; break;
		case RBE_EXP_20:        power =  5; break;
		case RBE_EXP_40:        power =  5; break;
		case RBE_EXP_80:        power =  5; break;
		case RBE_DISEASE:       power =  5; break;
		case RBE_TIME:          power =  5; break;
		case RBE_EXP_VAMP:      power =  5; break;
		case RBE_DR_MANA:       power =  5; break;
		case RBE_SUPERHURT:     power = 60; break;
		}


		/* Monster hits */
		if (!effect || check_hit2(power, rlev, ac, m_ptr->stunned))
		{
			/* Describe the attack method */
			switch (method)
			{
			case RBM_HIT:
				{
#ifdef JP
act = "%sを殴った。";
#else
					act = "hits %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_TOUCH:
				{
#ifdef JP
act = "%sを触った。";
#else
					act = "touches %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_PUNCH:
				{
#ifdef JP
act = "%sをパンチした。";
#else
					act = "punches %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_KICK:
				{
#ifdef JP
act = "%sを蹴った。";
#else
					act = "kicks %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_CLAW:
				{
#ifdef JP
act = "%sをひっかいた。";
#else
					act = "claws %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_BITE:
				{
#ifdef JP
act = "%sを噛んだ。";
#else
					act = "bites %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_STING:
				{
#ifdef JP
act = "%sを刺した。";
#else
					act = "stings %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_SLASH:
				{
#ifdef JP
act = "%sを斬った。";
#else
					act = "slashes %s.";
#endif

					break;
				}

			case RBM_BUTT:
				{
#ifdef JP
act = "%sを角で突いた。";
#else
					act = "butts %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_CRUSH:
				{
#ifdef JP
act = "%sに体当りした。";
#else
					act = "crushes %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_ENGULF:
				{
#ifdef JP
act = "%sを飲み込んだ。";
#else
					act = "engulfs %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_CHARGE:
				{
#ifdef JP
act = "%sに請求書をよこした。";
#else
					act = "charges %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_CRAWL:
				{
#ifdef JP
act = "%sの体の上を這い回った。";
#else
					act = "crawls on %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_DROOL:
				{
#ifdef JP
act = "%sによだれをたらした。";
#else
					act = "drools on %s.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_SPIT:
				{
#ifdef JP
act = "%sに唾を吐いた。";
#else
					act = "spits on %s.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_EXPLODE:
				{
					if (see_either) disturb(1, 0);
#ifdef JP
act = "爆発した。";
#else
					act = "explodes.";
#endif

					explode = TRUE;
					touched = FALSE;
					break;
				}

			case RBM_GAZE:
				{
#ifdef JP
act = "%sをにらんだ。";
#else
					act = "gazes at %s.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_WAIL:
				{
#ifdef JP
act = "%sに泣きついた。";
#else
					act = "wails at %s.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_SPORE:
				{
#ifdef JP
act = "%sに胞子を飛ばした。";
#else
					act = "releases spores at %s.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_XXX4:
				{
#ifdef JP
act = "%sにXXX4を飛ばした。";
#else
					act = "projects XXX4's at %s.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_BEG:
				{
#ifdef JP
act = "%sに金をせがんだ。";
#else
					act = "begs %s for money.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_INSULT:
				{
#ifdef JP
act = "%sを侮辱した。";
#else
					act = "insults %s.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_MOAN:
				{
#ifdef JP
act = "%sにむかってうめいた。";
#else
					act = "moans at %s.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_SHOW:
				{
#ifdef JP
act = "%sにむかって歌った。";
#else
					act = "sings to %s.";
#endif

					touched = FALSE;
					break;
				}
			}

			/* Message */
			if (act && see_either)
			{
				if (do_silly_attack)
				{
					act = silly_attacks[randint0(MAX_SILLY_ATTACK)];
				}
				strfmt(temp, act, t_name);
#ifdef JP
				msg_format("%^sは%s", m_name, temp);
#else
				msg_format("%^s %s", m_name, temp);
#endif

			}

			/* Hack -- assume all attacks are obvious */
			obvious = TRUE;

			/* Roll out the damage */
			damage = damroll(d_dice, d_side);

			/* Assume no healing effect */
			heal_effect = FALSE;

			pt = GF_MISSILE;

			/* Apply appropriate damage */
			switch (effect)
			{
			case 0:
				{
					damage = 0;
					pt = 0;
					break;
				}

			case RBE_SUPERHURT:
				{
					if ((randint1(rlev*2+250) > (ac+200)) || one_in_(13)) {
						int tmp_damage = damage-(damage*((ac < 150) ? ac : 150)/250);
						damage = MAX(damage, tmp_damage*2);
						break;
					}
				}
			case RBE_HURT:
				{
					damage -= (damage * ((ac < 150) ? ac : 150) / 250);
					break;
				}

			case RBE_POISON:
			case RBE_DISEASE:
				{
					pt = GF_POIS;
					break;
				}

			case RBE_UN_BONUS:
			case RBE_UN_POWER:
				{
					pt = GF_DISENCHANT;
					break;
				}

			case RBE_EAT_FOOD:
			case RBE_EAT_LITE:
			case RBE_DR_MANA:
				{
					pt = damage = 0;
					break;
				}

			case RBE_EAT_ITEM:
			case RBE_EAT_GOLD:
				{
					pt = damage = 0;
					if (one_in_(2)) blinked = TRUE;
					break;
				}

			case RBE_ACID:
				{
					pt = GF_ACID;
					break;
				}

			case RBE_ELEC:
				{
					pt = GF_ELEC;
					break;
				}

			case RBE_FIRE:
				{
					pt = GF_FIRE;
					break;
				}

			case RBE_COLD:
				{
					pt = GF_COLD;
					break;
				}

			case RBE_BLIND:
				{
					break;
				}

			case RBE_CONFUSE:
				{
					pt = GF_CONFUSION;
					break;
				}

			case RBE_TERRIFY:
				{
					pt = GF_TURN_ALL;
					break;
				}

			case RBE_PARALYZE:
				{
					pt = GF_OLD_SLEEP; /* sort of close... */
					break;
				}

			case RBE_LOSE_STR:
			case RBE_LOSE_INT:
			case RBE_LOSE_WIS:
			case RBE_LOSE_DEX:
			case RBE_LOSE_CON:
			case RBE_LOSE_CHR:
			case RBE_LOSE_ALL:
				{
					break;
				}
			case RBE_SHATTER:
				{
					if (damage > 23)
					{
						earthquake(m_ptr->fy, m_ptr->fx, 8);
					}
					break;
				}
			case RBE_EXP_10:
			case RBE_EXP_20:
			case RBE_EXP_40:
			case RBE_EXP_80:
				{
					pt = GF_NETHER;
					break;
				}
			case RBE_TIME:
				{
					pt = GF_TIME;
					break;
				}
			case RBE_EXP_VAMP:
				{
					pt = GF_OLD_DRAIN;
					heal_effect = TRUE;
					break;
				}

			default:
				{
					pt = 0;
					break;
				}
			}

			if (pt)
			{
				/* Do damage if not exploding */
				if (!explode)
				{
					project(m_idx, 0, t_ptr->fy, t_ptr->fx,
						(pt == GF_OLD_SLEEP ? r_ptr->level : damage), pt, PROJECT_KILL | PROJECT_STOP | PROJECT_MONSTER | PROJECT_NO_REF, -1);
				}

				if (heal_effect)
				{
					if ((monster_living(tr_ptr)) && (damage > 2))
					{
						bool did_heal = FALSE;

						if (m_ptr->hp < m_ptr->maxhp) did_heal = TRUE;

						/* Heal */
						m_ptr->hp += damroll(4, damage / 6);
						if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

						/* Redraw (later) if needed */
						if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
						if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);

						/* Special message */
						if (see_m && did_heal)
						{
#ifdef JP
msg_format("%sは体力を回復したようだ。", m_name);
#else
							msg_format("%^s appears healthier.", m_name);
#endif

						}
					}
				}

				if (touched)
				{
					/* Aura fire */
					if ((tr_ptr->flags2 & RF2_AURA_FIRE) &&
						!(r_ptr->flags3 & RF3_IM_FIRE) &&
					        !(r_ptr->flags3 & RF3_RES_ALL) && m_ptr->r_idx)
					{
						if (see_either)
						{
							blinked = FALSE;
#ifdef JP
msg_format("%^sは突然熱くなった！", m_name);
#else
							msg_format("%^s is suddenly very hot!", m_name);
#endif

							if (t_ptr->ml)
								tr_ptr->r_flags2 |= RF2_AURA_FIRE;
						}
						project(t_idx, 0, m_ptr->fy, m_ptr->fx,
							damroll (1 + ((tr_ptr->level) / 26),
							1 + ((tr_ptr->level) / 17)),
							GF_FIRE, PROJECT_KILL | PROJECT_STOP | PROJECT_MONSTER | PROJECT_NO_REF, -1);
					}

					/* Aura cold */
					if ((tr_ptr->flags3 & RF3_AURA_COLD) &&
						!(r_ptr->flags3 & RF3_IM_COLD) &&
					        !(r_ptr->flags3 & RF3_RES_ALL) && m_ptr->r_idx)
					{
						if (see_either)
						{
							blinked = FALSE;
#ifdef JP
msg_format("%^sは突然寒くなった！", m_name);
#else
							msg_format("%^s is suddenly very cold!", m_name);
#endif

							if (t_ptr->ml)
								tr_ptr->r_flags3 |= RF3_AURA_COLD;
						}
						project(t_idx, 0, m_ptr->fy, m_ptr->fx,
							damroll (1 + ((tr_ptr->level) / 26),
							1 + ((tr_ptr->level) / 17)),
							GF_COLD, PROJECT_KILL | PROJECT_STOP | PROJECT_MONSTER | PROJECT_NO_REF, -1);
					}

					/* Aura elec */
					if ((tr_ptr->flags2 & (RF2_AURA_ELEC)) &&
					        !(r_ptr->flags3 & (RF3_IM_ELEC)) &&
					        !(r_ptr->flags3 & RF3_RES_ALL) && m_ptr->r_idx)
					{
						if (see_either)
						{
							blinked = FALSE;
#ifdef JP
msg_format("%^sは電撃を食らった！", m_name);
#else
							msg_format("%^s gets zapped!", m_name);
#endif

							if (t_ptr->ml)
								tr_ptr->r_flags2 |= RF2_AURA_ELEC;
						}
						project(t_idx, 0, m_ptr->fy, m_ptr->fx,
							damroll (1 + ((tr_ptr->level) / 26),
							1 + ((tr_ptr->level) / 17)),
							GF_ELEC, PROJECT_KILL | PROJECT_STOP | PROJECT_MONSTER | PROJECT_NO_REF, -1);
					}

				}
			}
		}

		/* Monster missed player */
		else
		{
			/* Analyze failed attacks */
			switch (method)
			{
			case RBM_HIT:
			case RBM_TOUCH:
			case RBM_PUNCH:
			case RBM_KICK:
			case RBM_CLAW:
			case RBM_BITE:
			case RBM_STING:
			case RBM_SLASH:
			case RBM_BUTT:
			case RBM_CRUSH:
			case RBM_ENGULF:
			case RBM_CHARGE:
				{
					/* Visible monsters */
					if (see_m)
					{
						/* Message */
#ifdef JP
msg_format("%sは%^sの攻撃をかわした。", t_name,m_name);
#else
						msg_format("%^s misses %s.", m_name, t_name);
#endif

					}

					break;
				}
			}
		}


		/* Analyze "visible" monsters only */
		if (see_m && !do_silly_attack)
		{
			/* Count "obvious" attacks (and ones that cause damage) */
			if (obvious || damage || (r_ptr->r_blows[ap_cnt] > 10))
			{
				/* Count attacks of this type */
				if (r_ptr->r_blows[ap_cnt] < MAX_UCHAR)
				{
					r_ptr->r_blows[ap_cnt]++;
				}
			}
		}
	}

	if (explode)
	{
		sound(SOUND_EXPLODE);

		/* Cancel Invulnerability */
		if (m_ptr->invulner) m_ptr->invulner = 0;

#ifdef JP
mon_take_hit_mon(FALSE, m_idx, m_ptr->hp + 1, &fear, "は爆発して粉々になった。", m_idx);
#else
		mon_take_hit_mon(FALSE, m_idx, m_ptr->hp + 1, &fear, " explodes into tiny shreds.", m_idx);
#endif


		blinked = FALSE;
	}


	/* Blink away */
	if (blinked)
	{
		if (see_m)
		{
#ifdef JP
msg_print("泥棒は笑って逃げた！");
#else
			msg_print("The thief flees laughing!");
#endif

		}
		else if (known)
		{
			mon_fight = TRUE;
		}

		teleport_away(m_idx, MAX_SIGHT * 2 + 5, FALSE);
	}

	return TRUE;
}


/*
 * Hack -- local "player stealth" value (see below)
 */
static u32b noise = 0L;


/*
 * Process a monster
 *
 * The monster is known to be within 100 grids of the player
 *
 * In several cases, we directly update the monster lore
 *
 * Note that a monster is only allowed to "reproduce" if there
 * are a limited number of "reproducing" monsters on the current
 * level.  This should prevent the level from being "swamped" by
 * reproducing monsters.  It also allows a large mass of mice to
 * prevent a louse from multiplying, but this is a small price to
 * pay for a simple multiplication method.
 *
 * XXX Monster fear is slightly odd, in particular, monsters will
 * fixate on opening a door even if they cannot open it.  Actually,
 * the same thing happens to normal monsters when they hit a door
 *
 * XXX XXX XXX In addition, monsters which *cannot* open or bash
 * down a door will still stand there trying to open it...
 *
 * XXX Technically, need to check for monster in the way
 * combined with that monster being in a wall (or door?)
 *
 * A "direction" of "5" means "pick a random direction".
 */
static void process_monster(int m_idx)
{
	monster_type    *m_ptr = &m_list[m_idx];
	monster_race    *r_ptr = &r_info[m_ptr->r_idx];

	int             i, d, oy, ox, ny, nx;

	int             mm[8];

	cave_type       *c_ptr;

	monster_type    *y_ptr;

	bool            do_turn;
	bool            do_move;
	bool            do_view;

	bool            did_open_door;
	bool            did_bash_door;
	bool            did_take_item;
	bool            did_kill_item;
	bool            did_move_body;
	bool            did_pass_wall;
	bool            did_kill_wall;
	bool            gets_angry = FALSE;
	bool            can_pass_wall;
	bool            aware = TRUE;

	bool            fear;

	if ((m_idx == p_ptr->riding) && !(r_ptr->flags7 & RF7_RIDING))
	{
		if (rakuba(0, TRUE))
		{
#ifdef JP
			msg_print("地面に落とされた。");
#else
			char m_name[80];
			monster_desc(m_name, &m_list[p_ptr->riding], 0);
			msg_format("You have fallen from %s.", m_name);
#endif
		}
	}

	if ((m_ptr->mflag2 & MFLAG_CHAMELEON) && one_in_(13) && !m_ptr->csleep)
	{
		choose_new_monster(m_idx, FALSE, 0);
		r_ptr = &r_info[m_ptr->r_idx];
	}

	/* Players hidden in shadow are almost imperceptable. -LM- */
	if (p_ptr->special_defense & NINJA_S_STEALTH)
	{
		int tmp = p_ptr->lev*6+(p_ptr->skill_stl+10)*4;
		if (p_ptr->monlite) tmp /= 3;
		if (p_ptr->cursed & TRC_AGGRAVATE) tmp /= 2;
		if (r_ptr->level > (p_ptr->lev*p_ptr->lev/20+10)) tmp /= 3;
		/* Low-level monsters will find it difficult to locate the player. */
		if (randint0(tmp) > (r_ptr->level+20)) aware = FALSE;
	}

	/* Quantum monsters are odd */
	if (r_ptr->flags2 & (RF2_QUANTUM))
	{
		/* Sometimes skip move */
		if (!randint0(2)) return;

		/* Sometimes die */
		if (!randint0((m_idx % 100) + 10) && !(r_ptr->flags1 & RF1_QUESTOR))
		{
			bool sad = FALSE;

			if (is_pet(m_ptr) && !(m_ptr->ml))
				sad = TRUE;

			if (m_ptr->ml)
			{
				char m_name[80];

				/* Acquire the monster name */
				monster_desc(m_name, m_ptr, 0);

				/* Oops */
#ifdef JP
msg_format("%sは消え去った！", m_name);
#else
				msg_format("%^s disappears!", m_name);
#endif

			}

			/* Generate treasure, etc */
			monster_death(m_idx, FALSE);

			/* Delete the monster */
			delete_monster_idx(m_idx);

			if (sad)
			{
#ifdef JP
msg_print("少しの間悲しい気分になった。");
#else
				msg_print("You feel sad for a moment.");
#endif

			}

			return;
		}
	}

	if (m_ptr->r_idx == MON_SHURYUUDAN)
#ifdef JP
		mon_take_hit_mon(FALSE, m_idx, 1, &fear, "は爆発して粉々になった。", m_idx);
#else
		mon_take_hit_mon(FALSE, m_idx, 1, &fear, " explodes into tiny shreds.", m_idx);
#endif

	if ((is_pet(m_ptr) || is_friendly(m_ptr)) && ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_UNIQUE_7)) && !p_ptr->inside_battle)
	{
		static int riding_pinch = 0;

		if (m_ptr->hp < m_ptr->maxhp/3)
		{
			bool level_teleport = TRUE;
			char m_name[80];
			monster_desc(m_name, m_ptr, 0);

			if (m_idx == p_ptr->riding && riding_pinch < 2)
			{
#ifdef JP
msg_format("%sは傷の痛さの余りあなたの束縛から逃れようとしている。", m_name);
#else
					msg_format("%^s seems to be in so much pain, and trying to escape from your restriction.", m_name);
#endif
				riding_pinch++;
				level_teleport = FALSE;
				disturb(1, 0);
			}
			else if (m_ptr->ml)
			{
				if (m_idx == p_ptr->riding)
				{
#ifdef JP
msg_format("%sはあなたの束縛から脱出した。", m_name);
#else
					msg_format("%^s succeeded to escape from your restriction!", m_name);
#endif
				}
				if ((r_ptr->flags2 & RF2_CAN_SPEAK) && (m_ptr->r_idx != MON_GRIP) && (m_ptr->r_idx != MON_WOLF) && (m_ptr->r_idx != MON_FANG))
				{
#ifdef JP
msg_format("%^s「ピンチだ！退却させてもらう！」", m_name);
#else
					msg_format("%^s says 'It is the pinch! I will retreat'.", m_name);
#endif
				}
#ifdef JP
msg_format("%^sがテレポート・レベルの巻物を読んだ。", m_name);
#else
				msg_format("%^s read a scroll of teleport level.", m_name);
#endif
#ifdef JP
msg_format("%^sが消え去った。", m_name);
#else
				msg_format("%^s disappears.", m_name);
#endif
			}

			if (level_teleport)
			{
				delete_monster_idx(m_idx);

				if (m_idx == p_ptr->riding)
				{
					if (rakuba(-1, FALSE))
					{
#ifdef JP
msg_print("地面に落とされた。");
#else
                                                msg_print("You have fallen from riding pet.");
#endif
					}
				}
				return;
			}
		}
		else
			if (m_idx == p_ptr->riding) riding_pinch = 0;
			
	}

	/* Handle Invulnerability */
	if (m_ptr->invulner)
	{
		/* Reduce by one, note if expires */
		m_ptr->invulner--;

		if (!(m_ptr->invulner) && m_ptr->ml)
		{
			char m_name[80];

			/* Acquire the monster name */
			monster_desc(m_name, m_ptr, 0);

			/* Dump a message */
#ifdef JP
msg_format("%^sはもう無敵でない。", m_name);
#else
			msg_format("%^s is no longer invulnerable.", m_name);
#endif

			m_ptr->energy_need += ENERGY_NEED();
			if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
			if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);
		}
	}

	/* Handle fast */
	if (m_ptr->fast)
	{
		/* Reduce by one, note if expires */
		m_ptr->fast--;

		if (!(m_ptr->fast) && m_ptr->ml)
		{
			char m_name[80];

			/* Acquire the monster name */
			monster_desc(m_name, m_ptr, 0);

			/* Dump a message */
#ifdef JP
msg_format("%^sはもう加速されていない。", m_name);
#else
			msg_format("%^s is no longer fast.", m_name);
#endif

			if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
			if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);
		}
	}

	/* Handle slow */
	if (m_ptr->slow)
	{
		/* Reduce by one, note if expires */
		m_ptr->slow--;

		if (!(m_ptr->slow) && m_ptr->ml)
		{
			char m_name[80];

			/* Acquire the monster name */
			monster_desc(m_name, m_ptr, 0);

			/* Dump a message */
#ifdef JP
msg_format("%^sはもう減速されていない。", m_name);
#else
			msg_format("%^s is no longer slow.", m_name);
#endif

			if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
			if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);
		}
	}

	/* Handle "sleep" */
	if (m_ptr->csleep)
	{
		u32b notice = 0;

		/* Hack -- handle non-aggravation */
		if (!(p_ptr->cursed & TRC_AGGRAVATE)) notice = randint0(1024);

		/* Nightmare monsters are more alert */
		if (ironman_nightmare) notice /= 2;

		/* Hack -- See if monster "notices" player */
		if ((notice * notice * notice) <= noise)
		{
			/* Hack -- amount of "waking" */
			int d = 1;

			/* Wake up faster near the player */
			if (m_ptr->cdis < 50) d = (100 / m_ptr->cdis);

			/* Hack -- handle aggravation */
			if (p_ptr->cursed & TRC_AGGRAVATE) d = m_ptr->csleep;

			/* Still asleep */
			if (m_ptr->csleep > d)
			{
				/* Monster wakes up "a little bit" */
				m_ptr->csleep -= d;

				/* Notice the "not waking up" */
				if (m_ptr->ml)
				{
					/* Hack -- Count the ignores */
					if (r_ptr->r_ignore < MAX_UCHAR)
					{
						r_ptr->r_ignore++;
					}
				}
			}

			/* Just woke up */
			else
			{
				/* Reset sleep counter */
				m_ptr->csleep = 0;

				/* Notice the "waking up" */
				if (m_ptr->ml)
				{
					char m_name[80];

					/* Acquire the monster name */
					monster_desc(m_name, m_ptr, 0);

					/* Dump a message */
#ifdef JP
msg_format("%^sが目を覚ました。", m_name);
#else
					msg_format("%^s wakes up.", m_name);
#endif


					/* Redraw the health bar */
					if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
					if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);

					if (r_ptr->flags7 & (RF7_HAS_LITE_1 | RF7_HAS_LITE_2))
						p_ptr->update |= (PU_MON_LITE);

					/* Hack -- Count the wakings */
					if (r_ptr->r_wake < MAX_UCHAR)
					{
						r_ptr->r_wake++;
					}
				}
			}
		}

		/* Still sleeping */
		if (m_ptr->csleep) return;
	}


	/* Handle "stun" */
	if (m_ptr->stunned)
	{
		int d = 1;

		/* Make a "saving throw" against stun */
		if (randint0(10000) <= r_ptr->level * r_ptr->level)
		{
			/* Recover fully */
			d = m_ptr->stunned;
		}

		/* Hack -- Recover from stun */
		if (m_ptr->stunned > d)
		{
			/* Recover somewhat */
			m_ptr->stunned -= d;
		}

		/* Fully recover */
		else
		{
			/* Recover fully */
			m_ptr->stunned = 0;

			/* Message if visible */
			if (m_ptr->ml)
			{
				char m_name[80];

				/* Acquire the monster name */
				monster_desc(m_name, m_ptr, 0);

				/* Dump a message */
#ifdef JP
msg_format("%^sは朦朧状態から立ち直った。", m_name);
#else
				msg_format("%^s is no longer stunned.", m_name);
#endif

				if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
				if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);
			}
		}

		/* Still stunned */
		if (m_ptr->stunned && one_in_(2)) return;
	}


	/* Handle confusion */
	if (m_ptr->confused)
	{
		/* Amount of "boldness" */
		int d = randint1(r_ptr->level / 20 + 1);

		/* Still confused */
		if (m_ptr->confused > d)
		{
			/* Reduce the confusion */
			m_ptr->confused -= d;
		}

		/* Recovered */
		else
		{
			/* No longer confused */
			m_ptr->confused = 0;

			/* Message if visible */
			if (m_ptr->ml)
			{
				char m_name[80];

				/* Acquire the monster name */
				monster_desc(m_name, m_ptr, 0);

				/* Dump a message */
#ifdef JP
msg_format("%^sは混乱から立ち直った。", m_name);
#else
				msg_format("%^s is no longer confused.", m_name);
#endif

				if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
				if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);
			}
		}
	}

	if (p_ptr->riding == m_idx)
	{
		p_ptr->update |= (PU_BONUS);
	}

	/* No one wants to be your friend if you're aggravating */
	if (is_friendly(m_ptr) && (p_ptr->cursed & TRC_AGGRAVATE))
		gets_angry = TRUE;

	/* Paranoia... no pet uniques outside wizard mode -- TY */
	if (is_pet(m_ptr) &&
	    ((((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_UNIQUE_7)) &&
	      ((p_ptr->align > 9 && (r_ptr->flags3 & RF3_EVIL)) ||
	      (p_ptr->align < -9 && (r_ptr->flags3 & RF3_GOOD))))
	     || (r_ptr->flags3 & RF3_RES_ALL)))
	{
			gets_angry = TRUE;
	}

	if (p_ptr->inside_battle) gets_angry = FALSE;

	if (gets_angry)
	{
		char m_name[80];
		monster_desc(m_name, m_ptr, 0);
#ifdef JP
msg_format("%^sは突然敵にまわった！", m_name);
#else
		msg_format("%^s suddenly becomes hostile!", m_name);
#endif

		set_hostile(m_ptr);
	}

	/* Handle "fear" */
	if (m_ptr->monfear)
	{
		/* Amount of "boldness" */
		int d = randint1(r_ptr->level / 20 + 1);

		/* Still afraid */
		if (m_ptr->monfear > d)
		{
			/* Reduce the fear */
			m_ptr->monfear -= d;
		}

		/* Recover from fear, take note if seen */
		else
		{
			/* No longer afraid */
			m_ptr->monfear = 0;

			/* Visual note */
			if (m_ptr->ml)
			{
				char m_name[80];
				char m_poss[80];

				/* Acquire the monster name/poss */
				monster_desc(m_name, m_ptr, 0);
				monster_desc(m_poss, m_ptr, 0x22);

				/* Dump a message */
#ifdef JP
msg_format("%^sは勇気を取り戻した。", m_name);
#else
				msg_format("%^s recovers %s courage.", m_name, m_poss);
#endif

				if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
			}
		}
	}

	/* Get the origin */
	oy = m_ptr->fy;
	ox = m_ptr->fx;


	/* Attempt to "multiply" if able and allowed */
	if ((r_ptr->flags2 & RF2_MULTIPLY) && (num_repro < MAX_REPRO))
	{
		int k, y, x;

		/* Count the adjacent monsters */
		for (k = 0, y = oy - 1; y <= oy + 1; y++)
		{
			for (x = ox - 1; x <= ox + 1; x++)
			{
				/* Ignore locations off of edge */
				if (!in_bounds2(y, x)) continue;

				if (cave[y][x].m_idx) k++;
			}
		}

		/* Hack -- multiply slower in crowded areas */
		if ((k < 4) && (!k || !randint0(k * MON_MULT_ADJ)))
		{
			/* Try to multiply */
			if (multiply_monster(m_idx, FALSE, (is_pet(m_ptr) ? PM_FORCE_PET : 0)))
			{
				/* Take note if visible */
				if (m_ptr->ml)
				{
					r_ptr->r_flags2 |= (RF2_MULTIPLY);
				}

				/* Multiplying takes energy */
				return;
			}
		}
	}


	if (!p_ptr->inside_battle)
	{
		/* Hack! "Cyber" monster makes noise... */
		if (m_ptr->ap_r_idx == MON_CYBER &&
		    one_in_(CYBERNOISE) &&
		    !m_ptr->ml && (m_ptr->cdis <= MAX_SIGHT))
		{
			if (disturb_minor) disturb(FALSE, FALSE);
#ifdef JP
msg_print("重厚な足音が聞こえた。");
#else
			msg_print("You hear heavy steps.");
#endif

		}

		/* Some monsters can speak */
		if ((r_info[m_ptr->ap_r_idx].flags2 & RF2_CAN_SPEAK) && aware &&
			one_in_(SPEAK_CHANCE) &&
			player_has_los_bold(oy, ox))
		{
			char m_name[80];
			char monmessage[1024];
			cptr filename;

			/* Acquire the monster name/poss */
			if (m_ptr->ml)
				monster_desc(m_name, m_ptr, 0);
			else
#ifdef JP
strcpy(m_name, "それ");
#else
				strcpy(m_name, "It");
#endif


			/* Select the file for monster quotes */
			if (m_ptr->monfear)
#ifdef JP
filename = "monfear_j.txt";
#else
				filename = "monfear.txt";
#endif

			else if (is_pet(m_ptr))
#ifdef JP
filename = "monpet_j.txt";
#else
				filename = "monpet.txt";
#endif

			else if (is_friendly(m_ptr))
#ifdef JP
filename = "monfrien_j.txt";
#else
				filename = "monfrien.txt";
#endif

			else
#ifdef JP
				filename = "monspeak_j.txt";
#else
				filename = "monspeak.txt";
#endif


			/* Get the monster line */
			if (get_rnd_line(filename, m_ptr->ap_r_idx, monmessage) == 0)
			{
				/* Say something */
#ifdef JP
msg_format("%^s%s", m_name, monmessage);
#else
				msg_format("%^s %s", m_name, monmessage);
#endif

			}
		}
	}

	/* Attempt to cast a spell */
	if (aware && make_attack_spell(m_idx)) return;

	/*
	 * Attempt to cast a spell at an enemy other than the player
	 * (may slow the game a smidgeon, but I haven't noticed.)
	 */
	if (monst_spell_monst(m_idx)) return;

	can_pass_wall = ((r_ptr->flags2 & RF2_PASS_WALL) && ((m_idx != p_ptr->riding) || (p_ptr->pass_wall)));

	/* Hack -- Assume no movement */
	mm[0] = mm[1] = mm[2] = mm[3] = 0;
	mm[4] = mm[5] = mm[6] = mm[7] = 0;


	/* Confused -- 100% random */
	if (m_ptr->confused || !aware)
	{
		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
	}

	/* 75% random movement */
	else if ((r_ptr->flags1 & RF1_RAND_50) &&
				(r_ptr->flags1 & RF1_RAND_25) &&
	         (randint0(100) < 75))
	{
		/* Memorize flags */
		if (m_ptr->ml) r_ptr->r_flags1 |= (RF1_RAND_50);
		if (m_ptr->ml) r_ptr->r_flags1 |= (RF1_RAND_25);

		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
	}

	/* 50% random movement */
	else if ((r_ptr->flags1 & RF1_RAND_50) &&
				(randint0(100) < 50))
	{
		/* Memorize flags */
		if (m_ptr->ml) r_ptr->r_flags1 |= (RF1_RAND_50);

		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
	}

	/* 25% random movement */
	else if ((r_ptr->flags1 & RF1_RAND_25) &&
				(randint0(100) < 25))
	{
		/* Memorize flags */
		if (m_ptr->ml) r_ptr->r_flags1 |= RF1_RAND_25;

		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
	}

	/* Can't reach player - find something else to hit */
	else if ((r_ptr->flags1 & RF1_NEVER_MOVE) && (m_ptr->cdis > 1))
	{
		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;

		/* Look for an enemy */
#if 0  /* Hack - Too slow.  Mimic pits are horrible with this on. */
		get_enemy_dir(m_idx, mm);
#endif /* 0 */
	}

	/* Pets will follow the player */
	else if (is_pet(m_ptr))
	{
		/* Are we trying to avoid the player? */
		bool avoid = ((p_ptr->pet_follow_distance < 0) &&
						  (m_ptr->cdis <= (0 - p_ptr->pet_follow_distance)));

		/* Do we want to find the player? */
		bool lonely = (!avoid && (m_ptr->cdis > p_ptr->pet_follow_distance));

		/* Should we find the player if we can't find a monster? */
		bool distant = (m_ptr->cdis > PET_SEEK_DIST);

		/* by default, move randomly */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;

		/* Look for an enemy */
		if (!get_enemy_dir(m_idx, mm))
		{
			/* Find the player if necessary */
			if (avoid || lonely || distant)
			{
				/* Remember the leash length */
				int dis = p_ptr->pet_follow_distance;

				/* Hack -- adjust follow distance temporarily */
				if (p_ptr->pet_follow_distance > PET_SEEK_DIST)
				{
					p_ptr->pet_follow_distance = PET_SEEK_DIST;
				}

				/* Find the player */
				get_moves(m_idx, mm);

				/* Restore the leash */
				p_ptr->pet_follow_distance = dis;
			}
		}
	}

	/* Friendly monster movement */
	else if (!is_hostile(m_ptr))
	{
		/* by default, move randomly */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;

		/* Look for an enemy */
		get_enemy_dir(m_idx, mm);
	}
	/* Normal movement */
	else if (stupid_monsters)
	{
		/* Logical moves */
		get_moves(m_idx, mm);
	}
	else
	{
		/* Logical moves, may do nothing */
		if (!get_moves(m_idx, mm)) return;
	}

	/* Assume nothing */
	do_turn = FALSE;
	do_move = FALSE;
	do_view = FALSE;

	/* Assume nothing */
	did_open_door = FALSE;
	did_bash_door = FALSE;
	did_take_item = FALSE;
	did_kill_item = FALSE;
	did_move_body = FALSE;
	did_pass_wall = FALSE;
	did_kill_wall = FALSE;


	/* Take a zero-terminated array of "directions" */
	for (i = 0; mm[i]; i++)
	{
		/* Get the direction */
		d = mm[i];

		/* Hack -- allow "randomized" motion */
		if (d == 5) d = ddd[randint0(8)];

		/* Get the destination */
		ny = oy + ddy[d];
		nx = ox + ddx[d];

		/* Ignore locations off of edge */
		if (!in_bounds2(ny, nx)) continue;

		/* Access that cave grid */
		c_ptr = &cave[ny][nx];

		/* Access that cave grid's contents */
		y_ptr = &m_list[c_ptr->m_idx];

		/* Floor is open? */
		if (cave_floor_grid(c_ptr))
		{
			/* Go ahead and move */
			do_move = TRUE;
		}

		/* Hack -- player 'in' wall */
		else if ((ny == py) && (nx == px))
		{
			do_move = TRUE;
		}

		else if (c_ptr->m_idx)
		{
			/* Possibly a monster to attack */
			do_move = TRUE;
		}

		/* Permanent wall */
		else if ((c_ptr->feat >= FEAT_PERM_EXTRA) &&
			(c_ptr->feat <= FEAT_PERM_SOLID))
		{
			do_move = FALSE;
		}

		/* Hack -- semi-transparent terrains are no obstacle */
		else if (c_ptr->feat == FEAT_TREES)
		{
			do_move = TRUE;
		}

		/* Hack -- semi-transparent terrains are no obstacle */
		else if ((c_ptr->feat == FEAT_MOUNTAIN) && ((r_ptr->flags2 & RF2_KILL_WALL) || (!dun_level && ((r_ptr->flags7 & RF7_CAN_FLY) || (r_ptr->flags8 & RF8_WILD_MOUNTAIN)))))
		{
			do_move = TRUE;
		}


		/* Monster moves through walls (and doors) */
		else if (can_pass_wall)
		{
			/* Pass through walls/doors/rubble */
			do_move = TRUE;

			/* Monster went through a wall */
			did_pass_wall = TRUE;
		}

		/* Monster destroys walls (and doors) */
		else if ((r_ptr->flags2 & RF2_KILL_WALL) && (m_idx != p_ptr->riding))
		{
			/* Eat through walls/doors/rubble */
			do_move = TRUE;

			/* Monster destroyed a wall */
			did_kill_wall = TRUE;

			if (one_in_(GRINDNOISE))
			{
#ifdef JP
msg_print("ギシギシいう音が聞こえる。");
#else
				msg_print("There is a grinding sound.");
#endif

			}

			/* Forget the wall */
			c_ptr->info &= ~(CAVE_MARK);

			/* Notice */
			c_ptr->feat = floor_type[randint0(100)];

			/* Note changes to viewable region */
			if (player_has_los_bold(ny, nx)) do_view = TRUE;
		}

		/* Handle doors and secret doors */
		else if (((c_ptr->feat >= FEAT_DOOR_HEAD) &&
		          (c_ptr->feat <= FEAT_DOOR_TAIL)) ||
		          (c_ptr->feat == FEAT_SECRET))
		{
			bool may_bash = TRUE;

			/* Assume no move allowed */
			do_move = FALSE;

			/* Creature can open doors. */
			if ((r_ptr->flags2 & RF2_OPEN_DOOR) &&
				 (!is_pet(m_ptr) || (p_ptr->pet_extra_flags & PF_OPEN_DOORS)))
			{
				/* Closed doors and secret doors */
				if ((c_ptr->feat == FEAT_DOOR_HEAD) ||
					(c_ptr->feat == FEAT_SECRET))
				{
					/* The door is open */
					did_open_door = TRUE;

					/* Do not bash the door */
					may_bash = FALSE;

					/* Assume no move allowed */
					do_move = TRUE;
				}

				/* Locked doors (not jammed) */
				else if (c_ptr->feat < FEAT_DOOR_HEAD + 0x08)
				{
					int k;

					/* Door power */
					k = ((c_ptr->feat - FEAT_DOOR_HEAD) & 0x07);

					/* Try to unlock it XXX XXX XXX */
					if (randint0(m_ptr->hp / 10) > k)
					{
						/* Unlock the door */
						cave_set_feat(ny, nx, FEAT_DOOR_HEAD + 0x00);

						/* Do not bash the door */
						may_bash = FALSE;
					}
				}
			}

			/* Stuck doors -- attempt to bash them down if allowed */
			if (may_bash && (r_ptr->flags2 & RF2_BASH_DOOR) &&
				(!is_pet(m_ptr) || (p_ptr->pet_extra_flags & PF_OPEN_DOORS)))
			{
				int k;

				/* Door power */
				k = ((c_ptr->feat - FEAT_DOOR_HEAD) & 0x07);

				/* Attempt to Bash XXX XXX XXX */
				if (randint0(m_ptr->hp / 10) > k)
				{
					/* Message */
#ifdef JP
msg_print("ドアを叩き開ける音がした！");
#else
					msg_print("You hear a door burst open!");
#endif


					/* Disturb (sometimes) */
					if (disturb_minor) disturb(0, 0);

					/* The door was bashed open */
					did_bash_door = TRUE;

					/* Hack -- fall into doorway */
					do_move = TRUE;
				}
			}


			/* Deal with doors in the way */
			if (did_open_door || did_bash_door)
			{
				/* Break down the door */
				if (did_bash_door && (randint0(100) < 50))
				{
					cave_set_feat(ny, nx, FEAT_BROKEN);
				}

				/* Open the door */
				else
				{
					cave_set_feat(ny, nx, FEAT_OPEN);
				}

				/* Handle viewable doors */
				if (player_has_los_bold(ny, nx)) do_view = TRUE;
			}
		}

		/* Hack -- check for Glyph of Warding */
		if (do_move && (c_ptr->feat == FEAT_GLYPH) &&
		    !((r_ptr->flags1 & RF1_NEVER_BLOW) && (py == ny) && (px == nx)))
		{
			/* Assume no move allowed */
			do_move = FALSE;

			/* Break the ward */
			if (!is_pet(m_ptr) && (randint1(BREAK_GLYPH) < r_ptr->level))
			{
				/* Describe observable breakage */
				if (c_ptr->info & CAVE_MARK)
				{
#ifdef JP
msg_print("守りのルーンが壊れた！");
#else
					msg_print("The rune of protection is broken!");
#endif

				}

				/* Forget the rune */
				c_ptr->info &= ~(CAVE_MARK);

				/* Break the rune */
				c_ptr->feat = floor_type[randint0(100)];

				/* Allow movement */
				do_move = TRUE;

				/* Notice */
				note_spot(ny, nx);
			}
		}
		else if (do_move && (c_ptr->feat == FEAT_MINOR_GLYPH) &&
		         !((r_ptr->flags1 & RF1_NEVER_BLOW) && (py == ny) && (px == nx)))
		{
			/* Assume no move allowed */
			do_move = FALSE;

			/* Break the ward */
			if (!is_pet(m_ptr))
			{
				/* Break the ward */
				if (randint1(BREAK_MINOR_GLYPH) > r_ptr->level)
				{
					/* Describe observable breakage */
					if (c_ptr->info & CAVE_MARK)
					{
#ifdef JP
msg_print("ルーンが爆発した！");
#else
						msg_print("The rune explodes!");
#endif

						project(0, 2, ny, nx, 2 * (p_ptr->lev + damroll(7, 7)), GF_MANA, (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_REF | PROJECT_NO_HANGEKI), -1);
					}
				}
				else
				{
#ifdef JP
msg_print("爆発のルーンは解除された。");
#else
					msg_print("An explosive rune was disarmed.");
#endif
				}

				/* Forget the rune */
				c_ptr->info &= ~(CAVE_MARK);

				/* Break the rune */
				c_ptr->feat = floor_type[randint0(100)];
				note_spot(ny, nx);
				lite_spot(ny, nx);

				if (!m_ptr->r_idx) return;
				/* Allow movement */
				do_move = TRUE;
			}
		}
		if (do_move && (ny == py) && (nx == px) && (d_info[dungeon_type].flags1 & DF1_NO_MELEE))
		{
			do_move = FALSE;
		}

		/* Some monsters never attack */
		if (do_move && (ny == py) && (nx == px) &&
			(r_ptr->flags1 & RF1_NEVER_BLOW))
		{
			/* Hack -- memorize lack of attacks */
			if (m_ptr->ml) r_ptr->r_flags1 |= (RF1_NEVER_BLOW);

			/* Do not move */
			do_move = FALSE;
		}

		/* The player is in the way.  Attack him. */
		if (do_move && (ny == py) && (nx == px))
		{
			if (!p_ptr->riding || one_in_(2))
			{
				/* Do the attack */
				(void)make_attack_normal(m_idx);

				/* Do not move */
				do_move = FALSE;

				/* Took a turn */
				do_turn = TRUE;
			}
		}

		if ((c_ptr->feat >= FEAT_PATTERN_START) &&
			(c_ptr->feat <= FEAT_PATTERN_XTRA2) &&
			!do_turn && !(r_ptr->flags7 & RF7_CAN_FLY))
		{
			do_move = FALSE;
		}


		/* A monster is in the way */
		if (do_move && c_ptr->m_idx)
		{
			monster_race *z_ptr = &r_info[y_ptr->r_idx];
			monster_type *m2_ptr = &m_list[c_ptr->m_idx];

			/* Assume no movement */
			do_move = FALSE;

			/* Attack 'enemies' */
			if (((r_ptr->flags2 & (RF2_KILL_BODY)) &&
				  (r_ptr->mexp * r_ptr->level > z_ptr->mexp * z_ptr->level) &&
				  (cave_floor_grid(c_ptr)) &&
			     (c_ptr->m_idx != p_ptr->riding)) ||
				 are_enemies(m_ptr, m2_ptr) || m_ptr->confused)
			{
				do_move = FALSE;

				if (r_ptr->flags2 & RF2_KILL_BODY) r_ptr->r_flags2 |= (RF2_KILL_BODY);

				/* attack */
				if ((m2_ptr->r_idx) && (m2_ptr->hp >= 0))
				{
					if (monst_attack_monst(m_idx, cave[ny][nx].m_idx))
					return;
				}
			}

			/* Push past weaker monsters (unless leaving a wall) */
			else if ((r_ptr->flags2 & RF2_MOVE_BODY) &&
				(r_ptr->mexp > z_ptr->mexp) && cave_floor_grid(c_ptr) &&
				(cave_floor_grid(&cave[m_ptr->fy][m_ptr->fx])) &&
				 (c_ptr->m_idx != p_ptr->riding))
			{
				/* Allow movement */
				do_move = TRUE;

				/* Monster pushed past another monster */
				did_move_body = TRUE;

				/* XXX XXX XXX Message */
			}
		}

		/*
		 * Check if monster can cross terrain
		 * This is checked after the normal attacks
		 * to allow monsters to attack an enemy,
		 * even if it can't enter the terrain.
		 */
		if (do_move && !monster_can_cross_terrain(c_ptr->feat, r_ptr))
		{
			/* Assume no move allowed */
			do_move = FALSE;
		}

		/* Some monsters never move */
		if (do_move && (r_ptr->flags1 & RF1_NEVER_MOVE))
		{
			/* Hack -- memorize lack of attacks */
			if (m_ptr->ml) r_ptr->r_flags1 |= (RF1_NEVER_MOVE);

			/* Do not move */
			do_move = FALSE;
		}

		if (m_idx == p_ptr->riding)
		{
			if (!p_ptr->riding_ryoute && !(m_list[p_ptr->riding].monfear)) do_move = FALSE;
		}

		/* Creature has been allowed move */
		if (do_move)
		{
			s16b this_o_idx, next_o_idx;

			/* Take a turn */
			do_turn = TRUE;

			/* Hack -- Update the old location */
			cave[oy][ox].m_idx = c_ptr->m_idx;

			if (cave[ny][nx].feat == FEAT_TREES)
			{
				if (r_ptr->flags2 & RF2_KILL_WALL)
				{
					c_ptr->feat = FEAT_GRASS;
				}
				if (!(r_ptr->flags7 & RF7_CAN_FLY) && !(r_ptr->flags8 & RF8_WILD_WOOD))
				{
					m_ptr->energy_need += ENERGY_NEED();
				}
			}

			/* Mega-Hack -- move the old monster, if any */
			if (c_ptr->m_idx)
			{
				/* Move the old monster */
				y_ptr->fy = oy;
				y_ptr->fx = ox;

				/* Update the old monster */
				update_mon(c_ptr->m_idx, TRUE);

				/* Wake up the moved monster */
				m_list[c_ptr->m_idx].csleep = 0;
			}

			/* Hack -- Update the new location */
			c_ptr->m_idx = m_idx;

			/* Move the monster */
			m_ptr->fy = ny;
			m_ptr->fx = nx;

			/* Update the monster */
			update_mon(m_idx, TRUE);

			if (p_ptr->riding == m_idx)
			{
				py = ny;
				px = nx;
			}

			/* Redraw the old grid */
			lite_spot(oy, ox);

			/* Redraw the new grid */
			lite_spot(ny, nx);

			if (p_ptr->riding == m_idx)
			{
				verify_panel();

				/* Update stuff */
				p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

				/* Update the monsters */
				p_ptr->update |= (PU_DISTANCE);

				/* Window stuff */
				p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
			}

			/* Possible disturb */
			if (m_ptr->ml && (disturb_move ||
				((m_ptr->mflag & MFLAG_VIEW) &&
				disturb_near)))
			{
				/* Disturb */
				if (is_hostile(m_ptr))
					disturb(0, 0);
			}

			/* Scan all objects in the grid */
			for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
			{
				object_type *o_ptr;

				/* Acquire object */
				o_ptr = &o_list[this_o_idx];

				/* Acquire next object */
				next_o_idx = o_ptr->next_o_idx;

				/* Skip gold */
				if (o_ptr->tval == TV_GOLD) continue;

				/*
				 * Skip "real" corpses and statues, to avoid extreme
				 * silliness like a novice rogue pockets full of statues
				 * and corpses.
				 */
				if ((o_ptr->tval == TV_CORPSE) ||
				    (o_ptr->tval == TV_STATUE)) continue;

				/* Take or Kill objects on the floor */
				if ((r_ptr->flags2 & (RF2_TAKE_ITEM | RF2_KILL_ITEM)) &&
					 (!is_pet(m_ptr) || (p_ptr->pet_extra_flags & PF_PICKUP_ITEMS)))
				{
					u32b f1, f2, f3;

					u32b flg2 = 0L;
					u32b flg3 = 0L;

					char m_name[80];
					char o_name[MAX_NLEN];

					/* Extract some flags */
					object_flags(o_ptr, &f1, &f2, &f3);

					/* Acquire the object name */
					object_desc(o_name, o_ptr, TRUE, 3);

					/* Acquire the monster name */
					monster_desc(m_name, m_ptr, 0x04);

					/* React to objects that hurt the monster */
					if (f1 & TR1_KILL_DRAGON) flg3 |= (RF3_DRAGON);
					if (f1 & TR1_SLAY_DRAGON) flg3 |= (RF3_DRAGON);
					if (f1 & TR1_SLAY_TROLL)  flg3 |= (RF3_TROLL);
					if (f1 & TR1_SLAY_GIANT)  flg3 |= (RF3_GIANT);
					if (f1 & TR1_SLAY_ORC)    flg3 |= (RF3_ORC);
					if (f1 & TR1_SLAY_DEMON)  flg3 |= (RF3_DEMON);
					if (f1 & TR1_SLAY_UNDEAD) flg3 |= (RF3_UNDEAD);
					if (f1 & TR1_SLAY_ANIMAL) flg3 |= (RF3_ANIMAL);
					if (f1 & TR1_SLAY_EVIL)   flg3 |= (RF3_EVIL);
					if (f3 & TR3_SLAY_HUMAN)  flg2 |= (RF2_HUMAN);

					/* The object cannot be picked up by the monster */
					if (artifact_p(o_ptr) || (r_ptr->flags3 & flg3) || (r_ptr->flags2 & flg2) ||
						(o_ptr->art_name))
					{
						/* Only give a message for "take_item" */
						if ((r_ptr->flags2 & (RF2_TAKE_ITEM)) && (r_ptr->flags2 & (RF2_STUPID)))
						{
							/* Take note */
							did_take_item = TRUE;

							/* Describe observable situations */
							if (m_ptr->ml && player_can_see_bold(ny, nx))
							{
								/* Dump a message */
#ifdef JP
msg_format("%^sは%sを拾おうとしたが、だめだった。", m_name, o_name);
#else
msg_format("%^s tries to pick up %s, but fails.", m_name, o_name);
#endif
							}
						}
					}

					/* Pick up the item */
					else if (r_ptr->flags2 & RF2_TAKE_ITEM)
					{
						/* Take note */
						did_take_item = TRUE;

						/* Describe observable situations */
						if (player_can_see_bold(ny, nx))
						{
							/* Dump a message */
#ifdef JP
msg_format("%^sが%sを拾った。", m_name, o_name);
#else
							msg_format("%^s picks up %s.", m_name, o_name);
#endif

						}

						/* Excise the object */
						excise_object_idx(this_o_idx);

						/* Forget mark */
						o_ptr->marked = FALSE;

						/* Forget location */
						o_ptr->iy = o_ptr->ix = 0;

						/* Memorize monster */
						o_ptr->held_m_idx = m_idx;

						/* Build a stack */
						o_ptr->next_o_idx = m_ptr->hold_o_idx;

						/* Carry object */
						m_ptr->hold_o_idx = this_o_idx;
					}

					/* Destroy the item if not a pet */
					else if (!is_pet(m_ptr))
					{
						/* Take note */
						did_kill_item = TRUE;

						/* Describe observable situations */
						if (player_has_los_bold(ny, nx))
						{
							/* Dump a message */
#ifdef JP
msg_format("%^sが%sを破壊した。", m_name, o_name);
#else
							msg_format("%^s destroys %s.", m_name, o_name);
#endif

						}

						/* Delete the object */
						delete_object_idx(this_o_idx);
					}
				}
			}
		}

		/* Stop when done */
		if (do_turn) break;
	}

	/*
	 *  Forward movements failed, but now received LOS attack!
	 *  Try to flow by smell.
	 */
	if (p_ptr->no_flowed && i > 2 &&  m_ptr->target_y)
		m_ptr->mflag2 &= ~MFLAG_NOFLOW;

	/* If we haven't done anything, try casting a spell again */
	if (!do_turn && !do_move && !m_ptr->monfear && !stupid_monsters && !(p_ptr->riding == m_idx) && aware)
	{
		/* Cast spell */
		if (make_attack_spell(m_idx)) return;
	}


	/* Notice changes in view */
	if (do_view)
	{
		/* Update some things */
		p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS | PU_MON_LITE);
	}

	/* Notice changes in view */
	if (do_move && ((r_ptr->flags7 & (RF7_SELF_LITE_1 | RF7_SELF_LITE_2)) || ((r_ptr->flags7 & (RF7_HAS_LITE_1 | RF7_HAS_LITE_2)) && !p_ptr->inside_battle)))
	{
		/* Update some things */
		p_ptr->update |= (PU_MON_LITE);
	}

	/* Learn things from observable monster */
	if (m_ptr->ml)
	{
		/* Monster opened a door */
		if (did_open_door) r_ptr->r_flags2 |= (RF2_OPEN_DOOR);

		/* Monster bashed a door */
		if (did_bash_door) r_ptr->r_flags2 |= (RF2_BASH_DOOR);

		/* Monster tried to pick something up */
		if (did_take_item) r_ptr->r_flags2 |= (RF2_TAKE_ITEM);

		/* Monster tried to crush something */
		if (did_kill_item) r_ptr->r_flags2 |= (RF2_KILL_ITEM);

		/* Monster pushed past another monster */
		if (did_move_body) r_ptr->r_flags2 |= (RF2_MOVE_BODY);

		/* Monster passed through a wall */
		if (did_pass_wall) r_ptr->r_flags2 |= (RF2_PASS_WALL);

		/* Monster destroyed a wall */
		if (did_kill_wall) r_ptr->r_flags2 |= (RF2_KILL_WALL);
	}


	/* Hack -- get "bold" if out of options */
	if (!do_turn && !do_move && m_ptr->monfear && aware)
	{
		/* No longer afraid */
		m_ptr->monfear = 0;

		/* Message if seen */
		if (m_ptr->ml)
		{
			char m_name[80];

			/* Acquire the monster name */
			monster_desc(m_name, m_ptr, 0);

			/* Dump a message */
#ifdef JP
msg_format("%^sは戦いを決意した！", m_name);
#else
			msg_format("%^s turns to fight!", m_name);
#endif

			chg_virtue(V_COMPASSION, -1);
		}

		/* XXX XXX XXX Actually do something now (?) */
	}
}

/*
 * Process all the "live" monsters, once per game turn.
 *
 * During each game turn, we scan through the list of all the "live" monsters,
 * (backwards, so we can excise any "freshly dead" monsters), energizing each
 * monster, and allowing fully energized monsters to move, attack, pass, etc.
 *
 * Note that monsters can never move in the monster array (except when the
 * "compact_monsters()" function is called by "dungeon()" or "save_player()").
 *
 * This function is responsible for at least half of the processor time
 * on a normal system with a "normal" amount of monsters and a player doing
 * normal things.
 *
 * When the player is resting, virtually 90% of the processor time is spent
 * in this function, and its children, "process_monster()" and "make_move()".
 *
 * Most of the rest of the time is spent in "update_view()" and "lite_spot()",
 * especially when the player is running.
 *
 * Note the special "MFLAG_BORN" flag, which allows us to ignore "fresh"
 * monsters while they are still being "born".  A monster is "fresh" only
 * during the turn in which it is created, and we use the "hack_m_idx" to
 * determine if the monster is yet to be processed during the current turn.
 *
 * Note the special "MFLAG_NICE" flag, which allows the player to get one
 * move before any "nasty" monsters get to use their spell attacks.
 *
 * Note that when the "knowledge" about the currently tracked monster
 * changes (flags, attacks, spells), we induce a redraw of the monster
 * recall window.
 */
void process_monsters(void)
{
	int             i;
	int             fx, fy;

	bool            test;

	monster_type    *m_ptr;
	monster_race    *r_ptr;

	int             old_monster_race_idx;

	u32b    old_r_flags1 = 0L;
	u32b    old_r_flags2 = 0L;
	u32b    old_r_flags3 = 0L;
	u32b    old_r_flags4 = 0L;
	u32b    old_r_flags5 = 0L;
	u32b    old_r_flags6 = 0L;

	byte    old_r_blows0 = 0;
	byte    old_r_blows1 = 0;
	byte    old_r_blows2 = 0;
	byte    old_r_blows3 = 0;

	byte    old_r_cast_inate = 0;
	byte    old_r_cast_spell = 0;

	int speed;

        /* Clear monster fighting indicator */
        mon_fight = FALSE;

	/* Memorize old race */
	old_monster_race_idx = p_ptr->monster_race_idx;

	/* Acquire knowledge */
	if (p_ptr->monster_race_idx)
	{
		/* Acquire current monster */
		r_ptr = &r_info[p_ptr->monster_race_idx];

		/* Memorize flags */
		old_r_flags1 = r_ptr->r_flags1;
		old_r_flags2 = r_ptr->r_flags2;
		old_r_flags3 = r_ptr->r_flags3;
		old_r_flags4 = r_ptr->r_flags4;
		old_r_flags5 = r_ptr->r_flags5;
		old_r_flags6 = r_ptr->r_flags6;

		/* Memorize blows */
		old_r_blows0 = r_ptr->r_blows[0];
		old_r_blows1 = r_ptr->r_blows[1];
		old_r_blows2 = r_ptr->r_blows[2];
		old_r_blows3 = r_ptr->r_blows[3];

		/* Memorize castings */
		old_r_cast_inate = r_ptr->r_cast_inate;
		old_r_cast_spell = r_ptr->r_cast_spell;
	}


	/* Hack -- calculate the "player noise" */
	noise = (1L << (30 - p_ptr->skill_stl));


	/* Process the monsters (backwards) */
	for (i = m_max - 1; i >= 1; i--)
	{
		/* Access the monster */
		m_ptr = &m_list[i];
		r_ptr = &r_info[m_ptr->r_idx];

		/* Handle "leaving" */
		if (p_ptr->leaving) break;

		/* Ignore "dead" monsters */
		if (!m_ptr->r_idx) continue;

		if (p_ptr->wild_mode) continue;


		/* Handle "fresh" monsters */
		if (m_ptr->mflag & MFLAG_BORN)
		{
			/* No longer "fresh" */
			m_ptr->mflag &= ~(MFLAG_BORN);

			/* Skip */
			continue;
		}

		/* Hack -- Require proximity */
		if (m_ptr->cdis >= 100) continue;


		/* Access the location */
		fx = m_ptr->fx;
		fy = m_ptr->fy;

		/* Flow by smell is allowed */
		if (!stupid_monsters && !p_ptr->no_flowed)
		{
			m_ptr->mflag2 &= ~MFLAG_NOFLOW;
		}

		/* Assume no move */
		test = FALSE;

		/* Handle "sensing radius" */
		if (m_ptr->cdis <= (is_pet(m_ptr) ? (r_ptr->aaf > 20 ? 20 : r_ptr->aaf) : r_ptr->aaf))
		{
			/* We can "sense" the player */
			test = TRUE;
		}

		/* Handle "sight" and "aggravation" */
		else if ((m_ptr->cdis <= MAX_SIGHT) &&
			(player_has_los_bold(fy, fx) || (p_ptr->cursed & TRC_AGGRAVATE)))
		{
			/* We can "see" or "feel" the player */
			test = TRUE;
		}

#if 0 /* (cave[py][px].when == cave[fy][fx].when) is always FALSE... */
		/* Hack -- Monsters can "smell" the player from far away */
		/* Note that most monsters have "aaf" of "20" or so */
		else if (!stupid_monsters && !(m_ptr->mflag2 & MFLAG_NOFLOW) &&
			(cave_floor_bold(py, px) || (cave[py][px].feat == FEAT_TREES)) &&
			(cave[py][px].when == cave[fy][fx].when) &&
			(cave[fy][fx].dist < MONSTER_FLOW_DEPTH) &&
			(cave[fy][fx].dist < r_ptr->aaf))
		{
			/* We can "smell" the player */
			test = TRUE;
		}
#endif
		else if (m_ptr->target_y) test = TRUE;

		/* Do nothing */
		if (!test) continue;


		if (p_ptr->riding == i)
			speed = p_ptr->pspeed;
		else
		{
			speed = MIN(199, m_ptr->mspeed);

			/* Monsters move quickly in Nightmare mode */
			if (ironman_nightmare)
			{
				speed = MIN(199, m_ptr->mspeed + 5);
			}

			if (m_ptr->fast) speed = MIN(199, speed + 10);
			if (m_ptr->slow) speed = MAX(0, speed - 10);
		}

		/* Give this monster some energy */
		m_ptr->energy_need -= extract_energy[speed];

		/* Not enough energy to move */
		if (m_ptr->energy_need > 0) continue;

		/* Use up "some" energy */
		m_ptr->energy_need += ENERGY_NEED();


		/* Save global index */
		hack_m_idx = i;

		/* Process the monster */
		process_monster(i);

		reset_target(m_ptr);

		/* Give up flow_by_smell when it might useless */
		if (p_ptr->no_flowed && one_in_(3))
			m_ptr->mflag2 |= MFLAG_NOFLOW;

		/* Hack -- notice death or departure */
		if (!p_ptr->playing || p_ptr->is_dead) break;

		/* Notice leaving */
		if (p_ptr->leaving) break;
	}

	/* Reset global index */
	hack_m_idx = 0;


	/* Tracking a monster race (the same one we were before) */
	if (p_ptr->monster_race_idx && (p_ptr->monster_race_idx == old_monster_race_idx))
	{
		/* Acquire monster race */
		r_ptr = &r_info[p_ptr->monster_race_idx];

		/* Check for knowledge change */
		if ((old_r_flags1 != r_ptr->r_flags1) ||
			(old_r_flags2 != r_ptr->r_flags2) ||
			(old_r_flags3 != r_ptr->r_flags3) ||
			(old_r_flags4 != r_ptr->r_flags4) ||
			(old_r_flags5 != r_ptr->r_flags5) ||
			(old_r_flags6 != r_ptr->r_flags6) ||
			(old_r_blows0 != r_ptr->r_blows[0]) ||
			(old_r_blows1 != r_ptr->r_blows[1]) ||
			(old_r_blows2 != r_ptr->r_blows[2]) ||
			(old_r_blows3 != r_ptr->r_blows[3]) ||
			(old_r_cast_inate != r_ptr->r_cast_inate) ||
			(old_r_cast_spell != r_ptr->r_cast_spell))
		{
			/* Window stuff */
			p_ptr->window |= (PW_MONSTER);
		}
	}
}



bool process_the_world(int num, int who, bool vs_player)
{
	monster_type *m_ptr = &m_list[hack_m_idx];  /* the world monster */

	if(world_monster) return (FALSE);

	if(vs_player)
	{
		char m_name[80];
		monster_desc(m_name, m_ptr, 0);

		if (who == 1)
#ifdef JP
			msg_print("「『ザ・ワールド』！時は止まった！」");
#else
			msg_format("%s yells 'The World! Time has stopped!'", m_name);
#endif
		else if (who == 3)
#ifdef JP
			msg_print("「時よ！」");
#else
		        msg_format("%s yells 'Time!'", m_name);
#endif
		else msg_print("hek!");

		msg_print(NULL);
	}

	world_monster = TRUE;

	if (vs_player) do_cmd_redraw();

	while(num--)
	{
		if(!m_ptr->r_idx) break;
		process_monster(hack_m_idx);

		reset_target(m_ptr);

		/* Notice stuff */
		if (p_ptr->notice) notice_stuff();

		/* Update stuff */
		if (p_ptr->update) update_stuff();

		/* Redraw stuff */
		if (p_ptr->redraw) redraw_stuff();

		/* Redraw stuff */
		if (p_ptr->window) window_stuff();

		/* Delay */
		if (vs_player) Term_xtra(TERM_XTRA_DELAY, 500);
	}

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Update monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	world_monster = FALSE;
	if (vs_player || los(py, px, m_ptr->fy, m_ptr->fx))
	{
#ifdef JP
		msg_print("「時は動きだす…」");
#else
		msg_print("You feel time flowing around you once more.");
#endif
		msg_print(NULL);
	}

	handle_stuff();

	return (TRUE);
}


void monster_gain_exp(int m_idx, int s_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_race *s_ptr = &r_info[s_idx];
	int new_exp;

	if (p_ptr->inside_battle) return;

	if (!r_ptr->next_exp) return;

	new_exp = s_ptr->mexp * s_ptr->level / (r_ptr->level + 2);
	if (m_idx == p_ptr->riding) new_exp = (new_exp + 1) / 2;
	if (!dun_level) new_exp /= 5;
	m_ptr->exp += new_exp;
	if (m_ptr->mflag2 & MFLAG_CHAMELEON) return;

	if (m_ptr->exp >= r_ptr->next_exp)
	{
		char m_name[80];
		int old_hp = m_ptr->hp;
		int old_maxhp = m_ptr->max_maxhp;
		int old_r_idx = m_ptr->r_idx;
		int i;

		monster_desc(m_name, m_ptr, 0);
		m_ptr->r_idx = r_ptr->next_r_idx;
		m_ptr->ap_r_idx = m_ptr->r_idx;
		r_ptr = &r_info[m_ptr->r_idx];
		if (r_ptr->flags1 & RF1_FORCE_MAXHP)
		{
			m_ptr->max_maxhp = maxroll(r_ptr->hdice, r_ptr->hside);
		}
		else
		{
			m_ptr->max_maxhp = damroll(r_ptr->hdice, r_ptr->hside);
		}
		if (ironman_nightmare)
		{
			u32b hp = m_ptr->max_maxhp * 2L;

			m_ptr->max_maxhp = (s16b)MIN(30000, hp);
		}
		m_ptr->maxhp = m_ptr->max_maxhp;
		m_ptr->hp = old_hp * m_ptr->maxhp / old_maxhp;

		/* Extract the monster base speed */
		m_ptr->mspeed = r_ptr->speed;

		/* Hack -- small racial variety */
		if (!(r_ptr->flags1 & RF1_UNIQUE) && !p_ptr->inside_arena)
		{
			/* Allow some small variation per monster */
		  if(one_in_(4)){
			i = extract_energy[r_ptr->speed] / 3;
			if (i) m_ptr->mspeed += rand_spread(0, i);
		  }
		  else{
			i = extract_energy[r_ptr->speed] / 10;
			if (i) m_ptr->mspeed += rand_spread(0, i);
		  }
		}

		if (m_ptr->mspeed > 199) m_ptr->mspeed = 199;

		m_ptr->exp = 0;

		if (is_pet(m_ptr) || m_ptr->ml)
		{
#ifdef JP
			msg_format("%sは%sに進化した。", m_name, r_name + r_ptr->name);
#else
			msg_format("%^s evolved into %s.", m_name, r_name + r_ptr->name);
#endif
			r_info[old_r_idx].r_xtra1 |= MR1_SINKA;
		}
		update_mon(m_idx, FALSE);
		lite_spot(m_ptr->fy, m_ptr->fx);
	}
	if (m_idx == p_ptr->riding) p_ptr->update |= PU_BONUS;
}
