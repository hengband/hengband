/*!
 * @file xtra2.c
 * @brief 雑多なその他の処理2 / effects of various "objects"
 * @date 2014/02/06
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2014 Deskull rearranged comment for Doxygen.
 */


#include "angband.h"
#include "util.h"
#include "core.h"
#include "targeting.h"

#include "bldg.h"
#include "cmd-pet.h"
#include "dungeon-file.h"
#include "object-curse.h"
#include "object-flavor.h"
#include "monster.h"
#include "monsterrace-hook.h"
#include "objectkind-hook.h"
#include "sort.h"
#include "spells-summon.h"
#include "grid.h"
#include "floor.h"
#include "floor-events.h"
#include "floor-town.h"
#include "player-move.h"
#include "player-status.h"
#include "monster-status.h"
#include "view-mainwindow.h"
#include "feature.h"
#include "quest.h"
#include "dungeon.h"
#include "spells.h"

/*!
 * @brief コンソール上におけるマップ表示の左上位置を返す /
 * Calculates current boundaries Called below and from "do_cmd_locate()".
 * @return なし
 */
void panel_bounds_center(void)
{
	TERM_LEN wid, hgt;

	get_screen_size(&wid, &hgt);

	panel_row_max = panel_row_min + hgt - 1;
	panel_row_prt = panel_row_min - 1;
	panel_col_max = panel_col_min + wid - 1;
	panel_col_prt = panel_col_min - 13;
}

/*!
 * @brief フォーカスを当てるべきマップ描画の基準座標を指定する
 * @param y 変更先のフロアY座標
 * @param x 変更先のフロアX座標
 * @details
 * Handle a request to change the current panel
 * Return TRUE if the panel was changed.
 * Also used in do_cmd_locate
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
static bool change_panel_xy(POSITION y, POSITION x)
{
	POSITION dy = 0, dx = 0;
	TERM_LEN wid, hgt;

	get_screen_size(&wid, &hgt);

	if (y < panel_row_min) dy = -1;
	if (y > panel_row_max) dy = 1;
	if (x < panel_col_min) dx = -1;
	if (x > panel_col_max) dx = 1;

	if (!dy && !dx) return (FALSE);

	return change_panel(dy, dx);
}


/*!
 * @brief マップ描画のフォーカスを当てるべき座標を更新する
 * @details
 * Given an row (y) and col (x), this routine detects when a move
 * off the screen has occurred and figures new borders. -RAK-
 * "Update" forces a "full update" to take place.
 * The map is reprinted if necessary, and "TRUE" is returned.
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
void verify_panel(void)
{
	POSITION y = p_ptr->y;
	POSITION x = p_ptr->x;
	TERM_LEN wid, hgt;

	int prow_min;
	int pcol_min;
	int max_prow_min;
	int max_pcol_min;

	get_screen_size(&wid, &hgt);

	max_prow_min = current_floor_ptr->height - hgt;
	max_pcol_min = current_floor_ptr->width - wid;

	/* Bounds checking */
	if (max_prow_min < 0) max_prow_min = 0;
	if (max_pcol_min < 0) max_pcol_min = 0;

		/* Center on player */
	if (center_player && (center_running || !running))
	{
		/* Center vertically */
		prow_min = y - hgt / 2;
		if (prow_min < 0) prow_min = 0;
		else if (prow_min > max_prow_min) prow_min = max_prow_min;

		/* Center horizontally */
		pcol_min = x - wid / 2;
		if (pcol_min < 0) pcol_min = 0;
		else if (pcol_min > max_pcol_min) pcol_min = max_pcol_min;
	}
	else
	{
		prow_min = panel_row_min;
		pcol_min = panel_col_min;

		/* Scroll screen when 2 grids from top/bottom edge */
		if (y > panel_row_max - 2)
		{
			while (y > prow_min + hgt-1 - 2)
			{
				prow_min += (hgt / 2);
			}
		}

		if (y < panel_row_min + 2)
		{
			while (y < prow_min + 2)
			{
				prow_min -= (hgt / 2);
			}
		}

		if (prow_min > max_prow_min) prow_min = max_prow_min;
		if (prow_min < 0) prow_min = 0;

		/* Scroll screen when 4 grids from left/right edge */
		if (x > panel_col_max - 4)
		{
			while (x > pcol_min + wid-1 - 4)
			{
				pcol_min += (wid / 2);
			}
		}
		
		if (x < panel_col_min + 4)
		{
			while (x < pcol_min + 4)
			{
				pcol_min -= (wid / 2);
			}
		}

		if (pcol_min > max_pcol_min) pcol_min = max_pcol_min;
		if (pcol_min < 0) pcol_min = 0;
	}

	/* Check for "no change" */
	if ((prow_min == panel_row_min) && (pcol_min == panel_col_min)) return;

	/* Save the new panel info */
	panel_row_min = prow_min;
	panel_col_min = pcol_min;

	/* Hack -- optional disturb on "panel change" */
	if (disturb_panel && !center_player) disturb(FALSE, FALSE);

	panel_bounds_center();

	p_ptr->update |= (PU_MONSTERS);
	p_ptr->redraw |= (PR_MAP);
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
}


/*** Targeting Code ***/

/*
 * Determine is a monster makes a reasonable target
 *
 * The concept of "targeting" was stolen from "Morgul" (?)
 *
 * The player can target any location, or any "target-able" monster.
 *
 * Currently, a monster is "target_able" if it is visible, and if
 * the player can hit it with a projection, and the player is not
 * hallucinating.  This allows use of "use closest target" macros.
 *
 * Future versions may restrict the ability to target "trappers"
 * and "mimics", but the semantics is a little bit weird.
 */
bool target_able(MONSTER_IDX m_idx)
{
	monster_type *m_ptr = &current_floor_ptr->m_list[m_idx];

	/* Monster must be alive */
	if (!monster_is_valid(m_ptr)) return (FALSE);

	/* Hack -- no targeting hallucinations */
	if (p_ptr->image) return (FALSE);

	/* Monster must be visible */
	if (!m_ptr->ml) return (FALSE);

	if (p_ptr->riding && (p_ptr->riding == m_idx)) return (TRUE);

	/* Monster must be projectable */
	if (!projectable(p_ptr->y, p_ptr->x, m_ptr->fy, m_ptr->fx)) return (FALSE);

	/* Hack -- Never target trappers */
	/* if (CLEAR_ATTR && (CLEAR_CHAR)) return (FALSE); */

	/* Assume okay */
	return (TRUE);
}




/*
 * Update (if necessary) and verify (if possible) the target.
 *
 * We return TRUE if the target is "okay" and FALSE otherwise.
 */
bool target_okay(void)
{
	/* Accept stationary targets */
	if (target_who < 0) return (TRUE);

	/* Check moving targets */
	if (target_who > 0)
	{
		/* Accept reasonable targets */
		if (target_able(target_who))
		{
			monster_type *m_ptr = &current_floor_ptr->m_list[target_who];

			/* Acquire monster location */
			target_row = m_ptr->fy;
			target_col = m_ptr->fx;

			/* Good target */
			return (TRUE);
		}
	}

	/* Assume no target */
	return (FALSE);
}


/*
 * Hack -- help "select" a location (see below)
 */
static POSITION_IDX target_pick(POSITION y1, POSITION x1, POSITION dy, POSITION dx)
{
	POSITION_IDX i, v;
	POSITION x2, y2, x3, y3, x4, y4;
	POSITION_IDX b_i = -1, b_v = 9999;


	/* Scan the locations */
	for (i = 0; i < tmp_pos.n; i++)
	{
		/* Point 2 */
		x2 = tmp_pos.x[i];
		y2 = tmp_pos.y[i];

		/* Directed distance */
		x3 = (x2 - x1);
		y3 = (y2 - y1);

		/* Verify quadrant */
		if (dx && (x3 * dx <= 0)) continue;
		if (dy && (y3 * dy <= 0)) continue;

		x4 = ABS(x3);
		y4 = ABS(y3);

		/* Verify quadrant */
		if (dy && !dx && (x4 > y4)) continue;
		if (dx && !dy && (y4 > x4)) continue;

		/* Approximate Double Distance */
		v = ((x4 > y4) ? (x4 + x4 + y4) : (y4 + y4 + x4));

		/* Penalize location */
		if ((b_i >= 0) && (v >= b_v)) continue;
		b_i = i; b_v = v;
	}
	return (b_i);
}


/*
 * Hack -- determine if a given location is "interesting"
 */
static bool target_set_accept(POSITION y, POSITION x)
{
	grid_type *g_ptr;
	OBJECT_IDX this_o_idx, next_o_idx = 0;

	if (!(in_bounds(y, x))) return (FALSE);

	/* Player grid is always interesting */
	if (player_bold(y, x)) return (TRUE);

	if (p_ptr->image) return (FALSE);

	g_ptr = &current_floor_ptr->grid_array[y][x];

	/* Visible monsters */
	if (g_ptr->m_idx)
	{
		monster_type *m_ptr = &current_floor_ptr->m_list[g_ptr->m_idx];

		/* Visible monsters */
		if (m_ptr->ml) return (TRUE);
	}

	/* Scan all objects in the grid */
	for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;
		o_ptr = &current_floor_ptr->o_list[this_o_idx];
		next_o_idx = o_ptr->next_o_idx;

		/* Memorized object */
		if (o_ptr->marked & OM_FOUND) return (TRUE);
	}

	/* Interesting memorized features */
	if (g_ptr->info & (CAVE_MARK))
	{
		/* Notice object features */
		if (g_ptr->info & CAVE_OBJECT) return (TRUE);

		/* Feature code (applying "mimic" field) */
		if (have_flag(f_info[get_feat_mimic(g_ptr)].flags, FF_NOTICE)) return TRUE;
	}

	return (FALSE);
}


/*
 * Prepare the "temp" array for "target_set"
 *
 * Return the number of target_able monsters in the set.
 */
static void target_set_prepare(BIT_FLAGS mode)
{
	POSITION y, x;
	POSITION min_hgt, max_hgt, min_wid, max_wid;

	if (mode & TARGET_KILL)
	{
		/* Inner range */
		min_hgt = MAX((p_ptr->y - MAX_RANGE), 0);
		max_hgt = MIN((p_ptr->y + MAX_RANGE), current_floor_ptr->height - 1);
		min_wid = MAX((p_ptr->x - MAX_RANGE), 0);
		max_wid = MIN((p_ptr->x + MAX_RANGE), current_floor_ptr->width - 1);
	}
	else /* not targetting */
	{
		/* Inner panel */
		min_hgt = panel_row_min;
		max_hgt = panel_row_max;
		min_wid = panel_col_min;
		max_wid = panel_col_max;
	}

	/* Reset "temp" array */
	tmp_pos.n = 0;

	/* Scan the current panel */
	for (y = min_hgt; y <= max_hgt; y++)
	{
		for (x = min_wid; x <= max_wid; x++)
		{
			grid_type *g_ptr;

			/* Require "interesting" contents */
			if (!target_set_accept(y, x)) continue;

			g_ptr = &current_floor_ptr->grid_array[y][x];

			/* Require target_able monsters for "TARGET_KILL" */
			if ((mode & (TARGET_KILL)) && !target_able(g_ptr->m_idx)) continue;

			if ((mode & (TARGET_KILL)) && !target_pet && is_pet(&current_floor_ptr->m_list[g_ptr->m_idx])) continue;

			/* Save the location */
			tmp_pos.x[tmp_pos.n] = x;
			tmp_pos.y[tmp_pos.n] = y;
			tmp_pos.n++;
		}
	}

	/* Set the sort hooks */
	if (mode & (TARGET_KILL))
	{
		ang_sort(tmp_pos.x, tmp_pos.y, tmp_pos.n, ang_sort_comp_distance, ang_sort_swap_distance);
	}
	else
	{
		ang_sort(tmp_pos.x, tmp_pos.y, tmp_pos.n, ang_sort_comp_importance, ang_sort_swap_distance);
	}

	if (p_ptr->riding && target_pet && (tmp_pos.n > 1) && (mode & (TARGET_KILL)))
	{
		POSITION tmp;

		tmp = tmp_pos.y[0];
		tmp_pos.y[0] = tmp_pos.y[1];
		tmp_pos.y[1] = tmp;
		tmp = tmp_pos.x[0];
		tmp_pos.x[0] = tmp_pos.x[1];
		tmp_pos.x[1] = tmp;
	}
}

void target_set_prepare_look(void){
	target_set_prepare(TARGET_LOOK);
}


/*
 * Evaluate number of kill needed to gain level
 */
static void evaluate_monster_exp(char *buf, monster_type *m_ptr)
{
	monster_race *ap_r_ptr = &r_info[m_ptr->ap_r_idx];
	u32b num;
	s32b exp_mon, exp_adv;
	u32b exp_mon_frac, exp_adv_frac;

	if ((p_ptr->lev >= PY_MAX_LEVEL) || (p_ptr->prace == RACE_ANDROID))
	{
		sprintf(buf,"**");
		return;
	}
	else if (!ap_r_ptr->r_tkills || (m_ptr->mflag2 & MFLAG2_KAGE))
	{
		if (!p_ptr->wizard)
		{
			sprintf(buf,"??");
			return;
		}
	}


	/* The monster's experience point (assuming average monster speed) */
	exp_mon = ap_r_ptr->mexp * ap_r_ptr->level;
	exp_mon_frac = 0;
	s64b_div(&exp_mon, &exp_mon_frac, 0, (p_ptr->max_plv + 2));


	/* Total experience value for next level */
	exp_adv = player_exp[p_ptr->lev -1] * p_ptr->expfact;
	exp_adv_frac = 0;
	s64b_div(&exp_adv, &exp_adv_frac, 0, 100);

	/* Experience value need to get */
	s64b_sub(&exp_adv, &exp_adv_frac, p_ptr->exp, p_ptr->exp_frac);


	/* You need to kill at least one monster to get any experience */
	s64b_add(&exp_adv, &exp_adv_frac, exp_mon, exp_mon_frac);
	s64b_sub(&exp_adv, &exp_adv_frac, 0, 1);

	/* Extract number of monsters needed */
	s64b_div(&exp_adv, &exp_adv_frac, exp_mon, exp_mon_frac);

	/* If 999 or more monsters needed, only display "999". */
	num = MIN(999, exp_adv_frac);

	/* Display the number */
	sprintf(buf,"%03ld", (long int)num);
}


bool show_gold_on_floor = FALSE;

/*
 * Examine a grid, return a keypress.
 *
 * The "mode" argument contains the "TARGET_LOOK" bit flag, which
 * indicates that the "space" key should scan through the contents
 * of the grid, instead of simply returning immediately.  This lets
 * the "look" command get complete information, without making the
 * "target" command annoying.
 *
 * The "info" argument contains the "commands" which should be shown
 * inside the "[xxx]" text.  This string must never be empty, or grids
 * containing monsters will be displayed with an extra comma.
 *
 * Note that if a monster is in the grid, we update both the monster
 * recall info and the health bar info to track that monster.
 *
 * Eventually, we may allow multiple objects per grid, or objects
 * and terrain features in the same grid. 
 *
 * This function must handle blindness/hallucination.
 */
static char target_set_aux(POSITION y, POSITION x, BIT_FLAGS mode, concptr info)
{
	grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];
	OBJECT_IDX this_o_idx, next_o_idx = 0;
	concptr s1 = "", s2 = "", s3 = "", x_info = "";
	bool boring = TRUE;
	FEAT_IDX feat;
	feature_type *f_ptr;
	char query = '\001';
	char out_val[MAX_NLEN+80];
	OBJECT_IDX floor_list[23];
	ITEM_NUMBER floor_num = 0;

	/* Scan all objects in the grid */
	if (easy_floor)
	{
		floor_num = scan_floor(floor_list, y, x, 0x02);

		if (floor_num)
		{
			x_info = _("x物 ", "x,");
		}
	}

	/* Hack -- under the player */
	if (player_bold(y, x))
	{
#ifdef JP
		s1 = "あなたは";
		s2 = "の上";
		s3 = "にいる";
#else
		s1 = "You are ";
		s2 = "on ";
#endif
	}
	else
	{
		s1 = _("ターゲット:", "Target:");
	}

	/* Hack -- hallucination */
	if (p_ptr->image)
	{
		concptr name = _("何か奇妙な物", "something strange");

		/* Display a message */
#ifdef JP
		sprintf(out_val, "%s%s%s%s [%s]", s1, name, s2, s3, info);
#else
		sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, name, info);
#endif

		prt(out_val, 0, 0);
		move_cursor_relative(y, x);
		query = inkey();

		/* Stop on everything but "return" */
		if ((query != '\r') && (query != '\n')) return query;

		/* Repeat forever */
		return 0;
	}


	/* Actual monsters */
	if (g_ptr->m_idx && current_floor_ptr->m_list[g_ptr->m_idx].ml)
	{
		monster_type *m_ptr = &current_floor_ptr->m_list[g_ptr->m_idx];
		monster_race *ap_r_ptr = &r_info[m_ptr->ap_r_idx];
		GAME_TEXT m_name[MAX_NLEN];
		bool recall = FALSE;

		/* Not boring */
		boring = FALSE;

		monster_desc(m_name, m_ptr, MD_INDEF_VISIBLE);
		monster_race_track(m_ptr->ap_r_idx);
		health_track(g_ptr->m_idx);
		handle_stuff();

		/* Interact */
		while (1)
		{
			char acount[10];

			/* Recall */
			if (recall)
			{
				screen_save();

				/* Recall on screen */
				screen_roff(m_ptr->ap_r_idx, 0);

				/* Hack -- Complete the prompt (again) */
				Term_addstr(-1, TERM_WHITE, format(_("  [r思 %s%s]", "  [r,%s%s]"), x_info, info));

				query = inkey();

				screen_load();

				/* Normal commands */
				if (query != 'r') break;

				/* Toggle recall */
				recall = FALSE;

				/* Cleare recall text and repeat */
				continue;
			}

			/*** Normal ***/

			/* Describe, and prompt for recall */
			evaluate_monster_exp(acount, m_ptr);

#ifdef JP
			sprintf(out_val, "[%s]%s%s(%s)%s%s [r思 %s%s]", acount, s1, m_name, look_mon_desc(m_ptr, 0x01), s2, s3, x_info, info);
#else
			sprintf(out_val, "[%s]%s%s%s%s(%s) [r, %s%s]", acount, s1, s2, s3, m_name, look_mon_desc(m_ptr, 0x01), x_info, info);
#endif

			prt(out_val, 0, 0);

			/* Place cursor */
			move_cursor_relative(y, x);

			query = inkey();

			/* Normal commands */
			if (query != 'r') break;

			/* Toggle recall */
			recall = TRUE;
		}

		/* Always stop at "normal" keys */
		if ((query != '\r') && (query != '\n') && (query != ' ') && (query != 'x')) return query;

		/* Sometimes stop at "space" key */
		if ((query == ' ') && !(mode & (TARGET_LOOK))) return query;

		/* Change the intro */
		s1 = _("それは", "It is ");

		/* Hack -- take account of gender */
		if (ap_r_ptr->flags1 & (RF1_FEMALE)) s1 = _("彼女は", "She is ");
		else if (ap_r_ptr->flags1 & (RF1_MALE)) s1 = _("彼は", "He is ");

		/* Use a preposition */
#ifdef JP
		s2 = "を";
		s3 = "持っている";
#else
		s2 = "carrying ";
#endif


		/* Scan all objects being carried */
		for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
		{
			GAME_TEXT o_name[MAX_NLEN];

			object_type *o_ptr;
			o_ptr = &current_floor_ptr->o_list[this_o_idx];
			next_o_idx = o_ptr->next_o_idx;

			object_desc(o_name, o_ptr, 0);

#ifdef JP
			sprintf(out_val, "%s%s%s%s[%s]", s1, o_name, s2, s3, info);
#else
			sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, o_name, info);
#endif

			prt(out_val, 0, 0);
			move_cursor_relative(y, x);
			query = inkey();

			/* Always stop at "normal" keys */
			if ((query != '\r') && (query != '\n') && (query != ' ') && (query != 'x')) return query;

			/* Sometimes stop at "space" key */
			if ((query == ' ') && !(mode & (TARGET_LOOK))) return query;

			/* Change the intro */
			s2 = _("をまた", "also carrying ");
		}

		/* Use a preposition */
#ifdef JP
		s2 = "の上";
		s3 = "にいる";
#else
		s2 = "on ";
#endif
	}

	if (floor_num)
	{
		int min_width = 0;

		while (1)
		{
			if (floor_num == 1)
			{
				GAME_TEXT o_name[MAX_NLEN];

				object_type *o_ptr;
				o_ptr = &current_floor_ptr->o_list[floor_list[0]];

				object_desc(o_name, o_ptr, 0);

#ifdef JP
				sprintf(out_val, "%s%s%s%s[%s]", s1, o_name, s2, s3, info);
#else
				sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, o_name, info);
#endif

				prt(out_val, 0, 0);
				move_cursor_relative(y, x);

				query = inkey();

				/* End this grid */
				return query;
			}

			/* Provide one cushion before item listing  */
			if (boring)
			{
				/* Display rough information about items */
#ifdef JP
				sprintf(out_val, "%s %d個のアイテム%s%s ['x'で一覧, %s]", s1, (int)floor_num, s2, s3, info);
#else
				sprintf(out_val, "%s%s%sa pile of %d items [x,%s]", s1, s2, s3, (int)floor_num, info);
#endif

				prt(out_val, 0, 0);
				move_cursor_relative(y, x);

				query = inkey();

				/* No request for listing */
				if (query != 'x' && query != ' ') return query;
			}


			/** Display list of items **/

			/* Continue scrolling list if requested */
			while (1)
			{
				int i;
				OBJECT_IDX o_idx;
				screen_save();

				/* Display */
				show_gold_on_floor = TRUE;
				(void)show_floor(0, y, x, &min_width);
				show_gold_on_floor = FALSE;

				/* Prompt */
#ifdef JP
				sprintf(out_val, "%s %d個のアイテム%s%s [Enterで次へ, %s]", s1, (int)floor_num, s2, s3, info);
#else
				sprintf(out_val, "%s%s%sa pile of %d items [Enter,%s]", s1, s2, s3, (int)floor_num, info);
#endif
				prt(out_val, 0, 0);

				query = inkey();
				screen_load();

				/* Exit unless 'Enter' */
				if (query != '\n' && query != '\r')
				{
					return query;
				}

				/* Get the object being moved. */
				o_idx = g_ptr->o_idx;
 
				/* Only rotate a pile of two or more objects. */
				if (!(o_idx && current_floor_ptr->o_list[o_idx].next_o_idx)) continue;

				/* Remove the first object from the list. */
				excise_object_idx(o_idx);

				/* Find end of the list. */
				i = g_ptr->o_idx;
				while (current_floor_ptr->o_list[i].next_o_idx)
					i = current_floor_ptr->o_list[i].next_o_idx;

				/* Add after the last object. */
				current_floor_ptr->o_list[i].next_o_idx = o_idx;

				/* Loop and re-display the list */
			}
		}

		/* NOTREACHED */
	}

	/* Scan all objects in the grid */
	for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;
		o_ptr = &current_floor_ptr->o_list[this_o_idx];
		next_o_idx = o_ptr->next_o_idx;

		if (o_ptr->marked & OM_FOUND)
		{
			GAME_TEXT o_name[MAX_NLEN];

			/* Not boring */
			boring = FALSE;

			object_desc(o_name, o_ptr, 0);

#ifdef JP
			sprintf(out_val, "%s%s%s%s[%s]", s1, o_name, s2, s3, info);
#else
			sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, o_name, info);
#endif

			prt(out_val, 0, 0);
			move_cursor_relative(y, x);
			query = inkey();

			/* Always stop at "normal" keys */
			if ((query != '\r') && (query != '\n') && (query != ' ') && (query != 'x')) return query;

			/* Sometimes stop at "space" key */
			if ((query == ' ') && !(mode & TARGET_LOOK)) return query;

			/* Change the intro */
			s1 = _("それは", "It is ");

			/* Plurals */
			if (o_ptr->number != 1) s1 = _("それらは", "They are ");

			/* Preposition */
#ifdef JP
			s2 = "の上";
			s3 = "に見える";
#else
			s2 = "on ";
#endif

		}
	}


	/* Feature code (applying "mimic" field) */
	feat = get_feat_mimic(g_ptr);

	/* Require knowledge about grid, or ability to see grid */
	if (!(g_ptr->info & CAVE_MARK) && !player_can_see_bold(y, x))
	{
		/* Forget feature */
		feat = feat_none;
	}

	f_ptr = &f_info[feat];

	/* Terrain feature if needed */
	if (boring || have_flag(f_ptr->flags, FF_REMEMBER))
	{
		concptr name;

		/* Hack -- special handling for quest entrances */
		if (have_flag(f_ptr->flags, FF_QUEST_ENTER))
		{
			/* Set the quest number temporary */
			IDX old_quest = p_ptr->inside_quest;
			int j;

			/* Clear the text */
			for (j = 0; j < 10; j++) quest_text[j][0] = '\0';
			quest_text_line = 0;

			p_ptr->inside_quest = g_ptr->special;

			/* Get the quest text */
			init_flags = INIT_NAME_ONLY;

			process_dungeon_file("q_info.txt", 0, 0, 0, 0);

			name = format(_("クエスト「%s」(%d階相当)", "the entrance to the quest '%s'(level %d)"), 
						quest[g_ptr->special].name, quest[g_ptr->special].level);

			/* Reset the old quest number */
			p_ptr->inside_quest = old_quest;
		}

		/* Hack -- special handling for building doors */
		else if (have_flag(f_ptr->flags, FF_BLDG) && !p_ptr->inside_arena)
		{
			name = building[f_ptr->subtype].name;
		}
		else if (have_flag(f_ptr->flags, FF_ENTRANCE))
		{
			name = format(_("%s(%d階相当)", "%s(level %d)"), d_text + d_info[g_ptr->special].text, d_info[g_ptr->special].mindepth);
		}
		else if (have_flag(f_ptr->flags, FF_TOWN))
		{
			name = town_info[g_ptr->special].name;
		}
		else if (p_ptr->wild_mode && (feat == feat_floor))
		{
			name = _("道", "road");
		}
		else
		{
			name = f_name + f_ptr->name;
		}


		/* Pick a prefix */
		if (*s2 &&
		    ((!have_flag(f_ptr->flags, FF_MOVE) && !have_flag(f_ptr->flags, FF_CAN_FLY)) ||
		     (!have_flag(f_ptr->flags, FF_LOS) && !have_flag(f_ptr->flags, FF_TREE)) ||
		     have_flag(f_ptr->flags, FF_TOWN)))
		{
			s2 = _("の中", "in ");
		}

		/* Hack -- special introduction for store & building doors -KMW- */
		if (have_flag(f_ptr->flags, FF_STORE) ||
		    have_flag(f_ptr->flags, FF_QUEST_ENTER) ||
		    (have_flag(f_ptr->flags, FF_BLDG) && !p_ptr->inside_arena) ||
		    have_flag(f_ptr->flags, FF_ENTRANCE))
		{
			s2 = _("の入口", "");
		}
#ifndef JP
		else if (have_flag(f_ptr->flags, FF_FLOOR) ||
			 have_flag(f_ptr->flags, FF_TOWN) ||
			 have_flag(f_ptr->flags, FF_SHALLOW) ||
			 have_flag(f_ptr->flags, FF_DEEP))
		{
			s3 ="";
		}
		else
		{
			/* Pick proper indefinite article */
			s3 = (is_a_vowel(name[0])) ? "an " : "a ";
		}
#endif

		/* Display a message */
		if (p_ptr->wizard)
		{
			char f_idx_str[32];
			if (g_ptr->mimic) sprintf(f_idx_str, "%d/%d", g_ptr->feat, g_ptr->mimic);
			else sprintf(f_idx_str, "%d", g_ptr->feat);
#ifdef JP
			sprintf(out_val, "%s%s%s%s[%s] %x %s %d %d %d (%d,%d) %d", s1, name, s2, s3, info, (unsigned int)g_ptr->info, f_idx_str, g_ptr->dist, g_ptr->cost, g_ptr->when, (int)y, (int)x, travel.cost[y][x]);
#else
			sprintf(out_val, "%s%s%s%s [%s] %x %s %d %d %d (%d,%d)", s1, s2, s3, name, info, g_ptr->info, f_idx_str, g_ptr->dist, g_ptr->cost, g_ptr->when, (int)y, (int)x);
#endif
		}
		else
#ifdef JP
			sprintf(out_val, "%s%s%s%s[%s]", s1, name, s2, s3, info);
#else
			sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, name, info);
#endif

		prt(out_val, 0, 0);
		move_cursor_relative(y, x);
		query = inkey();

		/* Always stop at "normal" keys */
		if ((query != '\r') && (query != '\n') && (query != ' ')) return query;
	}

	/* Stop on everything but "return" */
	if ((query != '\r') && (query != '\n')) return query;

	/* Repeat forever */
	return 0;
}


/*
 * Handle "target" and "look".
 *
 * Note that this code can be called from "get_aim_dir()".
 *
 * All locations must be on the current panel.  Consider the use of
 * "panel_bounds()" to allow "off-panel" targets, perhaps by using
 * some form of "scrolling" the map around the cursor.  
 * That is, consider the possibility of "auto-scrolling" the screen
 * while the cursor moves around.  This may require changes in the
 * "update_monster()" code to allow "visibility" even if off panel, and
 * may require dynamic recalculation of the "temp" grid set.
 *
 * Hack -- targeting/observing an "outer border grid" may induce
 * problems, so this is not currently allowed.
 *
 * The player can use the direction keys to move among "interesting"
 * grids in a heuristic manner, or the "space", "+", and "-" keys to
 * move through the "interesting" grids in a sequential manner, or
 * can enter "location" mode, and use the direction keys to move one
 * grid at a time in any direction.  The "t" (set target) command will
 * only target a monster (as opposed to a location) if the monster is
 * target_able and the "interesting" mode is being used.
 *
 * The current grid is described using the "look" method above, and
 * a new command may be entered at any time, but note that if the
 * "TARGET_LOOK" bit flag is set (or if we are in "location" mode,
 * where "space" has no obvious meaning) then "space" will scan
 * through the description of the current grid until done, instead
 * of immediately jumping to the next "interesting" grid.  This
 * allows the "target" command to retain its old semantics.
 *
 * The "*", "+", and "-" keys may always be used to jump immediately
 * to the next (or previous) interesting grid, in the proper mode.
 *
 * The "return" key may always be used to scan through a complete
 * grid description (forever).
 *
 * This command will cancel any old target, even if used from
 * inside the "look" command.
 */
bool target_set(BIT_FLAGS mode)
{
	int i, d, m, t, bd;
	POSITION y = p_ptr->y;
	POSITION x = p_ptr->x;

	bool done = FALSE;
	bool flag = TRUE;
	char query;
	char info[80];
	char same_key;
	grid_type *g_ptr;
	TERM_LEN wid, hgt;
	
	get_screen_size(&wid, &hgt);

	/* Cancel target */
	target_who = 0;

	if (rogue_like_commands)
	{
		same_key = 'x';
	}
	else
	{
		same_key = 'l';
	}

	/* Prepare the "temp" array */
	target_set_prepare(mode);

	/* Start near the player */
	m = 0;

	/* Interact */
	while (!done)
	{
		/* Interesting grids */
		if (flag && tmp_pos.n)
		{
			y = tmp_pos.y[m];
			x = tmp_pos.x[m];

			/* Set forcus */
			change_panel_xy(y, x);

			if (!(mode & TARGET_LOOK)) prt_path(y, x);

			/* Access */
			g_ptr = &current_floor_ptr->grid_array[y][x];

			/* Allow target */
			if (target_able(g_ptr->m_idx))
			{
				strcpy(info, _("q止 t決 p自 o現 +次 -前", "q,t,p,o,+,-,<dir>"));
			}

			/* Dis-allow target */
			else
			{
				strcpy(info, _("q止 p自 o現 +次 -前", "q,p,o,+,-,<dir>"));
			}

			if (cheat_sight)
			{
				char cheatinfo[30];
				sprintf(cheatinfo, " LOS:%d, PROJECTABLE:%d",
					los(p_ptr->y, p_ptr->x, y, x), projectable(p_ptr->y, p_ptr->x, y, x));
				strcat(info, cheatinfo);
			}
			
			/* Describe and Prompt */
			while (TRUE){
				query = target_set_aux(y, x, mode, info);
				if(query)break;
			}

			/* Assume no "direction" */
			d = 0;

			if (use_menu)
			{
				if (query == '\r') query = 't';
			}  

			/* Analyze */
			switch (query)
			{
				case ESCAPE:
				case 'q':
				{
					done = TRUE;
					break;
				}

				case 't':
				case '.':
				case '5':
				case '0':
				{
					if (target_able(g_ptr->m_idx))
					{
						health_track(g_ptr->m_idx);
						target_who = g_ptr->m_idx;
						target_row = y;
						target_col = x;
						done = TRUE;
					}
					else
					{
						bell();
					}
					break;
				}

				case ' ':
				case '*':
				case '+':
				{
					if (++m == tmp_pos.n)
					{
						m = 0;
						if (!expand_list) done = TRUE;
					}
					break;
				}

				case '-':
				{
					if (m-- == 0)
					{
						m = tmp_pos.n - 1;
						if (!expand_list) done = TRUE;
					}
					break;
				}

				case 'p':
				{
					/* Recenter the map around the player */
					verify_panel();
					p_ptr->update |= (PU_MONSTERS);
					p_ptr->redraw |= (PR_MAP);
					p_ptr->window |= (PW_OVERHEAD);
					handle_stuff();

					/* Recalculate interesting grids */
					target_set_prepare(mode);

					y = p_ptr->y;
					x = p_ptr->x;
				}

				case 'o':
				{
					flag = FALSE;
					break;
				}

				case 'm':
				{
					break;
				}

				default:
				{
					if(query == same_key)
					{
						if (++m == tmp_pos.n)
						{
							m = 0;
							if (!expand_list) done = TRUE;
						}
					}
					else
					{
						/* Extract the action (if any) */
						d = get_keymap_dir(query);

						if (!d) bell();
						break;
					}
				}
			}
			/* Hack -- move around */
			if (d)
			{
				/* Modified to scroll to monster */
				POSITION y2 = panel_row_min;
				POSITION x2 = panel_col_min;

				/* Find a new monster */
				i = target_pick(tmp_pos.y[m], tmp_pos.x[m], ddy[d], ddx[d]);

				/* Request to target past last interesting grid */
				while (flag && (i < 0))
				{
					/* Note the change */
					if (change_panel(ddy[d], ddx[d]))
					{
						int v = tmp_pos.y[m];
						int u = tmp_pos.x[m];

						/* Recalculate interesting grids */
						target_set_prepare(mode);

						/* Look at interesting grids */
						flag = TRUE;

						/* Find a new monster */
						i = target_pick(v, u, ddy[d], ddx[d]);

						/* Use that grid */
						if (i >= 0) m = i;
					}

					/* Nothing interesting */
					else
					{
						POSITION dx = ddx[d];
						POSITION dy = ddy[d];

						/* Restore previous position */
						panel_row_min = y2;
						panel_col_min = x2;
						panel_bounds_center();

						p_ptr->update |= (PU_MONSTERS);
						p_ptr->redraw |= (PR_MAP);
						p_ptr->window |= (PW_OVERHEAD);
						handle_stuff();

						/* Recalculate interesting grids */
						target_set_prepare(mode);

						/* Look at boring grids */
						flag = FALSE;

						/* Move */
						x += dx;
						y += dy;

						/* Do not move horizontally if unnecessary */
						if (((x < panel_col_min + wid / 2) && (dx > 0)) ||
							 ((x > panel_col_min + wid / 2) && (dx < 0)))
						{
							dx = 0;
						}

						/* Do not move vertically if unnecessary */
						if (((y < panel_row_min + hgt / 2) && (dy > 0)) ||
							 ((y > panel_row_min + hgt / 2) && (dy < 0)))
						{
							dy = 0;
						}

						/* Apply the motion */
						if ((y >= panel_row_min+hgt) || (y < panel_row_min) ||
						    (x >= panel_col_min+wid) || (x < panel_col_min))
						{
							if (change_panel(dy, dx)) target_set_prepare(mode);
						}

						/* Slide into legality */
						if (x >= current_floor_ptr->width-1) x = current_floor_ptr->width - 2;
						else if (x <= 0) x = 1;

						/* Slide into legality */
						if (y >= current_floor_ptr->height-1) y = current_floor_ptr->height- 2;
						else if (y <= 0) y = 1;
					}
				}

				/* Use that grid */
				m = i;
			}
		}

		/* Arbitrary grids */
		else
		{
			bool move_fast = FALSE;

			if (!(mode & TARGET_LOOK)) prt_path(y, x);

			/* Access */
			g_ptr = &current_floor_ptr->grid_array[y][x];

			/* Default prompt */
			strcpy(info, _("q止 t決 p自 m近 +次 -前", "q,t,p,m,+,-,<dir>"));

			if (cheat_sight)
			{
				char cheatinfo[30];
				sprintf(cheatinfo, " LOS:%d, PROJECTABLE:%d",
					los(p_ptr->y, p_ptr->x, y, x),
					projectable(p_ptr->y, p_ptr->x, y, x));
				strcat(info, cheatinfo);
			}

			/* Describe and Prompt (enable "TARGET_LOOK") */
			while ((query = target_set_aux(y, x, mode | TARGET_LOOK, info)) == 0);

			/* Assume no direction */
			d = 0;

			if (use_menu)
			{
				if (query == '\r') query = 't';
			}  

			/* Analyze the keypress */
			switch (query)
			{
				case ESCAPE:
				case 'q':
				{
					done = TRUE;
					break;
				}

				case 't':
				case '.':
				case '5':
				case '0':
				{
					target_who = -1;
					target_row = y;
					target_col = x;
					done = TRUE;
					break;
				}

				case 'p':
				{
					/* Recenter the map around the player */
					verify_panel();
					p_ptr->update |= (PU_MONSTERS);
					p_ptr->redraw |= (PR_MAP);
					p_ptr->window |= (PW_OVERHEAD);
					handle_stuff();

					/* Recalculate interesting grids */
					target_set_prepare(mode);

					y = p_ptr->y;
					x = p_ptr->x;
				}

				case 'o':
				{
					break;
				}

				case ' ':
				case '*':
				case '+':
				case '-':
				case 'm':
				{
					flag = TRUE;

					m = 0;
					bd = 999;

					/* Pick a nearby monster */
					for (i = 0; i < tmp_pos.n; i++)
					{
						t = distance(y, x, tmp_pos.y[i], tmp_pos.x[i]);

						/* Pick closest */
						if (t < bd)
						{
							m = i;
							bd = t;
						}
					}

					/* Nothing interesting */
					if (bd == 999) flag = FALSE;

					break;
				}

				default:
				{
					/* Extract the action (if any) */
					d = get_keymap_dir(query);

					/* XTRA HACK MOVEFAST */
					if (isupper(query)) move_fast = TRUE;

					if (!d) bell();
					break;
				}
			}

			/* Handle "direction" */
			if (d)
			{
				POSITION dx = ddx[d];
				POSITION dy = ddy[d];

				/* XTRA HACK MOVEFAST */
				if (move_fast)
				{
					int mag = MIN(wid / 2, hgt / 2);
					x += dx * mag;
					y += dy * mag;
				}
				else
				{
					x += dx;
					y += dy;
				}

				/* Do not move horizontally if unnecessary */
				if (((x < panel_col_min + wid / 2) && (dx > 0)) ||
					 ((x > panel_col_min + wid / 2) && (dx < 0)))
				{
					dx = 0;
				}

				/* Do not move vertically if unnecessary */
				if (((y < panel_row_min + hgt / 2) && (dy > 0)) ||
					 ((y > panel_row_min + hgt / 2) && (dy < 0)))
				{
					dy = 0;
				}

				/* Apply the motion */
				if ((y >= panel_row_min + hgt) || (y < panel_row_min) ||
					 (x >= panel_col_min + wid) || (x < panel_col_min))
				{
					if (change_panel(dy, dx)) target_set_prepare(mode);
				}

				/* Slide into legality */
				if (x >= current_floor_ptr->width-1) x = current_floor_ptr->width - 2;
				else if (x <= 0) x = 1;

				/* Slide into legality */
				if (y >= current_floor_ptr->height-1) y = current_floor_ptr->height- 2;
				else if (y <= 0) y = 1;
			}
		}
	}

	/* Forget */
	tmp_pos.n = 0;

	/* Clear the top line */
	prt("", 0, 0);

	/* Recenter the map around the player */
	verify_panel();
	p_ptr->update |= (PU_MONSTERS);
	p_ptr->redraw |= (PR_MAP);
	p_ptr->window |= (PW_OVERHEAD);
	handle_stuff();

	/* Failure to set target */
	if (!target_who) return (FALSE);

	/* Success */
	return (TRUE);
}


/*
 * Get an "aiming direction" from the user.
 *
 * The "dir" is loaded with 1,2,3,4,6,7,8,9 for "actual direction", and
 * "0" for "current target", and "-1" for "entry aborted".
 *
 * Note that "Force Target", if set, will pre-empt user interaction,
 * if there is a usable target already set.
 *
 * Note that confusion over-rides any (explicit?) user choice.
 */
bool get_aim_dir(DIRECTION *dp)
{
	DIRECTION dir;
	char	command;
	concptr	p;
	COMMAND_CODE code;

	(*dp) = 0;

	/* Global direction */
	dir = command_dir;

	/* Hack -- auto-target if requested */
	if (use_old_target && target_okay()) dir = 5;

	if (repeat_pull(&code))
	{
		/* Confusion? */

		/* Verify */
		if (!(code == 5 && !target_okay()))
		{
/*			return (TRUE); */
			dir = (DIRECTION)code;
		}
	}
	*dp = (DIRECTION)code;

	/* Ask until satisfied */
	while (!dir)
	{
		/* Choose a prompt */
		if (!target_okay())
		{
			p = _("方向 ('*'でターゲット選択, ESCで中断)? ", "Direction ('*' to choose a target, Escape to cancel)? ");
		}
		else
		{
			p = _("方向 ('5'でターゲットへ, '*'でターゲット再選択, ESCで中断)? ", "Direction ('5' for target, '*' to re-target, Escape to cancel)? ");
		}

		/* Get a command (or Cancel) */
		if (!get_com(p, &command, TRUE)) break;

		if (use_menu)
		{
			if (command == '\r') command = 't';
		}  

		/* Convert various keys to "standard" keys */
		switch (command)
		{
			/* Use current target */
			case 'T':
			case 't':
			case '.':
			case '5':
			case '0':
			{
				dir = 5;
				break;
			}

			/* Set new target */
			case '*':
			case ' ':
			case '\r':
			{
				if (target_set(TARGET_KILL)) dir = 5;
				break;
			}

			default:
			{
				/* Extract the action (if any) */
				dir = get_keymap_dir(command);

				break;
			}
		}

		/* Verify requested targets */
		if ((dir == 5) && !target_okay()) dir = 0;

		/* Error */
		if (!dir) bell();
	}

	/* No direction */
	if (!dir)
	{
		project_length = 0; /* reset to default */
		return (FALSE);
	}

	/* Save the direction */
	command_dir = dir;

	/* Check for confusion */
	if (p_ptr->confused)
	{
		/* Random direction */
		dir = ddd[randint0(8)];
	}

	/* Notice confusion */
	if (command_dir != dir)
	{
		/* Warn the user */
		msg_print(_("あなたは混乱している。", "You are confused."));
	}

	/* Save direction */
	(*dp) = dir;

/*	repeat_push(dir); */
	repeat_push((COMMAND_CODE)command_dir);

	/* A "valid" direction was entered */
	return (TRUE);
}


bool get_direction(DIRECTION *dp, bool allow_under, bool with_steed)
{
	DIRECTION dir;
	concptr prompt;
	COMMAND_CODE code;

	(*dp) = 0;

	/* Global direction */
	dir = command_dir;

	if (repeat_pull(&code))
	{
		dir = (DIRECTION)code;
		/*		return (TRUE); */
	}
	*dp = (DIRECTION)code;

	if (allow_under)
	{
		prompt = _("方向 ('.'足元, ESCで中断)? ", "Direction ('.' at feet, Escape to cancel)? ");
	}
	else
	{
		prompt = _("方向 (ESCで中断)? ", "Direction (Escape to cancel)? ");
	}

	/* Get a direction */
	while (!dir)
	{
		char ch;

		/* Get a command (or Cancel) */
		if (!get_com(prompt, &ch, TRUE)) break;

		/* Look down */
		if ((allow_under) && ((ch == '5') || (ch == '-') || (ch == '.')))
		{
			dir = 5;
		}
		else
		{
			/* Look up the direction */
			dir = get_keymap_dir(ch);

			if (!dir) bell();
		}
	}

	/* Prevent weirdness */
	if ((dir == 5) && (!allow_under)) dir = 0;

	/* Aborted */
	if (!dir) return (FALSE);

	/* Save desired direction */
	command_dir = dir;

	/* Apply "confusion" */
	if (p_ptr->confused)
	{
		/* Standard confusion */
		if (randint0(100) < 75)
		{
			/* Random direction */
			dir = ddd[randint0(8)];
		}
	}
	else if (p_ptr->riding && with_steed)
	{
		monster_type *m_ptr = &current_floor_ptr->m_list[p_ptr->riding];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		if (MON_CONFUSED(m_ptr))
		{
			/* Standard confusion */
			if (randint0(100) < 75)
			{
				/* Random direction */
				dir = ddd[randint0(8)];
			}
		}
		else if ((r_ptr->flags1 & RF1_RAND_50) && (r_ptr->flags1 & RF1_RAND_25) && (randint0(100) < 50))
		{
			/* Random direction */
			dir = ddd[randint0(8)];
		}
		else if ((r_ptr->flags1 & RF1_RAND_50) && (randint0(100) < 25))
		{
			/* Random direction */
			dir = ddd[randint0(8)];
		}
	}

	/* Notice confusion */
	if (command_dir != dir)
	{
		if (p_ptr->confused)
		{
			/* Warn the user */
			msg_print(_("あなたは混乱している。", "You are confused."));
		}
		else
		{
			GAME_TEXT m_name[MAX_NLEN];
			monster_type *m_ptr = &current_floor_ptr->m_list[p_ptr->riding];

			monster_desc(m_name, m_ptr, 0);
			if (MON_CONFUSED(m_ptr))
			{
				msg_format(_("%sは混乱している。", "%^s is confusing."), m_name);
			}
			else
			{
				msg_format(_("%sは思い通りに動いてくれない。", "You cannot control %s."), m_name);
			}
		}
	}

	/* Save direction */
	(*dp) = dir;

	/*	repeat_push(dir); */
	repeat_push((COMMAND_CODE)command_dir);

	/* Success */
	return (TRUE);
}

/*
 * @brief 進行方向を指定する(騎乗対象の混乱の影響を受ける) / Request a "movement" direction (1,2,3,4,6,7,8,9) from the user,
 * and place it into "command_dir", unless we already have one.
 *
 * This function should be used for all "repeatable" commands, such as
 * run, walk, open, close, bash, disarm, spike, tunnel, etc, as well
 * as all commands which must reference a grid adjacent to the player,
 * and which may not reference the grid under the player.  Note that,
 * for example, it is no longer possible to "disarm" or "open" chests
 * in the same grid as the player.
 *
 * Direction "5" is illegal and will (cleanly) abort the command.
 *
 * This function tracks and uses the "global direction", and uses
 * that as the "desired direction", to which "confusion" is applied.
 */
bool get_rep_dir(DIRECTION *dp, bool under)
{
	DIRECTION dir;
	concptr prompt;
	COMMAND_CODE code;

	(*dp) = 0;

	/* Global direction */
	dir = command_dir;

	if (repeat_pull(&code))
	{
		dir = (DIRECTION)code;
/*		return (TRUE); */
	}
	*dp = (DIRECTION)code;

	if (under)
	{
		prompt = _("方向 ('.'足元, ESCで中断)? ", "Direction ('.' at feet, Escape to cancel)? ");
	}
	else
	{
		prompt = _("方向 (ESCで中断)? ", "Direction (Escape to cancel)? ");
	}
	
	/* Get a direction */
	while (!dir)
	{
		char ch;

		/* Get a command (or Cancel) */
		if (!get_com(prompt, &ch, TRUE)) break;

		/* Look down */
		if ((under) && ((ch == '5') || (ch == '-') || (ch == '.')))
		{
			dir = 5;
		}
		else
		{
			/* Look up the direction */
			dir = get_keymap_dir(ch);

			if (!dir) bell();
		}
	}

	/* Prevent weirdness */
	if ((dir == 5) && (!under)) dir = 0;

	/* Aborted */
	if (!dir) return (FALSE);

	/* Save desired direction */
	command_dir = dir;

	/* Apply "confusion" */
	if (p_ptr->confused)
	{
		/* Standard confusion */
		if (randint0(100) < 75)
		{
			/* Random direction */
			dir = ddd[randint0(8)];
		}
	}
	else if (p_ptr->riding)
	{
		monster_type *m_ptr = &current_floor_ptr->m_list[p_ptr->riding];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		if (MON_CONFUSED(m_ptr))
		{
			/* Standard confusion */
			if (randint0(100) < 75)
			{
				/* Random direction */
				dir = ddd[randint0(8)];
			}
		}
		else if ((r_ptr->flags1 & RF1_RAND_50) && (r_ptr->flags1 & RF1_RAND_25) && (randint0(100) < 50))
		{
			/* Random direction */
			dir = ddd[randint0(8)];
		}
		else if ((r_ptr->flags1 & RF1_RAND_50) && (randint0(100) < 25))
		{
			/* Random direction */
			dir = ddd[randint0(8)];
		}
	}

	/* Notice confusion */
	if (command_dir != dir)
	{
		if (p_ptr->confused)
		{
			/* Warn the user */
			msg_print(_("あなたは混乱している。", "You are confused."));
		}
		else
		{
			GAME_TEXT m_name[MAX_NLEN];
			monster_type *m_ptr = &current_floor_ptr->m_list[p_ptr->riding];

			monster_desc(m_name, m_ptr, 0);
			if (MON_CONFUSED(m_ptr))
			{
				msg_format(_("%sは混乱している。", "%^s is confusing."), m_name);
			}
			else
			{
				msg_format(_("%sは思い通りに動いてくれない。", "You cannot control %s."), m_name);
			}
		}
	}

	/* Save direction */
	(*dp) = dir;

/*	repeat_push(dir); */
	repeat_push((COMMAND_CODE)command_dir);

	/* Success */
	return (TRUE);
}


/*
 * XAngband: determine if a given location is "interesting"
 * based on target_set_accept function.
 */
static bool tgt_pt_accept(POSITION y, POSITION x)
{
	grid_type *g_ptr;

	if (!(in_bounds(y, x))) return (FALSE);

	/* Player grid is always interesting */
	if ((y == p_ptr->y) && (x == p_ptr->x)) return (TRUE);

	if (p_ptr->image) return (FALSE);

	g_ptr = &current_floor_ptr->grid_array[y][x];

	/* Interesting memorized features */
	if (g_ptr->info & (CAVE_MARK))
	{
		/* Notice stairs */
		if (cave_have_flag_grid(g_ptr, FF_LESS)) return (TRUE);
		if (cave_have_flag_grid(g_ptr, FF_MORE)) return (TRUE);

		/* Notice quest features */
		if (cave_have_flag_grid(g_ptr, FF_QUEST_ENTER)) return (TRUE);
		if (cave_have_flag_grid(g_ptr, FF_QUEST_EXIT)) return (TRUE);
	}

	return (FALSE);
}


/*
 * XAngband: Prepare the "temp" array for "tget_pt"
 * based on target_set_prepare funciton.
 */
static void tgt_pt_prepare(void)
{
	POSITION y, x;

	/* Reset "temp" array */
	tmp_pos.n = 0;

	if (!expand_list) return;

	/* Scan the current panel */
	for (y = 1; y < current_floor_ptr->height; y++)
	{
		for (x = 1; x < current_floor_ptr->width; x++)
		{
			/* Require "interesting" contents */
			if (!tgt_pt_accept(y, x)) continue;

			/* Save the location */
			tmp_pos.x[tmp_pos.n] = x;
			tmp_pos.y[tmp_pos.n] = y;
			tmp_pos.n++;
		}
	}

	/* Sort the positions */
	ang_sort(tmp_pos.x, tmp_pos.y, tmp_pos.n, ang_sort_comp_distance, ang_sort_swap_distance);
}

/*
 * old -- from PsiAngband.
 */
bool tgt_pt(POSITION *x_ptr, POSITION *y_ptr)
{
	char ch = 0;
	int d, n = 0;
	POSITION x, y;
	bool success = FALSE;

	TERM_LEN wid, hgt;

	get_screen_size(&wid, &hgt);

	x = p_ptr->x;
	y = p_ptr->y;

	if (expand_list) 
	{
		tgt_pt_prepare();
	}

	msg_print(_("場所を選んでスペースキーを押して下さい。", "Select a point and press space."));
	msg_flag = FALSE; /* prevents "-more-" message. */

	while ((ch != ESCAPE) && !success)
	{
		bool move_fast = FALSE;

		move_cursor_relative(y, x);
		ch = inkey();
		switch (ch)
		{
		case ESCAPE:
			break;
		case ' ':
		case 't':
		case '.':
		case '5':
		case '0':
			/* illegal place */
			if (player_bold(y, x)) ch = 0;

			/* okay place */
			else success = TRUE;

			break;

		/* XAngband: Move cursor to stairs */
		case '>':
		case '<':
			if (expand_list && tmp_pos.n)
			{
				int dx, dy;
				int cx = (panel_col_min + panel_col_max) / 2;
				int cy = (panel_row_min + panel_row_max) / 2;

				n++;

				/* Skip stairs which have defferent distance */
				for (; n < tmp_pos.n; ++ n)
				{
					grid_type *g_ptr = &current_floor_ptr->grid_array[tmp_pos.y[n]][tmp_pos.x[n]];

					if (cave_have_flag_grid(g_ptr, FF_STAIRS) &&
					    cave_have_flag_grid(g_ptr, ch == '>' ? FF_MORE : FF_LESS))
					{
						/* Found */
						break;
					}
				}

				if (n == tmp_pos.n)	/* Loop out taget list */
				{
					n = 0;
					y = p_ptr->y;
					x = p_ptr->x;
					verify_panel();	/* Move cursor to player */

					p_ptr->update |= (PU_MONSTERS);

					p_ptr->redraw |= (PR_MAP);

					p_ptr->window |= (PW_OVERHEAD);
					handle_stuff();
				}
				else	/* move cursor to next stair and change panel */
				{
					y = tmp_pos.y[n];
					x = tmp_pos.x[n];

					dy = 2 * (y - cy) / hgt;
					dx = 2 * (x - cx) / wid;
					if (dy || dx) change_panel(dy, dx);
				}
			}
			break;

		default:
			/* Look up the direction */
			d = get_keymap_dir(ch);

			/* XTRA HACK MOVEFAST */
			if (isupper(ch)) move_fast = TRUE;

			/* Handle "direction" */
			if (d)
			{
				int dx = ddx[d];
				int dy = ddy[d];

				/* XTRA HACK MOVEFAST */
				if (move_fast)
				{
					int mag = MIN(wid / 2, hgt / 2);
					x += dx * mag;
					y += dy * mag;
				}
				else
				{
					x += dx;
					y += dy;
				}

				/* Do not move horizontally if unnecessary */
				if (((x < panel_col_min + wid / 2) && (dx > 0)) ||
					 ((x > panel_col_min + wid / 2) && (dx < 0)))
				{
					dx = 0;
				}

				/* Do not move vertically if unnecessary */
				if (((y < panel_row_min + hgt / 2) && (dy > 0)) ||
					 ((y > panel_row_min + hgt / 2) && (dy < 0)))
				{
					dy = 0;
				}

				/* Apply the motion */
				if ((y >= panel_row_min + hgt) || (y < panel_row_min) ||
					 (x >= panel_col_min + wid) || (x < panel_col_min))
				{
					/* if (change_panel(dy, dx)) target_set_prepare(mode); */
					change_panel(dy, dx);
				}

				/* Slide into legality */
				if (x >= current_floor_ptr->width-1) x = current_floor_ptr->width - 2;
				else if (x <= 0) x = 1;

				/* Slide into legality */
				if (y >= current_floor_ptr->height-1) y = current_floor_ptr->height- 2;
				else if (y <= 0) y = 1;

			}
			break;
		}
	}

	/* Clear the top line */
	prt("", 0, 0);

	/* Recenter the map around the player */
	verify_panel();

	p_ptr->update |= (PU_MONSTERS);

	p_ptr->redraw |= (PR_MAP);

	p_ptr->window |= (PW_OVERHEAD);
	handle_stuff();

	*x_ptr = x;
	*y_ptr = y;
	return success;
}


bool get_hack_dir(DIRECTION *dp)
{
	DIRECTION dir;
	concptr    p;
	char    command;

	(*dp) = 0;

	/* Global direction */
	dir = 0;

	/* (No auto-targeting) */

	/* Ask until satisfied */
	while (!dir)
	{
		/* Choose a prompt */
		if (!target_okay())
		{
			p = _("方向 ('*'でターゲット選択, ESCで中断)? ", "Direction ('*' to choose a target, Escape to cancel)? ");
		}
		else
		{
			p = _("方向 ('5'でターゲットへ, '*'でターゲット再選択, ESCで中断)? ", "Direction ('5' for target, '*' to re-target, Escape to cancel)? ");
		}

		/* Get a command (or Cancel) */
		if (!get_com(p, &command, TRUE)) break;

		if (use_menu)
		{
			if (command == '\r') command = 't';
		}  

		/* Convert various keys to "standard" keys */
		switch (command)
		{
			/* Use current target */
			case 'T':
			case 't':
			case '.':
			case '5':
			case '0':
			{
				dir = 5;
				break;
			}

			/* Set new target */
			case '*':
			case ' ':
			case '\r':
			{
				if (target_set(TARGET_KILL)) dir = 5;
				break;
			}

			default:
			{
				/* Look up the direction */
				dir = get_keymap_dir(command);

				break;
			}
		}

		/* Verify requested targets */
		if ((dir == 5) && !target_okay()) dir = 0;

		/* Error */
		if (!dir) bell();
	}

	/* No direction */
	if (!dir) return (FALSE);

	/* Save the direction */
	command_dir = dir;

	/* Check for confusion */
	if (p_ptr->confused)
	{
		/* Random direction */
		dir = ddd[randint0(8)];
	}

	/* Notice confusion */
	if (command_dir != dir)
	{
		/* Warn the user */
		msg_print(_("あなたは混乱している。", "You are confused."));
	}

	/* Save direction */
	(*dp) = dir;

	/* A "valid" direction was entered */
	return (TRUE);
}
