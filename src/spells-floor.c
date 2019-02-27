#include "angband.h"
#include "spells-floor.h"

/*
 * Light up the dungeon using "clairvoyance"
 *
 * This function "illuminates" every grid in the dungeon, memorizes all
 * "objects", memorizes all grids as with magic mapping, and, under the
 * standard option settings (view_perma_grids but not view_torch_grids)
 * memorizes all floor grids too.
 *
 * Note that if "view_perma_grids" is not set, we do not memorize floor
 * grids, since this would defeat the purpose of "view_perma_grids", not
 * that anyone seems to play without this option.
 *
 * Note that if "view_torch_grids" is set, we do not memorize floor grids,
 * since this would prevent the use of "view_torch_grids" as a method to
 * keep track of what grids have been observed directly.
 */
void wiz_lite(bool ninja)
{
	OBJECT_IDX i;
	POSITION y, x;
	FEAT_IDX feat;
	feature_type *f_ptr;

	/* Memorize objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &current_floor_ptr->o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip held objects */
		if (o_ptr->held_m_idx) continue;

		/* Memorize */
		o_ptr->marked |= OM_FOUND;
	}

	/* Scan all normal grids */
	for (y = 1; y < current_floor_ptr->height - 1; y++)
	{
		/* Scan all normal grids */
		for (x = 1; x < current_floor_ptr->width - 1; x++)
		{
			grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];

			/* Memorize terrain of the grid */
			g_ptr->info |= (CAVE_KNOWN);

			/* Feature code (applying "mimic" field) */
			feat = get_feat_mimic(g_ptr);
			f_ptr = &f_info[feat];

			/* Process all non-walls */
			if (!have_flag(f_ptr->flags, FF_WALL))
			{
				/* Scan all neighbors */
				for (i = 0; i < 9; i++)
				{
					POSITION yy = y + ddy_ddd[i];
					POSITION xx = x + ddx_ddd[i];
					g_ptr = &current_floor_ptr->grid_array[yy][xx];

					/* Feature code (applying "mimic" field) */
					f_ptr = &f_info[get_feat_mimic(g_ptr)];

					/* Perma-lite the grid */
					if (!(d_info[p_ptr->dungeon_idx].flags1 & DF1_DARKNESS) && !ninja)
					{
						g_ptr->info |= (CAVE_GLOW);
					}

					/* Memorize normal features */
					if (have_flag(f_ptr->flags, FF_REMEMBER))
					{
						/* Memorize the grid */
						g_ptr->info |= (CAVE_MARK);
					}

					/* Perma-lit grids (newly and previously) */
					else if (g_ptr->info & CAVE_GLOW)
					{
						/* Normally, memorize floors (see above) */
						if (view_perma_grids && !view_torch_grids)
						{
							/* Memorize the grid */
							g_ptr->info |= (CAVE_MARK);
						}
					}
				}
			}
		}
	}

	p_ptr->update |= (PU_MONSTERS);
	p_ptr->redraw |= (PR_MAP);
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	if (p_ptr->special_defense & NINJA_S_STEALTH)
	{
		if (current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].info & CAVE_GLOW) set_superstealth(FALSE);
	}
}


/*
 * Forget the dungeon map (ala "Thinking of Maud...").
 */
void wiz_dark(void)
{
	OBJECT_IDX i;
	POSITION y, x;

	/* Forget every grid */
	for (y = 1; y < current_floor_ptr->height - 1; y++)
	{
		for (x = 1; x < current_floor_ptr->width - 1; x++)
		{
			grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];

			/* Process the grid */
			g_ptr->info &= ~(CAVE_MARK | CAVE_IN_DETECT | CAVE_KNOWN);
			g_ptr->info |= (CAVE_UNSAFE);
		}
	}

	/* Forget every grid on horizontal edge */
	for (x = 0; x < current_floor_ptr->width; x++)
	{
		current_floor_ptr->grid_array[0][x].info &= ~(CAVE_MARK);
		current_floor_ptr->grid_array[current_floor_ptr->height - 1][x].info &= ~(CAVE_MARK);
	}

	/* Forget every grid on vertical edge */
	for (y = 1; y < (current_floor_ptr->height - 1); y++)
	{
		current_floor_ptr->grid_array[y][0].info &= ~(CAVE_MARK);
		current_floor_ptr->grid_array[y][current_floor_ptr->width - 1].info &= ~(CAVE_MARK);
	}

	/* Forget all objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &current_floor_ptr->o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip held objects */
		if (o_ptr->held_m_idx) continue;

		/* Forget the object */
		o_ptr->marked &= OM_TOUCHED;
	}

	/* Forget travel route when we have forgotten map */
	forget_travel_flow();

	p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_MON_LITE);
	p_ptr->update |= (PU_MONSTERS);
	p_ptr->redraw |= (PR_MAP);
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
}
