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
				/* Get the cave grid */
				cave_type *c_ptr = &cave[y][x];

				/* Assume lit */
				c_ptr->info |= (CAVE_GLOW);

				/* Hack -- Memorize lit grids if allowed */
				if (view_perma_grids) c_ptr->info |= (CAVE_MARK);

				/* Hack -- Notice spot */
				note_spot(y, x);
			}
		}
	}

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS | PU_MON_LITE);

	p_ptr->redraw |= (PR_MAP);

	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	if (p_ptr->special_defense & NINJA_S_STEALTH)
	{
		if (cave[p_ptr->y][p_ptr->x].info & CAVE_GLOW) set_superstealth(FALSE);
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
				/* Get the cave grid */
				cave_type *c_ptr = &cave[y][x];

				/* Feature code (applying "mimic" field) */
				feature_type *f_ptr = &f_info[get_feat_mimic(c_ptr)];

				if (!is_mirror_grid(c_ptr) && !have_flag(f_ptr->flags, FF_QUEST_ENTER) &&
					!have_flag(f_ptr->flags, FF_ENTRANCE))
				{
					/* Assume dark */
					c_ptr->info &= ~(CAVE_GLOW);

					if (!have_flag(f_ptr->flags, FF_REMEMBER))
					{
						/* Forget the normal floor grid */
						c_ptr->info &= ~(CAVE_MARK);

						/* Hack -- Notice spot */
						note_spot(y, x);
					}
				}
			}

			/* Glow deep lava and building entrances */
			glow_deep_lava_and_bldg();
		}
	}

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS | PU_MON_LITE);

	p_ptr->redraw |= (PR_MAP);

	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	if (p_ptr->special_defense & NINJA_S_STEALTH)
	{
		if (cave[p_ptr->y][p_ptr->x].info & CAVE_GLOW) set_superstealth(FALSE);
	}

}
