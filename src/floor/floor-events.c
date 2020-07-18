#include "floor/floor-events.h"
#include "cmd-io/cmd-dump.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/floor.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/disturbance-options.h"
#include "game-option/map-screen-options.h"
#include "grid/feature-flag-types.h"
#include "grid/grid.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-status.h"
#include "perception/object-perception.h"
#include "object-enchant/object-ego.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object/object-kind.h"
#include "object/object-mark-types.h"
#include "object/object-value.h"
#include "object-enchant/special-object-flags.h"
#include "player/special-defense-types.h"
#include "sv-definition/sv-amulet-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-ring-types.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

static bool mon_invis;
static POSITION mon_fy, mon_fx;

void day_break(player_type *subject_ptr)
{
	POSITION y, x;
	msg_print(_("夜が明けた。", "The sun has risen."));

	floor_type *floor_ptr = subject_ptr->current_floor_ptr;
	if (!subject_ptr->wild_mode)
	{
		/* Hack -- Scan the town */
		for (y = 0; y < floor_ptr->height; y++)
		{
			for (x = 0; x < floor_ptr->width; x++)
			{
				grid_type *g_ptr = &floor_ptr->grid_array[y][x];

				/* Assume lit */
				g_ptr->info |= (CAVE_GLOW);

				/* Hack -- Memorize lit grids if allowed */
				if (view_perma_grids) g_ptr->info |= (CAVE_MARK);

				note_spot(subject_ptr, y, x);
			}
		}
	}

	subject_ptr->update |= (PU_MONSTERS | PU_MON_LITE);
	subject_ptr->redraw |= (PR_MAP);
	subject_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	if (subject_ptr->special_defense & NINJA_S_STEALTH)
	{
		if (floor_ptr->grid_array[subject_ptr->y][subject_ptr->x].info & CAVE_GLOW) set_superstealth(subject_ptr, FALSE);
	}

}

void night_falls(player_type *subject_ptr)
{
	POSITION y, x;
	msg_print(_("日が沈んだ。", "The sun has fallen."));

	floor_type *floor_ptr = subject_ptr->current_floor_ptr;
	if (!subject_ptr->wild_mode)
	{
		/* Hack -- Scan the town */
		for (y = 0; y < floor_ptr->height; y++)
		{
			for (x = 0; x < floor_ptr->width; x++)
			{
				grid_type *g_ptr = &floor_ptr->grid_array[y][x];

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

						note_spot(subject_ptr, y, x);
					}
				}
			}

			glow_deep_lava_and_bldg(subject_ptr);
		}
	}

	subject_ptr->update |= (PU_MONSTERS | PU_MON_LITE);
	subject_ptr->redraw |= (PR_MAP);
	subject_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	if (subject_ptr->special_defense & NINJA_S_STEALTH)
	{
		if (floor_ptr->grid_array[subject_ptr->y][subject_ptr->x].info & CAVE_GLOW) set_superstealth(subject_ptr, FALSE);
	}

}

/*!
 * @brief 現在フロアに残っている敵モンスターの数を返す /
 * @return 現在の敵モンスターの数
 */
MONSTER_NUMBER count_all_hostile_monsters(floor_type *floor_ptr)
{
	MONSTER_NUMBER number_mon = 0;

	for (POSITION x = 0; x < floor_ptr->width; ++x)
	{
		for (POSITION y = 0; y < floor_ptr->height; ++y)
		{
			MONSTER_IDX m_idx = floor_ptr->grid_array[y][x].m_idx;

			if (m_idx > 0 && is_hostile(&floor_ptr->m_list[m_idx]))
			{
				++number_mon;
			}
		}
	}

	return number_mon;
}

/*!
 * ダンジョンの雰囲気を計算するための非線形基準値 / Dungeon rating is no longer linear
 */
#define RATING_BOOST(delta) (delta * delta + 50 * delta)

 /*!
  * @brief ダンジョンの雰囲気を算出する。
  * / Examine all monsters and unidentified objects, and get the feeling of current dungeon floor
  * @return 算出されたダンジョンの雰囲気ランク
  */
static byte get_dungeon_feeling(player_type *subject_ptr)
{
    floor_type *floor_ptr = subject_ptr->current_floor_ptr;
	if (!floor_ptr->dun_level) return 0;

	/* Examine each monster */
	const int base = 10;
	int rating = 0;
	for (MONSTER_IDX i = 1; i < floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &floor_ptr->m_list[i];
		monster_race *r_ptr;
		int delta = 0;
		if (!monster_is_valid(m_ptr)) continue;

		if (is_pet(m_ptr)) continue;

		r_ptr = &r_info[m_ptr->r_idx];

		if (r_ptr->flags1 & (RF1_UNIQUE))
		{
			/* Nearly out-of-depth unique monsters */
			if (r_ptr->level + 10 > floor_ptr->dun_level)
			{
				/* Boost rating by twice delta-depth */
				delta += (r_ptr->level + 10 - floor_ptr->dun_level) * 2 * base;
			}
		}
		else
		{
			/* Out-of-depth monsters */
			if (r_ptr->level > floor_ptr->dun_level)
			{
				/* Boost rating by delta-depth */
				delta += (r_ptr->level - floor_ptr->dun_level) * base;
			}
		}

		/* Unusually crowded monsters get a little bit of rating boost */
		if (r_ptr->flags1 & RF1_FRIENDS)
		{
			if (5 <= get_monster_crowd_number(floor_ptr, i)) delta += 1;
		}
		else
		{
			if (2 <= get_monster_crowd_number(floor_ptr, i)) delta += 1;
		}


		rating += RATING_BOOST(delta);
	}

	/* Examine each unidentified object */
	for (MONSTER_IDX i = 1; i < floor_ptr->o_max; i++)
	{
		object_type *o_ptr = &floor_ptr->o_list[i];
		object_kind *k_ptr = &k_info[o_ptr->k_idx];
		int delta = 0;

		if (!object_is_valid(o_ptr)) continue;

		/* Skip known objects */
		if (object_is_known(o_ptr))
		{
			/* Touched? */
			if (o_ptr->marked & OM_TOUCHED) continue;
		}

		/* Skip pseudo-known objects */
		if (o_ptr->ident & IDENT_SENSE) continue;

		/* Ego objects */
		if (object_is_ego(o_ptr))
		{
			ego_item_type *e_ptr = &e_info[o_ptr->name2];

			delta += e_ptr->rating * base;
		}

		/* Artifacts */
		if (object_is_artifact(o_ptr))
		{
			PRICE cost = object_value_real(subject_ptr, o_ptr);

			delta += 10 * base;
			if (cost > 10000L) delta += 10 * base;
			if (cost > 50000L) delta += 10 * base;
			if (cost > 100000L) delta += 10 * base;

			/* Special feeling */
			if (!preserve_mode) return 1;
		}

		if (o_ptr->tval == TV_DRAG_ARMOR) delta += 30 * base;
		if (o_ptr->tval == TV_SHIELD && o_ptr->sval == SV_DRAGON_SHIELD) delta += 5 * base;
		if (o_ptr->tval == TV_GLOVES && o_ptr->sval == SV_SET_OF_DRAGON_GLOVES) delta += 5 * base;
		if (o_ptr->tval == TV_BOOTS && o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE) delta += 5 * base;
		if (o_ptr->tval == TV_HELM && o_ptr->sval == SV_DRAGON_HELM) delta += 5 * base;
		if (o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_SPEED && !object_is_cursed(o_ptr)) delta += 25 * base;
		if (o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_LORDLY && !object_is_cursed(o_ptr)) delta += 15 * base;
		if (o_ptr->tval == TV_AMULET && o_ptr->sval == SV_AMULET_THE_MAGI && !object_is_cursed(o_ptr)) delta += 15 * base;

		/* Out-of-depth objects */
		if (!object_is_cursed(o_ptr) && !object_is_broken(o_ptr) && k_ptr->level > floor_ptr->dun_level)
		{
			/* Rating increase */
			delta += (k_ptr->level - floor_ptr->dun_level) * base;
		}

		rating += RATING_BOOST(delta);
	}


	if (rating > RATING_BOOST(1000)) return 2;
	if (rating > RATING_BOOST(800)) return 3;
	if (rating > RATING_BOOST(600)) return 4;
	if (rating > RATING_BOOST(400)) return 5;
	if (rating > RATING_BOOST(300)) return 6;
	if (rating > RATING_BOOST(200)) return 7;
	if (rating > RATING_BOOST(100)) return 8;
	if (rating > RATING_BOOST(0)) return 9;
	return 10;
}

/*!
 * @brief ダンジョンの雰囲気を更新し、変化があった場合メッセージを表示する
 * / Update dungeon feeling, and announce it if changed
 * @return なし
 */
void update_dungeon_feeling(player_type *subject_ptr)
{
	/* No feeling on the surface */
	floor_type *floor_ptr = subject_ptr->current_floor_ptr;
	if (!floor_ptr->dun_level) return;

	/* No feeling in the arena */
	if (subject_ptr->phase_out) return;

	/* Extract delay time */
	int delay = MAX(10, 150 - subject_ptr->skill_fos) * (150 - floor_ptr->dun_level) * TURNS_PER_TICK / 100;

	/* Not yet felt anything */
	if (current_world_ptr->game_turn < subject_ptr->feeling_turn + delay && !cheat_xtra) return;

	/* Extract quest number (if any) */
	int quest_num = quest_number(subject_ptr, floor_ptr->dun_level);

	/* No feeling in a quest */
	if (quest_num &&
		(is_fixed_quest_idx(quest_num) &&
			!((quest_num == QUEST_OBERON) || (quest_num == QUEST_SERPENT) ||
				!(quest[quest_num].flags & QUEST_FLAG_PRESET)))) return;


	/* Get new dungeon feeling */
	byte new_feeling = get_dungeon_feeling(subject_ptr);

	/* Remember last time updated */
	subject_ptr->feeling_turn = current_world_ptr->game_turn;

	/* No change */
	if (subject_ptr->feeling == new_feeling) return;

	/* Dungeon feeling is changed */
	subject_ptr->feeling = new_feeling;

	/* Announce feeling */
	do_cmd_feeling(subject_ptr);

	select_floor_music(subject_ptr);

	/* Update the level indicator */
	subject_ptr->redraw |= (PR_DEPTH);

	if (disturb_minor) disturb(subject_ptr, FALSE, FALSE);
}


/*
 * Glow deep lava and building entrances in the floor
 */
void glow_deep_lava_and_bldg(player_type *subject_ptr)
{
	/* Not in the darkness dungeon */
	if (d_info[subject_ptr->dungeon_idx].flags1 & DF1_DARKNESS) return;

	floor_type *floor_ptr = subject_ptr->current_floor_ptr;
	for (POSITION y = 0; y < floor_ptr->height; y++)
	{
		for (POSITION x = 0; x < floor_ptr->width; x++)
		{
			grid_type *g_ptr;
			g_ptr = &floor_ptr->grid_array[y][x];

			if (!have_flag(f_info[get_feat_mimic(g_ptr)].flags, FF_GLOW))
				continue;

			for (DIRECTION i = 0; i < 9; i++)
			{
				POSITION yy = y + ddy_ddd[i];
				POSITION xx = x + ddx_ddd[i];
				if (!in_bounds2(floor_ptr, yy, xx)) continue;
				floor_ptr->grid_array[yy][xx].info |= CAVE_GLOW;
			}
		}
	}

	subject_ptr->update |= (PU_VIEW | PU_LITE | PU_MON_LITE);
	subject_ptr->redraw |= (PR_MAP);
}


/*
 * Actually erase the entire "lite" array, redrawing every grid
 */
void forget_lite(floor_type *floor_ptr)
{
	/* None to forget */
	if (!floor_ptr->lite_n) return;

	/* Clear them all */
	for (int i = 0; i < floor_ptr->lite_n; i++)
	{
		POSITION y = floor_ptr->lite_y[i];
		POSITION x = floor_ptr->lite_x[i];

		/* Forget "LITE" flag */
		floor_ptr->grid_array[y][x].info &= ~(CAVE_LITE);
	}

	floor_ptr->lite_n = 0;
}


/*
 * Update the set of grids "illuminated" by the player's lite.
 *
 * This routine needs to use the results of "update_view()"
 *
 * Note that "blindness" does NOT affect "torch lite".  Be careful!
 *
 * We optimize most lites (all non-artifact lites) by using "obvious"
 * facts about the results of "small" lite radius, and we attempt to
 * list the "nearby" grids before the more "distant" ones in the
 * array of torch-lit grids.
 *
 * We assume that "radius zero" lite is in fact no lite at all.
 *
 *     Torch     Lantern     Artifacts
 *     (etc)
 *                              ***
 *                 ***         *****
 *      ***       *****       *******
 *      *@*       **@**       ***@***
 *      ***       *****       *******
 *                 ***         *****
 *                              ***
 */
void update_lite(player_type *subject_ptr)
{
	POSITION p = subject_ptr->cur_lite;
	grid_type *g_ptr;

	/*** Save the old "lite" grids for later ***/

	/* Clear them all */
	floor_type *floor_ptr = subject_ptr->current_floor_ptr;
	for (int i = 0; i < floor_ptr->lite_n; i++)
	{
		POSITION y = floor_ptr->lite_y[i];
		POSITION x = floor_ptr->lite_x[i];

		/* Mark the grid as not "lite" */
		floor_ptr->grid_array[y][x].info &= ~(CAVE_LITE);

		/* Mark the grid as "seen" */
		floor_ptr->grid_array[y][x].info |= (CAVE_TEMP);

		/* Add it to the "seen" set */
		tmp_pos.y[tmp_pos.n] = y;
		tmp_pos.x[tmp_pos.n] = x;
		tmp_pos.n++;
	}

	/* None left */
	floor_ptr->lite_n = 0;


	/*** Collect the new "lite" grids ***/

	/* Radius 1 -- torch radius */
	if (p >= 1)
	{
		/* Player grid */
		cave_lite_hack(floor_ptr, subject_ptr->y, subject_ptr->x);

		/* Adjacent grid */
		cave_lite_hack(floor_ptr, subject_ptr->y + 1, subject_ptr->x);
		cave_lite_hack(floor_ptr, subject_ptr->y - 1, subject_ptr->x);
		cave_lite_hack(floor_ptr, subject_ptr->y, subject_ptr->x + 1);
		cave_lite_hack(floor_ptr, subject_ptr->y, subject_ptr->x - 1);

		/* Diagonal grids */
		cave_lite_hack(floor_ptr, subject_ptr->y + 1, subject_ptr->x + 1);
		cave_lite_hack(floor_ptr, subject_ptr->y + 1, subject_ptr->x - 1);
		cave_lite_hack(floor_ptr, subject_ptr->y - 1, subject_ptr->x + 1);
		cave_lite_hack(floor_ptr, subject_ptr->y - 1, subject_ptr->x - 1);
	}

	/* Radius 2 -- lantern radius */
	if (p >= 2)
	{
		/* South of the player */
		if (cave_los_bold(floor_ptr, subject_ptr->y + 1, subject_ptr->x))
		{
			cave_lite_hack(floor_ptr, subject_ptr->y + 2, subject_ptr->x);
			cave_lite_hack(floor_ptr, subject_ptr->y + 2, subject_ptr->x + 1);
			cave_lite_hack(floor_ptr, subject_ptr->y + 2, subject_ptr->x - 1);
		}

		/* North of the player */
		if (cave_los_bold(floor_ptr, subject_ptr->y - 1, subject_ptr->x))
		{
			cave_lite_hack(floor_ptr, subject_ptr->y - 2, subject_ptr->x);
			cave_lite_hack(floor_ptr, subject_ptr->y - 2, subject_ptr->x + 1);
			cave_lite_hack(floor_ptr, subject_ptr->y - 2, subject_ptr->x - 1);
		}

		/* East of the player */
		if (cave_los_bold(floor_ptr, subject_ptr->y, subject_ptr->x + 1))
		{
			cave_lite_hack(floor_ptr, subject_ptr->y, subject_ptr->x + 2);
			cave_lite_hack(floor_ptr, subject_ptr->y + 1, subject_ptr->x + 2);
			cave_lite_hack(floor_ptr, subject_ptr->y - 1, subject_ptr->x + 2);
		}

		/* West of the player */
		if (cave_los_bold(floor_ptr, subject_ptr->y, subject_ptr->x - 1))
		{
			cave_lite_hack(floor_ptr, subject_ptr->y, subject_ptr->x - 2);
			cave_lite_hack(floor_ptr, subject_ptr->y + 1, subject_ptr->x - 2);
			cave_lite_hack(floor_ptr, subject_ptr->y - 1, subject_ptr->x - 2);
		}
	}

	/* Radius 3+ -- artifact radius */
	if (p >= 3)
	{
		int d;

		/* Paranoia -- see "LITE_MAX" */
		if (p > 14) p = 14;

		/* South-East of the player */
		if (cave_los_bold(floor_ptr, subject_ptr->y + 1, subject_ptr->x + 1))
		{
			cave_lite_hack(floor_ptr, subject_ptr->y + 2, subject_ptr->x + 2);
		}

		/* South-West of the player */
		if (cave_los_bold(floor_ptr, subject_ptr->y + 1, subject_ptr->x - 1))
		{
			cave_lite_hack(floor_ptr, subject_ptr->y + 2, subject_ptr->x - 2);
		}

		/* North-East of the player */
		if (cave_los_bold(floor_ptr, subject_ptr->y - 1, subject_ptr->x + 1))
		{
			cave_lite_hack(floor_ptr, subject_ptr->y - 2, subject_ptr->x + 2);
		}

		/* North-West of the player */
		if (cave_los_bold(floor_ptr, subject_ptr->y - 1, subject_ptr->x - 1))
		{
			cave_lite_hack(floor_ptr, subject_ptr->y - 2, subject_ptr->x - 2);
		}

		/* Maximal north */
		POSITION min_y = subject_ptr->y - p;
		if (min_y < 0) min_y = 0;

		/* Maximal south */
		POSITION max_y = subject_ptr->y + p;
		if (max_y > floor_ptr->height - 1) max_y = floor_ptr->height - 1;

		/* Maximal west */
		POSITION min_x = subject_ptr->x - p;
		if (min_x < 0) min_x = 0;

		/* Maximal east */
		POSITION max_x = subject_ptr->x + p;
		if (max_x > floor_ptr->width - 1) max_x = floor_ptr->width - 1;

		/* Scan the maximal box */
		for (POSITION y = min_y; y <= max_y; y++)
		{
			for (POSITION x = min_x; x <= max_x; x++)
			{
				int dy = (subject_ptr->y > y) ? (subject_ptr->y - y) : (y - subject_ptr->y);
				int dx = (subject_ptr->x > x) ? (subject_ptr->x - x) : (x - subject_ptr->x);

				/* Skip the "central" grids (above) */
				if ((dy <= 2) && (dx <= 2)) continue;

				/* Hack -- approximate the distance */
				d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));

				/* Skip distant grids */
				if (d > p) continue;

				/* Viewable, nearby, grids get "torch lit" */
				if (floor_ptr->grid_array[y][x].info & CAVE_VIEW)
				{
					/* This grid is "torch lit" */
					cave_lite_hack(floor_ptr, y, x);
				}
			}
		}
	}


	/*** Complete the algorithm ***/

	/* Draw the new grids */
	for (int i = 0; i < floor_ptr->lite_n; i++)
	{
		POSITION y = floor_ptr->lite_y[i];
		POSITION x = floor_ptr->lite_x[i];

		g_ptr = &floor_ptr->grid_array[y][x];

		/* Update fresh grids */
		if (g_ptr->info & (CAVE_TEMP)) continue;

		/* Add it to later visual update */
		cave_note_and_redraw_later(floor_ptr, g_ptr, y, x);
	}

	/* Clear them all */
	for (int i = 0; i < tmp_pos.n; i++)
	{
		POSITION y = tmp_pos.y[i];
		POSITION x = tmp_pos.x[i];

		g_ptr = &floor_ptr->grid_array[y][x];

		/* No longer in the array */
		g_ptr->info &= ~(CAVE_TEMP);

		/* Update stale grids */
		if (g_ptr->info & (CAVE_LITE)) continue;

		/* Add it to later visual update */
		cave_redraw_later(floor_ptr, g_ptr, y, x);
	}

	tmp_pos.n = 0;
	subject_ptr->update |= (PU_DELAY_VIS);
}


/*
 * Clear the viewable space
 */
void forget_view(floor_type *floor_ptr)
{
	/* None to forget */
	if (!floor_ptr->view_n) return;

	/* Clear them all */
	for (int i = 0; i < floor_ptr->view_n; i++)
	{
		POSITION y = floor_ptr->view_y[i];
		POSITION x = floor_ptr->view_x[i];
		grid_type *g_ptr;
		g_ptr = &floor_ptr->grid_array[y][x];

		/* Forget that the grid is viewable */
		g_ptr->info &= ~(CAVE_VIEW);
	}

	floor_ptr->view_n = 0;
}


/*
 * Helper function for "update_view()" below
 *
 * We are checking the "viewability" of grid (y,x) by the player.
 *
 * This function assumes that (y,x) is legal (i.e. on the map).
 *
 * Grid (y1,x1) is on the "diagonal" between (subject_ptr->y,subject_ptr->x) and (y,x)
 * Grid (y2,x2) is "adjacent", also between (subject_ptr->y,subject_ptr->x) and (y,x).
 *
 * Note that we are using the "CAVE_XTRA" field for marking grids as
 * "easily viewable".  This bit is cleared at the end of "update_view()".
 *
 * This function adds (y,x) to the "viewable set" if necessary.
 *
 * This function now returns "TRUE" if vision is "blocked" by grid (y,x).
 */
static bool update_view_aux(player_type *subject_ptr, POSITION y, POSITION x, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
	floor_type *floor_ptr = subject_ptr->current_floor_ptr;
	grid_type *g1_c_ptr;
	grid_type *g2_c_ptr;
	g1_c_ptr = &floor_ptr->grid_array[y1][x1];
	g2_c_ptr = &floor_ptr->grid_array[y2][x2];

	/* Check for walls */
	bool f1 = (cave_los_grid(g1_c_ptr));
	bool f2 = (cave_los_grid(g2_c_ptr));

	/* Totally blocked by physical walls */
	if (!f1 && !f2) return TRUE;

	/* Check for visibility */
	bool v1 = (f1 && (g1_c_ptr->info & (CAVE_VIEW)));
	bool v2 = (f2 && (g2_c_ptr->info & (CAVE_VIEW)));

	/* Totally blocked by "unviewable neighbors" */
	if (!v1 && !v2) return TRUE;

	grid_type *g_ptr;
	g_ptr = &floor_ptr->grid_array[y][x];

	/* Check for walls */
	bool wall = (!cave_los_grid(g_ptr));

	/* Check the "ease" of visibility */
	bool z1 = (v1 && (g1_c_ptr->info & (CAVE_XTRA)));
	bool z2 = (v2 && (g2_c_ptr->info & (CAVE_XTRA)));

	/* Hack -- "easy" plus "easy" yields "easy" */
	if (z1 && z2)
	{
		g_ptr->info |= (CAVE_XTRA);
		cave_view_hack(floor_ptr, g_ptr, y, x);
		return wall;
	}

	/* Hack -- primary "easy" yields "viewed" */
	if (z1)
	{
		cave_view_hack(floor_ptr, g_ptr, y, x);
		return wall;
	}

	/* Hack -- "view" plus "view" yields "view" */
	if (v1 && v2)
	{
		cave_view_hack(floor_ptr, g_ptr, y, x);
		return wall;
	}

	/* Mega-Hack -- the "los()" function works poorly on walls */
	if (wall)
	{
		cave_view_hack(floor_ptr, g_ptr, y, x);
		return wall;
	}

	/* Hack -- check line of sight */
	if (los(subject_ptr, subject_ptr->y, subject_ptr->x, y, x))
	{
		cave_view_hack(floor_ptr, g_ptr, y, x);
		return wall;
	}

	return TRUE;
}


/*
 * Calculate the viewable space
 *
 *  1: Process the player
 *  1a: The player is always (easily) viewable
 *  2: Process the diagonals
 *  2a: The diagonals are (easily) viewable up to the first wall
 *  2b: But never go more than 2/3 of the "full" distance
 *  3: Process the main axes
 *  3a: The main axes are (easily) viewable up to the first wall
 *  3b: But never go more than the "full" distance
 *  4: Process sequential "strips" in each of the eight octants
 *  4a: Each strip runs along the previous strip
 *  4b: The main axes are "previous" to the first strip
 *  4c: Process both "sides" of each "direction" of each strip
 *  4c1: Each side aborts as soon as possible
 *  4c2: Each side tells the next strip how far it has to check
 *
 * Note that the octant processing involves some pretty interesting
 * observations involving when a grid might possibly be viewable from
 * a given grid, and on the order in which the strips are processed.
 *
 * Note the use of the mathematical facts shown below, which derive
 * from the fact that (1 < sqrt(2) < 1.5), and that the length of the
 * hypotenuse of a right triangle is primarily determined by the length
 * of the longest side, when one side is small, and is strictly less
 * than one-and-a-half times as long as the longest side when both of
 * the sides are large.
 *
 *   if (manhatten(dy,dx) < R) then (hypot(dy,dx) < R)
 *   if (manhatten(dy,dx) > R*3/2) then (hypot(dy,dx) > R)
 *
 *   hypot(dy,dx) is approximated by (dx+dy+MAX(dx,dy)) / 2
 *
 * These observations are important because the calculation of the actual
 * value of "hypot(dx,dy)" is extremely expensive, involving square roots,
 * while for small values (up to about 20 or so), the approximations above
 * are correct to within an error of at most one grid or so.
 *
 * Observe the use of "full" and "over" in the code below, and the use of
 * the specialized calculation involving "limit", all of which derive from
 * the observations given above.  Basically, we note that the "circle" of
 * view is completely contained in an "octagon" whose bounds are easy to
 * determine, and that only a few steps are needed to derive the actual
 * bounds of the circle given the bounds of the octagon.
 *
 * Note that by skipping all the grids in the corners of the octagon, we
 * place an upper limit on the number of grids in the field of view, given
 * that "full" is never more than 20.  Of the 1681 grids in the "square" of
 * view, only about 1475 of these are in the "octagon" of view, and even
 * fewer are in the "circle" of view, so 1500 or 1536 is more than enough
 * entries to completely contain the actual field of view.
 *
 * Note also the care taken to prevent "running off the map".  The use of
 * explicit checks on the "validity" of the "diagonal", and the fact that
 * the loops are never allowed to "leave" the map, lets "update_view_aux()"
 * use the optimized "cave_los_bold()" macro, and to avoid the overhead
 * of multiple checks on the validity of grids.
 *
 * Note the "optimizations" involving the "se","sw","ne","nw","es","en",
 * "ws","wn" variables.  They work like this: While travelling down the
 * south-bound strip just to the east of the main south axis, as soon as
 * we get to a grid which does not "transmit" viewing, if all of the strips
 * preceding us (in this case, just the main axis) had terminated at or before
 * the same point, then we can stop, and reset the "max distance" to ourself.
 * So, each strip (named by major axis plus offset, thus "se" in this case)
 * maintains a "blockage" variable, initialized during the main axis step,
 * and checks it whenever a blockage is observed.  After processing each
 * strip as far as the previous strip told us to process, the next strip is
 * told not to go farther than the current strip's farthest viewable grid,
 * unless open space is still available.  This uses the "k" variable.
 *
 * Note the use of "inline" macros for efficiency.  The "cave_los_grid()"
 * macro is a replacement for "cave_los_bold()" which takes a pointer to
 * a grid instead of its location.  The "cave_view_hack()" macro is a
 * chunk of code which adds the given location to the "view" array if it
 * is not already there, using both the actual location and a pointer to
 * the grid.  See above.
 *
 * By the way, the purpose of this code is to reduce the dependancy on the
 * "los()" function which is slow, and, in some cases, not very accurate.
 *
 * It is very possible that I am the only person who fully understands this
 * function, and for that I am truly sorry, but efficiency was very important
 * and the "simple" version of this function was just not fast enough.  I am
 * more than willing to replace this function with a simpler one, if it is
 * equally efficient, and especially willing if the new function happens to
 * derive "reverse-line-of-sight" at the same time, since currently monsters
 * just use an optimized hack of "you see me, so I see you", and then use the
 * actual "projectable()" function to check spell attacks.
 */
void update_view(player_type *subject_ptr)
{
	int n, m, d, k, z;
	POSITION y, x;

	int se, sw, ne, nw, es, en, ws, wn;

	int full, over;

	floor_type *floor_ptr = subject_ptr->current_floor_ptr;
	POSITION y_max = floor_ptr->height - 1;
	POSITION x_max = floor_ptr->width - 1;

	grid_type *g_ptr;

	/*** Initialize ***/

	/* Optimize */
	if (view_reduce_view && !floor_ptr->dun_level)
	{
		/* Full radius (10) */
		full = MAX_SIGHT / 2;

		/* Octagon factor (15) */
		over = MAX_SIGHT * 3 / 4;
	}

	/* Normal */
	else
	{
		/* Full radius (20) */
		full = MAX_SIGHT;

		/* Octagon factor (30) */
		over = MAX_SIGHT * 3 / 2;
	}


	/*** Step 0 -- Begin ***/

	/* Save the old "view" grids for later */
	for (n = 0; n < floor_ptr->view_n; n++)
	{
		y = floor_ptr->view_y[n];
		x = floor_ptr->view_x[n];
		g_ptr = &floor_ptr->grid_array[y][x];

		/* Mark the grid as not in "view" */
		g_ptr->info &= ~(CAVE_VIEW);

		/* Mark the grid as "seen" */
		g_ptr->info |= (CAVE_TEMP);

		/* Add it to the "seen" set */
		tmp_pos.y[tmp_pos.n] = y;
		tmp_pos.x[tmp_pos.n] = x;
		tmp_pos.n++;
	}

	/* Start over with the "view" array */
	floor_ptr->view_n = 0;

	/*** Step 1 -- adjacent grids ***/

	/* Now start on the player */
	y = subject_ptr->y;
	x = subject_ptr->x;
	g_ptr = &floor_ptr->grid_array[y][x];

	/* Assume the player grid is easily viewable */
	g_ptr->info |= (CAVE_XTRA);

	/* Assume the player grid is viewable */
	cave_view_hack(floor_ptr, g_ptr, y, x);


	/*** Step 2 -- Major Diagonals ***/

	/* Hack -- Limit */
	z = full * 2 / 3;

	/* Scan south-east */
	for (d = 1; d <= z; d++)
	{
		g_ptr = &floor_ptr->grid_array[y + d][x + d];
		g_ptr->info |= (CAVE_XTRA);
		cave_view_hack(floor_ptr, g_ptr, y + d, x + d);
		if (!cave_los_grid(g_ptr)) break;
	}

	/* Scan south-west */
	for (d = 1; d <= z; d++)
	{
		g_ptr = &floor_ptr->grid_array[y + d][x - d];
		g_ptr->info |= (CAVE_XTRA);
		cave_view_hack(floor_ptr, g_ptr, y + d, x - d);
		if (!cave_los_grid(g_ptr)) break;
	}

	/* Scan north-east */
	for (d = 1; d <= z; d++)
	{
		g_ptr = &floor_ptr->grid_array[y - d][x + d];
		g_ptr->info |= (CAVE_XTRA);
		cave_view_hack(floor_ptr, g_ptr, y - d, x + d);
		if (!cave_los_grid(g_ptr)) break;
	}

	/* Scan north-west */
	for (d = 1; d <= z; d++)
	{
		g_ptr = &floor_ptr->grid_array[y - d][x - d];
		g_ptr->info |= (CAVE_XTRA);
		cave_view_hack(floor_ptr, g_ptr, y - d, x - d);
		if (!cave_los_grid(g_ptr)) break;
	}

	/*** Step 3 -- major axes ***/

	/* Scan south */
	for (d = 1; d <= full; d++)
	{
		g_ptr = &floor_ptr->grid_array[y + d][x];
		g_ptr->info |= (CAVE_XTRA);
		cave_view_hack(floor_ptr, g_ptr, y + d, x);
		if (!cave_los_grid(g_ptr)) break;
	}

	/* Initialize the "south strips" */
	se = sw = d;

	/* Scan north */
	for (d = 1; d <= full; d++)
	{
		g_ptr = &floor_ptr->grid_array[y - d][x];
		g_ptr->info |= (CAVE_XTRA);
		cave_view_hack(floor_ptr, g_ptr, y - d, x);
		if (!cave_los_grid(g_ptr)) break;
	}

	/* Initialize the "north strips" */
	ne = nw = d;

	/* Scan east */
	for (d = 1; d <= full; d++)
	{
		g_ptr = &floor_ptr->grid_array[y][x + d];
		g_ptr->info |= (CAVE_XTRA);
		cave_view_hack(floor_ptr, g_ptr, y, x + d);
		if (!cave_los_grid(g_ptr)) break;
	}

	/* Initialize the "east strips" */
	es = en = d;

	/* Scan west */
	for (d = 1; d <= full; d++)
	{
		g_ptr = &floor_ptr->grid_array[y][x - d];
		g_ptr->info |= (CAVE_XTRA);
		cave_view_hack(floor_ptr, g_ptr, y, x - d);
		if (!cave_los_grid(g_ptr)) break;
	}

	/* Initialize the "west strips" */
	ws = wn = d;


	/*** Step 4 -- Divide each "octant" into "strips" ***/

	/* Now check each "diagonal" (in parallel) */
	for (n = 1; n <= over / 2; n++)
	{
		POSITION ypn, ymn, xpn, xmn;

		/* Acquire the "bounds" of the maximal circle */
		z = over - n - n;
		if (z > full - n) z = full - n;
		while ((z + n + (n >> 1)) > full) z--;

		/* Access the four diagonal grids */
		ypn = y + n;
		ymn = y - n;
		xpn = x + n;
		xmn = x - n;

		/* South strip */
		if (ypn < y_max)
		{
			/* Maximum distance */
			m = MIN(z, y_max - ypn);

			/* East side */
			if ((xpn <= x_max) && (n < se))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(subject_ptr, ypn + d, xpn, ypn + d - 1, xpn - 1, ypn + d - 1, xpn))
					{
						if (n + d >= se) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				se = k + 1;
			}

			/* West side */
			if ((xmn >= 0) && (n < sw))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(subject_ptr, ypn + d, xmn, ypn + d - 1, xmn + 1, ypn + d - 1, xmn))
					{
						if (n + d >= sw) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				sw = k + 1;
			}
		}

		/* North strip */
		if (ymn > 0)
		{
			/* Maximum distance */
			m = MIN(z, ymn);

			/* East side */
			if ((xpn <= x_max) && (n < ne))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(subject_ptr, ymn - d, xpn, ymn - d + 1, xpn - 1, ymn - d + 1, xpn))
					{
						if (n + d >= ne) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				ne = k + 1;
			}

			/* West side */
			if ((xmn >= 0) && (n < nw))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(subject_ptr, ymn - d, xmn, ymn - d + 1, xmn + 1, ymn - d + 1, xmn))
					{
						if (n + d >= nw) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				nw = k + 1;
			}
		}

		/* East strip */
		if (xpn < x_max)
		{
			/* Maximum distance */
			m = MIN(z, x_max - xpn);

			/* South side */
			if ((ypn <= x_max) && (n < es))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(subject_ptr, ypn, xpn + d, ypn - 1, xpn + d - 1, ypn, xpn + d - 1))
					{
						if (n + d >= es) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				es = k + 1;
			}

			/* North side */
			if ((ymn >= 0) && (n < en))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(subject_ptr, ymn, xpn + d, ymn + 1, xpn + d - 1, ymn, xpn + d - 1))
					{
						if (n + d >= en) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				en = k + 1;
			}
		}

		/* West strip */
		if (xmn > 0)
		{
			/* Maximum distance */
			m = MIN(z, xmn);

			/* South side */
			if ((ypn <= y_max) && (n < ws))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(subject_ptr, ypn, xmn - d, ypn - 1, xmn - d + 1, ypn, xmn - d + 1))
					{
						if (n + d >= ws) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				ws = k + 1;
			}

			/* North side */
			if ((ymn >= 0) && (n < wn))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(subject_ptr, ymn, xmn - d, ymn + 1, xmn - d + 1, ymn, xmn - d + 1))
					{
						if (n + d >= wn) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				wn = k + 1;
			}
		}
	}

	/*** Step 5 -- Complete the algorithm ***/

	/* Update all the new grids */
	for (n = 0; n < floor_ptr->view_n; n++)
	{
		y = floor_ptr->view_y[n];
		x = floor_ptr->view_x[n];
		g_ptr = &floor_ptr->grid_array[y][x];

		/* Clear the "CAVE_XTRA" flag */
		g_ptr->info &= ~(CAVE_XTRA);

		/* Update only newly viewed grids */
		if (g_ptr->info & (CAVE_TEMP)) continue;

		/* Add it to later visual update */
		cave_note_and_redraw_later(floor_ptr, g_ptr, y, x);
	}

	/* Wipe the old grids, update as needed */
	for (n = 0; n < tmp_pos.n; n++)
	{
		y = tmp_pos.y[n];
		x = tmp_pos.x[n];
		g_ptr = &floor_ptr->grid_array[y][x];

		/* No longer in the array */
		g_ptr->info &= ~(CAVE_TEMP);

		/* Update only non-viewable grids */
		if (g_ptr->info & (CAVE_VIEW)) continue;

		/* Add it to later visual update */
		cave_redraw_later(floor_ptr, g_ptr, y, x);
	}

	tmp_pos.n = 0;
	subject_ptr->update |= (PU_DELAY_VIS);
}


/*!
 * @brief モンスターによる光量状態更新 / Add a square to the changes array
 * @param subject_ptr 主観となるクリーチャーの参照ポインタ
 * @param y Y座標
 * @param x X座標
 */
static void mon_lite_hack(player_type *subject_ptr, POSITION y, POSITION x)
{
	grid_type *g_ptr;
	int dpf, d;
	POSITION midpoint;

	/* We trust this grid is in bounds */
	/* if (!in_bounds2(y, x)) return; */

	g_ptr = &subject_ptr->current_floor_ptr->grid_array[y][x];

	/* Want a unlit square in view of the player */
	if ((g_ptr->info & (CAVE_MNLT | CAVE_VIEW)) != CAVE_VIEW) return;

	if (!cave_los_grid(g_ptr))
	{
		/* Hack -- Prevent monster lite leakage in walls */

		/* Horizontal walls between player and a monster */
		if (((y < subject_ptr->y) && (y > mon_fy)) || ((y > subject_ptr->y) && (y < mon_fy)))
		{
			dpf = subject_ptr->y - mon_fy;
			d = y - mon_fy;
			midpoint = mon_fx + ((subject_ptr->x - mon_fx) * ABS(d)) / ABS(dpf);

			/* Only first wall viewed from mid-x is lit */
			if (x < midpoint)
			{
				if (!cave_los_bold(subject_ptr->current_floor_ptr, y, x + 1)) return;
			}
			else if (x > midpoint)
			{
				if (!cave_los_bold(subject_ptr->current_floor_ptr, y, x - 1)) return;
			}

			/* Hack XXX XXX - Is it a wall and monster not in LOS? */
			else if (mon_invis) return;
		}

		/* Vertical walls between player and a monster */
		if (((x < subject_ptr->x) && (x > mon_fx)) || ((x > subject_ptr->x) && (x < mon_fx)))
		{
			dpf = subject_ptr->x - mon_fx;
			d = x - mon_fx;
			midpoint = mon_fy + ((subject_ptr->y - mon_fy) * ABS(d)) / ABS(dpf);

			/* Only first wall viewed from mid-y is lit */
			if (y < midpoint)
			{
				if (!cave_los_bold(subject_ptr->current_floor_ptr, y + 1, x)) return;
			}
			else if (y > midpoint)
			{
				if (!cave_los_bold(subject_ptr->current_floor_ptr, y - 1, x)) return;
			}

			/* Hack XXX XXX - Is it a wall and monster not in LOS? */
			else if (mon_invis) return;
		}
	}

	/* We trust tmp_pos.n does not exceed TEMP_MAX */

	/* New grid */
	if (!(g_ptr->info & CAVE_MNDK))
	{
		/* Save this square */
		tmp_pos.x[tmp_pos.n] = x;
		tmp_pos.y[tmp_pos.n] = y;
		tmp_pos.n++;
	}

	/* Darkened grid */
	else
	{
		/* No longer dark */
		g_ptr->info &= ~(CAVE_MNDK);
	}

	/* Light it */
	g_ptr->info |= CAVE_MNLT;
}


/*
 * Add a square to the changes array
 */
static void mon_dark_hack(player_type *subject_ptr, POSITION y, POSITION x)
{
	grid_type *g_ptr;
	int midpoint, dpf, d;

	/* We trust this grid is in bounds */
	/* if (!in_bounds2(y, x)) return; */

	g_ptr = &subject_ptr->current_floor_ptr->grid_array[y][x];

	/* Want a unlit and undarkened square in view of the player */
	if ((g_ptr->info & (CAVE_LITE | CAVE_MNLT | CAVE_MNDK | CAVE_VIEW)) != CAVE_VIEW) return;

	if (!cave_los_grid(g_ptr) && !cave_have_flag_grid(g_ptr, FF_PROJECT))
	{
		/* Hack -- Prevent monster dark lite leakage in walls */

		/* Horizontal walls between player and a monster */
		if (((y < subject_ptr->y) && (y > mon_fy)) || ((y > subject_ptr->y) && (y < mon_fy)))
		{
			dpf = subject_ptr->y - mon_fy;
			d = y - mon_fy;
			midpoint = mon_fx + ((subject_ptr->x - mon_fx) * ABS(d)) / ABS(dpf);

			/* Only first wall viewed from mid-x is lit */
			if (x < midpoint)
			{
				if (!cave_los_bold(subject_ptr->current_floor_ptr, y, x + 1) && !cave_have_flag_bold(subject_ptr->current_floor_ptr, y, x + 1, FF_PROJECT)) return;
			}
			else if (x > midpoint)
			{
				if (!cave_los_bold(subject_ptr->current_floor_ptr, y, x - 1) && !cave_have_flag_bold(subject_ptr->current_floor_ptr, y, x - 1, FF_PROJECT)) return;
			}

			/* Hack XXX XXX - Is it a wall and monster not in LOS? */
			else if (mon_invis) return;
		}

		/* Vertical walls between player and a monster */
		if (((x < subject_ptr->x) && (x > mon_fx)) || ((x > subject_ptr->x) && (x < mon_fx)))
		{
			dpf = subject_ptr->x - mon_fx;
			d = x - mon_fx;
			midpoint = mon_fy + ((subject_ptr->y - mon_fy) * ABS(d)) / ABS(dpf);

			/* Only first wall viewed from mid-y is lit */
			if (y < midpoint)
			{
				if (!cave_los_bold(subject_ptr->current_floor_ptr, y + 1, x) && !cave_have_flag_bold(subject_ptr->current_floor_ptr, y + 1, x, FF_PROJECT)) return;
			}
			else if (y > midpoint)
			{
				if (!cave_los_bold(subject_ptr->current_floor_ptr, y - 1, x) && !cave_have_flag_bold(subject_ptr->current_floor_ptr, y - 1, x, FF_PROJECT)) return;
			}

			/* Hack XXX XXX - Is it a wall and monster not in LOS? */
			else if (mon_invis) return;
		}
	}

	/* We trust tmp_pos.n does not exceed TEMP_MAX */

	/* Save this square */
	tmp_pos.x[tmp_pos.n] = x;
	tmp_pos.y[tmp_pos.n] = y;
	tmp_pos.n++;

	/* Darken it */
	g_ptr->info |= CAVE_MNDK;
}


/*
 * Update squares illuminated or darkened by monsters.
 *
 * Hack - use the CAVE_ROOM flag (renamed to be CAVE_MNLT) to
 * denote squares illuminated by monsters.
 *
 * The CAVE_TEMP and CAVE_XTRA flag are used to store the state during the
 * updating.  Only squares in view of the player, whos state
 * changes are drawn via lite_spot().
 */
void update_mon_lite(player_type *subject_ptr)
{
	void(*add_mon_lite)(player_type *, POSITION, POSITION);

	/* Non-Ninja player in the darkness */
	int dis_lim = ((d_info[subject_ptr->dungeon_idx].flags1 & DF1_DARKNESS) && !subject_ptr->see_nocto) ?
		(MAX_SIGHT / 2 + 1) : (MAX_SIGHT + 3);

	/* Clear all monster lit squares */
	floor_type *floor_ptr = subject_ptr->current_floor_ptr;
	for (int i = 0; i < floor_ptr->mon_lite_n; i++)
	{
		grid_type *g_ptr;
		g_ptr = &floor_ptr->grid_array[floor_ptr->mon_lite_y[i]][floor_ptr->mon_lite_x[i]];

		/* Set temp or xtra flag */
		g_ptr->info |= (g_ptr->info & CAVE_MNLT) ? CAVE_TEMP : CAVE_XTRA;

		/* Clear monster illumination flag */
		g_ptr->info &= ~(CAVE_MNLT | CAVE_MNDK);
	}

	/* Empty temp list of new squares to lite up */
	tmp_pos.n = 0;

	/* If a monster stops time, don't process */
	if (!current_world_ptr->timewalk_m_idx)
	{
		monster_type *m_ptr;
		monster_race *r_ptr;

		/* Loop through monsters, adding newly lit squares to changes list */
		for (int i = 1; i < floor_ptr->m_max; i++)
		{
			m_ptr = &floor_ptr->m_list[i];
			r_ptr = &r_info[m_ptr->r_idx];
			if (!monster_is_valid(m_ptr)) continue;

			/* Is it too far away? */
			if (m_ptr->cdis > dis_lim) continue;

			/* Get lite radius */
			int rad = 0;

			/* Note the radii are cumulative */
			if (r_ptr->flags7 & (RF7_HAS_LITE_1 | RF7_SELF_LITE_1)) rad++;
			if (r_ptr->flags7 & (RF7_HAS_LITE_2 | RF7_SELF_LITE_2)) rad += 2;
			if (r_ptr->flags7 & (RF7_HAS_DARK_1 | RF7_SELF_DARK_1)) rad--;
			if (r_ptr->flags7 & (RF7_HAS_DARK_2 | RF7_SELF_DARK_2)) rad -= 2;

			/* Exit if has no light */
			if (!rad) continue;

			int f_flag;
			if (rad > 0)
			{
				if (!(r_ptr->flags7 & (RF7_SELF_LITE_1 | RF7_SELF_LITE_2)) && (monster_csleep_remaining(m_ptr) || (!floor_ptr->dun_level && is_daytime()) || subject_ptr->phase_out)) continue;
				if (d_info[subject_ptr->dungeon_idx].flags1 & DF1_DARKNESS) rad = 1;
				add_mon_lite = mon_lite_hack;
				f_flag = FF_LOS;
			}
			else
			{
				if (!(r_ptr->flags7 & (RF7_SELF_DARK_1 | RF7_SELF_DARK_2)) && (monster_csleep_remaining(m_ptr) || (!floor_ptr->dun_level && !is_daytime()))) continue;
				add_mon_lite = mon_dark_hack;
				f_flag = FF_PROJECT;
				rad = -rad; /* Use absolute value */
			}

			mon_fx = m_ptr->fx;
			mon_fy = m_ptr->fy;

			/* Is the monster visible? */
			mon_invis = !(floor_ptr->grid_array[mon_fy][mon_fx].info & CAVE_VIEW);

			/* The square it is on */
			add_mon_lite(subject_ptr, mon_fy, mon_fx);

			/* Adjacent squares */
			add_mon_lite(subject_ptr, mon_fy + 1, mon_fx);
			add_mon_lite(subject_ptr, mon_fy - 1, mon_fx);
			add_mon_lite(subject_ptr, mon_fy, mon_fx + 1);
			add_mon_lite(subject_ptr, mon_fy, mon_fx - 1);
			add_mon_lite(subject_ptr, mon_fy + 1, mon_fx + 1);
			add_mon_lite(subject_ptr, mon_fy + 1, mon_fx - 1);
			add_mon_lite(subject_ptr, mon_fy - 1, mon_fx + 1);
			add_mon_lite(subject_ptr, mon_fy - 1, mon_fx - 1);

			/* Radius 2 */
			if (rad < 2) continue;

			/* South of the monster */
			grid_type *g_ptr;
			if (cave_have_flag_bold(subject_ptr->current_floor_ptr, mon_fy + 1, mon_fx, f_flag))
			{
				add_mon_lite(subject_ptr, mon_fy + 2, mon_fx + 1);
				add_mon_lite(subject_ptr, mon_fy + 2, mon_fx);
				add_mon_lite(subject_ptr, mon_fy + 2, mon_fx - 1);

				g_ptr = &floor_ptr->grid_array[mon_fy + 2][mon_fx];

				/* Radius 3 */
				if ((rad == 3) && cave_have_flag_grid(g_ptr, f_flag))
				{
					add_mon_lite(subject_ptr, mon_fy + 3, mon_fx + 1);
					add_mon_lite(subject_ptr, mon_fy + 3, mon_fx);
					add_mon_lite(subject_ptr, mon_fy + 3, mon_fx - 1);
				}
			}

			/* North of the monster */
			if (cave_have_flag_bold(subject_ptr->current_floor_ptr, mon_fy - 1, mon_fx, f_flag))
			{
				add_mon_lite(subject_ptr, mon_fy - 2, mon_fx + 1);
				add_mon_lite(subject_ptr, mon_fy - 2, mon_fx);
				add_mon_lite(subject_ptr, mon_fy - 2, mon_fx - 1);

				g_ptr = &floor_ptr->grid_array[mon_fy - 2][mon_fx];

				/* Radius 3 */
				if ((rad == 3) && cave_have_flag_grid(g_ptr, f_flag))
				{
					add_mon_lite(subject_ptr, mon_fy - 3, mon_fx + 1);
					add_mon_lite(subject_ptr, mon_fy - 3, mon_fx);
					add_mon_lite(subject_ptr, mon_fy - 3, mon_fx - 1);
				}
			}

			/* East of the monster */
			if (cave_have_flag_bold(subject_ptr->current_floor_ptr, mon_fy, mon_fx + 1, f_flag))
			{
				add_mon_lite(subject_ptr, mon_fy + 1, mon_fx + 2);
				add_mon_lite(subject_ptr, mon_fy, mon_fx + 2);
				add_mon_lite(subject_ptr, mon_fy - 1, mon_fx + 2);

				g_ptr = &floor_ptr->grid_array[mon_fy][mon_fx + 2];

				/* Radius 3 */
				if ((rad == 3) && cave_have_flag_grid(g_ptr, f_flag))
				{
					add_mon_lite(subject_ptr, mon_fy + 1, mon_fx + 3);
					add_mon_lite(subject_ptr, mon_fy, mon_fx + 3);
					add_mon_lite(subject_ptr, mon_fy - 1, mon_fx + 3);
				}
			}

			/* West of the monster */
			if (cave_have_flag_bold(subject_ptr->current_floor_ptr, mon_fy, mon_fx - 1, f_flag))
			{
				add_mon_lite(subject_ptr, mon_fy + 1, mon_fx - 2);
				add_mon_lite(subject_ptr, mon_fy, mon_fx - 2);
				add_mon_lite(subject_ptr, mon_fy - 1, mon_fx - 2);

				g_ptr = &floor_ptr->grid_array[mon_fy][mon_fx - 2];

				/* Radius 3 */
				if ((rad == 3) && cave_have_flag_grid(g_ptr, f_flag))
				{
					add_mon_lite(subject_ptr, mon_fy + 1, mon_fx - 3);
					add_mon_lite(subject_ptr, mon_fy, mon_fx - 3);
					add_mon_lite(subject_ptr, mon_fy - 1, mon_fx - 3);
				}
			}

			/* Radius 3 */
			if (rad != 3) continue;

			/* South-East of the monster */
			if (cave_have_flag_bold(subject_ptr->current_floor_ptr, mon_fy + 1, mon_fx + 1, f_flag))
			{
				add_mon_lite(subject_ptr, mon_fy + 2, mon_fx + 2);
			}

			/* South-West of the monster */
			if (cave_have_flag_bold(subject_ptr->current_floor_ptr, mon_fy + 1, mon_fx - 1, f_flag))
			{
				add_mon_lite(subject_ptr, mon_fy + 2, mon_fx - 2);
			}

			/* North-East of the monster */
			if (cave_have_flag_bold(subject_ptr->current_floor_ptr, mon_fy - 1, mon_fx + 1, f_flag))
			{
				add_mon_lite(subject_ptr, mon_fy - 2, mon_fx + 2);
			}

			/* North-West of the monster */
			if (cave_have_flag_bold(subject_ptr->current_floor_ptr, mon_fy - 1, mon_fx - 1, f_flag))
			{
				add_mon_lite(subject_ptr, mon_fy - 2, mon_fx - 2);
			}
		}
	}

	/* Save end of list of new squares */
	s16b end_temp = tmp_pos.n;

	/*
	 * Look at old set flags to see if there are any changes.
	 */
	for (int i = 0; i < floor_ptr->mon_lite_n; i++)
	{
		POSITION fx = floor_ptr->mon_lite_x[i];
		POSITION fy = floor_ptr->mon_lite_y[i];

		grid_type *g_ptr;
		g_ptr = &floor_ptr->grid_array[fy][fx];

		if (g_ptr->info & CAVE_TEMP) /* Pervious lit */
		{
			/* It it no longer lit? */
			if ((g_ptr->info & (CAVE_VIEW | CAVE_MNLT)) == CAVE_VIEW)
			{
				/* It is now unlit */
				/* Add it to later visual update */
				cave_note_and_redraw_later(floor_ptr, g_ptr, fy, fx);
			}
		}
		else /* Pervious darkened */
		{
			/* It it no longer darken? */
			if ((g_ptr->info & (CAVE_VIEW | CAVE_MNDK)) == CAVE_VIEW)
			{
				/* It is now undarken */
				/* Add it to later visual update */
				cave_note_and_redraw_later(floor_ptr, g_ptr, fy, fx);
			}
		}

		/* Add to end of temp array */
		tmp_pos.x[tmp_pos.n] = fx;
		tmp_pos.y[tmp_pos.n] = fy;
		tmp_pos.n++;
	}

	/* Clear the lite array */
	floor_ptr->mon_lite_n = 0;

	/* Copy the temp array into the lit array lighting the new squares. */
	for (int i = 0; i < end_temp; i++)
	{
		POSITION fx = tmp_pos.x[i];
		POSITION fy = tmp_pos.y[i];

		grid_type *g_ptr;
		g_ptr = &floor_ptr->grid_array[fy][fx];

		if (g_ptr->info & CAVE_MNLT) /* Lit */
		{
			/* The is the square newly lit and visible? */
			if ((g_ptr->info & (CAVE_VIEW | CAVE_TEMP)) == CAVE_VIEW)
			{
				/* It is now lit */
				/* Add it to later visual update */
				cave_note_and_redraw_later(floor_ptr, g_ptr, fy, fx);
			}
		}
		else /* Darkened */
		{
			/* The is the square newly darkened and visible? */
			if ((g_ptr->info & (CAVE_VIEW | CAVE_XTRA)) == CAVE_VIEW)
			{
				/* It is now darkened */
				/* Add it to later visual update */
				cave_note_and_redraw_later(floor_ptr, g_ptr, fy, fx);
			}
		}

		/* Save in the monster lit or darkened array */
		floor_ptr->mon_lite_x[floor_ptr->mon_lite_n] = fx;
		floor_ptr->mon_lite_y[floor_ptr->mon_lite_n] = fy;
		floor_ptr->mon_lite_n++;
	}

	/* Clear the temp flag for the old lit or darken grids */
	for (int i = end_temp; i < tmp_pos.n; i++)
	{
		/* We trust this grid is in bounds */

		floor_ptr->grid_array[tmp_pos.y[i]][tmp_pos.x[i]].info &= ~(CAVE_TEMP | CAVE_XTRA);
	}

	tmp_pos.n = 0;

	/* Mega-Hack -- Visual update later */
	subject_ptr->update |= (PU_DELAY_VIS);

	subject_ptr->monlite = (floor_ptr->grid_array[subject_ptr->y][subject_ptr->x].info & CAVE_MNLT) ? TRUE : FALSE;

	if (!(subject_ptr->special_defense & NINJA_S_STEALTH))
	{
		subject_ptr->old_monlite = subject_ptr->monlite;
		return;
	}

	if (subject_ptr->old_monlite == subject_ptr->monlite)
	{
		subject_ptr->old_monlite = subject_ptr->monlite;
		return;
	}

	if (subject_ptr->monlite)
	{
		msg_print(_("影の覆いが薄れた気がする。", "Your mantle of shadow becomes thin."));
	}
	else
	{
		msg_print(_("影の覆いが濃くなった！", "Your mantle of shadow is restored to its original darkness."));
	}

	subject_ptr->old_monlite = subject_ptr->monlite;
}


void clear_mon_lite(floor_type *floor_ptr)
{
	/* Clear all monster lit squares */
	for (int i = 0; i < floor_ptr->mon_lite_n; i++)
	{
		/* Point to grid */
		grid_type *g_ptr;
		g_ptr = &floor_ptr->grid_array[floor_ptr->mon_lite_y[i]][floor_ptr->mon_lite_x[i]];

		/* Clear monster illumination flag */
		g_ptr->info &= ~(CAVE_MNLT | CAVE_MNDK);
	}

	/* Empty the array */
	floor_ptr->mon_lite_n = 0;
}
