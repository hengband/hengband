#include "angband.h"

void day_break()
{
	POSITION y, x;
	msg_print(_("夜が明けた。", "The sun has risen."));

	if (!p_ptr->wild_mode)
	{
		/* Hack -- Scan the town */
		for (y = 0; y < cur_hgt; y++)
		{
			for (x = 0; x < cur_wid; x++)
			{
				grid_type *g_ptr = &grid_array[y][x];

				/* Assume lit */
				g_ptr->info |= (CAVE_GLOW);

				/* Hack -- Memorize lit grids if allowed */
				if (view_perma_grids) g_ptr->info |= (CAVE_MARK);

				/* Hack -- Notice spot */
				note_spot(y, x);
			}
		}
	}

	p_ptr->update |= (PU_MONSTERS | PU_MON_LITE);
	p_ptr->redraw |= (PR_MAP);
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	if (p_ptr->special_defense & NINJA_S_STEALTH)
	{
		if (grid_array[p_ptr->y][p_ptr->x].info & CAVE_GLOW) set_superstealth(FALSE);
	}

}

void night_falls(void)
{
	POSITION y, x;
	msg_print(_("日が沈んだ。", "The sun has fallen."));

	if (!p_ptr->wild_mode)
	{
		/* Hack -- Scan the town */
		for (y = 0; y < cur_hgt; y++)
		{
			for (x = 0; x < cur_wid; x++)
			{
				grid_type *g_ptr = &grid_array[y][x];

				/* Feature code (applying "mimic" field) */
				feature_type *f_ptr = &f_info[get_feat_mimic(g_ptr)];

				if (!is_mirror_grid(g_ptr) && !have_flag(f_ptr->flags, FF_QUEST_ENTER) &&
					!have_flag(f_ptr->flags, FF_ENTRANCE))
				{
					/* Assume dark */
					g_ptr->info &= ~(CAVE_GLOW);

					if (!have_flag(f_ptr->flags, FF_REMEMBER))
					{
						/* Forget the normal floor grid */
						g_ptr->info &= ~(CAVE_MARK);

						/* Hack -- Notice spot */
						note_spot(y, x);
					}
				}
			}

			/* Glow deep lava and building entrances */
			glow_deep_lava_and_bldg();
		}
	}

	p_ptr->update |= (PU_MONSTERS | PU_MON_LITE);
	p_ptr->redraw |= (PR_MAP);
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	if (p_ptr->special_defense & NINJA_S_STEALTH)
	{
		if (grid_array[p_ptr->y][p_ptr->x].info & CAVE_GLOW) set_superstealth(FALSE);
	}

}

/*!
 * @brief 現在フロアに残っている敵モンスターの数を返す /
 * @return 現在の敵モンスターの数
 */
MONSTER_NUMBER count_all_hostile_monsters(void)
{
	POSITION x, y;
	MONSTER_NUMBER number_mon = 0;

	for (x = 0; x < cur_wid; ++x)
	{
		for (y = 0; y < cur_hgt; ++y)
		{
			MONSTER_IDX m_idx = grid_array[y][x].m_idx;

			if (m_idx > 0 && is_hostile(&m_list[m_idx]))
			{
				++number_mon;
			}
		}
	}

	return number_mon;
}

