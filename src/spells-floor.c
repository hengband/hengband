#include "angband.h"
#include "util.h"

#include "dungeon.h"
#include "floor.h"
#include "spells-floor.h"
#include "grid.h"
#include "quest.h"
#include "cmd-basic.h"
#include "floor-save.h"
#include "player-effects.h"
#include "feature.h"

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
	for (i = 1; i < current_floor_ptr->o_max; i++)
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
	for (i = 1; i < current_floor_ptr->o_max; i++)
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

/*!
 * @brief 守りのルーン設置処理 /
 * Leave a "glyph of warding" which prevents monster movement
 * @return 実際に設置が行われた場合TRUEを返す
 */
bool warding_glyph(void)
{
	if (!cave_clean_bold(p_ptr->y, p_ptr->x))
	{
		msg_print(_("床上のアイテムが呪文を跳ね返した。", "The object resists the spell."));
		return FALSE;
	}

	/* Create a glyph */
	current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].info |= CAVE_OBJECT;
	current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].mimic = feat_glyph;

	note_spot(p_ptr->y, p_ptr->x);
	lite_spot(p_ptr->y, p_ptr->x);

	return TRUE;
}


/*!
 * @brief 爆発のルーン設置処理 /
 * Leave an "explosive rune" which prevents monster movement
 * @return 実際に設置が行われた場合TRUEを返す
 */
bool explosive_rune(void)
{
	if (!cave_clean_bold(p_ptr->y, p_ptr->x))
	{
		msg_print(_("床上のアイテムが呪文を跳ね返した。", "The object resists the spell."));
		return FALSE;
	}

	/* Create a glyph */
	current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].info |= CAVE_OBJECT;
	current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].mimic = feat_explosive_rune;

	note_spot(p_ptr->y, p_ptr->x);
	lite_spot(p_ptr->y, p_ptr->x);

	return TRUE;
}


/*!
 * @brief 鏡設置処理
 * @return 実際に設置が行われた場合TRUEを返す
 */
bool place_mirror(void)
{
	if (!cave_clean_bold(p_ptr->y, p_ptr->x))
	{
		msg_print(_("床上のアイテムが呪文を跳ね返した。", "The object resists the spell."));
		return FALSE;
	}

	/* Create a mirror */
	current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].info |= CAVE_OBJECT;
	current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].mimic = feat_mirror;

	/* Turn on the light */
	current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].info |= CAVE_GLOW;

	note_spot(p_ptr->y, p_ptr->x);
	lite_spot(p_ptr->y, p_ptr->x);
	update_local_illumination(p_ptr->y, p_ptr->x);

	return TRUE;
}

/*!
 * @brief プレイヤーの手による能動的な階段生成処理 /
 * Create stairs at or move previously created stairs into the player location.
 * @return なし
 */
void stair_creation(void)
{
	saved_floor_type *sf_ptr;
	saved_floor_type *dest_sf_ptr;

	bool up = TRUE;
	bool down = TRUE;
	FLOOR_IDX dest_floor_id = 0;


	/* Forbid up staircases on Ironman mode */
	if (ironman_downward) up = FALSE;

	/* Forbid down staircases on quest level */
	if (quest_number(current_floor_ptr->dun_level) || (current_floor_ptr->dun_level >= d_info[p_ptr->dungeon_idx].maxdepth)) down = FALSE;

	/* No effect out of standard dungeon floor */
	if (!current_floor_ptr->dun_level || (!up && !down) ||
		(p_ptr->inside_quest && is_fixed_quest_idx(p_ptr->inside_quest)) ||
		p_ptr->inside_arena || p_ptr->inside_battle)
	{
		/* arena or quest */
		msg_print(_("効果がありません！", "There is no effect!"));
		return;
	}

	/* Artifacts resists */
	if (!cave_valid_bold(p_ptr->y, p_ptr->x))
	{
		msg_print(_("床上のアイテムが呪文を跳ね返した。", "The object resists the spell."));
		return;
	}

	/* Destroy all objects in the grid */
	delete_object(p_ptr->y, p_ptr->x);

	/* Extract current floor data */
	sf_ptr = get_sf_ptr(p_ptr->floor_id);
	if (!sf_ptr)
	{
		/* No floor id? -- Create now! */
		p_ptr->floor_id = get_new_floor_id();
		sf_ptr = get_sf_ptr(p_ptr->floor_id);
	}

	/* Choose randomly */
	if (up && down)
	{
		if (randint0(100) < 50) up = FALSE;
		else down = FALSE;
	}

	/* Destination is already fixed */
	if (up)
	{
		if (sf_ptr->upper_floor_id) dest_floor_id = sf_ptr->upper_floor_id;
	}
	else
	{
		if (sf_ptr->lower_floor_id) dest_floor_id = sf_ptr->lower_floor_id;
	}


	/* Search old stairs leading to the destination */
	if (dest_floor_id)
	{
		POSITION x, y;

		for (y = 0; y < current_floor_ptr->height; y++)
		{
			for (x = 0; x < current_floor_ptr->width; x++)
			{
				grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];

				if (!g_ptr->special) continue;
				if (feat_uses_special(g_ptr->feat)) continue;
				if (g_ptr->special != dest_floor_id) continue;

				/* Remove old stairs */
				g_ptr->special = 0;
				cave_set_feat(y, x, feat_ground_type[randint0(100)]);
			}
		}
	}

	/* No old destination -- Get new one now */
	else
	{
		dest_floor_id = get_new_floor_id();

		/* Fix it */
		if (up)
			sf_ptr->upper_floor_id = dest_floor_id;
		else
			sf_ptr->lower_floor_id = dest_floor_id;
	}

	/* Extract destination floor data */
	dest_sf_ptr = get_sf_ptr(dest_floor_id);


	/* Create a staircase */
	if (up)
	{
		cave_set_feat(p_ptr->y, p_ptr->x,
			(dest_sf_ptr->last_visit && (dest_sf_ptr->dun_level <= current_floor_ptr->dun_level - 2)) ?
			feat_state(feat_up_stair, FF_SHAFT) : feat_up_stair);
	}
	else
	{
		cave_set_feat(p_ptr->y, p_ptr->x,
			(dest_sf_ptr->last_visit && (dest_sf_ptr->dun_level >= current_floor_ptr->dun_level + 2)) ?
			feat_state(feat_down_stair, FF_SHAFT) : feat_down_stair);
	}


	/* Connect this stairs to the destination */
	current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].special = dest_floor_id;
}

/*
 * Hack -- map the current panel (plus some) ala "magic mapping"
 */
void map_area(POSITION range)
{
	int i;
	POSITION x, y;
	grid_type *g_ptr;
	FEAT_IDX feat;
	feature_type *f_ptr;

	if (d_info[p_ptr->dungeon_idx].flags1 & DF1_DARKNESS) range /= 3;

	/* Scan that area */
	for (y = 1; y < current_floor_ptr->height - 1; y++)
	{
		for (x = 1; x < current_floor_ptr->width - 1; x++)
		{
			if (distance(p_ptr->y, p_ptr->x, y, x) > range) continue;

			g_ptr = &current_floor_ptr->grid_array[y][x];

			/* Memorize terrain of the grid */
			g_ptr->info |= (CAVE_KNOWN);

			/* Feature code (applying "mimic" field) */
			feat = get_feat_mimic(g_ptr);
			f_ptr = &f_info[feat];

			/* All non-walls are "checked" */
			if (!have_flag(f_ptr->flags, FF_WALL))
			{
				/* Memorize normal features */
				if (have_flag(f_ptr->flags, FF_REMEMBER))
				{
					/* Memorize the object */
					g_ptr->info |= (CAVE_MARK);
				}

				/* Memorize known walls */
				for (i = 0; i < 8; i++)
				{
					g_ptr = &current_floor_ptr->grid_array[y + ddy_ddd[i]][x + ddx_ddd[i]];

					/* Feature code (applying "mimic" field) */
					feat = get_feat_mimic(g_ptr);
					f_ptr = &f_info[feat];

					/* Memorize walls (etc) */
					if (have_flag(f_ptr->flags, FF_REMEMBER))
					{
						/* Memorize the walls */
						g_ptr->info |= (CAVE_MARK);
					}
				}
			}
		}
	}

	p_ptr->redraw |= (PR_MAP);
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
}
