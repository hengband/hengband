/* File: spells3.c */

/* Purpose: Spell code (part 3) */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"

/* Maximum number of tries for teleporting */
#define MAX_TRIES 100

/* 1/x chance of reducing stats (for elemental attacks) */
#define HURT_CHANCE 16


/*
 * Teleport a monster, normally up to "dis" grids away.
 *
 * Attempt to move the monster at least "dis/2" grids away.
 *
 * But allow variation to prevent infinite loops.
 */
bool teleport_away(int m_idx, int dis, bool dec_valour)
{
	int oy, ox, d, i, min;
	int tries = 0;
	int ny = 0, nx = 0;

	bool look = TRUE;

	monster_type *m_ptr = &m_list[m_idx];


	/* Paranoia */
	if (!m_ptr->r_idx) return (FALSE);

	/* Save the old location */
	oy = m_ptr->fy;
	ox = m_ptr->fx;

	/* Minimum distance */
	min = dis / 2;

	if (dec_valour &&
	    (((p_ptr->chp * 10) / p_ptr->mhp) > 5) &&
		(4+randint1(5) < ((p_ptr->chp * 10) / p_ptr->mhp)))
	{	
		chg_virtue(V_VALOUR, -1);
	}

	/* Look until done */
	while (look)
	{
		tries++;

		/* Verify max distance */
		if (dis > 200) dis = 200;

		/* Try several locations */
		for (i = 0; i < 500; i++)
		{
			/* Pick a (possibly illegal) location */
			while (1)
			{
				ny = rand_spread(oy, dis);
				nx = rand_spread(ox, dis);
				d = distance(oy, ox, ny, nx);
				if ((d >= min) && (d <= dis)) break;
			}

			/* Ignore illegal locations */
			if (!in_bounds(ny, nx)) continue;

			/* Require "empty" floor space */
			if (!cave_empty_bold(ny, nx)) continue;

			/* Hack -- no teleport onto glyph of warding */
			if (cave[ny][nx].feat == FEAT_GLYPH) continue;
			if (cave[ny][nx].feat == FEAT_MINOR_GLYPH) continue;

			/* ...nor onto the Pattern */
			if ((cave[ny][nx].feat >= FEAT_PATTERN_START) &&
			    (cave[ny][nx].feat <= FEAT_PATTERN_XTRA2)) continue;

			/* No teleporting into vaults and such */
			if (!(p_ptr->inside_quest || p_ptr->inside_arena))
				if (cave[ny][nx].info & CAVE_ICKY) continue;

			/* This grid looks good */
			look = FALSE;

			/* Stop looking */
			break;
		}

		/* Increase the maximum distance */
		dis = dis * 2;

		/* Decrease the minimum distance */
		min = min / 2;

		/* Stop after MAX_TRIES tries */
		if (tries > MAX_TRIES) return (FALSE);
	}

	/* Sound */
	sound(SOUND_TPOTHER);

	/* Update the new location */
	cave[ny][nx].m_idx = m_idx;

	/* Update the old location */
	cave[oy][ox].m_idx = 0;

	/* Move the monster */
	m_ptr->fy = ny;
	m_ptr->fx = nx;

	/* Forget the counter target */
	reset_target(m_ptr);

	/* Update the monster (new location) */
	update_mon(m_idx, TRUE);

	/* Redraw the old grid */
	lite_spot(oy, ox);

	/* Redraw the new grid */
	lite_spot(ny, nx);

	return (TRUE);
}



/*
 * Teleport monster next to the player
 */
void teleport_to_player(int m_idx, int power)
{
	int ny, nx, oy, ox, d, i, min;
	int attempts = 500;
	int dis = 2;
	bool look = TRUE;
	monster_type *m_ptr = &m_list[m_idx];


	/* Paranoia */
	if (!m_ptr->r_idx) return;

	/* "Skill" test */
	if (randint1(100) > power) return;

	/* Initialize */
	ny = m_ptr->fy;
	nx = m_ptr->fx;

	/* Save the old location */
	oy = m_ptr->fy;
	ox = m_ptr->fx;

	/* Minimum distance */
	min = dis / 2;

	/* Look until done */
	while (look && --attempts)
	{
		/* Verify max distance */
		if (dis > 200) dis = 200;

		/* Try several locations */
		for (i = 0; i < 500; i++)
		{
			/* Pick a (possibly illegal) location */
			while (1)
			{
				ny = rand_spread(py, dis);
				nx = rand_spread(px, dis);
				d = distance(py, px, ny, nx);
				if ((d >= min) && (d <= dis)) break;
			}

			/* Ignore illegal locations */
			if (!in_bounds(ny, nx)) continue;

			/* Require "empty" floor space */
			if (!cave_empty_bold(ny, nx)) continue;

			/* Hack -- no teleport onto glyph of warding */
			if (cave[ny][nx].feat == FEAT_GLYPH) continue;
			if (cave[ny][nx].feat == FEAT_MINOR_GLYPH) continue;

			/* ...nor onto the Pattern */
			if ((cave[ny][nx].feat >= FEAT_PATTERN_START) &&
			    (cave[ny][nx].feat <= FEAT_PATTERN_XTRA2)) continue;

			/* No teleporting into vaults and such */
			/* if (cave[ny][nx].info & (CAVE_ICKY)) continue; */

			/* This grid looks good */
			look = FALSE;

			/* Stop looking */
			break;
		}

		/* Increase the maximum distance */
		dis = dis * 2;

		/* Decrease the minimum distance */
		min = min / 2;
	}

	if (attempts < 1) return;

	/* Sound */
	sound(SOUND_TPOTHER);

	/* Update the new location */
	cave[ny][nx].m_idx = m_idx;

	/* Update the old location */
	cave[oy][ox].m_idx = 0;

	/* Move the monster */
	m_ptr->fy = ny;
	m_ptr->fx = nx;

	/* Update the monster (new location) */
	update_mon(m_idx, TRUE);

	/* Redraw the old grid */
	lite_spot(oy, ox);

	/* Redraw the new grid */
	lite_spot(ny, nx);

	p_ptr->update |= (PU_MON_LITE);
}


/*
 * Teleport the player to a location up to "dis" grids away.
 *
 * If no such spaces are readily available, the distance may increase.
 * Try very hard to move the player at least a quarter that distance.
 *
 * When long-range teleport effects are considered, there is a nasty
 * tendency to "bounce" the player between two or three different spots
 * because these are the only spots that are "far enough" way to satisfy
 * the algorithm.  Therefore, if the teleport distance is more than 50,
 * we decrease the minimum acceptable distance to try to increase randomness.
 * -GJW
 */
void teleport_player(int dis)
{
	int d, i, min, ox, oy;
	int tries = 0;

	int xx = -1, yy = -1;

	/* Initialize */
	int y = py;
	int x = px;

	bool look = TRUE;

	if (p_ptr->wild_mode) return;

	if (p_ptr->anti_tele)
	{
#ifdef JP
msg_print("不思議な力がテレポートを防いだ！");
#else
		msg_print("A mysterious force prevents you from teleporting!");
#endif

		return;
	}

	if (dis > 200) dis = 200; /* To be on the safe side... */

	/* Minimum distance */
	min = dis / (dis > 50 ? 3 : 2);

	/* Look until done */
	while (look)
	{
		tries++;

		/* Verify max distance */
		if (dis > 200) dis = 200;

		/* Try several locations */
		for (i = 0; i < 500; i++)
		{
			/* Pick a (possibly illegal) location */
			while (1)
			{
				y = rand_spread(py, dis);
				x = rand_spread(px, dis);
				d = distance(py, px, y, x);
				if ((d >= min) && (d <= dis)) break;
			}

			/* Ignore illegal locations */
			if (!in_bounds(y, x)) continue;

			/* Require "naked" floor space or trees */
			if (!(cave_naked_bold(y, x) ||
			    (cave[y][x].feat == FEAT_TREES))) continue;

			/* No teleporting into vaults and such */
			if (cave[y][x].info & CAVE_ICKY) continue;

			/* This grid looks good */
			look = FALSE;

			/* Stop looking */
			break;
		}

		/* Increase the maximum distance */
		dis = dis * 2;

		/* Decrease the minimum distance */
		min = min / 2;

		/* Stop after MAX_TRIES tries */
		if (tries > MAX_TRIES) return;
	}

	/* Sound */
	sound(SOUND_TELEPORT);

#ifdef JP
	if ((p_ptr->pseikaku == SEIKAKU_COMBAT) || (inventory[INVEN_BOW].name1 == ART_CRIMSON))
		msg_format("『こっちだぁ、%s』", player_name);
#endif

	/* Save the old location */
	oy = py;
	ox = px;

	/* Move the player */
	py = y;
	px = x;

	if (p_ptr->riding)
	{
		int tmp;
		tmp = cave[py][px].m_idx;
		cave[py][px].m_idx = cave[oy][ox].m_idx;
		cave[oy][ox].m_idx = tmp;
		m_list[p_ptr->riding].fy = py;
		m_list[p_ptr->riding].fx = px;
		update_mon(cave[py][px].m_idx, TRUE);
	}

	/* Redraw the old spot */
	lite_spot(oy, ox);

	/* Monsters with teleport ability may follow the player */
	while (xx < 2)
	{
		yy = -1;

		while (yy < 2)
		{
			if (xx == 0 && yy == 0)
			{
				/* Do nothing */
			}
			else
			{
				if (cave[oy+yy][ox+xx].m_idx)
				{
					if ((r_info[m_list[cave[oy+yy][ox+xx].m_idx].r_idx].flags6 & RF6_TPORT) &&
					    !(r_info[m_list[cave[oy+yy][ox+xx].m_idx].r_idx].flags3 & RF3_RES_TELE))
						/*
						 * The latter limitation is to avoid
						 * totally unkillable suckers...
						 */
					{
						if (!(m_list[cave[oy+yy][ox+xx].m_idx].csleep))
							teleport_to_player(cave[oy+yy][ox+xx].m_idx, r_info[m_list[cave[oy+yy][ox+xx].m_idx].r_idx].level);
					}
				}
			}
			yy++;
		}
		xx++;
	}

	forget_flow();

	/* Redraw the new spot */
	lite_spot(py, px);

	/* Check for new panel (redraw map) */
	verify_panel();

	/* Update stuff */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE);

	/* Update the monsters */
	p_ptr->update |= (PU_DISTANCE);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	/* Handle stuff XXX XXX XXX */
	handle_stuff();
}



/*
 * Teleport player to a grid near the given location
 *
 * This function is slightly obsessive about correctness.
 * This function allows teleporting into vaults (!)
 */
void teleport_player_to(int ny, int nx, bool no_tele)
{
	int y, x, oy, ox, dis = 0, ctr = 0;

	if (p_ptr->anti_tele && no_tele)
	{
#ifdef JP
msg_print("不思議な力がテレポートを防いだ！");
#else
		msg_print("A mysterious force prevents you from teleporting!");
#endif

		return;
	}

	/* Find a usable location */
	while (1)
	{
		/* Pick a nearby legal location */
		while (1)
		{
			y = rand_spread(ny, dis);
			x = rand_spread(nx, dis);
			if (in_bounds(y, x)) break;
		}

		/* Accept "naked" floor grids */
		if (no_tele)
		{
			if (cave_naked_bold(y, x) || (((cave[y][x].feat == FEAT_DEEP_LAVA) || (cave[y][x].feat == FEAT_DEEP_WATER)) && !cave[y][x].m_idx)) break;
		}
		else if (cave_empty_bold(y, x) || ((y == py) && (x == px))) break;

		/* Occasionally advance the distance */
		if (++ctr > (4 * dis * dis + 4 * dis + 1))
		{
			ctr = 0;
			dis++;
		}
	}

	/* Sound */
	sound(SOUND_TELEPORT);

	/* Save the old location */
	oy = py;
	ox = px;

	/* Move the player */
	py = y;
	px = x;

	if (p_ptr->riding)
	{
		int tmp;
		tmp = cave[py][px].m_idx;
		cave[py][px].m_idx = cave[oy][ox].m_idx;
		cave[oy][ox].m_idx = tmp;
		m_list[p_ptr->riding].fy = py;
		m_list[p_ptr->riding].fx = px;
		update_mon(cave[py][px].m_idx, TRUE);
	}

	forget_flow();

	/* Redraw the old spot */
	lite_spot(oy, ox);

	/* Redraw the new spot */
	lite_spot(py, px);

	/* Check for new panel (redraw map) */
	verify_panel();

	/* Update stuff */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE);

	/* Update the monsters */
	p_ptr->update |= (PU_DISTANCE);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	/* Handle stuff XXX XXX XXX */
	handle_stuff();
}



/*
 * Teleport the player one level up or down (random when legal)
 */
void teleport_player_level(void)
{
	/* No effect in arena or quest */
	if (p_ptr->inside_arena || (p_ptr->inside_quest && !random_quest_number(dun_level)) ||
	    (quest_number(dun_level) && (dun_level > 1) && ironman_downward))
	{
#ifdef JP
msg_print("効果がなかった。");
#else
		msg_print("There is no effect.");
#endif

		return;
	}

	if (p_ptr->anti_tele)
	{
#ifdef JP
msg_print("不思議な力がテレポートを防いだ！");
#else
		msg_print("A mysterious force prevents you from teleporting!");
#endif

		return;
	}

	if (ironman_downward || (dun_level <= d_info[dungeon_type].mindepth))
	{
#ifdef JP
msg_print("あなたは床を突き破って沈んでいく。");
#else
		msg_print("You sink through the floor.");
#endif
		if (!dun_level)
		{
			dungeon_type = p_ptr->recall_dungeon;
			p_ptr->oldpy = py;
			p_ptr->oldpx = px;
		}

		if (record_stair) do_cmd_write_nikki(NIKKI_TELE_LEV, 1, NULL);

		if (autosave_l) do_cmd_save_game(TRUE);

		if (!dun_level)
		{
			dun_level = d_info[dungeon_type].mindepth;
		}
		else
		{
			dun_level++;
		}

		/* Leaving */
		p_ptr->leaving = TRUE;
	}
	else if (quest_number(dun_level) || (dun_level >= d_info[dungeon_type].maxdepth))
	{
#ifdef JP
msg_print("あなたは天井を突き破って宙へ浮いていく。");
#else
		msg_print("You rise up through the ceiling.");
#endif


		if (record_stair) do_cmd_write_nikki(NIKKI_TELE_LEV, -1, NULL);

		if (autosave_l) do_cmd_save_game(TRUE);

		dun_level--;

		if (!dun_level) dungeon_type = 0;

		leave_quest_check();

		/* Leaving */
		p_ptr->inside_quest = 0;
		p_ptr->leaving = TRUE;
	}
	else if (randint0(100) < 50)
	{
#ifdef JP
msg_print("あなたは天井を突き破って宙へ浮いていく。");
#else
		msg_print("You rise up through the ceiling.");
#endif


		if (record_stair) do_cmd_write_nikki(NIKKI_TELE_LEV, -1, NULL);

		if (autosave_l) do_cmd_save_game(TRUE);

		dun_level--;

		if (!dun_level) dungeon_type = 0;

		/* Leaving */
		p_ptr->leaving = TRUE;
	}
	else
	{
#ifdef JP
msg_print("あなたは床を突き破って沈んでいく。");
#else
		msg_print("You sink through the floor.");
#endif

		if (!dun_level) dungeon_type = p_ptr->recall_dungeon;

		if (record_stair) do_cmd_write_nikki(NIKKI_TELE_LEV, 1, NULL);

		if (autosave_l) do_cmd_save_game(TRUE);

		dun_level++;

		/* Leaving */
		p_ptr->leaving = TRUE;
	}

	if (!dun_level && dungeon_type)
	{
		p_ptr->leaving_dungeon = TRUE;
		p_ptr->wilderness_y = d_info[dungeon_type].dy;
		p_ptr->wilderness_x = d_info[dungeon_type].dx;
		p_ptr->recall_dungeon = dungeon_type;
	}

	if (!dun_level) dungeon_type = 0;

	/* Sound */
	sound(SOUND_TPLEVEL);
}



static int choose_dungeon(cptr note)
{
	int select_dungeon;
	int i, num = 0;
	s16b *dun;

	/* Allocate the "dun" array */
	C_MAKE(dun, max_d_idx, s16b);

	screen_save();
	for(i = 1; i < max_d_idx; i++)
	{
		char buf[80];
		bool seiha = FALSE;

		if (!d_info[i].maxdepth) continue;
		if (!max_dlv[i]) continue;
		if (d_info[i].final_guardian)
		{
			if (!r_info[d_info[i].final_guardian].max_num) seiha = TRUE;
		}
		else if (max_dlv[i] == d_info[i].maxdepth) seiha = TRUE;

#ifdef JP
		sprintf(buf,"      %c) %c%-12s : 最大 %d 階", 'a'+num, seiha ? '!' : ' ', d_name + d_info[i].name, max_dlv[i]);
#else
		sprintf(buf,"      %c) %c%-16s : Max level %d", 'a'+num, seiha ? '!' : ' ', d_name + d_info[i].name, max_dlv[i]);
#endif
		prt(buf, 2+num, 14);
		dun[num++] = i;
	}
#ifdef JP
	prt(format("どのダンジョン%sしますか:", note), 0, 0);
#else
	prt(format("Which dungeon do you %s?: ", note), 0, 0);
#endif
	while(1)
	{
		i = inkey();
		if (i == ESCAPE)
		{
			/* Free the "dun" array */
			C_KILL(dun, max_d_idx, s16b);

			screen_load();
			return 0;
		}
		if (i >= 'a' && i <('a'+num))
		{
			select_dungeon = dun[i-'a'];
			break;
		}
		else bell();
	}
	screen_load();

	/* Free the "dun" array */
	C_KILL(dun, max_d_idx, s16b);

	return select_dungeon;
}


/*
 * Recall the player to town or dungeon
 */
bool recall_player(int turns)
{
	/*
	 * TODO: Recall the player to the last
	 * visited town when in the wilderness
	 */

	/* Ironman option */
	if (p_ptr->inside_arena || ironman_downward)
	{
#ifdef JP
msg_print("何も起こらなかった。");
#else
		msg_print("Nothing happens.");
#endif

		return TRUE;
	}

	if (dun_level && (max_dlv[dungeon_type] > dun_level) && !p_ptr->inside_quest && !p_ptr->word_recall)
	{
#ifdef JP
if (get_check("ここは最深到達階より浅い階です。この階に戻って来ますか？ "))
#else
		if (get_check("Reset recall depth? "))
#endif
		{
			max_dlv[dungeon_type] = dun_level;
			if (record_maxdeapth)
#ifdef JP
				do_cmd_write_nikki(NIKKI_TRUMP, dungeon_type, "帰還のときに");
#else
				do_cmd_write_nikki(NIKKI_TRUMP, dungeon_type, "when recall from dungeon");
#endif
		}

	}
	if (!p_ptr->word_recall)
	{
		if (!dun_level)
		{
			int select_dungeon;
#ifdef JP
			select_dungeon = choose_dungeon("に帰還");
#else
			select_dungeon = choose_dungeon("recall");
#endif
			if (!select_dungeon) return FALSE;
			p_ptr->recall_dungeon = select_dungeon;
		}
		p_ptr->word_recall = turns;
#ifdef JP
msg_print("回りの大気が張りつめてきた...");
#else
		msg_print("The air about you becomes charged...");
#endif

		p_ptr->redraw |= (PR_STATUS);
	}
	else
	{
		p_ptr->word_recall = 0;
#ifdef JP
msg_print("張りつめた大気が流れ去った...");
#else
		msg_print("A tension leaves the air around you...");
#endif

		p_ptr->redraw |= (PR_STATUS);
	}
	return TRUE;
}


bool word_of_recall(void)
{
	return(recall_player(randint0(21) + 15));
}


bool reset_recall(void)
{
	int select_dungeon, dummy = 0;
	char ppp[80];
	char tmp_val[160];

#ifdef JP
	select_dungeon = choose_dungeon("をセット");
#else
	select_dungeon = choose_dungeon("reset");
#endif

	if (!select_dungeon) return FALSE;
	/* Prompt */
#ifdef JP
sprintf(ppp, "何階にセットしますか (%d-%d):", d_info[select_dungeon].mindepth, max_dlv[select_dungeon]);
#else
	sprintf(ppp, "Reset to which level (%d-%d): ", d_info[select_dungeon].mindepth, max_dlv[select_dungeon]);
#endif


	/* Default */
	sprintf(tmp_val, "%d", MAX(dun_level, 1));

	/* Ask for a level */
	if (get_string(ppp, tmp_val, 10))
	{
		/* Extract request */
		dummy = atoi(tmp_val);

		/* Paranoia */
		if (dummy < 1) dummy = 1;

		/* Paranoia */
		if (dummy > max_dlv[select_dungeon]) dummy = max_dlv[select_dungeon];
		if (dummy < d_info[select_dungeon].mindepth) dummy = d_info[select_dungeon].mindepth;

		max_dlv[select_dungeon] = dummy;

		if (record_maxdeapth)
#ifdef JP
			do_cmd_write_nikki(NIKKI_TRUMP, select_dungeon, "フロア・リセットで");
#else
			do_cmd_write_nikki(NIKKI_TRUMP, select_dungeon, "using a scroll of reset recall");
#endif
					/* Accept request */
#ifdef JP
msg_format("%sの帰還レベルを %d 階にセット。", d_name+d_info[select_dungeon].name, dummy, dummy * 50);
#else
		msg_format("Recall depth set to level %d (%d').", dummy, dummy * 50);
#endif

	}
	else
	{
		return FALSE;
	}
	return TRUE;
}


/*
 * Apply disenchantment to the player's stuff
 *
 * XXX XXX XXX This function is also called from the "melee" code
 *
 * Return "TRUE" if the player notices anything
 */
bool apply_disenchant(int mode)
{
	int             t = 0;
	object_type     *o_ptr;
	char            o_name[MAX_NLEN];
	int to_h, to_d, to_a, pval;

	/* Pick a random slot */
	switch (randint1(8))
	{
		case 1: t = INVEN_RARM; break;
		case 2: t = INVEN_LARM; break;
		case 3: t = INVEN_BOW; break;
		case 4: t = INVEN_BODY; break;
		case 5: t = INVEN_OUTER; break;
		case 6: t = INVEN_HEAD; break;
		case 7: t = INVEN_HANDS; break;
		case 8: t = INVEN_FEET; break;
	}

	/* Get the item */
	o_ptr = &inventory[t];

	/* No item, nothing happens */
	if (!o_ptr->k_idx) return (FALSE);


	/* Nothing to disenchant */
	if ((o_ptr->to_h <= 0) && (o_ptr->to_d <= 0) && (o_ptr->to_a <= 0) && (o_ptr->pval <= 1))
	{
		/* Nothing to notice */
		return (FALSE);
	}


	/* Describe the object */
	object_desc(o_name, o_ptr, FALSE, 0);


	/* Artifacts have 71% chance to resist */
	if ((artifact_p(o_ptr) || o_ptr->art_name) && (randint0(100) < 71))
	{
		/* Message */
#ifdef JP
msg_format("%s(%c)は劣化を跳ね返した！",o_name, index_to_label(t) );
#else
		msg_format("Your %s (%c) resist%s disenchantment!",
			   o_name, index_to_label(t),
			   ((o_ptr->number != 1) ? "" : "s"));
#endif


		/* Notice */
		return (TRUE);
	}


	/* Memorize old value */
	to_h = o_ptr->to_h;
	to_d = o_ptr->to_d;
	to_a = o_ptr->to_a;
	pval = o_ptr->pval;

	/* Disenchant tohit */
	if (o_ptr->to_h > 0) o_ptr->to_h--;
	if ((o_ptr->to_h > 5) && (randint0(100) < 20)) o_ptr->to_h--;

	/* Disenchant todam */
	if (o_ptr->to_d > 0) o_ptr->to_d--;
	if ((o_ptr->to_d > 5) && (randint0(100) < 20)) o_ptr->to_d--;

	/* Disenchant toac */
	if (o_ptr->to_a > 0) o_ptr->to_a--;
	if ((o_ptr->to_a > 5) && (randint0(100) < 20)) o_ptr->to_a--;

	/* Disenchant pval (occasionally) */
	/* Unless called from wild_magic() */
	if ((o_ptr->pval > 1) && one_in_(13) && !(mode & 0x01)) o_ptr->pval--;

	if ((to_h != o_ptr->to_h) || (to_d != o_ptr->to_d) ||
	    (to_a != o_ptr->to_a) || (pval != o_ptr->pval))
	{
		/* Message */
#ifdef JP
		msg_format("%s(%c)は劣化してしまった！",
			   o_name, index_to_label(t) );
#else
		msg_format("Your %s (%c) %s disenchanted!",
			   o_name, index_to_label(t),
			   ((o_ptr->number != 1) ? "were" : "was"));
#endif

		chg_virtue(V_HARMONY, 1);
		chg_virtue(V_ENCHANT, -2);

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Window stuff */
		p_ptr->window |= (PW_EQUIP | PW_PLAYER);

		calc_android_exp();
	}

	/* Notice */
	return (TRUE);
}


void mutate_player(void)
{
	int max1, cur1, max2, cur2, ii, jj, i;

	/* Pick a pair of stats */
	ii = randint0(6);
	for (jj = ii; jj == ii; jj = randint0(6)) /* loop */;

	max1 = p_ptr->stat_max[ii];
	cur1 = p_ptr->stat_cur[ii];
	max2 = p_ptr->stat_max[jj];
	cur2 = p_ptr->stat_cur[jj];

	p_ptr->stat_max[ii] = max2;
	p_ptr->stat_cur[ii] = cur2;
	p_ptr->stat_max[jj] = max1;
	p_ptr->stat_cur[jj] = cur1;

	for (i=0;i<6;i++)
	{
		if(p_ptr->stat_max[i] > p_ptr->stat_max_max[i]) p_ptr->stat_max[i] = p_ptr->stat_max_max[i];
		if(p_ptr->stat_cur[i] > p_ptr->stat_max_max[i]) p_ptr->stat_cur[i] = p_ptr->stat_max_max[i];
	}

	p_ptr->update |= (PU_BONUS);
}


/*
 * Apply Nexus
 */
void apply_nexus(monster_type *m_ptr)
{
	switch (randint1(7))
	{
		case 1: case 2: case 3:
		{
			teleport_player(200);
			break;
		}

		case 4: case 5:
		{
			teleport_player_to(m_ptr->fy, m_ptr->fx, TRUE);
			break;
		}

		case 6:
		{
			if (randint0(100) < p_ptr->skill_sav)
			{
#ifdef JP
msg_print("しかし効力を跳ね返した！");
#else
				msg_print("You resist the effects!");
#endif

				break;
			}

			/* Teleport Level */
			teleport_player_level();
			break;
		}

		case 7:
		{
			if (randint0(100) < p_ptr->skill_sav)
			{
#ifdef JP
msg_print("しかし効力を跳ね返した！");
#else
				msg_print("You resist the effects!");
#endif

				break;
			}

#ifdef JP
msg_print("体がねじれ始めた...");
#else
			msg_print("Your body starts to scramble...");
#endif

			mutate_player();
			break;
		}
	}
}


/*
 * Charge a lite (torch or latern)
 */
void phlogiston(void)
{
	int max_flog = 0;
	object_type * o_ptr = &inventory[INVEN_LITE];

	/* It's a lamp */
	if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_LANTERN))
	{
		max_flog = FUEL_LAMP;
	}

	/* It's a torch */
	else if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_TORCH))
	{
		max_flog = FUEL_TORCH;
	}

	/* No torch to refill */
	else
	{
#ifdef JP
msg_print("燃素を消費するアイテムを装備していません。");
#else
		msg_print("You are not wielding anything which uses phlogiston.");
#endif

		return;
	}

	if (o_ptr->xtra4 >= max_flog)
	{
#ifdef JP
msg_print("このアイテムにはこれ以上燃素を補充できません。");
#else
		msg_print("No more phlogiston can be put in this item.");
#endif

		return;
	}

	/* Refuel */
	o_ptr->xtra4 += (max_flog / 2);

	/* Message */
#ifdef JP
msg_print("照明用アイテムに燃素を補充した。");
#else
	msg_print("You add phlogiston to your light item.");
#endif


	/* Comment */
	if (o_ptr->xtra4 >= max_flog)
	{
		o_ptr->xtra4 = max_flog;
#ifdef JP
msg_print("照明用アイテムは満タンになった。");
#else
		msg_print("Your light item is full.");
#endif

	}

	/* Recalculate torch */
	p_ptr->update |= (PU_TORCH);
}


static bool item_tester_hook_weapon_nobow(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_DIGGING:
		{
			return (TRUE);
		}
		case TV_SWORD:
		{
			if (o_ptr->sval != SV_DOKUBARI) return (TRUE);
		}
	}

	return (FALSE);
}

/*
 * Brand the current weapon
 */
void brand_weapon(int brand_type)
{
	int         item;
	object_type *o_ptr;
	cptr        q, s;


	/* Assume enchant weapon */
	item_tester_hook = item_tester_hook_weapon_nobow;
	item_tester_no_ryoute = TRUE;

	/* Get an item */
#ifdef JP
q = "どの武器を強化しますか? ";
s = "強化できる武器がない。";
#else
	q = "Enchant which weapon? ";
	s = "You have nothing to enchant.";
#endif

	if (!get_item(&item, q, s, (USE_EQUIP))) return;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}


	/* you can never modify artifacts / ego-items */
	/* you can never modify cursed items */
	/* TY: You _can_ modify broken items (if you're silly enough) */
	if (o_ptr->k_idx && !artifact_p(o_ptr) && !ego_item_p(o_ptr) &&
	    !o_ptr->art_name && !cursed_p(o_ptr) &&
	    !((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DOKUBARI)) &&
	    !((o_ptr->tval == TV_POLEARM) && (o_ptr->sval == SV_DEATH_SCYTHE)) &&
	    !((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DIAMOND_EDGE)))
	{
		cptr act = NULL;

		/* Let's get the name before it is changed... */
		char o_name[MAX_NLEN];
		object_desc(o_name, o_ptr, FALSE, 0);

		switch (brand_type)
		{
		case 17:
			if (o_ptr->tval == TV_SWORD)
			{
#ifdef JP
act = "は鋭さを増した！";
#else
				act = "becomes very sharp!";
#endif

				o_ptr->name2 = EGO_SHARPNESS;
				o_ptr->pval = m_bonus(5, dun_level) + 1;
			}
			else
			{
#ifdef JP
act = "は破壊力を増した！";
#else
				act = "seems very powerful.";
#endif

				o_ptr->name2 = EGO_EARTHQUAKES;
				o_ptr->pval = m_bonus(3, dun_level);
			}
			break;
		case 16:
#ifdef JP
act = "は人間の血を求めている！";
#else
			act = "seems looking for human!";
#endif

			o_ptr->name2 = EGO_SLAY_HUMAN;
			break;
		case 15:
#ifdef JP
act = "は電撃に覆われた！";
#else
			act = "coverd with lightning!";
#endif

			o_ptr->name2 = EGO_BRAND_ELEC;
			break;
		case 14:
#ifdef JP
act = "は酸に覆われた！";
#else
			act = "coated with acid!";
#endif

			o_ptr->name2 = EGO_BRAND_ACID;
			break;
		case 13:
#ifdef JP
act = "は邪悪なる怪物を求めている！";
#else
			act = "seems looking for evil monster!";
#endif

			o_ptr->name2 = EGO_SLAY_EVIL;
			break;
		case 12:
#ifdef JP
act = "は異世界の住人の肉体を求めている！";
#else
			act = "seems looking for demon!";
#endif

			o_ptr->name2 = EGO_SLAY_DEMON;
			break;
		case 11:
#ifdef JP
act = "は屍を求めている！";
#else
			act = "seems looking for undead!";
#endif

			o_ptr->name2 = EGO_SLAY_UNDEAD;
			break;
		case 10:
#ifdef JP
act = "は動物の血を求めている！";
#else
			act = "seems looking for animal!";
#endif

			o_ptr->name2 = EGO_SLAY_ANIMAL;
			break;
		case 9:
#ifdef JP
act = "はドラゴンの血を求めている！";
#else
			act = "seems looking for dragon!";
#endif

			o_ptr->name2 = EGO_SLAY_DRAGON;
			break;
		case 8:
#ifdef JP
act = "はトロルの血を求めている！";
#else
			act = "seems looking for troll!";
#endif

			o_ptr->name2 = EGO_SLAY_TROLL;
			break;
		case 7:
#ifdef JP
act = "はオークの血を求めている！";
#else
			act = "seems looking for orc!";
#endif

			o_ptr->name2 = EGO_SLAY_ORC;
			break;
		case 6:
#ifdef JP
act = "は巨人の血を求めている！";
#else
			act = "seems looking for giant!";
#endif

			o_ptr->name2 = EGO_SLAY_GIANT;
			break;
		case 5:
#ifdef JP
act = "は非常に不安定になったようだ。";
#else
			act = "seems very unstable now.";
#endif

			o_ptr->name2 = EGO_TRUMP;
			o_ptr->pval = randint1(2);
			break;
		case 4:
#ifdef JP
act = "は血を求めている！";
#else
			act = "thirsts for blood!";
#endif

			o_ptr->name2 = EGO_VAMPIRIC;
			break;
		case 3:
#ifdef JP
act = "は毒に覆われた。";
#else
			act = "is coated with poison.";
#endif

			o_ptr->name2 = EGO_BRAND_POIS;
			break;
		case 2:
#ifdef JP
act = "は純ログルスに飲み込まれた。";
#else
			act = "is engulfed in raw Logrus!";
#endif

			o_ptr->name2 = EGO_CHAOTIC;
			break;
		case 1:
#ifdef JP
act = "は炎のシールドに覆われた！";
#else
			act = "is covered in a fiery shield!";
#endif

			o_ptr->name2 = EGO_BRAND_FIRE;
			break;
		default:
#ifdef JP
act = "は深く冷たいブルーに輝いた！";
#else
			act = "glows deep, icy blue!";
#endif

			o_ptr->name2 = EGO_BRAND_COLD;
			break;
		}

#ifdef JP
msg_format("あなたの%s%s", o_name, act);
#else
		msg_format("Your %s %s", o_name, act);
#endif


		enchant(o_ptr, randint0(3) + 4, ENCH_TOHIT | ENCH_TODAM);

		o_ptr->discount = 99;
		chg_virtue(V_ENCHANT, 2);
	}
	else
	{
		if (flush_failure) flush();

#ifdef JP
msg_print("属性付加に失敗した。");
#else
		msg_print("The Branding failed.");
#endif

		chg_virtue(V_ENCHANT, -2);
	}
	calc_android_exp();
}


void call_the_(void)
{
	int i;

	if (cave_floor_bold(py - 1, px - 1) &&
	    cave_floor_bold(py - 1, px    ) &&
	    cave_floor_bold(py - 1, px + 1) &&
	    cave_floor_bold(py    , px - 1) &&
	    cave_floor_bold(py    , px + 1) &&
	    cave_floor_bold(py + 1, px - 1) &&
	    cave_floor_bold(py + 1, px    ) &&
	    cave_floor_bold(py + 1, px + 1))
	{
		for (i = 1; i < 10; i++)
		{
			if (i-5) fire_ball(GF_ROCKET, i, 175, 2);
		}

		for (i = 1; i < 10; i++)
		{
			if (i-5) fire_ball(GF_MANA, i, 175, 3);
		}

		for (i = 1; i < 10; i++)
		{
			if (i-5) fire_ball(GF_NUKE, i, 175, 4);
		}
	}
	else
	{
#ifdef JP
msg_format("あなたは%sを壁に近すぎる場所で唱えてしまった！",
((mp_ptr->spell_book == TV_LIFE_BOOK) ? "祈り" : "呪文"));
msg_print("大きな爆発音があった！");
#else
		msg_format("You %s the %s too close to a wall!",
			((mp_ptr->spell_book == TV_LIFE_BOOK) ? "recite" : "cast"),
			((mp_ptr->spell_book == TV_LIFE_BOOK) ? "prayer" : "spell"));
		msg_print("There is a loud explosion!");
#endif


		if (destroy_area(py, px, 15 + p_ptr->lev + randint0(11), TRUE))
#ifdef JP
msg_print("ダンジョンが崩壊した...");
#else
			msg_print("The dungeon collapses...");
#endif

		else
#ifdef JP
msg_print("ダンジョンは大きく揺れた。");
#else
			msg_print("The dungeon trembles.");
#endif


#ifdef JP
take_hit(DAMAGE_NOESCAPE, 100 + randint1(150), "自殺的な虚無招来", -1);
#else
		take_hit(DAMAGE_NOESCAPE, 100 + randint1(150), "a suicidal Call the Void", -1);
#endif

	}
}


/*
 * Fetch an item (teleport it right underneath the caster)
 */
void fetch(int dir, int wgt, bool require_los)
{
	int             ty, tx, i;
	bool            flag;
	cave_type       *c_ptr;
	object_type     *o_ptr;
	char            o_name[MAX_NLEN];

	/* Check to see if an object is already there */
	if (cave[py][px].o_idx)
	{
#ifdef JP
msg_print("自分の足の下にある物は取れません。");
#else
		msg_print("You can't fetch when you're already standing on something.");
#endif

		return;
	}

	/* Use a target */
	if (dir == 5 && target_okay())
	{
		tx = target_col;
		ty = target_row;

		if (distance(py, px, ty, tx) > MAX_RANGE)
		{
#ifdef JP
msg_print("そんなに遠くにある物は取れません！");
#else
			msg_print("You can't fetch something that far away!");
#endif

			return;
		}

		c_ptr = &cave[ty][tx];

		/* We need an item to fetch */
		if (!c_ptr->o_idx)
		{
#ifdef JP
msg_print("そこには何もありません。");
#else
			msg_print("There is no object at this place.");
#endif

			return;
		}

		/* No fetching from vault */
		if (c_ptr->info & CAVE_ICKY)
		{
#ifdef JP
msg_print("アイテムがコントロールを外れて落ちた。");
#else
			msg_print("The item slips from your control.");
#endif

			return;
		}

		/* We need to see the item */
		if (require_los && !player_has_los_bold(ty, tx))
		{
#ifdef JP
msg_print("そこはあなたの視界に入っていません。");
#else
			msg_print("You have no direct line of sight to that location.");
#endif

			return;
		}
	}
	else
	{
		/* Use a direction */
		ty = py; /* Where to drop the item */
		tx = px;
		flag = FALSE;

		do
		{
			ty += ddy[dir];
			tx += ddx[dir];
			c_ptr = &cave[ty][tx];

			if ((distance(py, px, ty, tx) > MAX_RANGE) ||
			    !cave_floor_bold(ty, tx)) return;
		}
		while (!c_ptr->o_idx);
	}

	o_ptr = &o_list[c_ptr->o_idx];

	if (o_ptr->weight > wgt)
	{
		/* Too heavy to 'fetch' */
#ifdef JP
msg_print("そのアイテムは重過ぎます。");
#else
		msg_print("The object is too heavy.");
#endif

		return;
	}

	i = c_ptr->o_idx;
	c_ptr->o_idx = o_ptr->next_o_idx;
	cave[py][px].o_idx = i; /* 'move' it */
	o_ptr->next_o_idx = 0;
	o_ptr->iy = (byte)py;
	o_ptr->ix = (byte)px;

	object_desc(o_name, o_ptr, TRUE, 0);
#ifdef JP
msg_format("%^sがあなたの足元に飛んできた。", o_name);
#else
	msg_format("%^s flies through the air to your feet.", o_name);
#endif


	note_spot(py, px);
	p_ptr->redraw |= PR_MAP;
}


void alter_reality(void)
{
	if (!quest_number(dun_level) && dun_level)
	{
#ifdef JP
msg_print("世界が変わった！");
#else
		msg_print("The world changes!");
#endif


		if (autosave_l) do_cmd_save_game(TRUE);

		/* Leaving */
		p_ptr->leaving = TRUE;
	}
	else
	{
#ifdef JP
msg_print("世界が少しの間変化したようだ。");
#else
		msg_print("The world seems to change for a moment!");
#endif

	}
}


/*
 * Leave a "glyph of warding" which prevents monster movement
 */
bool warding_glyph(void)
{
	/* XXX XXX XXX */
	if (!cave_clean_bold(py, px))
	{
#ifdef JP
msg_print("床上のアイテムが呪文を跳ね返した。");
#else
		msg_print("The object resists the spell.");
#endif

		return FALSE;
	}

	/* Create a glyph */
	cave_set_feat(py, px, FEAT_GLYPH);

	return TRUE;
}

bool warding_mirror(void)
{
	/* XXX XXX XXX */
	if (!cave_clean_bold(py, px))
	{
#ifdef JP
msg_print("床上のアイテムが呪文を跳ね返した。");
#else
		msg_print("The object resists the spell.");
#endif

		return FALSE;
	}

	/* Create a mirror */
	cave[py][px].info |= CAVE_IN_MIRROR;

	/* Turn on the light */
	cave[py][px].info |= CAVE_GLOW;

	/* Notice */
	note_spot(py, px);
	
	/* Redraw */
	lite_spot(py, px);

	return TRUE;
}


/*
 * Leave an "explosive rune" which prevents monster movement
 */
bool explosive_rune(void)
{
	/* XXX XXX XXX */
	if (!cave_clean_bold(py, px))
	{
#ifdef JP
msg_print("床上のアイテムが呪文を跳ね返した。");
#else
		msg_print("The object resists the spell.");
#endif

		return FALSE;
	}

	/* Create a glyph */
	cave_set_feat(py, px, FEAT_MINOR_GLYPH);

	return TRUE;
}


/*
 * Identify everything being carried.
 * Done by a potion of "self knowledge".
 */
void identify_pack(void)
{
	int i;

	/* Simply identify and know every item */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Identify it */
		identify_item(o_ptr);
	}
}


/*
 * Used by the "enchant" function (chance of failure)
 * (modified for Zangband, we need better stuff there...) -- TY
 */
static int enchant_table[16] =
{
	0, 10,  50, 100, 200,
	300, 400, 500, 650, 800,
	950, 987, 993, 995, 998,
	1000
};


/*
 * Removes curses from items in inventory
 *
 * Note that Items which are "Perma-Cursed" (The One Ring,
 * The Crown of Morgoth) can NEVER be uncursed.
 *
 * Note that if "all" is FALSE, then Items which are
 * "Heavy-Cursed" (Mormegil, Calris, and Weapons of Morgul)
 * will not be uncursed.
 */
static int remove_curse_aux(int all)
{
	int i, cnt = 0;

	/* Attempt to uncurse items being worn */
	for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Uncursed already */
		if (!cursed_p(o_ptr)) continue;

		/* Heavily Cursed Items need a special spell */
		if (!all && (o_ptr->curse_flags & TRC_HEAVY_CURSE)) continue;

		/* Perma-Cursed Items can NEVER be uncursed */
		if (o_ptr->curse_flags & TRC_PERMA_CURSE)
		{
			/* Uncurse it */
			o_ptr->curse_flags &= (TRC_CURSED | TRC_HEAVY_CURSE | TRC_PERMA_CURSE);
			continue;
		}

		/* Uncurse it */
		o_ptr->curse_flags = 0L;

		/* Hack -- Assume felt */
		o_ptr->ident |= (IDENT_SENSE);

		/* Take note */
		o_ptr->feeling = FEEL_NONE;

		/* Recalculate the bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Window stuff */
		p_ptr->window |= (PW_EQUIP);

		/* Count the uncursings */
		cnt++;
	}

	/* Return "something uncursed" */
	return (cnt);
}


/*
 * Remove most curses
 */
bool remove_curse(void)
{
	return (remove_curse_aux(FALSE));
}

/*
 * Remove all curses
 */
bool remove_all_curse(void)
{
	return (remove_curse_aux(TRUE));
}


/*
 * Turns an object into gold, gain some of its value in a shop
 */
bool alchemy(void)
{
	int item, amt = 1;
	int old_number;
	long price;
	bool force = FALSE;
	object_type *o_ptr;
	char o_name[MAX_NLEN];
	char out_val[MAX_NLEN+40];

	cptr q, s;

	/* Hack -- force destruction */
	if (command_arg > 0) force = TRUE;

	/* Get an item */
#ifdef JP
q = "どのアイテムを金に変えますか？";
s = "金に変えられる物がありません。";
#else
	q = "Turn which item to gold? ";
	s = "You have nothing to turn to gold.";
#endif

	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return (FALSE);

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}


	/* See how many items */
	if (o_ptr->number > 1)
	{
		/* Get a quantity */
		amt = get_quantity(NULL, o_ptr->number);

		/* Allow user abort */
		if (amt <= 0) return FALSE;
	}


	/* Describe the object */
	old_number = o_ptr->number;
	o_ptr->number = amt;
	object_desc(o_name, o_ptr, TRUE, 3);
	o_ptr->number = old_number;

	/* Verify unless quantity given */
	if (!force)
	{
		if (confirm_destroy || (object_value(o_ptr) > 0))
		{
			/* Make a verification */
#ifdef JP
sprintf(out_val, "本当に%sを金に変えますか？", o_name);
#else
			sprintf(out_val, "Really turn %s to gold? ", o_name);
#endif

			if (!get_check(out_val)) return FALSE;
		}
	}

	/* Artifacts cannot be destroyed */
	if (!can_player_destroy_object(o_ptr))
	{
		/* Message */
#ifdef JP
		msg_format("%sを金に変えることに失敗した。", o_name);
#else
		msg_format("You fail to turn %s to gold!", o_name);
#endif

		/* Done */
		return FALSE;
	}

	price = object_value_real(o_ptr);

	if (price <= 0)
	{
		/* Message */
#ifdef JP
msg_format("%sをニセの金に変えた。", o_name);
#else
		msg_format("You turn %s to fool's gold.", o_name);
#endif

	}
	else
	{
		price /= 3;

		if (amt > 1) price *= amt;

		if (price > 30000) price = 30000;
#ifdef JP
msg_format("%sを＄%d の金に変えた。", o_name, price);
#else
		msg_format("You turn %s to %ld coins worth of gold.", o_name, price);
#endif

		p_ptr->au += price;

		/* Redraw gold */
		p_ptr->redraw |= (PR_GOLD);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

	}

	/* Eliminate the item (from the pack) */
	if (item >= 0)
	{
		inven_item_increase(item, -amt);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Eliminate the item (from the floor) */
	else
	{
		floor_item_increase(0 - item, -amt);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}

	return TRUE;
}


/*
 * Create stairs at the player location
 */
void stair_creation(void)
{
	/* XXX XXX XXX */
	if (!cave_valid_bold(py, px))
	{
#ifdef JP
msg_print("床上のアイテムが呪文を跳ね返した。");
#else
		msg_print("The object resists the spell.");
#endif

		return;
	}

	/* XXX XXX XXX */
	delete_object(py, px);

	/* Create a staircase */
	if (p_ptr->inside_arena || (p_ptr->inside_quest && (p_ptr->inside_quest < MIN_RANDOM_QUEST)) || p_ptr->inside_battle || !dun_level)
	{
		/* arena or quest */
#ifdef JP
msg_print("効果がありません！");
#else
		msg_print("There is no effect!");
#endif

	}
	else if (ironman_downward)
	{
		/* Town/wilderness or Ironman */
		cave_set_feat(py, px, FEAT_MORE);
	}
	else if (quest_number(dun_level) || (dun_level >= d_info[dungeon_type].maxdepth))
	{
		/* Quest level */
		cave_set_feat(py, px, FEAT_LESS);
	}
	else if (randint0(100) < 50)
	{
		cave_set_feat(py, px, FEAT_MORE);
	}
	else
	{
		cave_set_feat(py, px, FEAT_LESS);
	}
}


/*
 * Hook to specify "weapon"
 */
bool item_tester_hook_weapon(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_DIGGING:
		case TV_BOW:
		case TV_BOLT:
		case TV_ARROW:
		case TV_SHOT:
		{
			return (TRUE);
		}
		case TV_SWORD:
		{
			if (o_ptr->sval != SV_DOKUBARI) return (TRUE);
		}
	}

	return (FALSE);
}

static bool item_tester_hook_weapon2(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_SWORD:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_DIGGING:
		case TV_BOW:
		case TV_BOLT:
		case TV_ARROW:
		case TV_SHOT:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Hook to specify "armour"
 */
bool item_tester_hook_armour(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_DRAG_ARMOR:
		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_CROWN:
		case TV_HELM:
		case TV_BOOTS:
		case TV_GLOVES:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


static bool item_tester_hook_corpse(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_CORPSE:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Check if an object is weapon or armour (but not arrow, bolt, or shot)
 */
bool item_tester_hook_weapon_armour(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_SWORD:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_DIGGING:
		case TV_BOW:
		case TV_BOLT:
		case TV_ARROW:
		case TV_SHOT:
		case TV_DRAG_ARMOR:
		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_CROWN:
		case TV_HELM:
		case TV_BOOTS:
		case TV_GLOVES:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Check if an object is nameless weapon or armour
 */
static bool item_tester_hook_nameless_weapon_armour(object_type *o_ptr)
{
	if (o_ptr->name1 || o_ptr->art_name || o_ptr->name2 || o_ptr->xtra3)
		return FALSE;

	switch (o_ptr->tval)
	{
		case TV_SWORD:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_DIGGING:
		case TV_BOW:
		case TV_BOLT:
		case TV_ARROW:
		case TV_SHOT:
		case TV_DRAG_ARMOR:
		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_CROWN:
		case TV_HELM:
		case TV_BOOTS:
		case TV_GLOVES:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Break the curse of an item
 */
static void break_curse(object_type *o_ptr)
{
	if (cursed_p(o_ptr) && !(o_ptr->curse_flags & TRC_PERMA_CURSE) && !(o_ptr->curse_flags & TRC_HEAVY_CURSE) && (randint0(100) < 25))
	{
#ifdef JP
msg_print("かけられていた呪いが打ち破られた！");
#else
		msg_print("The curse is broken!");
#endif

		o_ptr->curse_flags = 0L;

		o_ptr->ident |= (IDENT_SENSE);

		o_ptr->feeling = FEEL_NONE;
	}
}


/*
 * Enchants a plus onto an item. -RAK-
 *
 * Revamped!  Now takes item pointer, number of times to try enchanting,
 * and a flag of what to try enchanting.  Artifacts resist enchantment
 * some of the time, and successful enchantment to at least +0 might
 * break a curse on the item. -CFT-
 *
 * Note that an item can technically be enchanted all the way to +15 if
 * you wait a very, very, long time.  Going from +9 to +10 only works
 * about 5% of the time, and from +10 to +11 only about 1% of the time.
 *
 * Note that this function can now be used on "piles" of items, and
 * the larger the pile, the lower the chance of success.
 */
bool enchant(object_type *o_ptr, int n, int eflag)
{
	int     i, chance, prob;
	bool    res = FALSE;
	bool    a = (artifact_p(o_ptr) || o_ptr->art_name);
	bool    force = (eflag & ENCH_FORCE);


	/* Large piles resist enchantment */
	prob = o_ptr->number * 100;

	/* Missiles are easy to enchant */
	if ((o_ptr->tval == TV_BOLT) ||
	    (o_ptr->tval == TV_ARROW) ||
	    (o_ptr->tval == TV_SHOT))
	{
		prob = prob / 20;
	}

	/* Try "n" times */
	for (i = 0; i < n; i++)
	{
		/* Hack -- Roll for pile resistance */
		if (!force && randint0(prob) >= 100) continue;

		/* Enchant to hit */
		if (eflag & ENCH_TOHIT)
		{
			if (o_ptr->to_h < 0) chance = 0;
			else if (o_ptr->to_h > 15) chance = 1000;
			else chance = enchant_table[o_ptr->to_h];

			if (force || ((randint1(1000) > chance) && (!a || (randint0(100) < 50))))
			{
				o_ptr->to_h++;
				res = TRUE;

				/* only when you get it above -1 -CFT */
				if (o_ptr->to_h >= 0)
					break_curse(o_ptr);
			}
		}

		/* Enchant to damage */
		if (eflag & ENCH_TODAM)
		{
			if (o_ptr->to_d < 0) chance = 0;
			else if (o_ptr->to_d > 15) chance = 1000;
			else chance = enchant_table[o_ptr->to_d];

			if (force || ((randint1(1000) > chance) && (!a || (randint0(100) < 50))))
			{
				o_ptr->to_d++;
				res = TRUE;

				/* only when you get it above -1 -CFT */
				if (o_ptr->to_d >= 0)
					break_curse(o_ptr);
			}
		}

		/* Enchant to armor class */
		if (eflag & ENCH_TOAC)
		{
			if (o_ptr->to_a < 0) chance = 0;
			else if (o_ptr->to_a > 15) chance = 1000;
			else chance = enchant_table[o_ptr->to_a];

			if (force || ((randint1(1000) > chance) && (!a || (randint0(100) < 50))))
			{
				o_ptr->to_a++;
				res = TRUE;

				/* only when you get it above -1 -CFT */
				if (o_ptr->to_a >= 0)
					break_curse(o_ptr);
			}
		}
	}

	/* Failure */
	if (!res) return (FALSE);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	calc_android_exp();

	/* Success */
	return (TRUE);
}



/*
 * Enchant an item (in the inventory or on the floor)
 * Note that "num_ac" requires armour, else weapon
 * Returns TRUE if attempted, FALSE if cancelled
 */
bool enchant_spell(int num_hit, int num_dam, int num_ac)
{
	int         item;
	bool        okay = FALSE;
	object_type *o_ptr;
	char        o_name[MAX_NLEN];
	cptr        q, s;


	/* Assume enchant weapon */
	item_tester_hook = item_tester_hook_weapon;
	item_tester_no_ryoute = TRUE;

	/* Enchant armor if requested */
	if (num_ac) item_tester_hook = item_tester_hook_armour;

	/* Get an item */
#ifdef JP
q = "どのアイテムを強化しますか? ";
s = "強化できるアイテムがない。";
#else
	q = "Enchant which item? ";
	s = "You have nothing to enchant.";
#endif

	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}


	/* Description */
	object_desc(o_name, o_ptr, FALSE, 0);

	/* Describe */
#ifdef JP
msg_format("%s は明るく輝いた！",
    o_name);
#else
	msg_format("%s %s glow%s brightly!",
		   ((item >= 0) ? "Your" : "The"), o_name,
		   ((o_ptr->number > 1) ? "" : "s"));
#endif


	/* Enchant */
	if (enchant(o_ptr, num_hit, ENCH_TOHIT)) okay = TRUE;
	if (enchant(o_ptr, num_dam, ENCH_TODAM)) okay = TRUE;
	if (enchant(o_ptr, num_ac, ENCH_TOAC)) okay = TRUE;

	/* Failure */
	if (!okay)
	{
		/* Flush */
		if (flush_failure) flush();

		/* Message */
#ifdef JP
msg_print("強化に失敗した。");
#else
		msg_print("The enchantment failed.");
#endif

		if (one_in_(3)) chg_virtue(V_ENCHANT, -1);
	}
	else
		chg_virtue(V_ENCHANT, 1);

	calc_android_exp();

	/* Something happened */
	return (TRUE);
}


bool artifact_scroll(void)
{
	int             item;
	bool            okay = FALSE;
	object_type     *o_ptr;
	char            o_name[MAX_NLEN];
	cptr            q, s;


	item_tester_no_ryoute = TRUE;
	/* Enchant weapon/armour */
	item_tester_hook = item_tester_hook_nameless_weapon_armour;

	/* Get an item */
#ifdef JP
q = "どのアイテムを強化しますか? ";
s = "強化できるアイテムがない。";
#else
	q = "Enchant which item? ";
	s = "You have nothing to enchant.";
#endif

	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}


	/* Description */
	object_desc(o_name, o_ptr, FALSE, 0);

	/* Describe */
#ifdef JP
msg_format("%s は眩い光を発した！",o_name);
#else
	msg_format("%s %s radiate%s a blinding light!",
	          ((item >= 0) ? "Your" : "The"), o_name,
	          ((o_ptr->number > 1) ? "" : "s"));
#endif

	if (o_ptr->name1 || o_ptr->art_name)
	{
#ifdef JP
msg_format("%sは既に伝説のアイテムです！",
    o_name  );
#else
		msg_format("The %s %s already %s!",
		    o_name, ((o_ptr->number > 1) ? "are" : "is"),
		    ((o_ptr->number > 1) ? "artifacts" : "an artifact"));
#endif

		okay = FALSE;
	}

	else if (o_ptr->name2)
	{
#ifdef JP
msg_format("%sは既に名のあるアイテムです！",
    o_name );
#else
		msg_format("The %s %s already %s!",
		    o_name, ((o_ptr->number > 1) ? "are" : "is"),
		    ((o_ptr->number > 1) ? "ego items" : "an ego item"));
#endif

		okay = FALSE;
	}

	else if (o_ptr->xtra3)
	{
#ifdef JP
msg_format("%sは既に強化されています！",
    o_name );
#else
		msg_format("The %s %s already %s!",
		    o_name, ((o_ptr->number > 1) ? "are" : "is"),
		    ((o_ptr->number > 1) ? "kaji items" : "an kaji item"));
#endif
	}

	else
	{
		if (o_ptr->number > 1)
		{
#ifdef JP
msg_print("複数のアイテムに魔法をかけるだけのエネルギーはありません！");
msg_format("%d 個の%sが壊れた！",(o_ptr->number)-1, o_name);
#else
			msg_print("Not enough enough energy to enchant more than one object!");
			msg_format("%d of your %s %s destroyed!",(o_ptr->number)-1, o_name, (o_ptr->number>2?"were":"was"));
#endif

			if (item >= 0)
			{
				inven_item_increase(item, 1-(o_ptr->number));
			}
			else
			{
				floor_item_increase(0-item, 1-(o_ptr->number));
			}
		}
		okay = create_artifact(o_ptr, TRUE);
	}

	/* Failure */
	if (!okay)
	{
		/* Flush */
		if (flush_failure) flush();

		/* Message */
#ifdef JP
msg_print("強化に失敗した。");
#else
		msg_print("The enchantment failed.");
#endif

		if (one_in_(3)) chg_virtue(V_ENCHANT, -1);
	}
	else
		chg_virtue(V_ENCHANT, 1);

	calc_android_exp();

	/* Something happened */
	return (TRUE);
}


/*
 * Identify an object
 */
bool identify_item(object_type *o_ptr)
{
	bool old_known = FALSE;
	char o_name[MAX_NLEN];

	/* Description */
	object_desc(o_name, o_ptr, TRUE, 3);

	if (o_ptr->ident & IDENT_KNOWN)
		old_known = TRUE;

	if (!(o_ptr->ident & (IDENT_MENTAL)))
	{
		if ((o_ptr->art_name) || (artifact_p(o_ptr)) || one_in_(5))
			chg_virtue(V_KNOWLEDGE, 1);
	}

	/* Identify it fully */
	object_aware(o_ptr);
	object_known(o_ptr);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	strcpy(record_o_name, o_name);
	record_turn = turn;

	/* Description */
	object_desc(o_name, o_ptr, TRUE, 0);

	if(record_fix_art && !old_known && artifact_p(o_ptr))
		do_cmd_write_nikki(NIKKI_ART, 0, o_name);
	if(record_rand_art && !old_known && o_ptr->art_name)
		do_cmd_write_nikki(NIKKI_ART, 0, o_name);

	return old_known;
}


static bool item_tester_hook_identify(object_type *o_ptr)
{
	return (bool)!object_known_p(o_ptr);
}

static bool item_tester_hook_identify_weapon_armour(object_type *o_ptr)
{
	if (object_known_p(o_ptr))
		return FALSE;
	return item_tester_hook_weapon_armour(o_ptr);
}

/*
 * Identify an object in the inventory (or on the floor)
 * This routine does *not* automatically combine objects.
 * Returns TRUE if something was identified, else FALSE.
 */
bool ident_spell(bool only_equip, bool wait_optimize)
{
	int             item;
	object_type     *o_ptr;
	char            o_name[MAX_NLEN];
	cptr            q, s;
	bool old_known;
	int idx;

	item_tester_no_ryoute = TRUE;

	if (only_equip)
		item_tester_hook = item_tester_hook_identify_weapon_armour;
	else
		item_tester_hook = item_tester_hook_identify;

	if (!can_get_item())
	{
		if (only_equip)
		{
			item_tester_hook = item_tester_hook_weapon_armour;
		}
		else
		{
			item_tester_hook = NULL;
		}
	}

	/* Get an item */
#ifdef JP
q = "どのアイテムを鑑定しますか? ";
s = "鑑定するべきアイテムがない。";
#else
	q = "Identify which item? ";
	s = "You have nothing to identify.";
#endif

	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	/* Identify it */
	old_known = identify_item(o_ptr);

	/* Description */
	object_desc(o_name, o_ptr, TRUE, 3);

	/* Describe */
	if (item >= INVEN_RARM)
	{
#ifdef JP
		msg_format("%^s: %s(%c)。", describe_use(item), o_name, index_to_label(item));
#else
		msg_format("%^s: %s (%c).", describe_use(item), o_name, index_to_label(item));
#endif
	}
	else if (item >= 0)
	{
#ifdef JP
		msg_format("ザック中: %s(%c)。", o_name, index_to_label(item));
#else
		msg_format("In your pack: %s (%c).", o_name, index_to_label(item));
#endif
	}
	else
	{
#ifdef JP
		msg_format("床上: %s。", o_name);
#else
		msg_format("On the ground: %s.", o_name);
#endif
	}

	/* Auto-inscription/destroy */
	idx = is_autopick(o_ptr);
	auto_inscribe_item(item, idx);
	if (!old_known) auto_destroy_item(item, idx, wait_optimize);

	/* Something happened */
	return (TRUE);
}


/*
 * Mundanify an object in the inventory (or on the floor)
 * This routine does *not* automatically combine objects.
 * Returns TRUE if something was mundanified, else FALSE.
 */
bool mundane_spell(bool only_equip)
{
	int             item;
	object_type     *o_ptr;
	cptr            q, s;

	if (only_equip) item_tester_hook = item_tester_hook_weapon_armour;
	item_tester_no_ryoute = TRUE;

	/* Get an item */
#ifdef JP
q = "どれを使いますか？";
s = "使えるものがありません。";
#else
	q = "Use which item? ";
	s = "You have nothing you can use.";
#endif

	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	/* Oops */
#ifdef JP
msg_print("まばゆい閃光が走った！");
#else
	msg_print("There is a bright flash of light!");
#endif
{
  byte iy = o_ptr->iy;                /* Y-position on map, or zero */
  byte ix = o_ptr->ix;                /* X-position on map, or zero */
  s16b next_o_idx= o_ptr->next_o_idx; /* Next object in stack (if any) */
  byte marked=o_ptr->marked;          /* Object is marked */
  s16b weight = (o_ptr->number*o_ptr->weight);

   /* Wipe it clean */
   object_prep(o_ptr, o_ptr->k_idx);

  o_ptr->iy=iy;
  o_ptr->ix=ix;
  o_ptr->next_o_idx=next_o_idx;
  o_ptr->marked=marked;
  if (item >= 0) p_ptr->total_weight += (o_ptr->weight - weight);
}
	calc_android_exp();

	/* Something happened */
	return (TRUE);
}



static bool item_tester_hook_identify_fully(object_type *o_ptr)
{
	return (bool)(!object_known_p(o_ptr) || !(o_ptr->ident & IDENT_MENTAL));
}

static bool item_tester_hook_identify_fully_weapon_armour(object_type *o_ptr)
{
	if (!item_tester_hook_identify_fully(o_ptr))
		return FALSE;
	return item_tester_hook_weapon_armour(o_ptr);
}

/*
 * Fully "identify" an object in the inventory  -BEN-
 * This routine returns TRUE if an item was identified.
 */
bool identify_fully(bool only_equip, bool wait_optimize)
{
	int             item;
	object_type     *o_ptr;
	char            o_name[MAX_NLEN];
	cptr            q, s;
	bool old_known;
	int idx;

	item_tester_no_ryoute = TRUE;
	if (only_equip)
		item_tester_hook = item_tester_hook_identify_fully_weapon_armour;
	else
		item_tester_hook = item_tester_hook_identify_fully;

	if (!can_get_item())
	{
		if (only_equip)
			item_tester_hook = item_tester_hook_weapon_armour;
		else
			item_tester_hook = NULL;
	}

	/* Get an item */
#ifdef JP
q = "どのアイテムを鑑定しますか? ";
s = "鑑定するべきアイテムがない。";
#else
	q = "Identify which item? ";
	s = "You have nothing to identify.";
#endif

	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	/* Identify it */
	old_known = identify_item(o_ptr);

	/* Mark the item as fully known */
	o_ptr->ident |= (IDENT_MENTAL);

	/* Handle stuff */
	handle_stuff();

	/* Description */
	object_desc(o_name, o_ptr, TRUE, 3);

	/* Describe */
	if (item >= INVEN_RARM)
	{
#ifdef JP
		msg_format("%^s: %s(%c)。", describe_use(item), o_name, index_to_label(item));
#else
		msg_format("%^s: %s (%c).", describe_use(item), o_name, index_to_label(item));
#endif


	}
	else if (item >= 0)
	{
#ifdef JP
		msg_format("ザック中: %s(%c)。", o_name, index_to_label(item));
#else
		msg_format("In your pack: %s (%c).", o_name, index_to_label(item));
#endif
	}
	else
	{
#ifdef JP
		msg_format("床上: %s。", o_name);
#else
		msg_format("On the ground: %s.", o_name);
#endif
	}

	/* Describe it fully */
	(void)identify_fully_aux(o_ptr);

	/* Auto-inscription/destroy */
	idx = is_autopick(o_ptr);
	auto_inscribe_item(item, idx);
	if (!old_known) auto_destroy_item(item, idx, wait_optimize);

	/* Success */
	return (TRUE);
}




/*
 * Hook for "get_item()".  Determine if something is rechargable.
 */
bool item_tester_hook_recharge(object_type *o_ptr)
{
	/* Recharge staffs */
	if (o_ptr->tval == TV_STAFF) return (TRUE);

	/* Recharge wands */
	if (o_ptr->tval == TV_WAND) return (TRUE);

	/* Hack -- Recharge rods */
	if (o_ptr->tval == TV_ROD) return (TRUE);

	/* Nope */
	return (FALSE);
}


/*
 * Recharge a wand/staff/rod from the pack or on the floor.
 * This function has been rewritten in Oangband and ZAngband.
 *
 * Sorcery/Arcane -- Recharge  --> recharge(plev * 4)
 * Chaos -- Arcane Binding     --> recharge(90)
 *
 * Scroll of recharging        --> recharge(130)
 * Artifact activation/Thingol --> recharge(130)
 *
 * It is harder to recharge high level, and highly charged wands,
 * staffs, and rods.  The more wands in a stack, the more easily and
 * strongly they recharge.  Staffs, however, each get fewer charges if
 * stacked.
 *
 * XXX XXX XXX Beware of "sliding index errors".
 */
bool recharge(int power)
{
	int item, lev;
	int recharge_strength, recharge_amount;

	object_type *o_ptr;
	object_kind *k_ptr;

	bool fail = FALSE;
	byte fail_type = 1;

	cptr q, s;
	char o_name[MAX_NLEN];

	/* Only accept legal items */
	item_tester_hook = item_tester_hook_recharge;

	/* Get an item */
#ifdef JP
q = "どのアイテムに魔力を充填しますか? ";
s = "魔力を充填すべきアイテムがない。";
#else
	q = "Recharge which item? ";
	s = "You have nothing to recharge.";
#endif

	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return (FALSE);

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	/* Get the object kind. */
	k_ptr = &k_info[o_ptr->k_idx];

	/* Extract the object "level" */
	lev = get_object_level(o_ptr);


	/* Recharge a rod */
	if (o_ptr->tval == TV_ROD)
	{
		/* Extract a recharge strength by comparing object level to power. */
		recharge_strength = ((power > lev/2) ? (power - lev/2) : 0) / 5;


		/* Back-fire */
		if (one_in_(recharge_strength))
		{
			/* Activate the failure code. */
			fail = TRUE;
		}

		/* Recharge */
		else
		{
			/* Recharge amount */
			recharge_amount = (power * damroll(3, 2));

			/* Recharge by that amount */
			if (o_ptr->timeout > recharge_amount)
				o_ptr->timeout -= recharge_amount;
			else
				o_ptr->timeout = 0;
		}
	}


	/* Recharge wand/staff */
	else
	{
		/* Extract a recharge strength by comparing object level to power.
		 * Divide up a stack of wands' charges to calculate charge penalty.
		 */
		if ((o_ptr->tval == TV_WAND) && (o_ptr->number > 1))
			recharge_strength = (100 + power - lev -
			(8 * o_ptr->pval / o_ptr->number)) / 15;

		/* All staffs, unstacked wands. */
		else recharge_strength = (100 + power - lev -
			(8 * o_ptr->pval)) / 15;

		/* Paranoia */
		if (recharge_strength < 0) recharge_strength = 0;

		/* Back-fire */
		if (one_in_(recharge_strength))
		{
			/* Activate the failure code. */
			fail = TRUE;
		}

		/* If the spell didn't backfire, recharge the wand or staff. */
		else
		{
			/* Recharge based on the standard number of charges. */
			recharge_amount = randint1(1 + k_ptr->pval / 2);

			/* Multiple wands in a stack increase recharging somewhat. */
			if ((o_ptr->tval == TV_WAND) && (o_ptr->number > 1))
			{
				recharge_amount +=
					(randint1(recharge_amount * (o_ptr->number - 1))) / 2;
				if (recharge_amount < 1) recharge_amount = 1;
				if (recharge_amount > 12) recharge_amount = 12;
			}

			/* But each staff in a stack gets fewer additional charges,
			 * although always at least one.
			 */
			if ((o_ptr->tval == TV_STAFF) && (o_ptr->number > 1))
			{
				recharge_amount /= o_ptr->number;
				if (recharge_amount < 1) recharge_amount = 1;
			}

			/* Recharge the wand or staff. */
			o_ptr->pval += recharge_amount;


			/* Hack -- we no longer "know" the item */
			o_ptr->ident &= ~(IDENT_KNOWN);

			/* Hack -- we no longer think the item is empty */
			o_ptr->ident &= ~(IDENT_EMPTY);
		}
	}


	/* Inflict the penalties for failing a recharge. */
	if (fail)
	{
		/* Artifacts are never destroyed. */
		if (artifact_p(o_ptr))
		{
			object_desc(o_name, o_ptr, TRUE, 0);
#ifdef JP
msg_format("魔力が逆流した！%sは完全に魔力を失った。", o_name);
#else
			msg_format("The recharging backfires - %s is completely drained!", o_name);
#endif


			/* Artifact rods. */
			if ((o_ptr->tval == TV_ROD) && (o_ptr->timeout < 10000))
				o_ptr->timeout = (o_ptr->timeout + 100) * 2;

			/* Artifact wands and staffs. */
			else if ((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_STAFF))
				o_ptr->pval = 0;
		}
		else
		{
			/* Get the object description */
			object_desc(o_name, o_ptr, FALSE, 0);

			/*** Determine Seriousness of Failure ***/

			/* Mages recharge objects more safely. */
			if (p_ptr->pclass == CLASS_MAGE || p_ptr->pclass == CLASS_HIGH_MAGE || p_ptr->pclass == CLASS_SORCERER || p_ptr->pclass == CLASS_MAGIC_EATER || p_ptr->pclass == CLASS_BLUE_MAGE)
			{
				/* 10% chance to blow up one rod, otherwise draining. */
				if (o_ptr->tval == TV_ROD)
				{
					if (one_in_(10)) fail_type = 2;
					else fail_type = 1;
				}
				/* 75% chance to blow up one wand, otherwise draining. */
				else if (o_ptr->tval == TV_WAND)
				{
					if (!one_in_(3)) fail_type = 2;
					else fail_type = 1;
				}
				/* 50% chance to blow up one staff, otherwise no effect. */
				else if (o_ptr->tval == TV_STAFF)
				{
					if (one_in_(2)) fail_type = 2;
					else fail_type = 0;
				}
			}

			/* All other classes get no special favors. */
			else
			{
				/* 33% chance to blow up one rod, otherwise draining. */
				if (o_ptr->tval == TV_ROD)
				{
					if (one_in_(3)) fail_type = 2;
					else fail_type = 1;
				}
				/* 20% chance of the entire stack, else destroy one wand. */
				else if (o_ptr->tval == TV_WAND)
				{
					if (one_in_(5)) fail_type = 3;
					else fail_type = 2;
				}
				/* Blow up one staff. */
				else if (o_ptr->tval == TV_STAFF)
				{
					fail_type = 2;
				}
			}

			/*** Apply draining and destruction. ***/

			/* Drain object or stack of objects. */
			if (fail_type == 1)
			{
				if (o_ptr->tval == TV_ROD)
				{
#ifdef JP
msg_print("魔力が逆噴射して、ロッドからさらに魔力を吸い取ってしまった！");
#else
					msg_print("The recharge backfires, draining the rod further!");
#endif

					if (o_ptr->timeout < 10000)
						o_ptr->timeout = (o_ptr->timeout + 100) * 2;
				}
				else if (o_ptr->tval == TV_WAND)
				{
#ifdef JP
msg_format("%sは破損を免れたが、魔力が全て失われた。", o_name);
#else
					msg_format("You save your %s from destruction, but all charges are lost.", o_name);
#endif

					o_ptr->pval = 0;
				}
				/* Staffs aren't drained. */
			}

			/* Destroy an object or one in a stack of objects. */
			if (fail_type == 2)
			{
				if (o_ptr->number > 1)
#ifdef JP
msg_format("乱暴な魔法のために%sが一本壊れた！", o_name);
#else
					msg_format("Wild magic consumes one of your %s!", o_name);
#endif

				else
#ifdef JP
msg_format("乱暴な魔法のために%sが壊れた！", o_name);
#else
					msg_format("Wild magic consumes your %s!", o_name);
#endif


				/* Reduce rod stack maximum timeout, drain wands. */
				if (o_ptr->tval == TV_ROD) o_ptr->timeout = (o_ptr->number - 1) * k_ptr->pval;
				if (o_ptr->tval == TV_WAND) o_ptr->pval = 0;

				/* Reduce and describe inventory */
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
					floor_item_describe(0 - item);
					floor_item_optimize(0 - item);
				}
			}

			/* Destroy all members of a stack of objects. */
			if (fail_type == 3)
			{
				if (o_ptr->number > 1)
#ifdef JP
msg_format("乱暴な魔法のために%sが全て壊れた！", o_name);
#else
					msg_format("Wild magic consumes all your %s!", o_name);
#endif

				else
#ifdef JP
msg_format("乱暴な魔法のために%sが壊れた！", o_name);
#else
					msg_format("Wild magic consumes your %s!", o_name);
#endif



				/* Reduce and describe inventory */
				if (item >= 0)
				{
					inven_item_increase(item, -999);
					inven_item_describe(item);
					inven_item_optimize(item);
				}

				/* Reduce and describe floor item */
				else
				{
					floor_item_increase(0 - item, -999);
					floor_item_describe(0 - item);
					floor_item_optimize(0 - item);
				}
			}
		}
	}

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN);

	/* Something was done */
	return (TRUE);
}


/*
 * Bless a weapon
 */
bool bless_weapon(void)
{
	int             item;
	object_type     *o_ptr;
	u32b            f1, f2, f3;
	char            o_name[MAX_NLEN];
	cptr            q, s;

	item_tester_no_ryoute = TRUE;
	/* Assume enchant weapon */
	item_tester_hook = item_tester_hook_weapon2;

	/* Get an item */
#ifdef JP
q = "どのアイテムを祝福しますか？";
s = "祝福できる武器がありません。";
#else
	q = "Bless which weapon? ";
	s = "You have weapon to bless.";
#endif

	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR)))
		return FALSE;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}


	/* Description */
	object_desc(o_name, o_ptr, FALSE, 0);

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3);

	if (cursed_p(o_ptr))
	{
		if (((o_ptr->curse_flags & TRC_HEAVY_CURSE) && (randint1(100) < 33)) ||
		    (o_ptr->curse_flags & TRC_PERMA_CURSE))
		{
#ifdef JP
msg_format("%sを覆う黒いオーラは祝福を跳ね返した！",
    o_name);
#else
			msg_format("The black aura on %s %s disrupts the blessing!",
			    ((item >= 0) ? "your" : "the"), o_name);
#endif

			return TRUE;
		}

#ifdef JP
msg_format("%s から邪悪なオーラが消えた。",
    o_name);
#else
		msg_format("A malignant aura leaves %s %s.",
		    ((item >= 0) ? "your" : "the"), o_name);
#endif


		/* Uncurse it */
		o_ptr->curse_flags = 0L;

		/* Hack -- Assume felt */
		o_ptr->ident |= (IDENT_SENSE);

		/* Take note */
		o_ptr->feeling = FEEL_NONE;

		/* Recalculate the bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Window stuff */
		p_ptr->window |= (PW_EQUIP);
	}

	/*
	 * Next, we try to bless it. Artifacts have a 1/3 chance of
	 * being blessed, otherwise, the operation simply disenchants
	 * them, godly power negating the magic. Ok, the explanation
	 * is silly, but otherwise priests would always bless every
	 * artifact weapon they find. Ego weapons and normal weapons
	 * can be blessed automatically.
	 */
	if (f3 & TR3_BLESSED)
	{
#ifdef JP
msg_format("%s は既に祝福されている。",
    o_name    );
#else
		msg_format("%s %s %s blessed already.",
		    ((item >= 0) ? "Your" : "The"), o_name,
		    ((o_ptr->number > 1) ? "were" : "was"));
#endif

		return TRUE;
	}

	if (!(o_ptr->art_name || o_ptr->name1 || o_ptr->name2) || one_in_(3))
	{
		/* Describe */
#ifdef JP
msg_format("%sは輝いた！",
     o_name);
#else
		msg_format("%s %s shine%s!",
		    ((item >= 0) ? "Your" : "The"), o_name,
		    ((o_ptr->number > 1) ? "" : "s"));
#endif

		o_ptr->art_flags3 |= TR3_BLESSED;
		o_ptr->discount = 99;
	}
	else
	{
		bool dis_happened = FALSE;

#ifdef JP
msg_print("その武器は祝福を嫌っている！");
#else
		msg_print("The weapon resists your blessing!");
#endif


		/* Disenchant tohit */
		if (o_ptr->to_h > 0)
		{
			o_ptr->to_h--;
			dis_happened = TRUE;
		}

		if ((o_ptr->to_h > 5) && (randint0(100) < 33)) o_ptr->to_h--;

		/* Disenchant todam */
		if (o_ptr->to_d > 0)
		{
			o_ptr->to_d--;
			dis_happened = TRUE;
		}

		if ((o_ptr->to_d > 5) && (randint0(100) < 33)) o_ptr->to_d--;

		/* Disenchant toac */
		if (o_ptr->to_a > 0)
		{
			o_ptr->to_a--;
			dis_happened = TRUE;
		}

		if ((o_ptr->to_a > 5) && (randint0(100) < 33)) o_ptr->to_a--;

		if (dis_happened)
		{
#ifdef JP
msg_print("周囲が凡庸な雰囲気で満ちた...");
#else
			msg_print("There is a static feeling in the air...");
#endif

#ifdef JP
msg_format("%s は劣化した！",
     o_name    );
#else
			msg_format("%s %s %s disenchanted!",
			    ((item >= 0) ? "Your" : "The"), o_name,
			    ((o_ptr->number > 1) ? "were" : "was"));
#endif

		}
	}

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->window |= (PW_EQUIP | PW_PLAYER);

	calc_android_exp();

	return TRUE;
}


/*
 * pulish shield
 */
bool pulish_shield(void)
{
	int             item;
	object_type     *o_ptr;
	u32b            f1, f2, f3;
	char            o_name[MAX_NLEN];
	cptr            q, s;

	item_tester_no_ryoute = TRUE;
	/* Assume enchant weapon */
	item_tester_tval = TV_SHIELD;

	/* Get an item */
#ifdef JP
q = "どの盾を磨きますか？";
s = "磨く盾がありません。";
#else
	q = "Pulish which weapon? ";
	s = "You have weapon to pulish.";
#endif

	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR)))
		return FALSE;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}


	/* Description */
	object_desc(o_name, o_ptr, FALSE, 0);

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3);

	if (o_ptr->k_idx && !artifact_p(o_ptr) && !ego_item_p(o_ptr) &&
	    !o_ptr->art_name && !cursed_p(o_ptr) && (o_ptr->sval != SV_SHIELD_OF_DEFLECTION))
	{
#ifdef JP
msg_format("%sは輝いた！", o_name);
#else
		msg_format("%s %s shine%s!",
		    ((item >= 0) ? "Your" : "The"), o_name,
		    ((o_ptr->number > 1) ? "" : "s"));
#endif
		o_ptr->name2 = EGO_REFLECTION;
		enchant(o_ptr, randint0(3) + 4, ENCH_TOAC);

		o_ptr->discount = 99;
		chg_virtue(V_ENCHANT, 2);

		return TRUE;
	}
	else
	{
		if (flush_failure) flush();

#ifdef JP
msg_print("失敗した。");
#else
		msg_print("Failed.");
#endif

		chg_virtue(V_ENCHANT, -2);
	}
	calc_android_exp();

	return FALSE;
}


/*
 * Potions "smash open" and cause an area effect when
 * (1) they are shattered while in the player's inventory,
 * due to cold (etc) attacks;
 * (2) they are thrown at a monster, or obstacle;
 * (3) they are shattered by a "cold ball" or other such spell
 * while lying on the floor.
 *
 * Arguments:
 *    who   ---  who caused the potion to shatter (0=player)
 *          potions that smash on the floor are assumed to
 *          be caused by no-one (who = 1), as are those that
 *          shatter inside the player inventory.
 *          (Not anymore -- I changed this; TY)
 *    y, x  --- coordinates of the potion (or player if
 *          the potion was in her inventory);
 *    o_ptr --- pointer to the potion object.
 */
bool potion_smash_effect(int who, int y, int x, int k_idx)
{
	int     radius = 2;
	int     dt = 0;
	int     dam = 0;
	bool    ident = FALSE;
	bool    angry = FALSE;

	object_kind *k_ptr = &k_info[k_idx];

	switch (k_ptr->sval)
	{
		case SV_POTION_SALT_WATER:
		case SV_POTION_SLIME_MOLD:
		case SV_POTION_LOSE_MEMORIES:
		case SV_POTION_DEC_STR:
		case SV_POTION_DEC_INT:
		case SV_POTION_DEC_WIS:
		case SV_POTION_DEC_DEX:
		case SV_POTION_DEC_CON:
		case SV_POTION_DEC_CHR:
		case SV_POTION_WATER:   /* perhaps a 'water' attack? */
		case SV_POTION_APPLE_JUICE:
			return TRUE;

		case SV_POTION_INFRAVISION:
		case SV_POTION_DETECT_INVIS:
		case SV_POTION_SLOW_POISON:
		case SV_POTION_CURE_POISON:
		case SV_POTION_BOLDNESS:
		case SV_POTION_RESIST_HEAT:
		case SV_POTION_RESIST_COLD:
		case SV_POTION_HEROISM:
		case SV_POTION_BESERK_STRENGTH:
		case SV_POTION_RES_STR:
		case SV_POTION_RES_INT:
		case SV_POTION_RES_WIS:
		case SV_POTION_RES_DEX:
		case SV_POTION_RES_CON:
		case SV_POTION_RES_CHR:
		case SV_POTION_INC_STR:
		case SV_POTION_INC_INT:
		case SV_POTION_INC_WIS:
		case SV_POTION_INC_DEX:
		case SV_POTION_INC_CON:
		case SV_POTION_INC_CHR:
		case SV_POTION_AUGMENTATION:
		case SV_POTION_ENLIGHTENMENT:
		case SV_POTION_STAR_ENLIGHTENMENT:
		case SV_POTION_SELF_KNOWLEDGE:
		case SV_POTION_EXPERIENCE:
		case SV_POTION_RESISTANCE:
		case SV_POTION_INVULNERABILITY:
		case SV_POTION_NEW_LIFE:
			/* All of the above potions have no effect when shattered */
			return FALSE;
		case SV_POTION_SLOWNESS:
			dt = GF_OLD_SLOW;
			dam = 5;
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_POISON:
			dt = GF_POIS;
			dam = 3;
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_BLINDNESS:
			dt = GF_DARK;
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_CONFUSION: /* Booze */
			dt = GF_OLD_CONF;
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_SLEEP:
			dt = GF_OLD_SLEEP;
			angry = TRUE;
			ident = TRUE;
			break;
		case SV_POTION_RUINATION:
		case SV_POTION_DETONATIONS:
			dt = GF_SHARDS;
			dam = damroll(25, 25);
			angry = TRUE;
			ident = TRUE;
			break;
		case SV_POTION_DEATH:
			dt = GF_DEATH_RAY;    /* !! */
			dam = k_ptr->level * 10;
			angry = TRUE;
			radius = 1;
			ident = TRUE;
			break;
		case SV_POTION_SPEED:
			dt = GF_OLD_SPEED;
			ident = TRUE;
			break;
		case SV_POTION_CURE_LIGHT:
			dt = GF_OLD_HEAL;
			dam = damroll(2, 3);
			ident = TRUE;
			break;
		case SV_POTION_CURE_SERIOUS:
			dt = GF_OLD_HEAL;
			dam = damroll(4, 3);
			ident = TRUE;
			break;
		case SV_POTION_CURE_CRITICAL:
		case SV_POTION_CURING:
			dt = GF_OLD_HEAL;
			dam = damroll(6, 3);
			ident = TRUE;
			break;
		case SV_POTION_HEALING:
			dt = GF_OLD_HEAL;
			dam = damroll(10, 10);
			ident = TRUE;
			break;
		case SV_POTION_RESTORE_EXP:
			dt = GF_STAR_HEAL;
			dam = 0;
			radius = 1;
			ident = TRUE;
			break;
		case SV_POTION_LIFE:
			dt = GF_STAR_HEAL;
			dam = damroll(50, 50);
			radius = 1;
			ident = TRUE;
			break;
		case SV_POTION_STAR_HEALING:
			dt = GF_OLD_HEAL;
			dam = damroll(50, 50);
			radius = 1;
			ident = TRUE;
			break;
		case SV_POTION_RESTORE_MANA:   /* MANA */
			dt = GF_MANA;
			dam = damroll(10, 10);
			radius = 1;
			ident = TRUE;
			break;
		default:
			/* Do nothing */  ;
	}

	(void)project(who, radius, y, x, dam, dt,
	    (PROJECT_JUMP | PROJECT_ITEM | PROJECT_KILL), -1);

	/* XXX  those potions that explode need to become "known" */
	return angry;
}


/*
 * Hack -- Display all known spells in a window
 *
 * XXX XXX XXX Need to analyze size of the window.
 *
 * XXX XXX XXX Need more color coding.
 */
void display_spell_list(void)
{
	int             i, j;
	int             y, x;
	int             m[9];
	magic_type      *s_ptr;
	char            name[80];
	char            out_val[160];


	/* Erase window */
	clear_from(0);

	/* They have too many spells to list */
	if (p_ptr->pclass == CLASS_SORCERER) return;
	if (p_ptr->pclass == CLASS_RED_MAGE) return;

	/* mind.c type classes */
	if ((p_ptr->pclass == CLASS_MINDCRAFTER) ||
	    (p_ptr->pclass == CLASS_BERSERKER) ||
	    (p_ptr->pclass == CLASS_NINJA) ||
	    (p_ptr->pclass == CLASS_MIRROR_MASTER) ||
	    (p_ptr->pclass == CLASS_FORCETRAINER))
	{
		int             i;
		int             y = 1;
		int             x = 1;
		int             minfail = 0;
		int             plev = p_ptr->lev;
		int             chance = 0;
		mind_type       spell;
		char            comment[80];
		char            psi_desc[80];
		int             use_mind;
		bool use_hp = FALSE;

		/* Display a list of spells */
		prt("", y, x);
#ifdef JP
put_str("名前", y, x + 5);
put_str("Lv   MP 失率 効果", y, x + 35);
#else
		put_str("Name", y, x + 5);
		put_str("Lv Mana Fail Info", y, x + 35);
#endif

		switch(p_ptr->pclass)
		{
		case CLASS_MINDCRAFTER: use_mind = MIND_MINDCRAFTER;break;
		case CLASS_FORCETRAINER:          use_mind = MIND_KI;break;
		case CLASS_BERSERKER: use_mind = MIND_BERSERKER; use_hp = TRUE; break;
		case CLASS_MIRROR_MASTER: use_mind = MIND_MIRROR_MASTER; break;
		case CLASS_NINJA: use_mind = MIND_NINJUTSU; use_hp = TRUE; break;
		default:                use_mind = 0;break;
		}

		/* Dump the spells */
		for (i = 0; i < MAX_MIND_POWERS; i++)
		{
			byte a = TERM_WHITE;

			/* Access the available spell */
			spell = mind_powers[use_mind].info[i];
			if (spell.min_lev > plev) break;

			/* Get the failure rate */
			chance = spell.fail;

			/* Reduce failure rate by "effective" level adjustment */
			chance -= 3 * (p_ptr->lev - spell.min_lev);

			/* Reduce failure rate by INT/WIS adjustment */
			chance -= 3 * (adj_mag_stat[p_ptr->stat_ind[mp_ptr->spell_stat]] - 1);

			if (!use_hp)
			{
				/* Not enough mana to cast */
				if (spell.mana_cost > p_ptr->csp)
				{
					chance += 5 * (spell.mana_cost - p_ptr->csp);
					a = TERM_ORANGE;
				}
			}
			else
			{
				/* Not enough hp to cast */
				if (spell.mana_cost > p_ptr->chp)
				{
					chance += 100;
					a = TERM_RED;
				}
			}

			/* Extract the minimum failure rate */
			minfail = adj_mag_fail[p_ptr->stat_ind[mp_ptr->spell_stat]];

			/* Minimum failure rate */
			if (chance < minfail) chance = minfail;

			/* Stunning makes spells harder */
			if (p_ptr->stun > 50) chance += 25;
			else if (p_ptr->stun) chance += 15;

			/* Always a 5 percent chance of working */
			if (chance > 95) chance = 95;

			/* Get info */
			mindcraft_info(comment, use_mind, i);

			/* Dump the spell */
			sprintf(psi_desc, "  %c) %-30s%2d %4d %3d%%%s",
			    I2A(i), spell.name,
			    spell.min_lev, spell.mana_cost, chance, comment);

			Term_putstr(x, y + i + 1, -1, a, psi_desc);
		}
		return;
	}

	/* Cannot read spellbooks */
	if (REALM_NONE == p_ptr->realm1) return;

	/* Normal spellcaster with books */

	/* Scan books */
	for (j = 0; j < ((p_ptr->realm2 > REALM_NONE) ? 2 : 1); j++)
	{
		int n = 0;

		/* Reset vertical */
		m[j] = 0;

		/* Vertical location */
		y = (j < 3) ? 0 : (m[j - 3] + 2);

		/* Horizontal location */
		x = 27 * (j % 3);

		/* Scan spells */
		for (i = 0; i < 32; i++)
		{
			byte a = TERM_WHITE;

			/* Access the spell */
			if (!is_magic((j < 1) ? p_ptr->realm1 : p_ptr->realm2))
			{
				s_ptr = &technic_info[((j < 1) ? p_ptr->realm1 : p_ptr->realm2) - MIN_TECHNIC][i % 32];
			}
			else
			{
				s_ptr = &mp_ptr->info[((j < 1) ? p_ptr->realm1 : p_ptr->realm2) - 1][i % 32];
			}

			strcpy(name, spell_names[technic2magic((j < 1) ? p_ptr->realm1 : p_ptr->realm2)-1][i % 32]);

			/* Illegible */
			if (s_ptr->slevel >= 99)
			{
				/* Illegible */
#ifdef JP
strcpy(name, "(判読不能)");
#else
				strcpy(name, "(illegible)");
#endif


				/* Unusable */
				a = TERM_L_DARK;
			}

			/* Forgotten */
			else if ((j < 1) ?
				((p_ptr->spell_forgotten1 & (1L << i))) :
				((p_ptr->spell_forgotten2 & (1L << (i % 32)))))
			{
				/* Forgotten */
				a = TERM_ORANGE;
			}

			/* Unknown */
			else if (!((j < 1) ?
				(p_ptr->spell_learned1 & (1L << i)) :
				(p_ptr->spell_learned2 & (1L << (i % 32)))))
			{
				/* Unknown */
				a = TERM_RED;
			}

			/* Untried */
			else if (!((j < 1) ?
				(p_ptr->spell_worked1 & (1L << i)) :
				(p_ptr->spell_worked2 & (1L << (i % 32)))))
			{
				/* Untried */
				a = TERM_YELLOW;
			}

			/* Dump the spell --(-- */
			sprintf(out_val, "%c/%c) %-20.20s",
				I2A(n / 8), I2A(n % 8), name);

			/* Track maximum */
			m[j] = y + n;

			/* Dump onto the window */
			Term_putstr(x, m[j], -1, a, out_val);

			/* Next */
			n++;
		}
	}
}


/*
 * Returns experience of a spell
 */
s16b experience_of_spell(int spell, int use_realm)
{
	if (p_ptr->pclass == CLASS_SORCERER) return 1600;
	else if (p_ptr->pclass == CLASS_RED_MAGE) return 1200;
	else if (use_realm == p_ptr->realm1) return p_ptr->spell_exp[spell];
	else if (use_realm == p_ptr->realm2) return p_ptr->spell_exp[spell + 32];
	else return 0;
}


/*
 * Returns spell chance of failure for spell -RAK-
 */
s16b spell_chance(int spell, int use_realm)
{
	int             chance, minfail;
	magic_type      *s_ptr;
	int             shouhimana;
	int penalty = (mp_ptr->spell_stat == A_WIS) ? 10 : 4;


	/* Paranoia -- must be literate */
	if (!mp_ptr->spell_book) return (100);

	if (use_realm == REALM_HISSATSU) return 0;

	/* Access the spell */
	if (!is_magic(use_realm))
	{
		s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
	}
	else
	{
		s_ptr = &mp_ptr->info[use_realm - 1][spell];
	}

	/* Extract the base spell failure rate */
	chance = s_ptr->sfail;

	/* Reduce failure rate by "effective" level adjustment */
	chance -= 3 * (p_ptr->lev - s_ptr->slevel);

	/* Reduce failure rate by INT/WIS adjustment */
	chance -= 3 * (adj_mag_stat[p_ptr->stat_ind[mp_ptr->spell_stat]] - 1);

	if (p_ptr->riding)
		chance += (MAX(r_info[m_list[p_ptr->riding].r_idx].level-p_ptr->skill_exp[GINOU_RIDING]/100-10,0));

	/* Extract mana consumption rate */
	shouhimana = s_ptr->smana*(3800 - experience_of_spell(spell, use_realm)) + 2399;

	if(p_ptr->dec_mana) shouhimana *= 3;
	else shouhimana *= 4;

	shouhimana /= 9600;
	if(shouhimana < 1) shouhimana = 1;

	/* Not enough mana to cast */
	if (shouhimana > p_ptr->csp)
	{
		chance += 5 * (shouhimana - p_ptr->csp);
	}

	chance += p_ptr->to_m_chance;
	if ((use_realm != p_ptr->realm1) && ((p_ptr->pclass == CLASS_MAGE) || (p_ptr->pclass == CLASS_PRIEST))) chance += 5;

	/* Extract the minimum failure rate */
	minfail = adj_mag_fail[p_ptr->stat_ind[mp_ptr->spell_stat]];

	/*
	 * Non mage/priest characters never get too good
	 * (added high mage, mindcrafter)
	 */
	if (mp_ptr->spell_xtra & MAGIC_FAIL_5PERCENT)
	{
		if (minfail < 5) minfail = 5;
	}

	/* Hack -- Priest prayer penalty for "edged" weapons  -DGK */
	if (((p_ptr->pclass == CLASS_PRIEST) || (p_ptr->pclass == CLASS_SORCERER)) && p_ptr->icky_wield[0]) chance += 25;
	if (((p_ptr->pclass == CLASS_PRIEST) || (p_ptr->pclass == CLASS_SORCERER)) && p_ptr->icky_wield[1]) chance += 25;

	if (p_ptr->heavy_spell) chance += 20;
	if(p_ptr->dec_mana && p_ptr->easy_spell) chance-=4;
	else if (p_ptr->easy_spell) chance-=3;
	else if (p_ptr->dec_mana) chance-=2;

	if ((use_realm == REALM_NATURE) && ((p_ptr->align > 50) || (p_ptr->align < -50))) chance += penalty;
	if (((use_realm == REALM_LIFE) || (use_realm == REALM_CRUSADE)) && (p_ptr->align < -20)) chance += penalty;
	if (((use_realm == REALM_DEATH) || (use_realm == REALM_DAEMON)) && (p_ptr->align > 20)) chance += penalty;

	/* Minimum failure rate */
	if (chance < minfail) chance = minfail;

	/* Stunning makes spells harder */
	if (p_ptr->stun > 50) chance += 25;
	else if (p_ptr->stun) chance += 15;

	/* Always a 5 percent chance of working */
	if (chance > 95) chance = 95;

	if ((use_realm == p_ptr->realm1) || (use_realm == p_ptr->realm2))
	{
		s16b exp = experience_of_spell(spell, use_realm);
		if(exp > 1399) chance--;
		if(exp > 1599) chance--;
	}
	if(p_ptr->dec_mana) chance--;
	if (p_ptr->heavy_spell) chance += 5;

	chance = MAX(chance,0);

	/* Return the chance */
	return (chance);
}



/*
 * Determine if a spell is "okay" for the player to cast or study
 * The spell must be legible, not forgotten, and also, to cast,
 * it must be known, and to study, it must not be known.
 */
bool spell_okay(int spell, bool learned, bool study_pray, int use_realm)
{
	magic_type *s_ptr;

	/* Access the spell */
	if (!is_magic(use_realm))
	{
		s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
	}
	else
	{
		s_ptr = &mp_ptr->info[use_realm - 1][spell];
	}

	/* Spell is illegal */
	if (s_ptr->slevel > p_ptr->lev) return (FALSE);

	/* Spell is forgotten */
	if ((use_realm == p_ptr->realm2) ?
	    (p_ptr->spell_forgotten2 & (1L << spell)) :
	    (p_ptr->spell_forgotten1 & (1L << spell)))
	{
		/* Never okay */
		return (FALSE);
	}

	if (p_ptr->pclass == CLASS_SORCERER) return (TRUE);
	if (p_ptr->pclass == CLASS_RED_MAGE) return (TRUE);

	/* Spell is learned */
	if ((use_realm == p_ptr->realm2) ?
	    (p_ptr->spell_learned2 & (1L << spell)) :
	    (p_ptr->spell_learned1 & (1L << spell)))
	{
		/* Always true */
		return (!study_pray);
	}

	/* Okay to study, not to cast */
	return (!learned);
}



/*
 * Extra information on a spell -DRS-
 *
 * We can use up to 14 characters of the buffer 'p'
 *
 * The strings in this function were extracted from the code in the
 * functions "do_cmd_cast()" and "do_cmd_pray()" and may be dated.
 */
static void spell_info(char *p, int spell, int use_realm)
{
	int plev = p_ptr->lev;

	/* See below */
	int orb = plev + (plev / ((p_ptr->pclass == CLASS_PRIEST ||
			    p_ptr->pclass == CLASS_HIGH_MAGE ||
			    p_ptr->pclass == CLASS_SORCERER) ? 2 : 4));

	int burst = plev + (plev / ((p_ptr->pclass == CLASS_MAGE ||
				     p_ptr->pclass == CLASS_HIGH_MAGE ||
				     p_ptr->pclass == CLASS_SORCERER) ? 2 : 4));
#ifdef JP
	cptr s_dam = "損傷:";
	cptr s_dur = "期間:";
	cptr s_range = "範囲:";
	cptr s_heal = "回復:";
	cptr s_random = "ランダム";
	cptr s_delay = "遅延:";
#else
	cptr s_dam = "dam ";
	cptr s_dur = "dur ";
	cptr s_range = "range ";
	cptr s_heal = "heal ";
	cptr s_random = "random";
	cptr s_delay = "delay ";
#endif
	/* Default */
	strcpy(p, "");

	/* Analyze the spell */
	switch (use_realm)
	{
	case REALM_LIFE: /* Life */
		switch (spell)
		{
		case  0: sprintf(p, " %s2d10", s_heal); break;
		case  1: sprintf(p, " %s12+d12", s_dur); break;
		case  2: sprintf(p, " %s%dd4", s_dam, 3 + ((plev - 1) / 5)); break;
		case  3: sprintf(p, " %s%d", s_dam, 10 + (plev / 2)); break;
		case  5: sprintf(p, " %s4d10", s_heal); break;
		case  9: sprintf(p, " %s%dd8", s_dam, 8 + ((plev - 1) / 5)); break;
		case 10: sprintf(p, " %s8d10", s_heal); break;
		case 11: sprintf(p, " %s20+d20", s_dur); break;
		case 14: sprintf(p, " %s300", s_heal); break;
		case 18: sprintf(p, " %sd%d", s_dam, 5 * plev); break;
		case 20: sprintf(p, " %s%dd15", s_dam, 5 + ((plev - 1) / 3)); break;
		case 21: sprintf(p, " %s15+d21", s_delay); break;
		case 29: sprintf(p, " %s2000", s_heal); break;
		case 31: sprintf(p, " %s%d+d%d", s_dur,(plev/2), (plev/2)); break;
		}
		break;
		
	case REALM_SORCERY: /* Sorcery */
		switch (spell)
		{
		case  1: sprintf(p, " %s10", s_range); break;
		case  3: sprintf(p, " %s%d", s_dam, 10 + (plev / 2)); break;
		case  5: sprintf(p, " %s%d", s_range, plev * 5); break;
		case 13: sprintf(p, " %s%d+d%d", s_dur, plev, plev + 20); break;
		case 18: sprintf(p, " %s25+d30", s_dur); break;
		case 22: sprintf(p, " %s15+d21", s_delay); break;
		case 23: sprintf(p, " %s%d", s_range, plev / 2 + 10); break;
		case 25: sprintf(p, " %s7d7+%d", s_dam, plev); break;
#ifdef JP
		case 26: sprintf(p, " 最大重量:%d.%dkg", lbtokg1(plev * 15),lbtokg2(plev * 15)); break;
#else
		case 26: sprintf(p, " max wgt %d", plev * 15 / 10); break;
#endif
		case 27: sprintf(p, " %s25+d30", s_dur); break;
		case 31: sprintf(p, " %s4+d4", s_dur); break;
		}
		break;
		
	case REALM_NATURE: /* Nature */
		switch (spell)
		{
#ifdef JP
		case  1: sprintf(p, " %s%dd4 射程%d", s_dam, (3 + ((plev - 1) / 5)), plev/6+2); break;
#else
		case  1: sprintf(p, " %s%dd4 rng %d", s_dam, (3 + ((plev - 1) / 5)), plev/6+2); break;
#endif
		case  4: sprintf(p, " %s%d", s_dam, 10 + (plev / 2)); break;
		case  6: sprintf(p, " %s20+d20", s_dur); break;
		case  7: sprintf(p, " %s2d8", s_heal); break;
		case  9: sprintf(p, " %s%dd8", s_dam, (3 + ((plev - 5) / 4))); break;
		case 11: sprintf(p, " %s%dd8", s_dam, (5 + ((plev - 5) / 4))); break;
		case 12: sprintf(p, " %s6d8", s_dam); break;
		case 15: sprintf(p, " %s500", s_heal); break;
		case 17: sprintf(p, " %s20+d30", s_dur); break;
		case 18: sprintf(p, " %s20+d20", s_dur); break;
#ifdef JP
		case 24: sprintf(p, " 半径:10"); break;
#else
		case 24: sprintf(p, " rad 10"); break;
#endif
		case 26: sprintf(p, " %s%d", s_dam, 70 + plev * 3 / 2); break;
		case 27: sprintf(p, " %s%d", s_dam, 90 + plev * 3 / 2); break;
		case 28: sprintf(p, " %s%d", s_dam, 100 + plev * 3 / 2); break;
		case 29: sprintf(p, " %s75", s_dam); break;
		case 31: sprintf(p, " %s%d+%d", s_dam, 4 * plev, 100 + plev); break;
		}
		break;
		
	case REALM_CHAOS: /* Chaos */
		switch (spell)
		{
		case  0: sprintf(p, " %s%dd4", s_dam, 3 + ((plev - 1) / 5)); break;
		case  2: sprintf(p, " %s%d", s_dam, 10 + (plev / 2)); break;
		case  4: sprintf(p, " %s3d5+%d", s_dam, burst); break;
		case  5: sprintf(p, " %s%dd8", s_dam, (6 + ((plev - 5) / 4))); break;
		case  6: sprintf(p, " %s%dd8", s_dam, (8 + ((plev - 5) / 4))); break;
		case  7: sprintf(p, " %s%d", s_range, plev * 5); break;
		case  8: sprintf(p, " %s", s_random); break;
		case  9: sprintf(p, " %s%dd8", s_dam, (10 + ((plev - 5) / 4))); break;
		case 10: sprintf(p, " %s%d", s_dam, (60 + plev)/2); break;
		case 11: sprintf(p, " %s%dd8", s_dam, (11 + ((plev - 5) / 4))); break;
		case 12: sprintf(p, " %s%d", s_dam, 55 + plev); break;
		case 15: sprintf(p, " %s%d", s_dam, 99 + plev*2); break;
		case 17: sprintf(p, " %s%dd8", s_dam, (5 + (plev / 10))); break;
		case 19: sprintf(p, " %s%d", s_dam, 70 + plev); break;
		case 21: sprintf(p, " %s%d", s_dam, 120 + plev*2); break;
		case 24: sprintf(p, " %s%dd8", s_dam, (9 + (plev / 10))); break;
#ifdef JP
		case 25: sprintf(p, " %s各%d", s_dam, plev * 2); break;
#else
		case 25: sprintf(p, " dam %d each", plev * 2); break;
#endif
		case 26: sprintf(p, " %s%d", s_dam, 150 + plev*3/2); break;
		case 27: sprintf(p, " %s150 / 250", s_dam); break;
		case 29: sprintf(p, " %s%d", s_dam, 300 + (plev * 4)); break;
		case 30: sprintf(p, " %s%d", s_dam, p_ptr->chp); break;
		case 31: sprintf(p, " %s3 * 175", s_dam); break;
		}
		break;
		
	case REALM_DEATH: /* Death */
		switch (spell)
		{
		case  1: sprintf(p, " %s%dd3", s_dam, (3 + ((plev - 1) / 5))); break;
		case  3: sprintf(p, " %s%d", s_dam, 10 + (plev / 2)); break;
		case  5: sprintf(p, " %s20+d20", s_dur); break;
		case  8: sprintf(p, " %s3d6+%d", s_dam, burst); break;
		case  9: sprintf(p, " %s%dd8", s_dam, (8 + ((plev - 5) / 4))); break;
		case 10: sprintf(p, " %s%d", s_dam, 30+plev); break;
#ifdef JP
		case 13: sprintf(p, " 損:%d+d%d", plev * 2, plev * 2); break;
#else
		case 13: sprintf(p, " d %d+d%d", plev * 2, plev * 2); break;
#endif
		case 16: sprintf(p, " %s25+d25", s_dur); break;
		case 17: sprintf(p, " %s", s_random); break;
		case 18: sprintf(p, " %s%dd8", s_dam, (4 + ((plev - 5) / 4))); break;
		case 19: sprintf(p, " %s25+d25", s_dur); break;
		case 21: sprintf(p, " %s3*100", s_dam); break;
		case 22: sprintf(p, " %sd%d", s_dam, plev * 3); break;
		case 23: sprintf(p, " %s%d", s_dam, 100 + plev * 2); break;
		case 27: sprintf(p, " %s%d+d%d", s_dur,10+plev/2, 10+plev/2); break;
		case 30: sprintf(p, " %s666", s_dam); break;
		case 31: sprintf(p, " %s%d+d%d", s_dur, (plev / 2), (plev / 2)); break;
		}
		break;
		
	case REALM_TRUMP: /* Trump */
		switch (spell)
		{
		case  0: sprintf(p, " %s10", s_range); break;
		case  2: sprintf(p, " %s", s_random); break;
		case  4: sprintf(p, " %s%d", s_range, plev * 4); break;
		case  5: sprintf(p, " %s25+d30", s_dur); break;
#ifdef JP
		case  8: sprintf(p, " 最大重量:%d.%d", lbtokg1(plev * 15 / 10),lbtokg2(plev * 15 / 10)); break;
#else
		case  8: sprintf(p, " max wgt %d", plev * 15 / 10); break;
#endif
		case 13: sprintf(p, " %s%d", s_range, plev / 2 + 10); break;
		case 14: sprintf(p, " %s15+d21", s_delay); break;
		case 26: sprintf(p, " %s%d", s_heal, plev * 10 + 200); break;
#ifdef JP
		case 28: sprintf(p, " %s各%d", s_dam, plev * 2); break;
#else
		case 28: sprintf(p, " %s%d each", s_dam, plev * 2); break;
#endif
		}
		break;
		
	case REALM_ARCANE: /* Arcane */
		switch (spell)
		{
		case  0: sprintf(p, " %s%dd3", s_dam, 3 + ((plev - 1) / 5)); break;
		case  4: sprintf(p, " %s10", s_range); break;
		case  5: sprintf(p, " %s2d%d", s_dam, plev / 2); break;
		case  7: sprintf(p, " %s2d8", s_heal); break;
		case 14:
		case 15:
		case 16:
		case 17: sprintf(p, " %s20+d20", s_dur); break;
		case 18: sprintf(p, " %s4d8", s_heal); break;
		case 19: sprintf(p, " %s%d", s_range, plev * 5); break;
		case 21: sprintf(p, " %s6d8", s_dam); break;
		case 24: sprintf(p, " %s24+d24", s_dur); break;
		case 28: sprintf(p, " %s%d", s_dam, 75 + plev); break;
		case 30: sprintf(p, " %s15+d21", s_delay); break;
		case 31: sprintf(p, " %s25+d30", s_dur); break;
		}
		break;
		
	case REALM_ENCHANT: /* Craft */
		switch (spell)
		{
		case 0: sprintf(p, " %s100+d100", s_dur); break;
		case 1: sprintf(p, " %s80+d80", s_dur); break;
		case 3:
		case 4:
		case 6:
		case 7:
		case 10:
		case 18: sprintf(p, " %s20+d20", s_dur); break;
		case 5: sprintf(p, " %s25+d25", s_dur); break;
		case 8: sprintf(p, " %s24+d24", s_dur); break;
		case 11: sprintf(p, " %s25+d25", s_dur); break;
		case 13: sprintf(p, " %s%d+d25", s_dur, plev * 3); break;
		case 15: sprintf(p, " %s%d+d%d", s_dur, plev/2, plev/2); break;
		case 16: sprintf(p, " %s25+d30", s_dur); break;
		case 17: sprintf(p, " %s30+d20", s_dur); break;
		case 19: sprintf(p, " %s%d+d%d", s_dur, plev+20, plev); break;
		case 20: sprintf(p, " %s50+d50", s_dur); break;
		case 23: sprintf(p, " %s20+d20", s_dur); break;
		case 31: sprintf(p, " %s13+d13", s_dur); break;
		}
		break;
		
	case REALM_DAEMON: /* Daemon */
		switch (spell)
		{
		case  0: sprintf(p, " %s%dd4", s_dam, 3 + ((plev - 1) / 5)); break;
		case  2: sprintf(p, " %s12+d12", s_dur); break;
		case  3: sprintf(p, " %s20+d20", s_dur); break;
		case  5: sprintf(p, " %s%dd8", s_dam, (6 + ((plev - 5) / 4))); break;
		case  7: sprintf(p, " %s3d6+%d", s_dam, burst); break;
		case 10: sprintf(p, " %s20+d20", s_dur); break;
		case 11: sprintf(p, " %s%dd8", s_dam, (11 + ((plev - 5) / 4))); break;
		case 12: sprintf(p, " %s%d", s_dam, 55 + plev); break;
		case 14: sprintf(p, " %s%d", s_dam, 100 + plev*3/2); break;
		case 16: sprintf(p, " %s30+d25", s_dur); break;
		case 17: sprintf(p, " %s20+d20", s_dur); break;
		case 18: sprintf(p, " %s%d", s_dam, 55 + plev); break;
		case 19: sprintf(p, " %s%d", s_dam, 80 + plev*3/2); break;
		case 20: sprintf(p, " %s%d+d%d", s_dur,10+plev/2, 10+plev/2); break;
		case 21: sprintf(p, " %sd%d+d%d", s_dam, 2 * plev, 2 * plev); break;
		case 22: sprintf(p, " %s%d", s_dam, 100 + plev*2); break;
		case 24: sprintf(p, " %s25+d25", s_dur); break;
		case 25: sprintf(p, " %s20+d20", s_dur); break;
		case 26: sprintf(p, " %s%d+%d", s_dam, 25+plev/2, 25+plev/2); break;
		case 29: sprintf(p, " %s%d", s_dam, plev*15); break;
		case 30: sprintf(p, " %s600", s_dam); break;
		case 31: sprintf(p, " %s15+d15", s_dur); break;
		}
		break;
		
	case REALM_CRUSADE: /* Crusade */
		switch (spell)
		{
		case  0: sprintf(p, " %s%dd4", s_dam, 3 + ((plev - 1) / 5)); break;
		case  5: sprintf(p, " %s%d", s_range, 25+plev/2); break;
#ifdef JP
		case  6: sprintf(p, " %s各%dd2", s_dam, 3+((plev-1)/9)); break;
#else
		case  6: sprintf(p, " %s%dd2 each", s_dam, 3+((plev-1)/9)); break;
#endif
		case  9: sprintf(p, " %s3d6+%d", s_dam, orb); break;
		case 10: sprintf(p, " %sd%d", s_dam, plev); break;
		case 12: sprintf(p, " %s24+d24", s_dur); break;
		case 13: sprintf(p, " %sd25+%d", s_dur, 3 * plev); break;
		case 14: sprintf(p, " %s%d", s_dam, plev*5); break;
#ifdef JP
		case 15: sprintf(p, " 損:d%d/回:100", 6 * plev); break;
#else
		case 15: sprintf(p, " dam:d%d/h100", 6 * plev); break;
#endif
		case 18: sprintf(p, " %s18+d18", s_dur); break;
		case 19: sprintf(p, " %sd%d", s_dam, 4 * plev); break;
		case 20: sprintf(p, " %sd%d", s_dam, 4 * plev); break;
		case 22: sprintf(p, " %s%d", s_dam, 2 * plev+100); break;
		case 24: sprintf(p, " %s25+d25", s_dur); break;
		case 28: sprintf(p, " %s10+d10", s_dur); break;
#ifdef JP
		case 29: sprintf(p, " %s各%d", s_dam, plev*3+25); break;
#else
		case 29: sprintf(p, " %s%d each", s_dam, plev*3+25); break;
#endif
#ifdef JP
		case 30: sprintf(p, " 回100/損%d+%d", plev * 4, plev*11/2); break;
#else
		case 30: sprintf(p, " h100/dm%d+%d", plev * 4, plev*11/2); break;
#endif
		}
		break;

	case REALM_MUSIC: /* Music */
		switch (spell)
		{
		case 2 : sprintf(p, " %s%dd4", s_dam, 4 + ((plev - 1) / 5)); break;
		case 4 : sprintf(p, " %s2d6", s_heal); break;
		case 9 : sprintf(p, " %sd%d", s_dam, plev * 3 / 2); break;
		case 13: sprintf(p, " %s%dd7", s_dam, 10 + (plev / 5)); break;
		case 20: sprintf(p, " %sd%d+d%d", s_dam, plev * 3, plev * 3); break;
		case 22: sprintf(p, " %s%dd10", s_dam, 15 + ((plev - 1) / 2)); break;
		case 27: sprintf(p, " %sd%d", s_dam, plev * 3); break;
		case 28: sprintf(p, " %s15d10", s_heal); break;
		case 30: sprintf(p, " %s%dd10", s_dam, 50 + plev); break;
		}
		break;
	default:
#ifdef JP
		sprintf(p, "未知のタイプ: %d", use_realm);
#else
		sprintf(p, "Unknown type: %d.", use_realm);
#endif
	}
}


/*
 * Print a list of spells (for browsing or casting or viewing)
 */
void print_spells(int target_spell, byte *spells, int num, int y, int x, int use_realm)
{
	int             i, spell, shougou, increment = 64;
	magic_type      *s_ptr;
	cptr            comment;
	char            info[80];
	char            out_val[160];
	byte            line_attr;
	int             shouhimana;
	char            ryakuji[5];
	char            buf[256];
	bool max = FALSE;


	if (((use_realm <= REALM_NONE) || (use_realm > MAX_REALM)) && p_ptr->wizard)
#ifdef JP
msg_print("警告！ print_spell が領域なしに呼ばれた");
#else
		msg_print("Warning! print_spells called with null realm");
#endif


	/* Title the list */
	prt("", y, x);
	if (use_realm == REALM_HISSATSU)
#ifdef JP
		strcpy(buf,"  Lv   MP");
#else
		strcpy(buf,"  Lv   SP");
#endif
	else
#ifdef JP
		strcpy(buf,"熟練度 Lv   MP 失率 効果");
#else
		strcpy(buf,"Profic Lv   SP Fail Effect");
#endif

#ifdef JP
put_str("名前", y, x + 5);
put_str(buf, y, x + 29);
#else
	put_str("Name", y, x + 5);
	put_str(buf, y, x + 29);
#endif

	if ((p_ptr->pclass == CLASS_SORCERER) || (p_ptr->pclass == CLASS_RED_MAGE)) increment = 0;
	else if (use_realm == p_ptr->realm1) increment = 0;
	else if (use_realm == p_ptr->realm2) increment = 32;

	/* Dump the spells */
	for (i = 0; i < num; i++)
	{
		/* Access the spell */
		spell = spells[i];

		/* Access the spell */
		if (!is_magic(use_realm))
		{
			s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
		}
		else
		{
			s_ptr = &mp_ptr->info[use_realm - 1][spell];
		}

		if (use_realm == REALM_HISSATSU)
			shouhimana = s_ptr->smana;
		else
		{
			s16b exp = experience_of_spell(spell, use_realm);

			/* Extract mana consumption rate */
			shouhimana = s_ptr->smana*(3800 - exp) + 2399;

			if(p_ptr->dec_mana) shouhimana *= 3;
			else shouhimana *= 4;

			shouhimana /= 9600;
			if(shouhimana < 1) shouhimana = 1;

			if ((increment == 64) || (s_ptr->slevel >= 99)) shougou = 0;
			else if (exp < 900) shougou = 0;
			else if (exp < 1200) shougou = 1;
			else if (exp < 1400) shougou = 2;
			else if (exp < 1600) shougou = 3;
			else shougou = 4;

			max = FALSE;
			if (!increment && (shougou == 4)) max = TRUE;
			else if ((increment == 32) && (shougou == 3)) max = TRUE;
			else if (s_ptr->slevel >= 99) max = TRUE;
			else if (p_ptr->pclass == CLASS_RED_MAGE) max = TRUE;

			strncpy(ryakuji,shougou_moji[shougou],4);
			ryakuji[3] = ']';
			ryakuji[4] = '\0';
		}

		if (use_menu && target_spell)
		{
			if (i == (target_spell-1))
#ifdef JP
				strcpy(out_val, "  》 ");
#else
				strcpy(out_val, "  >  ");
#endif
			else
				strcpy(out_val, "     ");
		}
		else sprintf(out_val, "  %c) ", I2A(i));
		/* Skip illegible spells */
		if (s_ptr->slevel >= 99)
		{
#ifdef JP
strcat(out_val, format("%-30s", "(判読不能)"));
#else
				strcat(out_val, format("%-30s", "(illegible)"));
#endif

				c_prt(TERM_L_DARK, out_val, y + i + 1, x);
				continue;
		}

		/* XXX XXX Could label spells above the players level */

		/* Get extra info */
		spell_info(info, spell, use_realm);

		/* Use that info */
		comment = info;

		/* Assume spell is known and tried */
		line_attr = TERM_WHITE;

		/* Analyze the spell */
		if ((p_ptr->pclass == CLASS_SORCERER) || (p_ptr->pclass == CLASS_RED_MAGE))
		{
			if (s_ptr->slevel > p_ptr->max_plv)
			{
#ifdef JP
comment = " 未知";
#else
				comment = " unknown";
#endif

				line_attr = TERM_L_BLUE;
			}
			else if (s_ptr->slevel > p_ptr->lev)
			{
#ifdef JP
comment = " 忘却";
#else
				comment = " forgotten";
#endif

				line_attr = TERM_YELLOW;
			}
		}
		else if ((use_realm != p_ptr->realm1) && (use_realm != p_ptr->realm2))
		{
#ifdef JP
comment = " 未知";
#else
			comment = " unknown";
#endif

			line_attr = TERM_L_BLUE;
		}
		else if ((use_realm == p_ptr->realm1) ?
		    ((p_ptr->spell_forgotten1 & (1L << spell))) :
		    ((p_ptr->spell_forgotten2 & (1L << spell))))
		{
#ifdef JP
comment = " 忘却";
#else
			comment = " forgotten";
#endif

			line_attr = TERM_YELLOW;
		}
		else if (!((use_realm == p_ptr->realm1) ?
		    (p_ptr->spell_learned1 & (1L << spell)) :
		    (p_ptr->spell_learned2 & (1L << spell))))
		{
#ifdef JP
comment = " 未知";
#else
			comment = " unknown";
#endif

			line_attr = TERM_L_BLUE;
		}
		else if (!((use_realm == p_ptr->realm1) ?
		    (p_ptr->spell_worked1 & (1L << spell)) :
		    (p_ptr->spell_worked2 & (1L << spell))))
		{
#ifdef JP
comment = " 未経験";
#else
			comment = " untried";
#endif

			line_attr = TERM_L_GREEN;
		}

		/* Dump the spell --(-- */
		if (use_realm == REALM_HISSATSU)
		{
			strcat(out_val, format("%-25s %2d %4d",
			    spell_names[technic2magic(use_realm)-1][spell], /* realm, spell */
			    s_ptr->slevel, shouhimana));
		}
		else
		{
			strcat(out_val, format("%-25s%c%-4s %2d %4d %3d%%%s",
			    spell_names[technic2magic(use_realm)-1][spell], /* realm, spell */
			    (max ? '!' : ' '), ryakuji,
			    s_ptr->slevel, shouhimana, spell_chance(spell, use_realm), comment));
		}
		c_prt(line_attr, out_val, y + i + 1, x);
	}

	/* Clear the bottom line */
	prt("", y + i + 1, x);
}


/*
 * Note that amulets, rods, and high-level spell books are immune
 * to "inventory damage" of any kind.  Also sling ammo and shovels.
 */


/*
 * Does a given class of objects (usually) hate acid?
 * Note that acid can either melt or corrode something.
 */
bool hates_acid(object_type *o_ptr)
{
	/* Analyze the type */
	switch (o_ptr->tval)
	{
		/* Wearable items */
		case TV_ARROW:
		case TV_BOLT:
		case TV_BOW:
		case TV_SWORD:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		{
			return (TRUE);
		}

		/* Staffs/Scrolls are wood/paper */
		case TV_STAFF:
		case TV_SCROLL:
		{
			return (TRUE);
		}

		/* Ouch */
		case TV_CHEST:
		{
			return (TRUE);
		}

		/* Junk is useless */
		case TV_SKELETON:
		case TV_BOTTLE:
		case TV_JUNK:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Does a given object (usually) hate electricity?
 */
bool hates_elec(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_RING:
		case TV_WAND:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Does a given object (usually) hate fire?
 * Hafted/Polearm weapons have wooden shafts.
 * Arrows/Bows are mostly wooden.
 */
bool hates_fire(object_type *o_ptr)
{
	/* Analyze the type */
	switch (o_ptr->tval)
	{
		/* Wearable */
		case TV_LITE:
		case TV_ARROW:
		case TV_BOW:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		{
			return (TRUE);
		}

		/* Books */
		case TV_LIFE_BOOK:
		case TV_SORCERY_BOOK:
		case TV_NATURE_BOOK:
		case TV_CHAOS_BOOK:
		case TV_DEATH_BOOK:
		case TV_TRUMP_BOOK:
		case TV_ARCANE_BOOK:
		case TV_ENCHANT_BOOK:
		case TV_DAEMON_BOOK:
		case TV_CRUSADE_BOOK:
		case TV_MUSIC_BOOK:
		case TV_HISSATSU_BOOK:
		{
			return (TRUE);
		}

		/* Chests */
		case TV_CHEST:
		{
			return (TRUE);
		}

		/* Staffs/Scrolls burn */
		case TV_STAFF:
		case TV_SCROLL:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Does a given object (usually) hate cold?
 */
bool hates_cold(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_POTION:
		case TV_FLASK:
		case TV_BOTTLE:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Melt something
 */
int set_acid_destroy(object_type *o_ptr)
{
	u32b f1, f2, f3;
	if (!hates_acid(o_ptr)) return (FALSE);
	object_flags(o_ptr, &f1, &f2, &f3);
	if (f3 & TR3_IGNORE_ACID) return (FALSE);
	return (TRUE);
}


/*
 * Electrical damage
 */
int set_elec_destroy(object_type *o_ptr)
{
	u32b f1, f2, f3;
	if (!hates_elec(o_ptr)) return (FALSE);
	object_flags(o_ptr, &f1, &f2, &f3);
	if (f3 & TR3_IGNORE_ELEC) return (FALSE);
	return (TRUE);
}


/*
 * Burn something
 */
int set_fire_destroy(object_type *o_ptr)
{
	u32b f1, f2, f3;
	if (!hates_fire(o_ptr)) return (FALSE);
	object_flags(o_ptr, &f1, &f2, &f3);
	if (f3 & TR3_IGNORE_FIRE) return (FALSE);
	return (TRUE);
}


/*
 * Freeze things
 */
int set_cold_destroy(object_type *o_ptr)
{
	u32b f1, f2, f3;
	if (!hates_cold(o_ptr)) return (FALSE);
	object_flags(o_ptr, &f1, &f2, &f3);
	if (f3 & TR3_IGNORE_COLD) return (FALSE);
	return (TRUE);
}


/*
 * Destroys a type of item on a given percent chance
 * Note that missiles are no longer necessarily all destroyed
 * Destruction taken from "melee.c" code for "stealing".
 * New-style wands and rods handled correctly. -LM-
 * Returns number of items destroyed.
 */
int inven_damage(inven_func typ, int perc)
{
	int         i, j, k, amt;
	object_type *o_ptr;
	char        o_name[MAX_NLEN];

	/* Multishadow effects is determined by turn */
	if( p_ptr->multishadow && (turn & 1) )return 0;

	if (p_ptr->inside_arena) return 0;

	/* Count the casualties */
	k = 0;

	/* Scan through the slots backwards */
	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Hack -- for now, skip artifacts */
		if (artifact_p(o_ptr) || o_ptr->art_name) continue;

		/* Give this item slot a shot at death */
		if ((*typ)(o_ptr))
		{
			/* Count the casualties */
			for (amt = j = 0; j < o_ptr->number; ++j)
			{
				if (randint0(100) < perc) amt++;
			}

			/* Some casualities */
			if (amt)
			{
				/* Get a description */
				object_desc(o_name, o_ptr, FALSE, 3);

				/* Message */
#ifdef JP
msg_format("%s(%c)が%s壊れてしまった！",
#else
				msg_format("%sour %s (%c) %s destroyed!",
#endif

#ifdef JP
o_name, index_to_label(i),
    ((o_ptr->number > 1) ?
    ((amt == o_ptr->number) ? "全部" :
    (amt > 1 ? "何個か" : "一個")) : "")    );
#else
				    ((o_ptr->number > 1) ?
				    ((amt == o_ptr->number) ? "All of y" :
				    (amt > 1 ? "Some of y" : "One of y")) : "Y"),
				    o_name, index_to_label(i),
				    ((amt > 1) ? "were" : "was"));
#endif

#ifdef JP
				if ((p_ptr->pseikaku == SEIKAKU_COMBAT) || (inventory[INVEN_BOW].name1 == ART_CRIMSON))
					msg_print("やりやがったな！");
#endif

				/* Potions smash open */
				if (object_is_potion(o_ptr))
				{
					(void)potion_smash_effect(0, py, px, o_ptr->k_idx);
				}

				/* Reduce the charges of rods/wands */
				reduce_charges(o_ptr, amt);

				/* Destroy "amt" items */
				inven_item_increase(i, -amt);
				inven_item_optimize(i);

				/* Count the casualties */
				k += amt;
			}
		}
	}

	/* Return the casualty count */
	return (k);
}


/*
 * Acid has hit the player, attempt to affect some armor.
 *
 * Note that the "base armor" of an object never changes.
 *
 * If any armor is damaged (or resists), the player takes less damage.
 */
static int minus_ac(void)
{
	object_type *o_ptr = NULL;
	u32b        f1, f2, f3;
	char        o_name[MAX_NLEN];


	/* Pick a (possibly empty) inventory slot */
	switch (randint1(7))
	{
		case 1: o_ptr = &inventory[INVEN_RARM]; break;
		case 2: o_ptr = &inventory[INVEN_LARM]; break;
		case 3: o_ptr = &inventory[INVEN_BODY]; break;
		case 4: o_ptr = &inventory[INVEN_OUTER]; break;
		case 5: o_ptr = &inventory[INVEN_HANDS]; break;
		case 6: o_ptr = &inventory[INVEN_HEAD]; break;
		case 7: o_ptr = &inventory[INVEN_FEET]; break;
	}

	/* Nothing to damage */
	if (!o_ptr->k_idx) return (FALSE);

	if (o_ptr->tval < TV_BOOTS) return (FALSE);

	/* No damage left to be done */
	if (o_ptr->ac + o_ptr->to_a <= 0) return (FALSE);


	/* Describe */
	object_desc(o_name, o_ptr, FALSE, 0);

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3);

	/* Object resists */
	if (f3 & TR3_IGNORE_ACID)
	{
#ifdef JP
msg_format("しかし%sには効果がなかった！", o_name);
#else
		msg_format("Your %s is unaffected!", o_name);
#endif


		return (TRUE);
	}

	/* Message */
#ifdef JP
msg_format("%sがダメージを受けた！", o_name);
#else
	msg_format("Your %s is damaged!", o_name);
#endif


	/* Damage the item */
	o_ptr->to_a--;

	/* Calculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->window |= (PW_EQUIP | PW_PLAYER);

	calc_android_exp();

	/* Item was damaged */
	return (TRUE);
}


/*
 * Hurt the player with Acid
 */
void acid_dam(int dam, cptr kb_str, int monspell)
{
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;
	bool double_resist = (p_ptr->oppose_acid  || music_singing(MUSIC_RESIST) || (p_ptr->special_defense & KATA_MUSOU));

	/* Total Immunity */
	if (p_ptr->immune_acid || (dam <= 0))
	{
		learn_spell(monspell);
		return;
	}

	/* Vulnerability (Ouch!) */
	if (p_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
	if (p_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;

	/* Resist the damage */
	if (p_ptr->resist_acid) dam = (dam + 2) / 3;
	if (double_resist) dam = (dam + 2) / 3;

	if ((!(double_resist || p_ptr->resist_acid)) &&
	    one_in_(HURT_CHANCE))
		(void)do_dec_stat(A_CHR);

	/* If any armor gets hit, defend the player */
	if (minus_ac()) dam = (dam + 1) / 2;

	/* Take damage */
	take_hit(DAMAGE_ATTACK, dam, kb_str, monspell);

	/* Inventory damage */
	if (!(double_resist && p_ptr->resist_acid))
		inven_damage(set_acid_destroy, inv);
}


/*
 * Hurt the player with electricity
 */
void elec_dam(int dam, cptr kb_str, int monspell)
{
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;
	bool double_resist = (p_ptr->oppose_elec  || music_singing(MUSIC_RESIST) || (p_ptr->special_defense & KATA_MUSOU));

	/* Total immunity */
	if (p_ptr->immune_elec || (dam <= 0))
	{
		learn_spell(monspell);
		return;
	}

	/* Vulnerability (Ouch!) */
	if (p_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
	if (p_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;
	if (p_ptr->prace == RACE_ANDROID) dam += dam / 3;

	/* Resist the damage */
	if (p_ptr->resist_elec) dam = (dam + 2) / 3;
	if (double_resist) dam = (dam + 2) / 3;

	if ((!(double_resist || p_ptr->resist_elec)) &&
	    one_in_(HURT_CHANCE))
		(void)do_dec_stat(A_DEX);

	/* Take damage */
	take_hit(DAMAGE_ATTACK, dam, kb_str, monspell);

	/* Inventory damage */
	if (!(double_resist && p_ptr->resist_elec))
		inven_damage(set_elec_destroy, inv);
}


/*
 * Hurt the player with Fire
 */
void fire_dam(int dam, cptr kb_str, int monspell)
{
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;
	bool double_resist = (p_ptr->oppose_fire  || music_singing(MUSIC_RESIST) || (p_ptr->special_defense & KATA_MUSOU));

	/* Totally immune */
	if (p_ptr->immune_fire || (dam <= 0))
	{
		learn_spell(monspell);
		return;
	}

	/* Vulnerability (Ouch!) */
	if (p_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
	if (prace_is_(RACE_ENT)) dam += dam / 3;
	if (p_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;

	/* Resist the damage */
	if (p_ptr->resist_fire) dam = (dam + 2) / 3;
	if (double_resist) dam = (dam + 2) / 3;

	if ((!(double_resist || p_ptr->resist_fire)) &&
	    one_in_(HURT_CHANCE))
		(void)do_dec_stat(A_STR);

	/* Take damage */
	take_hit(DAMAGE_ATTACK, dam, kb_str, monspell);

	/* Inventory damage */
	if (!(double_resist && p_ptr->resist_fire))
		inven_damage(set_fire_destroy, inv);
}


/*
 * Hurt the player with Cold
 */
void cold_dam(int dam, cptr kb_str, int monspell)
{
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;
	bool double_resist = (p_ptr->oppose_cold  || music_singing(MUSIC_RESIST) || (p_ptr->special_defense & KATA_MUSOU));

	/* Total immunity */
	if (p_ptr->immune_cold || (dam <= 0))
	{
		learn_spell(monspell);
		return;
	}

	/* Vulnerability (Ouch!) */
	if (p_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
	if (p_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;

	/* Resist the damage */
	if (p_ptr->resist_cold) dam = (dam + 2) / 3;
	if (double_resist) dam = (dam + 2) / 3;

	if ((!(double_resist || p_ptr->resist_cold)) &&
	    one_in_(HURT_CHANCE))
		(void)do_dec_stat(A_STR);

	/* Take damage */
	take_hit(DAMAGE_ATTACK, dam, kb_str, monspell);

	/* Inventory damage */
	if (!(double_resist && p_ptr->resist_cold))
		inven_damage(set_cold_destroy, inv);
}


bool rustproof(void)
{
	int         item;
	object_type *o_ptr;
	char        o_name[MAX_NLEN];
	cptr        q, s;

	item_tester_no_ryoute = TRUE;
	/* Select a piece of armour */
	item_tester_hook = item_tester_hook_armour;

	/* Get an item */
#ifdef JP
q = "どの防具に錆止めをしますか？";
s = "錆止めできるものがありません。";
#else
	q = "Rustproof which piece of armour? ";
	s = "You have nothing to rustproof.";
#endif

	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return FALSE;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}


	/* Description */
	object_desc(o_name, o_ptr, FALSE, 0);

	o_ptr->art_flags3 |= TR3_IGNORE_ACID;

	if ((o_ptr->to_a < 0) && !cursed_p(o_ptr))
	{
#ifdef JP
msg_format("%sは新品同様になった！",o_name);
#else
		msg_format("%s %s look%s as good as new!",
			((item >= 0) ? "Your" : "The"), o_name,
			((o_ptr->number > 1) ? "" : "s"));
#endif

		o_ptr->to_a = 0;
	}

#ifdef JP
msg_format("%sは腐食しなくなった。", o_name);
#else
	msg_format("%s %s %s now protected against corrosion.",
		((item >= 0) ? "Your" : "The"), o_name,
		((o_ptr->number > 1) ? "are" : "is"));
#endif


	calc_android_exp();

	return TRUE;
}


/*
 * Curse the players armor
 */
bool curse_armor(void)
{
	object_type *o_ptr;

	char o_name[MAX_NLEN];


	/* Curse the body armor */
	o_ptr = &inventory[INVEN_BODY];

	/* Nothing to curse */
	if (!o_ptr->k_idx) return (FALSE);


	/* Describe */
	object_desc(o_name, o_ptr, FALSE, 3);

	/* Attempt a saving throw for artifacts */
	if ((o_ptr->art_name || artifact_p(o_ptr)) && (randint0(100) < 50))
	{
		/* Cool */
#ifdef JP
msg_format("%sが%sを包み込もうとしたが、%sはそれを跳ね返した！",
"恐怖の暗黒オーラ", "防具", o_name);
#else
		msg_format("A %s tries to %s, but your %s resists the effects!",
		           "terrible black aura", "surround your armor", o_name);
#endif

	}

	/* not artifact or failed save... */
	else
	{
		/* Oops */
#ifdef JP
msg_format("恐怖の暗黒オーラがあなたの%sを包み込んだ！", o_name);
#else
		msg_format("A terrible black aura blasts your %s!", o_name);
#endif

		chg_virtue(V_ENCHANT, -5);

		/* Blast the armor */
		o_ptr->name1 = 0;
		o_ptr->name2 = EGO_BLASTED;
		o_ptr->to_a = 0 - randint1(5) - randint1(5);
		o_ptr->to_h = 0;
		o_ptr->to_d = 0;
		o_ptr->ac = 0;
		o_ptr->dd = 0;
		o_ptr->ds = 0;
		o_ptr->art_flags1 = 0;
		o_ptr->art_flags2 = 0;
		o_ptr->art_flags3 = 0;

		/* Curse it */
		o_ptr->curse_flags = TRC_CURSED;

		/* Break it */
		o_ptr->ident |= (IDENT_BROKEN);

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Recalculate mana */
		p_ptr->update |= (PU_MANA);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
	}

	return (TRUE);
}


/*
 * Curse the players weapon
 */
bool curse_weapon(bool force, int slot)
{
	object_type *o_ptr;

	char o_name[MAX_NLEN];


	/* Curse the weapon */
	o_ptr = &inventory[slot];

	/* Nothing to curse */
	if (!o_ptr->k_idx) return (FALSE);


	/* Describe */
	object_desc(o_name, o_ptr, FALSE, 3);

	/* Attempt a saving throw */
	if ((artifact_p(o_ptr) || o_ptr->art_name) && (randint0(100) < 50) && !force)
	{
		/* Cool */
#ifdef JP
msg_format("%sが%sを包み込もうとしたが、%sはそれを跳ね返した！",
"恐怖の暗黒オーラ", "武器", o_name);
#else
		msg_format("A %s tries to %s, but your %s resists the effects!",
		           "terrible black aura", "surround your weapon", o_name);
#endif

	}

	/* not artifact or failed save... */
	else
	{
		/* Oops */
#ifdef JP
if (!force) msg_format("恐怖の暗黒オーラがあなたの%sを包み込んだ！", o_name);
#else
		if (!force) msg_format("A terrible black aura blasts your %s!", o_name);
#endif

		chg_virtue(V_ENCHANT, -5);

		/* Shatter the weapon */
		o_ptr->name1 = 0;
		o_ptr->name2 = EGO_SHATTERED;
		o_ptr->to_h = 0 - randint1(5) - randint1(5);
		o_ptr->to_d = 0 - randint1(5) - randint1(5);
		o_ptr->to_a = 0;
		o_ptr->ac = 0;
		o_ptr->dd = 0;
		o_ptr->ds = 0;
		o_ptr->art_flags1 = 0;
		o_ptr->art_flags2 = 0;
		o_ptr->art_flags3 = 0;


		/* Curse it */
		o_ptr->curse_flags = TRC_CURSED;

		/* Break it */
		o_ptr->ident |= (IDENT_BROKEN);

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Recalculate mana */
		p_ptr->update |= (PU_MANA);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
	}

	/* Notice */
	return (TRUE);
}


/*
 * Enchant some bolts
 */
bool brand_bolts(void)
{
	int i;

	/* Use the first acceptable bolts */
	for (i = 0; i < INVEN_PACK; i++)
	{
		object_type *o_ptr = &inventory[i];

		/* Skip non-bolts */
		if (o_ptr->tval != TV_BOLT) continue;

		/* Skip artifacts and ego-items */
		if (o_ptr->art_name || artifact_p(o_ptr) || ego_item_p(o_ptr))
			continue;

		/* Skip cursed/broken items */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) continue;

		/* Randomize */
		if (randint0(100) < 75) continue;

		/* Message */
#ifdef JP
msg_print("クロスボウの矢が炎のオーラに包まれた！");
#else
		msg_print("Your bolts are covered in a fiery aura!");
#endif


		/* Ego-item */
		o_ptr->name2 = EGO_FLAME;

		/* Enchant */
		enchant(o_ptr, randint0(3) + 4, ENCH_TOHIT | ENCH_TODAM);

		/* Notice */
		return (TRUE);
	}

	/* Flush */
	if (flush_failure) flush();

	/* Fail */
#ifdef JP
msg_print("炎で強化するのに失敗した。");
#else
	msg_print("The fiery enchantment failed.");
#endif


	/* Notice */
	return (TRUE);
}


/*
 * Helper function -- return a "nearby" race for polymorphing
 *
 * Note that this function is one of the more "dangerous" ones...
 */
static s16b poly_r_idx(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	int i, r, lev1, lev2;

	/* Hack -- Uniques/Questors never polymorph */
	if ((r_ptr->flags1 & RF1_UNIQUE) ||
	    (r_ptr->flags1 & RF1_QUESTOR))
		return (r_idx);

	/* Allowable range of "levels" for resulting monster */
	lev1 = r_ptr->level - ((randint1(20) / randint1(9)) + 1);
	lev2 = r_ptr->level + ((randint1(20) / randint1(9)) + 1);

	/* Pick a (possibly new) non-unique race */
	for (i = 0; i < 1000; i++)
	{
		/* Pick a new race, using a level calculation */
		r = get_mon_num((dun_level + r_ptr->level) / 2 + 5);

		/* Handle failure */
		if (!r) break;

		/* Obtain race */
		r_ptr = &r_info[r];

		/* Ignore unique monsters */
		if (r_ptr->flags1 & RF1_UNIQUE) continue;

		/* Ignore monsters with incompatible levels */
		if ((r_ptr->level < lev1) || (r_ptr->level > lev2)) continue;

		/* Use that index */
		r_idx = r;

		/* Done */
		break;
	}

	/* Result */
	return (r_idx);
}


bool polymorph_monster(int y, int x)
{
	cave_type *c_ptr = &cave[y][x];
	monster_type *m_ptr = &m_list[c_ptr->m_idx];
	bool polymorphed = FALSE;
	int new_r_idx;
	int old_r_idx = m_ptr->r_idx;
	bool targeted = (target_who == c_ptr->m_idx) ? TRUE : FALSE;
	bool health_tracked = (p_ptr->health_who == c_ptr->m_idx) ? TRUE : FALSE;
	monster_type back_m;

	if (p_ptr->inside_arena || p_ptr->inside_battle) return (FALSE);

	if ((p_ptr->riding == c_ptr->m_idx) || (m_ptr->mflag2 & MFLAG_KAGE)) return (FALSE);

	/* Memorize the monster before polymorphing */
	back_m = *m_ptr;

	/* Pick a "new" monster race */
	new_r_idx = poly_r_idx(old_r_idx);

	/* Handle polymorph */
	if (new_r_idx != old_r_idx)
	{
		u32b mode = 0L;

		/* Get the monsters attitude */
		if (is_friendly(m_ptr)) mode |= PM_FORCE_FRIENDLY;
		if (is_pet(m_ptr)) mode |= PM_FORCE_PET;
		if (m_ptr->mflag2 & MFLAG_NOPET) mode |= PM_NO_PET;

		/* "Kill" the "old" monster */
		delete_monster_idx(c_ptr->m_idx);

		/* Create a new monster (no groups) */
		if (place_monster_aux(0, y, x, new_r_idx, mode))
		{
			/* Success */
			polymorphed = TRUE;
		}
		else
		{
			/* Placing the new monster failed */
			if (place_monster_aux(0, y, x, old_r_idx, (mode | PM_NO_KAGE | PM_IGNORE_TERRAIN)))
                                m_list[hack_m_idx_ii] = back_m;
		}

		if (targeted) target_who = hack_m_idx_ii;
		if (health_tracked) health_track(hack_m_idx_ii);
	}

	return polymorphed;
}


/*
 * Dimension Door
 */
bool dimension_door(void)
{
	int	plev = p_ptr->lev;
	int	x = 0, y = 0;

	if (!tgt_pt(&x, &y)) return FALSE;

	p_ptr->energy_need += (s16b)((s32b)(60 - plev) * ENERGY_NEED() / 100L);

	if (!cave_empty_bold(y, x) || (cave[y][x].info & CAVE_ICKY) ||
		(distance(y, x, py, px) > plev / 2 + 10) ||
		(!randint0(plev / 10 + 10)))
	{
		if( p_ptr->pclass != CLASS_MIRROR_MASTER ){
#ifdef JP
			msg_print("精霊界から物質界に戻る時うまくいかなかった！");
#else
			msg_print("You fail to exit the astral plane correctly!");
#endif
		}
		else
		{
#ifdef JP
			msg_print("鏡の世界をうまく通れなかった！");
#else
			msg_print("You fail to exit the astral plane correctly!");
#endif
		}
		p_ptr->energy_need += (s16b)((s32b)(60 - plev) * ENERGY_NEED() / 100L);
		teleport_player((plev+2)*2);
	}
	else
		teleport_player_to(y, x, TRUE);

	return (TRUE);
}


bool eat_magic(int power)
{
	object_type * o_ptr;
	object_kind *k_ptr;
	int lev, item;
	int recharge_strength = 0;

	bool fail = FALSE;
	byte fail_type = 1;

	cptr q, s;
	char o_name[MAX_NLEN];

	item_tester_hook = item_tester_hook_recharge;

	/* Get an item */
#ifdef JP
q = "どのアイテムから魔力を吸収しますか？";
s = "魔力を吸収できるアイテムがありません。";
#else
	q = "Drain which item? ";
	s = "You have nothing to drain.";
#endif

	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return FALSE;

	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}
	else
	{
		o_ptr = &o_list[0 - item];
	}

	k_ptr = &k_info[o_ptr->k_idx];
	lev = get_object_level(o_ptr);

	if (o_ptr->tval == TV_ROD)
	{
		recharge_strength = ((power > lev/2) ? (power - lev/2) : 0) / 5;

		/* Back-fire */
		if (one_in_(recharge_strength))
		{
			/* Activate the failure code. */
			fail = TRUE;
		}
		else
		{
			if (o_ptr->timeout > (o_ptr->number - 1) * k_ptr->pval)
			{
#ifdef JP
msg_print("充填中のロッドから魔力を吸収することはできません。");
#else
				msg_print("You can't absorb energy from a discharged rod.");
#endif

			}
			else
			{
				p_ptr->csp += lev;
				o_ptr->timeout += k_ptr->pval;
			}
		}
	}
	else
	{
		/* All staffs, wands. */
		recharge_strength = (100 + power - lev) / 15;

		/* Paranoia */
		if (recharge_strength < 0) recharge_strength = 0;

		/* Back-fire */
		if (one_in_(recharge_strength))
		{
			/* Activate the failure code. */
			fail = TRUE;
		}
		else
		{
			if (o_ptr->pval > 0)
			{
				p_ptr->csp += lev / 2;
				o_ptr->pval --;

				/* XXX Hack -- unstack if necessary */
				if ((o_ptr->tval == TV_STAFF) && (item >= 0) && (o_ptr->number > 1))
				{
					object_type forge;
					object_type *q_ptr;

					/* Get local object */
					q_ptr = &forge;

					/* Obtain a local object */
					object_copy(q_ptr, o_ptr);

					/* Modify quantity */
					q_ptr->number = 1;

					/* Restore the charges */
					o_ptr->pval++;

					/* Unstack the used item */
					o_ptr->number--;
					p_ptr->total_weight -= q_ptr->weight;
					item = inven_carry(q_ptr);

					/* Message */
#ifdef JP
					msg_print("杖をまとめなおした。");
#else
					msg_print("You unstack your staff.");
#endif

				}
			}
			else
			{
#ifdef JP
msg_print("吸収できる魔力がありません！");
#else
				msg_print("There's no energy there to absorb!");
#endif

			}
			if (!o_ptr->pval) o_ptr->ident |= IDENT_EMPTY;
		}
	}

	/* Inflict the penalties for failing a recharge. */
	if (fail)
	{
		/* Artifacts are never destroyed. */
		if (artifact_p(o_ptr))
		{
			object_desc(o_name, o_ptr, TRUE, 0);
#ifdef JP
msg_format("魔力が逆流した！%sは完全に魔力を失った。", o_name);
#else
			msg_format("The recharging backfires - %s is completely drained!", o_name);
#endif


			/* Artifact rods. */
			if (o_ptr->tval == TV_ROD)
				o_ptr->timeout = k_ptr->pval * o_ptr->number;

			/* Artifact wands and staffs. */
			else if ((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_STAFF))
				o_ptr->pval = 0;
		}
		else
		{
			/* Get the object description */
			object_desc(o_name, o_ptr, FALSE, 0);

			/*** Determine Seriousness of Failure ***/

			/* Mages recharge objects more safely. */
			if (p_ptr->pclass == CLASS_MAGE || p_ptr->pclass == CLASS_HIGH_MAGE || p_ptr->pclass == CLASS_SORCERER || p_ptr->pclass == CLASS_MAGIC_EATER || p_ptr->pclass == CLASS_BLUE_MAGE)
			{
				/* 10% chance to blow up one rod, otherwise draining. */
				if (o_ptr->tval == TV_ROD)
				{
					if (one_in_(10)) fail_type = 2;
					else fail_type = 1;
				}
				/* 75% chance to blow up one wand, otherwise draining. */
				else if (o_ptr->tval == TV_WAND)
				{
					if (!one_in_(3)) fail_type = 2;
					else fail_type = 1;
				}
				/* 50% chance to blow up one staff, otherwise no effect. */
				else if (o_ptr->tval == TV_STAFF)
				{
					if (one_in_(2)) fail_type = 2;
					else fail_type = 0;
				}
			}

			/* All other classes get no special favors. */
			else
			{
				/* 33% chance to blow up one rod, otherwise draining. */
				if (o_ptr->tval == TV_ROD)
				{
					if (one_in_(3)) fail_type = 2;
					else fail_type = 1;
				}
				/* 20% chance of the entire stack, else destroy one wand. */
				else if (o_ptr->tval == TV_WAND)
				{
					if (one_in_(5)) fail_type = 3;
					else fail_type = 2;
				}
				/* Blow up one staff. */
				else if (o_ptr->tval == TV_STAFF)
				{
					fail_type = 2;
				}
			}

			/*** Apply draining and destruction. ***/

			/* Drain object or stack of objects. */
			if (fail_type == 1)
			{
				if (o_ptr->tval == TV_ROD)
				{
#ifdef JP
msg_print("ロッドは破損を免れたが、魔力は全て失なわれた。");
#else
					msg_format("You save your rod from destruction, but all charges are lost.", o_name);
#endif

					o_ptr->timeout = k_ptr->pval * o_ptr->number;
				}
				else if (o_ptr->tval == TV_WAND)
				{
#ifdef JP
msg_format("%sは破損を免れたが、魔力が全て失われた。", o_name);
#else
					msg_format("You save your %s from destruction, but all charges are lost.", o_name);
#endif

					o_ptr->pval = 0;
				}
				/* Staffs aren't drained. */
			}

			/* Destroy an object or one in a stack of objects. */
			if (fail_type == 2)
			{
				if (o_ptr->number > 1)
				{
#ifdef JP
msg_format("乱暴な魔法のために%sが一本壊れた！", o_name);
#else
					msg_format("Wild magic consumes one of your %s!", o_name);
#endif

					/* Reduce rod stack maximum timeout, drain wands. */
					if (o_ptr->tval == TV_ROD) o_ptr->timeout -= k_ptr->pval;
					if (o_ptr->tval == TV_WAND) o_ptr->pval = o_ptr->pval * (o_ptr->number - 1) / o_ptr->number;

				}
				else
#ifdef JP
msg_format("乱暴な魔法のために%sが何本か壊れた！", o_name);
#else
					msg_format("Wild magic consumes your %s!", o_name);
#endif

				/* Reduce and describe inventory */
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
					floor_item_describe(0 - item);
					floor_item_optimize(0 - item);
				}
			}

			/* Destroy all members of a stack of objects. */
			if (fail_type == 3)
			{
				if (o_ptr->number > 1)
#ifdef JP
msg_format("乱暴な魔法のために%sが全て壊れた！", o_name);
#else
					msg_format("Wild magic consumes all your %s!", o_name);
#endif

				else
#ifdef JP
msg_format("乱暴な魔法のために%sが壊れた！", o_name);
#else
					msg_format("Wild magic consumes your %s!", o_name);
#endif



				/* Reduce and describe inventory */
				if (item >= 0)
				{
					inven_item_increase(item, -999);
					inven_item_describe(item);
					inven_item_optimize(item);
				}

				/* Reduce and describe floor item */
				else
				{
					floor_item_increase(0 - item, -999);
					floor_item_describe(0 - item);
					floor_item_optimize(0 - item);
				}
			}
		}
	}

	if (p_ptr->csp > p_ptr->msp)
	{
		p_ptr->csp = p_ptr->msp;
	}

	p_ptr->notice |= (PN_COMBINE | PN_REORDER);
	p_ptr->window |= (PW_INVEN);

	return TRUE;
}


bool summon_kin_player(int level, int y, int x, u32b mode)
{
	bool pet = (bool)(mode & PM_FORCE_PET);
	if (!pet) mode |= PM_NO_PET;

	switch (p_ptr->mimic_form)
	{
	case MIMIC_NONE:
		switch (p_ptr->prace)
		{
			case RACE_HUMAN:
			case RACE_AMBERITE:
			case RACE_BARBARIAN:
			case RACE_BEASTMAN:
			case RACE_DUNADAN:
				summon_kin_type = 'p';
				break;
			case RACE_HALF_ELF:
			case RACE_ELF:
			case RACE_HOBBIT:
			case RACE_GNOME:
			case RACE_DWARF:
			case RACE_HIGH_ELF:
			case RACE_NIBELUNG:
			case RACE_DARK_ELF:
			case RACE_MIND_FLAYER:
			case RACE_KUTA:
			case RACE_S_FAIRY:
				summon_kin_type = 'h';
				break;
			case RACE_HALF_ORC:
				summon_kin_type = 'o';
				break;
			case RACE_HALF_TROLL:
				summon_kin_type = 'T';
				break;
			case RACE_HALF_OGRE:
				summon_kin_type = 'O';
				break;
			case RACE_HALF_GIANT:
			case RACE_HALF_TITAN:
			case RACE_CYCLOPS:
				summon_kin_type = 'P';
				break;
			case RACE_YEEK:
				summon_kin_type = 'y';
				break;
			case RACE_KLACKON:
				summon_kin_type = 'K';
				break;
			case RACE_KOBOLD:
				summon_kin_type = 'k';
				break;
			case RACE_IMP:
				if (one_in_(13)) summon_kin_type = 'U';
				else summon_kin_type = 'u';
				break;
			case RACE_DRACONIAN:
				summon_kin_type = 'd';
				break;
			case RACE_GOLEM:
			case RACE_ANDROID:
				summon_kin_type = 'g';
				break;
			case RACE_SKELETON:
				if (one_in_(13)) summon_kin_type = 'L';
				else summon_kin_type = 's';
				break;
			case RACE_ZOMBIE:
				summon_kin_type = 'z';
				break;
			case RACE_VAMPIRE:
				summon_kin_type = 'V';
				break;
			case RACE_SPECTRE:
				summon_kin_type = 'G';
				break;
			case RACE_SPRITE:
				summon_kin_type = 'I';
				break;
			case RACE_ENT:
				summon_kin_type = '#';
				break;
			case RACE_ANGEL:
				summon_kin_type = 'A';
				break;
			case RACE_DEMON:
				summon_kin_type = 'U';
				break;
			default:
				summon_kin_type = 'p';
				break;
		}
		break;
	case MIMIC_DEMON:
		if (one_in_(13)) summon_kin_type = 'U';
		else summon_kin_type = 'u';
		break;
	case MIMIC_DEMON_LORD:
		summon_kin_type = 'U';
		break;
	case MIMIC_VAMPIRE:
		summon_kin_type = 'V';
		break;
	}	
	return summon_specific((pet ? -1 : 0), y, x, level, SUMMON_KIN, mode);
}
