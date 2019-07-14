#include "angband.h"
#include "core.h"
#include "util.h"
#include "term.h"

#include "monster.h"
#include "monster-status.h"
#include "monster-spell.h"
#include "artifact.h"
#include "avatar.h"
#include "player-status.h"
#include "player-skill.h"
#include "player-class.h"
#include "player-personality.h"
#include "object-hook.h"
#include "object-broken.h"
#include "floor.h"
#include "grid.h"
#include "spells.h"
#include "object-flavor.h"

#include "shoot.h"
#include "snipe.h"
#include "view-mainwindow.h"
#include "objectkind.h"
#include "targeting.h"

/*!
 * @brief 矢弾を射撃した際のスレイ倍率をかけた結果を返す /
 * Determines the odds of an object breaking when thrown at a monster
 * @param o_ptr 矢弾のオブジェクト構造体参照ポインタ
 * @param tdam 計算途中のダメージ量
 * @param m_ptr 目標モンスターの構造体参照ポインタ
 * @return スレイ倍率をかけたダメージ量
 */
static MULTIPLY tot_dam_aux_shot(object_type *o_ptr, HIT_POINT tdam, monster_type *m_ptr, SPELL_IDX snipe_type)
{
	MULTIPLY mult = 10;

	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	BIT_FLAGS flgs[TR_FLAG_SIZE];
	object_flags(o_ptr, flgs);

	/* Some "weapons" and "ammo" do extra damage */
	switch (o_ptr->tval)
	{
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	{
		if ((have_flag(flgs, TR_SLAY_ANIMAL)) && (r_ptr->flags3 & RF3_ANIMAL))
		{
			if (is_original_ap_and_seen(m_ptr))
			{
				r_ptr->r_flags3 |= RF3_ANIMAL;
			}
			if (mult < 17) mult = 17;
		}

		if ((have_flag(flgs, TR_KILL_ANIMAL)) && (r_ptr->flags3 & RF3_ANIMAL))
		{
			if (is_original_ap_and_seen(m_ptr))
			{
				r_ptr->r_flags3 |= RF3_ANIMAL;
			}
			if (mult < 27) mult = 27;
		}

		if ((have_flag(flgs, TR_SLAY_EVIL)) && (r_ptr->flags3 & RF3_EVIL))
		{
			if (is_original_ap_and_seen(m_ptr))
			{
				r_ptr->r_flags3 |= RF3_EVIL;
			}
			if (mult < 15) mult = 15;
		}

		if ((have_flag(flgs, TR_KILL_EVIL)) && (r_ptr->flags3 & RF3_EVIL))
		{
			if (is_original_ap_and_seen(m_ptr))
			{
				r_ptr->r_flags3 |= RF3_EVIL;
			}
			if (mult < 25) mult = 25;
		}

		if ((have_flag(flgs, TR_SLAY_HUMAN)) && (r_ptr->flags2 & RF2_HUMAN))
		{
			if (is_original_ap_and_seen(m_ptr))
			{
				r_ptr->r_flags2 |= RF2_HUMAN;
			}
			if (mult < 17) mult = 17;
		}

		if ((have_flag(flgs, TR_KILL_HUMAN)) && (r_ptr->flags2 & RF2_HUMAN))
		{
			if (is_original_ap_and_seen(m_ptr))
			{
				r_ptr->r_flags2 |= RF2_HUMAN;
			}
			if (mult < 27) mult = 27;
		}

		if ((have_flag(flgs, TR_SLAY_UNDEAD)) && (r_ptr->flags3 & RF3_UNDEAD))
		{
			if (is_original_ap_and_seen(m_ptr))
			{
				r_ptr->r_flags3 |= RF3_UNDEAD;
			}
			if (mult < 20) mult = 20;
		}

		if ((have_flag(flgs, TR_KILL_UNDEAD)) && (r_ptr->flags3 & RF3_UNDEAD))
		{
			if (is_original_ap_and_seen(m_ptr))
			{
				r_ptr->r_flags3 |= RF3_UNDEAD;
			}
			if (mult < 30) mult = 30;
		}

		if ((have_flag(flgs, TR_SLAY_DEMON)) && (r_ptr->flags3 & RF3_DEMON))
		{
			if (is_original_ap_and_seen(m_ptr))
			{
				r_ptr->r_flags3 |= RF3_DEMON;
			}
			if (mult < 20) mult = 20;
		}

		if ((have_flag(flgs, TR_KILL_DEMON)) && (r_ptr->flags3 & RF3_DEMON))
		{
			if (is_original_ap_and_seen(m_ptr))
			{
				r_ptr->r_flags3 |= RF3_DEMON;
			}
			if (mult < 30) mult = 30;
		}

		if ((have_flag(flgs, TR_SLAY_ORC)) && (r_ptr->flags3 & RF3_ORC))
		{
			if (is_original_ap_and_seen(m_ptr))
			{
				r_ptr->r_flags3 |= RF3_ORC;
			}
			if (mult < 20) mult = 20;
		}

		if ((have_flag(flgs, TR_KILL_ORC)) && (r_ptr->flags3 & RF3_ORC))
		{
			if (is_original_ap_and_seen(m_ptr))
			{
				r_ptr->r_flags3 |= RF3_ORC;
			}
			if (mult < 30) mult = 30;
		}

		if ((have_flag(flgs, TR_SLAY_TROLL)) && (r_ptr->flags3 & RF3_TROLL))
		{
			if (is_original_ap_and_seen(m_ptr))
			{
				r_ptr->r_flags3 |= RF3_TROLL;
			}

			if (mult < 20) mult = 20;
		}

		if ((have_flag(flgs, TR_KILL_TROLL)) && (r_ptr->flags3 & RF3_TROLL))
		{
			if (is_original_ap_and_seen(m_ptr))
			{
				r_ptr->r_flags3 |= RF3_TROLL;
			}
			if (mult < 30) mult = 30;
		}

		if ((have_flag(flgs, TR_SLAY_GIANT)) && (r_ptr->flags3 & RF3_GIANT))
		{
			if (is_original_ap_and_seen(m_ptr))
			{
				r_ptr->r_flags3 |= RF3_GIANT;
			}
			if (mult < 20) mult = 20;
		}

		if ((have_flag(flgs, TR_KILL_GIANT)) && (r_ptr->flags3 & RF3_GIANT))
		{
			if (is_original_ap_and_seen(m_ptr))
			{
				r_ptr->r_flags3 |= RF3_GIANT;
			}
			if (mult < 30) mult = 30;
		}

		if ((have_flag(flgs, TR_SLAY_DRAGON)) && (r_ptr->flags3 & RF3_DRAGON))
		{
			if (is_original_ap_and_seen(m_ptr))
			{
				r_ptr->r_flags3 |= RF3_DRAGON;
			}
			if (mult < 20) mult = 20;
		}

		if ((have_flag(flgs, TR_KILL_DRAGON)) && (r_ptr->flags3 & RF3_DRAGON))
		{
			if (is_original_ap_and_seen(m_ptr))
			{
				r_ptr->r_flags3 |= RF3_DRAGON;
			}
			if (mult < 30) mult = 30;
			if ((o_ptr->name1 == ART_BARD_ARROW) && (m_ptr->r_idx == MON_SMAUG) &&
				(p_ptr->inventory_list[INVEN_BOW].name1 == ART_BARD))
				mult *= 5;
		}

		if (have_flag(flgs, TR_BRAND_ACID))
		{
			/* Notice immunity */
			if (r_ptr->flagsr & RFR_EFF_IM_ACID_MASK)
			{
				if (is_original_ap_and_seen(m_ptr))
				{
					r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_ACID_MASK);
				}
			}
			else
			{
				if (mult < 17) mult = 17;
			}
		}

		if (have_flag(flgs, TR_BRAND_ELEC))
		{
			/* Notice immunity */
			if (r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK)
			{
				if (is_original_ap_and_seen(m_ptr))
				{
					r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK);
				}
			}
			else
			{
				if (mult < 17) mult = 17;
			}
		}

		if (have_flag(flgs, TR_BRAND_FIRE))
		{
			/* Notice immunity */
			if (r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK)
			{
				if (is_original_ap_and_seen(m_ptr))
				{
					r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK);
				}
			}
			/* Otherwise, take the damage */
			else
			{
				if (r_ptr->flags3 & RF3_HURT_FIRE)
				{
					if (mult < 25) mult = 25;
					if (is_original_ap_and_seen(m_ptr))
					{
						r_ptr->r_flags3 |= RF3_HURT_FIRE;
					}
				}
				else if (mult < 17) mult = 17;
			}
		}

		if (have_flag(flgs, TR_BRAND_COLD))
		{
			/* Notice immunity */
			if (r_ptr->flagsr & RFR_EFF_IM_COLD_MASK)
			{
				if (is_original_ap_and_seen(m_ptr))
				{
					r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_COLD_MASK);
				}
			}
			/* Otherwise, take the damage */
			else
			{
				if (r_ptr->flags3 & RF3_HURT_COLD)
				{
					if (mult < 25) mult = 25;
					if (is_original_ap_and_seen(m_ptr))
					{
						r_ptr->r_flags3 |= RF3_HURT_COLD;
					}
				}
				else if (mult < 17) mult = 17;
			}
		}

		if (have_flag(flgs, TR_BRAND_POIS))
		{
			/* Notice immunity */
			if (r_ptr->flagsr & RFR_EFF_IM_POIS_MASK)
			{
				if (is_original_ap_and_seen(m_ptr))
				{
					r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_POIS_MASK);
				}
			}
			/* Otherwise, take the damage */
			else
			{
				if (mult < 17) mult = 17;
			}
		}

		if ((have_flag(flgs, TR_FORCE_WEAPON)) && (p_ptr->csp > (p_ptr->msp / 30)))
		{
			p_ptr->csp -= (1 + (p_ptr->msp / 30));
			p_ptr->redraw |= (PR_MANA);
			mult = mult * 5 / 2;
		}
		break;
	}
	}

	/* Sniper */
	if (snipe_type) mult = tot_dam_aux_snipe(mult, m_ptr, snipe_type);

	/* Return the total damage */
	return (tdam * mult / 10);
}


/*!
 * @brief 射撃処理実行 /
 * Fire an object from the pack or floor.
 * @param item 射撃するオブジェクトの所持ID
 * @param j_ptr 射撃武器のオブジェクト参照ポインタ
 * @return なし
 * @details
 * <pre>
 * You may only fire items that "match" your missile launcher.
 * You must use slings + pebbles/shots, bows + arrows, xbows + bolts.
 * See "calc_bonuses()" for more calculations and such.
 * Note that "firing" a missile is MUCH better than "throwing" it.
 * Note: "unseen" monsters are very hard to hit.
 * Objects are more likely to break if they "attempt" to hit a monster.
 * Rangers (with Bows) and Anyone (with "Extra Shots") get extra shots.
 * The "extra shot" code works by decreasing the amount of energy
 * required to make each shot, spreading the shots out over time.
 * Note that when firing missiles, the launcher multiplier is applied
 * after all the bonuses are added in, making multipliers very useful.
 * Note that Bows of "Extra Might" get extra range and an extra bonus
 * for the damage multiplier.
 * Note that Bows of "Extra Shots" give an extra shot.
 * </pre>
 */
void exe_fire(INVENTORY_IDX item, object_type *j_ptr, SPELL_IDX snipe_type)
{
	DIRECTION dir;
	int i;
	POSITION y, x, ny, nx, ty, tx, prev_y, prev_x;
	int tdam_base, tdis, thits, tmul;
	int bonus, chance;
	int cur_dis, visible;
	PERCENTAGE j;

	object_type forge;
	object_type *q_ptr;

	object_type *o_ptr;

	bool hit_body = FALSE;

	GAME_TEXT o_name[MAX_NLEN];

	u16b path_g[512];	/* For calcuration of path length */

	int msec = delay_factor * delay_factor * delay_factor;

	/* STICK TO */
	bool stick_to = FALSE;

	/* Access the item (if in the pack) */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory_list[item];
	}
	else
	{
		o_ptr = &current_floor_ptr->o_list[0 - item];
	}

	/* Sniper - Cannot shot a single arrow twice */
	if ((snipe_type == SP_DOUBLE) && (o_ptr->number < 2)) snipe_type = SP_NONE;

	object_desc(o_name, o_ptr, OD_OMIT_PREFIX);

	/* Use the proper number of shots */
	thits = p_ptr->num_fire;

	/* Use a base distance */
	tdis = 10;

	/* Base damage from thrown object plus launcher bonus */
	tdam_base = damroll(o_ptr->dd, o_ptr->ds) + o_ptr->to_d + j_ptr->to_d;

	/* Actually "fire" the object */
	bonus = (p_ptr->to_h_b + o_ptr->to_h + j_ptr->to_h);
	if ((j_ptr->sval == SV_LIGHT_XBOW) || (j_ptr->sval == SV_HEAVY_XBOW))
		chance = (p_ptr->skill_thb + (p_ptr->weapon_exp[0][j_ptr->sval] / 400 + bonus) * BTH_PLUS_ADJ);
	else
		chance = (p_ptr->skill_thb + ((p_ptr->weapon_exp[0][j_ptr->sval] - (WEAPON_EXP_MASTER / 2)) / 200 + bonus) * BTH_PLUS_ADJ);

	p_ptr->energy_use = bow_energy(j_ptr->sval);
	tmul = bow_tmul(j_ptr->sval);

	/* Get extra "power" from "extra might" */
	if (p_ptr->xtra_might) tmul++;

	tmul = tmul * (100 + (int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);

	/* Boost the damage */
	tdam_base *= tmul;
	tdam_base /= 100;

	/* Base range */
	tdis = 13 + tmul / 80;
	if ((j_ptr->sval == SV_LIGHT_XBOW) || (j_ptr->sval == SV_HEAVY_XBOW))
	{
		if (p_ptr->concent)
			tdis -= (5 - (p_ptr->concent + 1) / 2);
		else
			tdis -= 5;
	}

	project_length = tdis + 1;

	/* Get a direction (or cancel) */
	if (!get_aim_dir(&dir))
	{
		free_turn(p_ptr);

		if (snipe_type == SP_AWAY) snipe_type = SP_NONE;

		/* need not to reset project_length (already did)*/

		return;
	}

	/* Predict the "target" location */
	tx = p_ptr->x + 99 * ddx[dir];
	ty = p_ptr->y + 99 * ddy[dir];

	/* Check for "target request" */
	if ((dir == 5) && target_okay())
	{
		tx = target_col;
		ty = target_row;
	}

	/* Get projection path length */
	tdis = project_path(path_g, project_length, p_ptr->y, p_ptr->x, ty, tx, PROJECT_PATH | PROJECT_THRU) - 1;

	project_length = 0; /* reset to default */

	/* Don't shoot at my feet */
	if (tx == p_ptr->x && ty == p_ptr->y)
	{
		free_turn(p_ptr);

		/* project_length is already reset to 0 */

		return;
	}


	/* Take a (partial) current_world_ptr->game_turn */
	p_ptr->energy_use = (p_ptr->energy_use / thits);
	p_ptr->is_fired = TRUE;

	/* Sniper - Difficult to shot twice at 1 current_world_ptr->game_turn */
	if (snipe_type == SP_DOUBLE)  p_ptr->concent = (p_ptr->concent + 1) / 2;

	/* Sniper - Repeat shooting when double shots */
	for (i = 0; i < ((snipe_type == SP_DOUBLE) ? 2 : 1); i++)
	{

		/* Start at the player */
		y = p_ptr->y;
		x = p_ptr->x;
		q_ptr = &forge;
		object_copy(q_ptr, o_ptr);

		/* Single object */
		q_ptr->number = 1;

		/* Reduce and describe p_ptr->inventory_list */
		if (item >= 0)
		{
			inven_item_increase(item, -1);
			inven_item_describe(item);
			inven_item_optimize(item);
		}

		/* Reduce and describe floor item */
		else
		{
			floor_item_increase(0 - item, -1);
			floor_item_optimize(0 - item);
		}

		sound(SOUND_SHOOT);
		handle_stuff();

		prev_y = y;
		prev_x = x;

		/* The shot does not hit yet */
		hit_body = FALSE;

		/* Travel until stopped */
		for (cur_dis = 0; cur_dis <= tdis; )
		{
			grid_type *g_ptr;

			/* Hack -- Stop at the target */
			if ((y == ty) && (x == tx)) break;

			/* Calculate the new location (see "project()") */
			ny = y;
			nx = x;
			mmove2(&ny, &nx, p_ptr->y, p_ptr->x, ty, tx);

			/* Shatter Arrow */
			if (snipe_type == SP_KILL_WALL)
			{
				g_ptr = &current_floor_ptr->grid_array[ny][nx];

				if (cave_have_flag_grid(g_ptr, FF_HURT_ROCK) && !g_ptr->m_idx)
				{
					if (g_ptr->info & (CAVE_MARK)) msg_print(_("岩が砕け散った。", "Wall rocks were shattered."));
					/* Forget the wall */
					g_ptr->info &= ~(CAVE_MARK);
					p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE);

					/* Destroy the wall */
					cave_alter_feat(ny, nx, FF_HURT_ROCK);

					hit_body = TRUE;
					break;
				}
			}

			/* Stopped by walls/doors */
			if (!cave_have_flag_bold(ny, nx, FF_PROJECT) && !current_floor_ptr->grid_array[ny][nx].m_idx) break;

			/* Advance the distance */
			cur_dis++;

			/* Sniper */
			if (snipe_type == SP_LITE)
			{
				current_floor_ptr->grid_array[ny][nx].info |= (CAVE_GLOW);
				note_spot(ny, nx);
				lite_spot(ny, nx);
			}

			/* The player can see the (on screen) missile */
			if (panel_contains(ny, nx) && player_can_see_bold(ny, nx))
			{
				char c = object_char(q_ptr);
				byte a = object_attr(q_ptr);

				/* Draw, Hilite, Fresh, Pause, Erase */
				print_rel(c, a, ny, nx);
				move_cursor_relative(ny, nx);
				Term_fresh();
				Term_xtra(TERM_XTRA_DELAY, msec);
				lite_spot(ny, nx);
				Term_fresh();
			}

			/* The player cannot see the missile */
			else
			{
				/* Pause anyway, for consistancy */
				Term_xtra(TERM_XTRA_DELAY, msec);
			}

			/* Sniper */
			if (snipe_type == SP_KILL_TRAP)
			{
				project(0, 0, ny, nx, 0, GF_KILL_TRAP,
					(PROJECT_JUMP | PROJECT_HIDE | PROJECT_GRID | PROJECT_ITEM), -1);
			}

			/* Sniper */
			if (snipe_type == SP_EVILNESS)
			{
				current_floor_ptr->grid_array[ny][nx].info &= ~(CAVE_GLOW | CAVE_MARK);
				note_spot(ny, nx);
				lite_spot(ny, nx);
			}

			prev_y = y;
			prev_x = x;

			/* Save the new location */
			x = nx;
			y = ny;

			/* Monster here, Try to hit it */
			if (current_floor_ptr->grid_array[y][x].m_idx)
			{
				grid_type *c_mon_ptr = &current_floor_ptr->grid_array[y][x];

				monster_type *m_ptr = &current_floor_ptr->m_list[c_mon_ptr->m_idx];
				monster_race *r_ptr = &r_info[m_ptr->r_idx];

				/* Check the visibility */
				visible = m_ptr->ml;

				/* Note the collision */
				hit_body = TRUE;

				if (MON_CSLEEP(m_ptr))
				{
					if (!(r_ptr->flags3 & RF3_EVIL) || one_in_(5)) chg_virtue(p_ptr, V_COMPASSION, -1);
					if (!(r_ptr->flags3 & RF3_EVIL) || one_in_(5)) chg_virtue(p_ptr, V_HONOUR, -1);
				}

				if ((r_ptr->level + 10) > p_ptr->lev)
				{
					int now_exp = p_ptr->weapon_exp[0][j_ptr->sval];
					if (now_exp < s_info[p_ptr->pclass].w_max[0][j_ptr->sval])
					{
						SUB_EXP amount = 0;
						if (now_exp < WEAPON_EXP_BEGINNER) amount = 80;
						else if (now_exp < WEAPON_EXP_SKILLED) amount = 25;
						else if ((now_exp < WEAPON_EXP_EXPERT) && (p_ptr->lev > 19)) amount = 10;
						else if (p_ptr->lev > 34) amount = 2;
						p_ptr->weapon_exp[0][j_ptr->sval] += amount;
						p_ptr->update |= (PU_BONUS);
					}
				}

				if (p_ptr->riding)
				{
					if ((p_ptr->skill_exp[GINOU_RIDING] < s_info[p_ptr->pclass].s_max[GINOU_RIDING])
						&& ((p_ptr->skill_exp[GINOU_RIDING] - (RIDING_EXP_BEGINNER * 2)) / 200 < r_info[current_floor_ptr->m_list[p_ptr->riding].r_idx].level)
						&& one_in_(2))
					{
						p_ptr->skill_exp[GINOU_RIDING] += 1;
						p_ptr->update |= (PU_BONUS);
					}
				}

				/* Did we hit it (penalize range) */
				if (test_hit_fire(chance - cur_dis, m_ptr, m_ptr->ml, o_name))
				{
					bool fear = FALSE;
					int tdam = tdam_base;

					/* Get extra damage from concentration */
					if (p_ptr->concent) tdam = boost_concentration_damage(tdam);

					/* Handle unseen monster */
					if (!visible)
					{
						/* Invisible monster */
						msg_format(_("%sが敵を捕捉した。", "The %s finds a mark."), o_name);
					}

					/* Handle visible monster */
					else
					{
						GAME_TEXT m_name[MAX_NLEN];

						/* Get "the monster" or "it" */
						monster_desc(m_name, m_ptr, 0);

						msg_format(_("%sが%sに命中した。", "The %s hits %s."), o_name, m_name);

						if (m_ptr->ml)
						{
							if (!p_ptr->image) monster_race_track(m_ptr->ap_r_idx);
							health_track(c_mon_ptr->m_idx);
						}
					}

					if (snipe_type == SP_NEEDLE)
					{
						if ((randint1(randint1(r_ptr->level / (3 + p_ptr->concent)) + (8 - p_ptr->concent)) == 1)
							&& !(r_ptr->flags1 & RF1_UNIQUE) && !(r_ptr->flags7 & RF7_UNIQUE2))
						{
							GAME_TEXT m_name[MAX_NLEN];

							/* Get "the monster" or "it" */
							monster_desc(m_name, m_ptr, 0);

							tdam = m_ptr->hp + 1;
							msg_format(_("%sの急所に突き刺さった！", "Your shot sticked on a fatal spot of %s!"), m_name);
						}
						else tdam = 1;
					}
					else
					{
						/* Apply special damage */
						tdam = tot_dam_aux_shot(q_ptr, tdam, m_ptr, snipe_type);
						tdam = critical_shot(q_ptr->weight, q_ptr->to_h, j_ptr->to_h, tdam);

						/* No negative damage */
						if (tdam < 0) tdam = 0;

						/* Modify the damage */
						tdam = mon_damage_mod(m_ptr, tdam, FALSE);
					}

					msg_format_wizard(CHEAT_MONSTER,
						_("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"),
						tdam, m_ptr->hp - tdam, m_ptr->maxhp, m_ptr->max_maxhp);

					/* Sniper */
					if (snipe_type == SP_EXPLODE)
					{
						u16b flg = (PROJECT_STOP | PROJECT_JUMP | PROJECT_KILL | PROJECT_GRID);

						sound(SOUND_EXPLODE); /* No explode sound - use breath fire instead */
						project(0, ((p_ptr->concent + 1) / 2 + 1), ny, nx, tdam, GF_MISSILE, flg, -1);
						break;
					}

					/* Sniper */
					if (snipe_type == SP_HOLYNESS)
					{
						current_floor_ptr->grid_array[ny][nx].info |= (CAVE_GLOW);
						note_spot(ny, nx);
						lite_spot(ny, nx);
					}

					/* Hit the monster, check for death */
					if (mon_take_hit(c_mon_ptr->m_idx, tdam, &fear, extract_note_dies(real_r_idx(m_ptr))))
					{
						/* Dead monster */
					}

					/* No death */
					else
					{
						/* STICK TO */
						if (object_is_fixed_artifact(q_ptr) &&
							(p_ptr->pclass != CLASS_SNIPER || p_ptr->concent == 0))
						{
							GAME_TEXT m_name[MAX_NLEN];

							monster_desc(m_name, m_ptr, 0);

							stick_to = TRUE;
							msg_format(_("%sは%sに突き刺さった！", "%^s have stuck into %s!"), o_name, m_name);
						}

						message_pain(c_mon_ptr->m_idx, tdam);

						/* Anger the monster */
						if (tdam > 0) anger_monster(m_ptr);

						if (fear && m_ptr->ml)
						{
							GAME_TEXT m_name[MAX_NLEN];
							sound(SOUND_FLEE);
							monster_desc(m_name, m_ptr, 0);
							msg_format(_("%^sは恐怖して逃げ出した！", "%^s flees in terror!"), m_name);
						}

						set_target(m_ptr, p_ptr->y, p_ptr->x);

						/* Sniper */
						if (snipe_type == SP_RUSH)
						{
							int n = randint1(5) + 3;
							MONSTER_IDX m_idx = c_mon_ptr->m_idx;

							for (; cur_dis <= tdis; )
							{
								POSITION ox = nx;
								POSITION oy = ny;

								if (!n) break;

								/* Calculate the new location (see "project()") */
								mmove2(&ny, &nx, p_ptr->y, p_ptr->x, ty, tx);

								/* Stopped by wilderness boundary */
								if (!in_bounds2(ny, nx)) break;

								/* Stopped by walls/doors */
								if (!player_can_enter(current_floor_ptr->grid_array[ny][nx].feat, 0)) break;

								/* Stopped by monsters */
								if (!cave_empty_bold(ny, nx)) break;

								current_floor_ptr->grid_array[ny][nx].m_idx = m_idx;
								current_floor_ptr->grid_array[oy][ox].m_idx = 0;

								m_ptr->fx = nx;
								m_ptr->fy = ny;

								update_monster(c_mon_ptr->m_idx, TRUE);

								lite_spot(ny, nx);
								lite_spot(oy, ox);

								Term_fresh();
								Term_xtra(TERM_XTRA_DELAY, msec);

								x = nx;
								y = ny;
								cur_dis++;
								n--;
							}
						}
					}
				}

				/* Sniper */
				if (snipe_type == SP_PIERCE)
				{
					if (p_ptr->concent < 1) break;
					p_ptr->concent--;
					continue;
				}

				/* Stop looking */
				break;
			}
		}

		/* Chance of breakage (during attacks) */
		j = (hit_body ? breakage_chance(q_ptr, p_ptr->pclass == CLASS_ARCHER, snipe_type) : 0);

		if (stick_to)
		{
			MONSTER_IDX m_idx = current_floor_ptr->grid_array[y][x].m_idx;
			monster_type *m_ptr = &current_floor_ptr->m_list[m_idx];
			OBJECT_IDX o_idx = o_pop();

			if (!o_idx)
			{
				msg_format(_("%sはどこかへ行った。", "The %s have gone to somewhere."), o_name);
				if (object_is_fixed_artifact(q_ptr))
				{
					a_info[j_ptr->name1].cur_num = 0;
				}
				return;
			}

			o_ptr = &current_floor_ptr->o_list[o_idx];
			object_copy(o_ptr, q_ptr);

			/* Forget mark */
			o_ptr->marked &= OM_TOUCHED;

			/* Forget location */
			o_ptr->iy = o_ptr->ix = 0;

			/* Memorize monster */
			o_ptr->held_m_idx = m_idx;

			/* Build a stack */
			o_ptr->next_o_idx = m_ptr->hold_o_idx;

			/* Carry object */
			m_ptr->hold_o_idx = o_idx;
		}
		else if (cave_have_flag_bold(y, x, FF_PROJECT))
		{
			/* Drop (or break) near that location */
			(void)drop_near(q_ptr, j, y, x);
		}
		else
		{
			/* Drop (or break) near that location */
			(void)drop_near(q_ptr, j, prev_y, prev_x);
		}

		/* Sniper - Repeat shooting when double shots */
	}

	/* Sniper - Loose his/her concentration after any shot */
	if (p_ptr->concent) reset_concentration(FALSE);
}


/*!
* @brief プレイヤーからモンスターへの射撃命中判定 /
* Determine if the player "hits" a monster (normal combat).
* @param chance 基本命中値
* @param m_ptr モンスターの構造体参照ポインタ
* @param vis 目標を視界に捕らえているならばTRUEを指定
* @param o_name メッセージ表示時のモンスター名
* @return 命中と判定された場合TRUEを返す
* @note Always miss 5%, always hit 5%, otherwise random.
*/
bool test_hit_fire(int chance, monster_type *m_ptr, int vis, char* o_name)
{
	int k;
	ARMOUR_CLASS ac;
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Percentile dice */
	k = randint1(100);

	/* Snipers with high-concentration reduce instant miss percentage.*/
	k += p_ptr->concent;

	/* Hack -- Instant miss or hit */
	if (k <= 5) return (FALSE);
	if (k > 95) return (TRUE);

	if (p_ptr->pseikaku == SEIKAKU_NAMAKE)
		if (one_in_(20)) return (FALSE);

	/* Never hit */
	if (chance <= 0) return (FALSE);

	ac = r_ptr->ac;
	if (p_ptr->concent)
	{
		ac *= (8 - p_ptr->concent);
		ac /= 8;
	}

	if (m_ptr->r_idx == MON_GOEMON && !MON_CSLEEP(m_ptr)) ac *= 3;

	/* Invisible monsters are harder to hit */
	if (!vis) chance = (chance + 1) / 2;

	/* Power competes against armor */
	if (randint0(chance) < (ac * 3 / 4))
	{
		if (m_ptr->r_idx == MON_GOEMON && !MON_CSLEEP(m_ptr))
		{
			GAME_TEXT m_name[MAX_NLEN];
			monster_desc(m_name, m_ptr, 0);
			msg_format(_("%sは%sを斬り捨てた！", "%s cuts down %s!"), m_name, o_name);
		}
		return (FALSE);
	}

	/* Assume hit */
	return (TRUE);
}




/*!
* @brief プレイヤーからモンスターへの射撃クリティカル判定 /
* Critical hits (from objects thrown by player) Factor in item weight, total plusses, and player level.
* @param weight 矢弾の重量
* @param plus_ammo 矢弾の命中修正
* @param plus_bow 弓の命中修正
* @param dam 現在算出中のダメージ値
* @return クリティカル修正が入ったダメージ値
*/
HIT_POINT critical_shot(WEIGHT weight, int plus_ammo, int plus_bow, HIT_POINT dam)
{
	int i, k;
	object_type *j_ptr = &p_ptr->inventory_list[INVEN_BOW];

	/* Extract "shot" power */
	i = p_ptr->to_h_b + plus_ammo;

	if (p_ptr->tval_ammo == TV_BOLT)
		i = (p_ptr->skill_thb + (p_ptr->weapon_exp[0][j_ptr->sval] / 400 + i) * BTH_PLUS_ADJ);
	else
		i = (p_ptr->skill_thb + ((p_ptr->weapon_exp[0][j_ptr->sval] - (WEAPON_EXP_MASTER / 2)) / 200 + i) * BTH_PLUS_ADJ);


	/* Snipers can shot more critically with crossbows */
	if (p_ptr->concent) i += ((i * p_ptr->concent) / 5);
	if ((p_ptr->pclass == CLASS_SNIPER) && (p_ptr->tval_ammo == TV_BOLT)) i *= 2;

	/* Good bow makes more critical */
	i += plus_bow * 8 * (p_ptr->concent ? p_ptr->concent + 5 : 5);

	/* Critical hit */
	if (randint1(10000) <= i)
	{
		k = weight * randint1(500);

		if (k < 900)
		{
			msg_print(_("手ごたえがあった！", "It was a good hit!"));
			dam += (dam / 2);
		}
		else if (k < 1350)
		{
			msg_print(_("かなりの手ごたえがあった！", "It was a great hit!"));
			dam *= 2;
		}
		else
		{
			msg_print(_("会心の一撃だ！", "It was a superb hit!"));
			dam *= 3;
		}
	}

	return (dam);
}




/*!
 * @brief 射撃武器の攻撃に必要な基本消費エネルギーを返す/Return bow energy
 * @param sval 射撃武器のアイテム副分類ID
 * @return 消費する基本エネルギー
 */
ENERGY bow_energy(OBJECT_SUBTYPE_VALUE sval)
{
	ENERGY energy = 10000;

	/* Analyze the launcher */
	switch (sval)
	{
		/* Sling and ammo */
	case SV_SLING:
	{
		energy = 8000;
		break;
	}

	/* Short Bow and Arrow */
	case SV_SHORT_BOW:
	{
		energy = 10000;
		break;
	}

	/* Long Bow and Arrow */
	case SV_LONG_BOW:
	{
		energy = 10000;
		break;
	}

	/* Bow of irresponsiblity and Arrow */
	case SV_NAMAKE_BOW:
	{
		energy = 7777;
		break;
	}

	/* Light Crossbow and Bolt */
	case SV_LIGHT_XBOW:
	{
		energy = 12000;
		break;
	}

	/* Heavy Crossbow and Bolt */
	case SV_HEAVY_XBOW:
	{
		energy = 13333;
		break;
	}
	}

	return (energy);
}


/*
 * Return bow tmul
 */
int bow_tmul(OBJECT_SUBTYPE_VALUE sval)
{
	int tmul = 0;

	/* Analyze the launcher */
	switch (sval)
	{
		/* Sling and ammo */
	case SV_SLING:
	{
		tmul = 2;
		break;
	}

	/* Short Bow and Arrow */
	case SV_SHORT_BOW:
	{
		tmul = 2;
		break;
	}

	/* Long Bow and Arrow */
	case SV_LONG_BOW:
	{
		tmul = 3;
		break;
	}

	/* Bow of irresponsiblity and Arrow */
	case SV_NAMAKE_BOW:
	{
		tmul = 3;
		break;
	}

	/* Light Crossbow and Bolt */
	case SV_LIGHT_XBOW:
	{
		tmul = 3;
		break;
	}

	/* Heavy Crossbow and Bolt */
	case SV_HEAVY_XBOW:
	{
		tmul = 4;
		break;
	}
	}

	return (tmul);
}


/*!
 * @brief 射撃時クリティカルによるダメージ期待値修正計算（スナイパーの集中処理と武器経験値） / critical happens at i / 10000
 * @param plus_ammo 矢弾のダメージ修正
 * @param plus_bow 弓のダメージ修正
 * @return ダメージ期待値
 * @note 基本ダメージ量と重量はこの部位では計算に加わらない。
 */
HIT_POINT calc_crit_ratio_shot(HIT_POINT plus_ammo, HIT_POINT plus_bow)
{
	HIT_POINT i;
	object_type *j_ptr = &p_ptr->inventory_list[INVEN_BOW];

	/* Extract "shot" power */
	i = p_ptr->to_h_b + plus_ammo;

	if (p_ptr->tval_ammo == TV_BOLT)
		i = (p_ptr->skill_thb + (p_ptr->weapon_exp[0][j_ptr->sval] / 400 + i) * BTH_PLUS_ADJ);
	else
		i = (p_ptr->skill_thb + ((p_ptr->weapon_exp[0][j_ptr->sval] - (WEAPON_EXP_MASTER / 2)) / 200 + i) * BTH_PLUS_ADJ);

	/* Snipers can shot more critically with crossbows */
	if (p_ptr->concent) i += ((i * p_ptr->concent) / 5);
	if ((p_ptr->pclass == CLASS_SNIPER) && (p_ptr->tval_ammo == TV_BOLT)) i *= 2;

	/* Good bow makes more critical */
	i += plus_bow * 8 * (p_ptr->concent ? p_ptr->concent + 5 : 5);

	if (i < 0) i = 0;

	return i;
}

/*!
 * @brief 射撃時クリティカルによるダメージ期待値修正計算（重量依存部分） / critical happens at i / 10000
 * @param weight 武器の重量
 * @param plus_ammo 矢弾のダメージ修正
 * @param plus_bow 弓のダメージ修正
 * @param dam 基本ダメージ量
 * @return ダメージ期待値
 */
HIT_POINT calc_expect_crit_shot(WEIGHT weight, int plus_ammo, int plus_bow, HIT_POINT dam)
{
	u32b num;
	int i, k, crit;
	i = calc_crit_ratio_shot(plus_ammo, plus_bow);

	k = 0;
	num = 0;

	crit = MIN(500, 900 / weight);
	num += dam * 3 / 2 * crit;
	k = crit;

	crit = MIN(500, 1350 / weight);
	crit -= k;
	num += dam * 2 * crit;
	k += crit;

	if (k < 500)
	{
		crit = 500 - k;
		num += dam * 3 * crit;
	}

	num /= 500;

	num *= i;
	num += (10000 - i) * dam;
	num /= 10000;

	return num;
}

/*!
 * @brief 攻撃時クリティカルによるダメージ期待値修正計算（重量と毒針処理） / critical happens at i / 10000
 * @param weight 武器の重量
 * @param plus 武器のダメージ修正
 * @param dam 基本ダメージ
 * @param meichuu 命中値
 * @param dokubari 毒針処理か否か
 * @return ダメージ期待値
 */
HIT_POINT calc_expect_crit(WEIGHT weight, int plus, HIT_POINT dam, s16b meichuu, bool dokubari)
{
	u32b k, num;
	int i;

	if (dokubari) return dam;

	i = (weight + (meichuu * 3 + plus * 5) + p_ptr->skill_thn);
	if (i < 0) i = 0;

	k = weight;
	num = 0;

	if (k < 400)						num += (2 * dam + 5) * (400 - k);
	if (k < 700)						num += (2 * dam + 10) * (MIN(700, k + 650) - MAX(400, k));
	if (k > (700 - 650) && k < 900)		num += (3 * dam + 15) * (MIN(900, k + 650) - MAX(700, k));
	if (k > (900 - 650) && k < 1300)		num += (3 * dam + 20) * (MIN(1300, k + 650) - MAX(900, k));
	if (k > (1300 - 650))					num += (7 * dam / 2 + 25) * MIN(650, k - (1300 - 650));

	num /= 650;
	if (p_ptr->pclass == CLASS_NINJA)
	{
		num *= i;
		num += (4444 - i) * dam;
		num /= 4444;
	}
	else
	{
		num *= i;
		num += (5000 - i) * dam;
		num /= 5000;
	}

	return num;
}

