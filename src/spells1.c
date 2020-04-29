/*!
 * @file spells1.c
 * @brief 魔法による遠隔処理の実装 / Spell projection
 * @date 2014/07/10
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * </pre>
 */

#include "angband.h"
#include "core.h"
#include "util.h"
#include "main/sound-definitions-table.h"
#include "cmd/cmd-pet.h"
#include "cmd/cmd-dump.h"
#include "player-class.h"
#include "monster.h"
#include "spells.h"
#include "gameterm.h"
#include "view/display-main-window.h"
#include "effect/effect-feature.h"
#include "effect/effect-item.h"
#include "effect/effect-player.h"
#include "effect/effect-monster.h"
#include "effect/spells-effect-util.h"

/*!
 * @brief 配置した鏡リストの次を取得する /
 * Get another mirror. for SEEKER
 * @param next_y 次の鏡のy座標を返す参照ポインタ
 * @param next_x 次の鏡のx座標を返す参照ポインタ
 * @param cury 現在の鏡のy座標
 * @param curx 現在の鏡のx座標
 */
static void next_mirror(player_type *creature_ptr, POSITION* next_y, POSITION* next_x, POSITION cury, POSITION curx)
{
	POSITION mirror_x[10], mirror_y[10]; /* 鏡はもっと少ない */
	int mirror_num = 0;	/* 鏡の数 */
	for (POSITION x = 0; x < creature_ptr->current_floor_ptr->width; x++)
	{
		for (POSITION y = 0; y < creature_ptr->current_floor_ptr->height; y++)
		{
			if (is_mirror_grid(&creature_ptr->current_floor_ptr->grid_array[y][x])) {
				mirror_y[mirror_num] = y;
				mirror_x[mirror_num] = x;
				mirror_num++;
			}
		}
	}

	if (mirror_num)
	{
		int num = randint0(mirror_num);
		*next_y = mirror_y[num];
		*next_x = mirror_x[num];
		return;
	}

	*next_y = cury + randint0(5) - 2;
	*next_x = curx + randint0(5) - 2;
}


/*
 * Find the distance from (x, y) to a line.
 */
POSITION dist_to_line(POSITION y, POSITION x, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
	POSITION py = y1 - y;
	POSITION px = x1 - x;
	POSITION ny = x2 - x1;
	POSITION nx = y1 - y2;
	POSITION pd = distance(y1, x1, y, x);
	POSITION nd = distance(y1, x1, y2, x2);

	if (pd > nd) return distance(y, x, y2, x2);

	nd = ((nd) ? ((py * ny + px * nx) / nd) : 0);
	return((nd >= 0) ? nd : 0 - nd);
}


/*
 *
 * Modified version of los() for calculation of disintegration balls.
 * Disintegration effects are stopped by permanent walls.
 */
bool in_disintegration_range(floor_type *floor_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
	POSITION delta_y = y2 - y1;
	POSITION delta_x = x2 - x1;
	POSITION absolute_y = ABS(delta_y);
	POSITION absolute_x = ABS(delta_x);
	if ((absolute_x < 2) && (absolute_y < 2)) return TRUE;

	POSITION scanner_y;
	if (!delta_x)
	{
		/* South -- check for walls */
		if (delta_y > 0)
		{
			for (scanner_y = y1 + 1; scanner_y < y2; scanner_y++)
			{
				if (cave_stop_disintegration(floor_ptr, scanner_y, x1)) return FALSE;
			}
		}

		/* North -- check for walls */
		else
		{
			for (scanner_y = y1 - 1; scanner_y > y2; scanner_y--)
			{
				if (cave_stop_disintegration(floor_ptr, scanner_y, x1)) return FALSE;
			}
		}

		return TRUE;
	}

	/* Directly East/West */
	POSITION scanner_x;
	if (!delta_y)
	{
		/* East -- check for walls */
		if (delta_x > 0)
		{
			for (scanner_x = x1 + 1; scanner_x < x2; scanner_x++)
			{
				if (cave_stop_disintegration(floor_ptr, y1, scanner_x)) return FALSE;
			}
		}

		/* West -- check for walls */
		else
		{
			for (scanner_x = x1 - 1; scanner_x > x2; scanner_x--)
			{
				if (cave_stop_disintegration(floor_ptr, y1, scanner_x)) return FALSE;
			}
		}

		return TRUE;
	}

	POSITION sign_x = (delta_x < 0) ? -1 : 1;
	POSITION sign_y = (delta_y < 0) ? -1 : 1;
	if (absolute_x == 1)
	{
		if (absolute_y == 2)
		{
			if (!cave_stop_disintegration(floor_ptr, y1 + sign_y, x1)) return TRUE;
		}
	}
	else if (absolute_y == 1)
	{
		if (absolute_x == 2)
		{
			if (!cave_stop_disintegration(floor_ptr, y1, x1 + sign_x)) return TRUE;
		}
	}

	POSITION scale_factor_2 = (absolute_x * absolute_y);
	POSITION scale_factor_1 = scale_factor_2 << 1;
	POSITION fraction_y;
	POSITION m; /* Slope, or 1/Slope, of LOS */
	if (absolute_x >= absolute_y)
	{
		fraction_y = absolute_y * absolute_y;
		m = fraction_y << 1;
		scanner_x = x1 + sign_x;
		if (fraction_y == scale_factor_2)
		{
			scanner_y = y1 + sign_y;
			fraction_y -= scale_factor_1;
		}
		else
		{
			scanner_y = y1;
		}

		/* Note (below) the case (qy == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (x2 - scanner_x)
		{
			if (cave_stop_disintegration(floor_ptr, scanner_y, scanner_x)) return FALSE;

			fraction_y += m;

			if (fraction_y < scale_factor_2)
			{
				scanner_x += sign_x;
			}
			else if (fraction_y > scale_factor_2)
			{
				scanner_y += sign_y;
				if (cave_stop_disintegration(floor_ptr, scanner_y, scanner_x)) return FALSE;
				fraction_y -= scale_factor_1;
				scanner_x += sign_x;
			}
			else
			{
				scanner_y += sign_y;
				fraction_y -= scale_factor_1;
				scanner_x += sign_x;
			}
		}

		return TRUE;
	}

	POSITION fraction_x = absolute_x * absolute_x;
	m = fraction_x << 1;
	scanner_y = y1 + sign_y;
	if (fraction_x == scale_factor_2)
	{
		scanner_x = x1 + sign_x;
		fraction_x -= scale_factor_1;
	}
	else
	{
		scanner_x = x1;
	}

	/* Note (below) the case (qx == f2), where */
	/* the LOS exactly meets the corner of a tile. */
	while (y2 - scanner_y)
	{
		if (cave_stop_disintegration(floor_ptr, scanner_y, scanner_x)) return FALSE;

		fraction_x += m;

		if (fraction_x < scale_factor_2)
		{
			scanner_y += sign_y;
		}
		else if (fraction_x > scale_factor_2)
		{
			scanner_x += sign_x;
			if (cave_stop_disintegration(floor_ptr, scanner_y, scanner_x)) return FALSE;
			fraction_x -= scale_factor_1;
			scanner_y += sign_y;
		}
		else
		{
			scanner_x += sign_x;
			fraction_x -= scale_factor_1;
			scanner_y += sign_y;
		}
	}

	return TRUE;
}


/*
 * breath shape
 */
void breath_shape(player_type *caster_ptr, u16b *path_g, int dist, int *pgrids, POSITION *gx, POSITION *gy, POSITION *gm, POSITION *pgm_rad, POSITION rad, POSITION y1, POSITION x1, POSITION y2, POSITION x2, EFFECT_ID typ)
{
	POSITION by = y1;
	POSITION bx = x1;
	int brad = 0;
	int brev = rad * rad / dist;
	int bdis = 0;
	int cdis;
	int path_n = 0;
	int mdis = distance(y1, x1, y2, x2) + rad;

	floor_type *floor_ptr = caster_ptr->current_floor_ptr;
	while (bdis <= mdis)
	{
		if ((0 < dist) && (path_n < dist))
		{
			POSITION ny = GRID_Y(path_g[path_n]);
			POSITION nx = GRID_X(path_g[path_n]);
			POSITION nd = distance(ny, nx, y1, x1);

			if (bdis >= nd)
			{
				by = ny;
				bx = nx;
				path_n++;
			}
		}

		/* Travel from center outward */
		for (cdis = 0; cdis <= brad; cdis++)
		{
			for (POSITION y = by - cdis; y <= by + cdis; y++)
			{
				for (POSITION x = bx - cdis; x <= bx + cdis; x++)
				{
					if (!in_bounds(floor_ptr, y, x)) continue;
					if (distance(y1, x1, y, x) != bdis) continue;
					if (distance(by, bx, y, x) != cdis) continue;

					switch (typ)
					{
					case GF_LITE:
					case GF_LITE_WEAK:
						/* Lights are stopped by opaque terrains */
						if (!los(caster_ptr, by, bx, y, x)) continue;
						break;
					case GF_DISINTEGRATE:
						/* Disintegration are stopped only by perma-walls */
						if (!in_disintegration_range(floor_ptr, by, bx, y, x)) continue;
						break;
					default:
						/* Ball explosions are stopped by walls */
						if (!projectable(caster_ptr, by, bx, y, x)) continue;
						break;
					}

					gy[*pgrids] = y;
					gx[*pgrids] = x;
					(*pgrids)++;
				}
			}
		}

		gm[bdis + 1] = *pgrids;
		brad = rad * (path_n + brev) / (dist + brev);
		bdis++;
	}

	*pgm_rad = bdis;
}


/*!
 * todo 似たような処理が山ほど並んでいる、何とかならないものか
 * @brief 汎用的なビーム/ボルト/ボール系処理のルーチン Generic "beam"/"bolt"/"ball" projection routine.
 * @param who 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source" monster (zero for "player")
 * @param rad 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param typ 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flag 効果フラグ / Extra bit flags (see PROJECT_xxxx)
 * @param monspell 効果元のモンスター魔法ID
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 */
bool project(player_type *caster_ptr, MONSTER_IDX who, POSITION rad, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ, BIT_FLAGS flag, int monspell)
{
	int i, t, dist;
	POSITION y1, x1;
	POSITION y2, x2;
	POSITION by, bx;
	int dist_hack = 0;
	POSITION y_saver, x_saver; /* For reflecting monsters */
	int msec = delay_factor * delay_factor * delay_factor;
	bool notice = FALSE;
	bool visual = FALSE;
	bool drawn = FALSE;
	bool breath = FALSE;
	bool blind = caster_ptr->blind != 0;
	bool old_hide = FALSE;
	int path_n = 0;
	u16b path_g[512];
	int grids = 0;
	POSITION gx[1024], gy[1024];
	POSITION gm[32];
	POSITION gm_rad = rad;
	bool jump = FALSE;
	GAME_TEXT who_name[MAX_NLEN];
	bool see_s_msg = TRUE;
	who_name[0] = '\0';
	rakubadam_p = 0;
	rakubadam_m = 0;
	monster_target_y = caster_ptr->y;
	monster_target_x = caster_ptr->x;

	if (flag & (PROJECT_JUMP))
	{
		x1 = x;
		y1 = y;
		flag &= ~(PROJECT_JUMP);
		jump = TRUE;
	}
	else if (who <= 0)
	{
		x1 = caster_ptr->x;
		y1 = caster_ptr->y;
	}
	else if (who > 0)
	{
		x1 = caster_ptr->current_floor_ptr->m_list[who].fx;
		y1 = caster_ptr->current_floor_ptr->m_list[who].fy;
		monster_desc(caster_ptr, who_name, &caster_ptr->current_floor_ptr->m_list[who], MD_WRONGDOER_NAME);
	}
	else
	{
		x1 = x;
		y1 = y;
	}

	y_saver = y1;
	x_saver = x1;
	y2 = y;
	x2 = x;

	if (flag & (PROJECT_THRU))
	{
		if ((x1 == x2) && (y1 == y2))
		{
			flag &= ~(PROJECT_THRU);
		}
	}

	if (rad < 0)
	{
		rad = 0 - rad;
		breath = TRUE;
		if (flag & PROJECT_HIDE) old_hide = TRUE;
		flag |= PROJECT_HIDE;
	}

	for (dist = 0; dist < 32; dist++) gm[dist] = 0;

	y = y1;
	x = x1;
	dist = 0;
	if (flag & (PROJECT_BEAM))
	{
		gy[grids] = y;
		gx[grids] = x;
		grids++;
	}

	switch (typ)
	{
	case GF_LITE:
	case GF_LITE_WEAK:
		if (breath || (flag & PROJECT_BEAM)) flag |= (PROJECT_LOS);
		break;
	case GF_DISINTEGRATE:
		flag |= (PROJECT_GRID);
		if (breath || (flag & PROJECT_BEAM)) flag |= (PROJECT_DISI);
		break;
	}

	/* Calculate the projection path */
	path_n = project_path(caster_ptr, path_g, (project_length ? project_length : MAX_RANGE), y1, x1, y2, x2, flag);
	handle_stuff(caster_ptr);

	if (typ == GF_SEEKER)
	{
		int j;
		int last_i = 0;
		project_m_n = 0;
		project_m_x = 0;
		project_m_y = 0;
		for (i = 0; i < path_n; ++i)
		{
			POSITION oy = y;
			POSITION ox = x;
			POSITION ny = GRID_Y(path_g[i]);
			POSITION nx = GRID_X(path_g[i]);
			y = ny;
			x = nx;
			gy[grids] = y;
			gx[grids] = x;
			grids++;

			if (!blind && !(flag & (PROJECT_HIDE)))
			{
				if (panel_contains(y, x) && player_has_los_bold(caster_ptr, y, x))
				{
					u16b p = bolt_pict(oy, ox, y, x, typ);
					TERM_COLOR a = PICT_A(p);
					SYMBOL_CODE c = PICT_C(p);
					print_rel(caster_ptr, c, a, y, x);
					move_cursor_relative(y, x);
					Term_fresh();
					Term_xtra(TERM_XTRA_DELAY, msec);
					lite_spot(caster_ptr, y, x);
					Term_fresh();
					if (flag & (PROJECT_BEAM))
					{
						p = bolt_pict(y, x, y, x, typ);
						a = PICT_A(p);
						c = PICT_C(p);
						print_rel(caster_ptr, c, a, y, x);
					}

					visual = TRUE;
				}
				else if (visual)
				{
					Term_xtra(TERM_XTRA_DELAY, msec);
				}
			}

			if (affect_item(caster_ptr, 0, 0, y, x, dam, GF_SEEKER))notice = TRUE;
			if (!is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[y][x]))
				continue;

			monster_target_y = y;
			monster_target_x = x;
			remove_mirror(caster_ptr, y, x);
			next_mirror(caster_ptr, &oy, &ox, y, x);
			path_n = i + project_path(caster_ptr, &(path_g[i + 1]), (project_length ? project_length : MAX_RANGE), y, x, oy, ox, flag);
			for (j = last_i; j <= i; j++)
			{
				y = GRID_Y(path_g[j]);
				x = GRID_X(path_g[j]);
				if (affect_monster(caster_ptr, 0, 0, y, x, dam, GF_SEEKER, flag, TRUE)) notice = TRUE;
				if (!who && (project_m_n == 1) && !jump && (caster_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx > 0))
				{
					monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[caster_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx];
					if (m_ptr->ml)
					{
						if (!caster_ptr->image) monster_race_track(caster_ptr, m_ptr->ap_r_idx);
						health_track(caster_ptr, caster_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx);
					}
				}

				(void)affect_feature(caster_ptr, 0, 0, y, x, dam, GF_SEEKER);
			}

			last_i = i;
		}

		for (i = last_i; i < path_n; i++)
		{
			POSITION py, px;
			py = GRID_Y(path_g[i]);
			px = GRID_X(path_g[i]);
			if (affect_monster(caster_ptr, 0, 0, py, px, dam, GF_SEEKER, flag, TRUE))
				notice = TRUE;
			if (!who && (project_m_n == 1) && !jump) {
				if (caster_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx > 0)
				{
					monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[caster_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx];

					if (m_ptr->ml)
					{
						if (!caster_ptr->image) monster_race_track(caster_ptr, m_ptr->ap_r_idx);
						health_track(caster_ptr, caster_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx);
					}
				}
			}

			(void)affect_feature(caster_ptr, 0, 0, py, px, dam, GF_SEEKER);
		}

		return notice;
	}
	else if (typ == GF_SUPER_RAY)
	{
		int j;
		int second_step = 0;
		project_m_n = 0;
		project_m_x = 0;
		project_m_y = 0;
		for (i = 0; i < path_n; ++i)
		{
			POSITION oy = y;
			POSITION ox = x;
			POSITION ny = GRID_Y(path_g[i]);
			POSITION nx = GRID_X(path_g[i]);
			y = ny;
			x = nx;
			gy[grids] = y;
			gx[grids] = x;
			grids++;
			{
				if (panel_contains(y, x) && player_has_los_bold(caster_ptr, y, x))
				{
					u16b p;
					TERM_COLOR a;
					SYMBOL_CODE c;
					p = bolt_pict(oy, ox, y, x, typ);
					a = PICT_A(p);
					c = PICT_C(p);
					print_rel(caster_ptr, c, a, y, x);
					move_cursor_relative(y, x);
					Term_fresh();
					Term_xtra(TERM_XTRA_DELAY, msec);
					lite_spot(caster_ptr, y, x);
					Term_fresh();
					if (flag & (PROJECT_BEAM))
					{
						p = bolt_pict(y, x, y, x, typ);
						a = PICT_A(p);
						c = PICT_C(p);
						print_rel(caster_ptr, c, a, y, x);
					}

					visual = TRUE;
				}
				else if (visual)
				{
					Term_xtra(TERM_XTRA_DELAY, msec);
				}
			}

			if (affect_item(caster_ptr, 0, 0, y, x, dam, GF_SUPER_RAY))notice = TRUE;
			if (!cave_have_flag_bold(caster_ptr->current_floor_ptr, y, x, FF_PROJECT))
			{
				if (second_step)continue;
				break;
			}

			if (is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[y][x]) && !second_step)
			{
				monster_target_y = y;
				monster_target_x = x;
				remove_mirror(caster_ptr, y, x);
				for (j = 0; j <= i; j++)
				{
					y = GRID_Y(path_g[j]);
					x = GRID_X(path_g[j]);
					(void)affect_feature(caster_ptr, 0, 0, y, x, dam, GF_SUPER_RAY);
				}

				path_n = i;
				second_step = i + 1;
				path_n += project_path(caster_ptr, &(path_g[path_n + 1]), (project_length ? project_length : MAX_RANGE), y, x, y - 1, x - 1, flag);
				path_n += project_path(caster_ptr, &(path_g[path_n + 1]), (project_length ? project_length : MAX_RANGE), y, x, y - 1, x, flag);
				path_n += project_path(caster_ptr, &(path_g[path_n + 1]), (project_length ? project_length : MAX_RANGE), y, x, y - 1, x + 1, flag);
				path_n += project_path(caster_ptr, &(path_g[path_n + 1]), (project_length ? project_length : MAX_RANGE), y, x, y, x - 1, flag);
				path_n += project_path(caster_ptr, &(path_g[path_n + 1]), (project_length ? project_length : MAX_RANGE), y, x, y, x + 1, flag);
				path_n += project_path(caster_ptr, &(path_g[path_n + 1]), (project_length ? project_length : MAX_RANGE), y, x, y + 1, x - 1, flag);
				path_n += project_path(caster_ptr, &(path_g[path_n + 1]), (project_length ? project_length : MAX_RANGE), y, x, y + 1, x, flag);
				path_n += project_path(caster_ptr, &(path_g[path_n + 1]), (project_length ? project_length : MAX_RANGE), y, x, y + 1, x + 1, flag);
			}
		}

		for (i = 0; i < path_n; i++)
		{
			POSITION py = GRID_Y(path_g[i]);
			POSITION px = GRID_X(path_g[i]);
			(void)affect_monster(caster_ptr, 0, 0, py, px, dam, GF_SUPER_RAY, flag, TRUE);
			if (!who && (project_m_n == 1) && !jump) {
				if (caster_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx > 0) {
					monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[caster_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx];

					if (m_ptr->ml)
					{
						if (!caster_ptr->image) monster_race_track(caster_ptr, m_ptr->ap_r_idx);
						health_track(caster_ptr, caster_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx);
					}
				}
			}

			(void)affect_feature(caster_ptr, 0, 0, py, px, dam, GF_SUPER_RAY);
		}

		return notice;
	}

	for (i = 0; i < path_n; ++i)
	{
		POSITION oy = y;
		POSITION ox = x;
		POSITION ny = GRID_Y(path_g[i]);
		POSITION nx = GRID_X(path_g[i]);
		if (flag & PROJECT_DISI)
		{
			if (cave_stop_disintegration(caster_ptr->current_floor_ptr, ny, nx) && (rad > 0)) break;
		}
		else if (flag & PROJECT_LOS)
		{
			if (!cave_los_bold(caster_ptr->current_floor_ptr, ny, nx) && (rad > 0)) break;
		}
		else
		{
			if (!cave_have_flag_bold(caster_ptr->current_floor_ptr, ny, nx, FF_PROJECT) && (rad > 0)) break;
		}

		y = ny;
		x = nx;
		if (flag & (PROJECT_BEAM))
		{
			gy[grids] = y;
			gx[grids] = x;
			grids++;
		}

		if (!blind && !(flag & (PROJECT_HIDE | PROJECT_FAST)))
		{
			if (panel_contains(y, x) && player_has_los_bold(caster_ptr, y, x))
			{
				u16b p;
				TERM_COLOR a;
				SYMBOL_CODE c;
				p = bolt_pict(oy, ox, y, x, typ);
				a = PICT_A(p);
				c = PICT_C(p);
				print_rel(caster_ptr, c, a, y, x);
				move_cursor_relative(y, x);
				Term_fresh();
				Term_xtra(TERM_XTRA_DELAY, msec);
				lite_spot(caster_ptr, y, x);
				Term_fresh();
				if (flag & (PROJECT_BEAM))
				{
					p = bolt_pict(y, x, y, x, typ);
					a = PICT_A(p);
					c = PICT_C(p);
					print_rel(caster_ptr, c, a, y, x);
				}

				visual = TRUE;
			}
			else if (visual)
			{
				Term_xtra(TERM_XTRA_DELAY, msec);
			}
		}
	}

	path_n = i;
	by = y;
	bx = x;
	if (breath && !path_n)
	{
		breath = FALSE;
		gm_rad = rad;
		if (!old_hide)
		{
			flag &= ~(PROJECT_HIDE);
		}
	}

	gm[0] = 0;
	gm[1] = grids;
	dist = path_n;
	dist_hack = dist;
	project_length = 0;

	/* If we found a "target", explode there */
	if (dist <= MAX_RANGE)
	{
		if ((flag & (PROJECT_BEAM)) && (grids > 0)) grids--;

		/*
		 * Create a conical breath attack
		 *
		 *       ***
		 *   ********
		 * D********@**
		 *   ********
		 *       ***
		 */
		if (breath)
		{
			flag &= ~(PROJECT_HIDE);
			breath_shape(caster_ptr, path_g, dist, &grids, gx, gy, gm, &gm_rad, rad, y1, x1, by, bx, typ);
		}
		else
		{
			for (dist = 0; dist <= rad; dist++)
			{
				for (y = by - dist; y <= by + dist; y++)
				{
					for (x = bx - dist; x <= bx + dist; x++)
					{
						if (!in_bounds2(caster_ptr->current_floor_ptr, y, x)) continue;
						if (distance(by, bx, y, x) != dist) continue;

						switch (typ)
						{
						case GF_LITE:
						case GF_LITE_WEAK:
							if (!los(caster_ptr, by, bx, y, x)) continue;
							break;
						case GF_DISINTEGRATE:
							if (!in_disintegration_range(caster_ptr->current_floor_ptr, by, bx, y, x)) continue;
							break;
						default:
							if (!projectable(caster_ptr, by, bx, y, x)) continue;
							break;
						}

						gy[grids] = y;
						gx[grids] = x;
						grids++;
					}
				}

				gm[dist + 1] = grids;
			}
		}
	}

	if (!grids) return FALSE;

	if (!blind && !(flag & (PROJECT_HIDE)))
	{
		for (t = 0; t <= gm_rad; t++)
		{
			for (i = gm[t]; i < gm[t + 1]; i++)
			{
				y = gy[i];
				x = gx[i];
				if (panel_contains(y, x) && player_has_los_bold(caster_ptr, y, x))
				{
					u16b p;
					TERM_COLOR a;
					SYMBOL_CODE c;
					drawn = TRUE;
					p = bolt_pict(y, x, y, x, typ);
					a = PICT_A(p);
					c = PICT_C(p);
					print_rel(caster_ptr, c, a, y, x);
				}
			}

			move_cursor_relative(by, bx);
			Term_fresh();
			if (visual || drawn)
			{
				Term_xtra(TERM_XTRA_DELAY, msec);
			}
		}

		if (drawn)
		{
			for (i = 0; i < grids; i++)
			{
				y = gy[i];
				x = gx[i];
				if (panel_contains(y, x) && player_has_los_bold(caster_ptr, y, x))
				{
					lite_spot(caster_ptr, y, x);
				}
			}

			move_cursor_relative(by, bx);
			Term_fresh();
		}
	}

	update_creature(caster_ptr);

	if (flag & PROJECT_KILL)
	{
		see_s_msg = (who > 0) ? is_seen(&caster_ptr->current_floor_ptr->m_list[who]) :
			(!who ? TRUE : (player_can_see_bold(caster_ptr, y1, x1) && projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y1, x1)));
	}

	if (flag & (PROJECT_GRID))
	{
		dist = 0;
		for (i = 0; i < grids; i++)
		{
			if (gm[dist + 1] == i) dist++;
			y = gy[i];
			x = gx[i];
			if (breath)
			{
				int d = dist_to_line(y, x, y1, x1, by, bx);
				if (affect_feature(caster_ptr, who, d, y, x, dam, typ)) notice = TRUE;
			}
			else
			{
				if (affect_feature(caster_ptr, who, dist, y, x, dam, typ)) notice = TRUE;
			}
		}
	}

	update_creature(caster_ptr);
	if (flag & (PROJECT_ITEM))
	{
		dist = 0;
		for (i = 0; i < grids; i++)
		{
			if (gm[dist + 1] == i) dist++;

			y = gy[i];
			x = gx[i];
			if (breath)
			{
				int d = dist_to_line(y, x, y1, x1, by, bx);
				if (affect_item(caster_ptr, who, d, y, x, dam, typ)) notice = TRUE;
			}
			else
			{
				if (affect_item(caster_ptr, who, dist, y, x, dam, typ)) notice = TRUE;
			}
		}
	}

	if (flag & (PROJECT_KILL))
	{
		project_m_n = 0;
		project_m_x = 0;
		project_m_y = 0;
		dist = 0;
		for (i = 0; i < grids; i++)
		{
			int effective_dist;
			if (gm[dist + 1] == i) dist++;

			y = gy[i];
			x = gx[i];
			if (grids <= 1)
			{
				monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[caster_ptr->current_floor_ptr->grid_array[y][x].m_idx];
				monster_race *ref_ptr = &r_info[m_ptr->r_idx];
				if ((flag & PROJECT_REFLECTABLE) && caster_ptr->current_floor_ptr->grid_array[y][x].m_idx && (ref_ptr->flags2 & RF2_REFLECTING) &&
					((caster_ptr->current_floor_ptr->grid_array[y][x].m_idx != caster_ptr->riding) || !(flag & PROJECT_PLAYER)) &&
					(!who || dist_hack > 1) && !one_in_(10))
				{
					POSITION t_y, t_x;
					int max_attempts = 10;
					do
					{
						t_y = y_saver - 1 + randint1(3);
						t_x = x_saver - 1 + randint1(3);
						max_attempts--;
					} while (max_attempts && in_bounds2u(caster_ptr->current_floor_ptr, t_y, t_x) && !projectable(caster_ptr, y, x, t_y, t_x));

					if (max_attempts < 1)
					{
						t_y = y_saver;
						t_x = x_saver;
					}

					sound(SOUND_REFLECT);
					if (is_seen(m_ptr))
					{
						if ((m_ptr->r_idx == MON_KENSHIROU) || (m_ptr->r_idx == MON_RAOU))
							msg_print(_("「北斗神拳奥義・二指真空把！」", "The attack bounces!"));
						else if (m_ptr->r_idx == MON_DIO)
							msg_print(_("ディオ・ブランドーは指一本で攻撃を弾き返した！", "The attack bounces!"));
						else
							msg_print(_("攻撃は跳ね返った！", "The attack bounces!"));
					}

					if (is_original_ap_and_seen(caster_ptr, m_ptr)) ref_ptr->r_flags2 |= RF2_REFLECTING;

					if (player_bold(caster_ptr, y, x) || one_in_(2)) flag &= ~(PROJECT_PLAYER);
					else flag |= PROJECT_PLAYER;

					project(caster_ptr, caster_ptr->current_floor_ptr->grid_array[y][x].m_idx, 0, t_y, t_x, dam, typ, flag, monspell);
					continue;
				}
			}

			/* Find the closest point in the blast */
			if (breath)
			{
				effective_dist = dist_to_line(y, x, y1, x1, by, bx);
			}
			else
			{
				effective_dist = dist;
			}

			if (caster_ptr->riding && player_bold(caster_ptr, y, x))
			{
				if (flag & PROJECT_PLAYER)
				{
					if (flag & (PROJECT_BEAM | PROJECT_REFLECTABLE | PROJECT_AIMED))
					{
						/*
						 * A beam or bolt is well aimed
						 * at the PLAYER!
						 * So don't affects the mount.
						 */
						continue;
					}
					else
					{
						/*
						 * The spell is not well aimed,
						 * So partly affect the mount too.
						 */
						effective_dist++;
					}
				}

				/*
				 * This grid is the original target.
				 * Or aimed on your horse.
				 */
				else if (((y == y2) && (x == x2)) || (flag & PROJECT_AIMED))
				{
					/* Hit the mount with full damage */
				}

				/*
				 * Otherwise this grid is not the
				 * original target, it means that line
				 * of fire is obstructed by this
				 * monster.
				 */
				 /*
				  * A beam or bolt will hit either
				  * player or mount.  Choose randomly.
				  */
				else if (flag & (PROJECT_BEAM | PROJECT_REFLECTABLE))
				{
					if (one_in_(2))
					{
						/* Hit the mount with full damage */
					}
					else
					{
						flag |= PROJECT_PLAYER;
						continue;
					}
				}

				/*
				 * The spell is not well aimed, so
				 * partly affect both player and
				 * mount.
				 */
				else
				{
					effective_dist++;
				}
			}

			if (affect_monster(caster_ptr, who, effective_dist, y, x, dam, typ, flag, see_s_msg)) notice = TRUE;
		}

		/* Player affected one monster (without "jumping") */
		if (!who && (project_m_n == 1) && !jump)
		{
			x = project_m_x;
			y = project_m_y;
			if (caster_ptr->current_floor_ptr->grid_array[y][x].m_idx > 0)
			{
				monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[caster_ptr->current_floor_ptr->grid_array[y][x].m_idx];

				if (m_ptr->ml)
				{
					if (!caster_ptr->image) monster_race_track(caster_ptr, m_ptr->ap_r_idx);
					health_track(caster_ptr, caster_ptr->current_floor_ptr->grid_array[y][x].m_idx);
				}
			}
		}
	}

	if (flag & (PROJECT_KILL))
	{
		dist = 0;
		for (i = 0; i < grids; i++)
		{
			int effective_dist;
			if (gm[dist + 1] == i) dist++;

			y = gy[i];
			x = gx[i];
			if (!player_bold(caster_ptr, y, x)) continue;

			/* Find the closest point in the blast */
			if (breath)
			{
				effective_dist = dist_to_line(y, x, y1, x1, by, bx);
			}
			else
			{
				effective_dist = dist;
			}

			if (caster_ptr->riding)
			{
				if (flag & PROJECT_PLAYER)
				{
					/* Hit the player with full damage */
				}

				/*
				 * Hack -- When this grid was not the
				 * original target, a beam or bolt
				 * would hit either player or mount,
				 * and should be choosen randomly.
				 *
				 * But already choosen to hit the
				 * mount at this point.
				 *
				 * Or aimed on your horse.
				 */
				else if (flag & (PROJECT_BEAM | PROJECT_REFLECTABLE | PROJECT_AIMED))
				{
					/*
					 * A beam or bolt is well aimed
					 * at the mount!
					 * So don't affects the player.
					 */
					continue;
				}
				else
				{
					/*
					 * The spell is not well aimed,
					 * So partly affect the player too.
					 */
					effective_dist++;
				}
			}

			if (affect_player(who, caster_ptr, who_name, effective_dist, y, x, dam, typ, flag, monspell)) notice = TRUE;
		}
	}

	if (caster_ptr->riding)
	{
		GAME_TEXT m_name[MAX_NLEN];
		monster_desc(caster_ptr, m_name, &caster_ptr->current_floor_ptr->m_list[caster_ptr->riding], 0);
		if (rakubadam_m > 0)
		{
			if (rakuba(caster_ptr, rakubadam_m, FALSE))
			{
				msg_format(_("%^sに振り落とされた！", "%^s has thrown you off!"), m_name);
			}
		}

		if (caster_ptr->riding && rakubadam_p > 0)
		{
			if (rakuba(caster_ptr, rakubadam_p, FALSE))
			{
				msg_format(_("%^sから落ちてしまった！", "You have fallen from %s."), m_name);
			}
		}
	}

	return (notice);
}


/*!
 * @brief 鏡魔法「封魔結界」の効果処理
 * @param dam ダメージ量
 * @return 効果があったらTRUEを返す
 */
bool binding_field(player_type *caster_ptr, HIT_POINT dam)
{
	POSITION mirror_x[10], mirror_y[10]; /* 鏡はもっと少ない */
	int mirror_num = 0;	/* 鏡の数 */
	int msec = delay_factor * delay_factor*delay_factor;

	/* 三角形の頂点 */
	POSITION point_x[3];
	POSITION point_y[3];

	/* Default target of monsterspell is player */
	monster_target_y = caster_ptr->y;
	monster_target_x = caster_ptr->x;

	for (POSITION x = 0; x < caster_ptr->current_floor_ptr->width; x++)
	{
		for (POSITION y = 0; y < caster_ptr->current_floor_ptr->height; y++)
		{
			if (is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[y][x]) &&
				distance(caster_ptr->y, caster_ptr->x, y, x) <= MAX_RANGE &&
				distance(caster_ptr->y, caster_ptr->x, y, x) != 0 &&
				player_has_los_bold(caster_ptr, y, x) &&
				projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y, x))
			{
				mirror_y[mirror_num] = y;
				mirror_x[mirror_num] = x;
				mirror_num++;
			}
		}
	}

	if (mirror_num < 2)return FALSE;

	point_x[0] = randint0(mirror_num);
	do {
		point_x[1] = randint0(mirror_num);
	} while (point_x[0] == point_x[1]);

	point_y[0] = mirror_y[point_x[0]];
	point_x[0] = mirror_x[point_x[0]];
	point_y[1] = mirror_y[point_x[1]];
	point_x[1] = mirror_x[point_x[1]];
	point_y[2] = caster_ptr->y;
	point_x[2] = caster_ptr->x;

	POSITION x = point_x[0] + point_x[1] + point_x[2];
	POSITION y = point_y[0] + point_y[1] + point_y[2];

	POSITION centersign = (point_x[0] * 3 - x)*(point_y[1] * 3 - y)
		- (point_y[0] * 3 - y)*(point_x[1] * 3 - x);
	if (centersign == 0)return FALSE;

	POSITION x1 = point_x[0] < point_x[1] ? point_x[0] : point_x[1];
	x1 = x1 < point_x[2] ? x1 : point_x[2];
	POSITION y1 = point_y[0] < point_y[1] ? point_y[0] : point_y[1];
	y1 = y1 < point_y[2] ? y1 : point_y[2];

	POSITION x2 = point_x[0] > point_x[1] ? point_x[0] : point_x[1];
	x2 = x2 > point_x[2] ? x2 : point_x[2];
	POSITION y2 = point_y[0] > point_y[1] ? point_y[0] : point_y[1];
	y2 = y2 > point_y[2] ? y2 : point_y[2];

	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			if (centersign*((point_x[0] - x)*(point_y[1] - y)
				- (point_y[0] - y)*(point_x[1] - x)) >= 0 &&
				centersign*((point_x[1] - x)*(point_y[2] - y)
					- (point_y[1] - y)*(point_x[2] - x)) >= 0 &&
				centersign*((point_x[2] - x)*(point_y[0] - y)
					- (point_y[2] - y)*(point_x[0] - x)) >= 0)
			{
				if (player_has_los_bold(caster_ptr, y, x) && projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y, x))
				{
					if (!(caster_ptr->blind)
						&& panel_contains(y, x))
					{
						u16b p = bolt_pict(y, x, y, x, GF_MANA);
						print_rel(caster_ptr, PICT_C(p), PICT_A(p), y, x);
						move_cursor_relative(y, x);
						Term_fresh();
						Term_xtra(TERM_XTRA_DELAY, msec);
					}
				}
			}
		}
	}

	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			if (centersign*((point_x[0] - x)*(point_y[1] - y)
				- (point_y[0] - y)*(point_x[1] - x)) >= 0 &&
				centersign*((point_x[1] - x)*(point_y[2] - y)
					- (point_y[1] - y)*(point_x[2] - x)) >= 0 &&
				centersign*((point_x[2] - x)*(point_y[0] - y)
					- (point_y[2] - y)*(point_x[0] - x)) >= 0)
			{
				if (player_has_los_bold(caster_ptr, y, x) && projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y, x))
				{
					(void)affect_feature(caster_ptr, 0, 0, y, x, dam, GF_MANA);
				}
			}
		}
	}

	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			if (centersign*((point_x[0] - x)*(point_y[1] - y)
				- (point_y[0] - y)*(point_x[1] - x)) >= 0 &&
				centersign*((point_x[1] - x)*(point_y[2] - y)
					- (point_y[1] - y)*(point_x[2] - x)) >= 0 &&
				centersign*((point_x[2] - x)*(point_y[0] - y)
					- (point_y[2] - y)*(point_x[0] - x)) >= 0)
			{
				if (player_has_los_bold(caster_ptr, y, x) && projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y, x))
				{
					(void)affect_item(caster_ptr, 0, 0, y, x, dam, GF_MANA);
				}
			}
		}
	}

	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			if (centersign*((point_x[0] - x)*(point_y[1] - y)
				- (point_y[0] - y)*(point_x[1] - x)) >= 0 &&
				centersign*((point_x[1] - x)*(point_y[2] - y)
					- (point_y[1] - y)*(point_x[2] - x)) >= 0 &&
				centersign*((point_x[2] - x)*(point_y[0] - y)
					- (point_y[2] - y)*(point_x[0] - x)) >= 0)
			{
				if (player_has_los_bold(caster_ptr, y, x) && projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y, x))
				{
					(void)affect_monster(caster_ptr, 0, 0, y, x, dam, GF_MANA,
						(PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP), TRUE);
				}
			}
		}
	}

	if (one_in_(7))
	{
		msg_print(_("鏡が結界に耐えきれず、壊れてしまった。", "The field broke a mirror"));
		remove_mirror(caster_ptr, point_y[0], point_x[0]);
	}

	return TRUE;
}


/*!
 * @brief 鏡魔法「鏡の封印」の効果処理
 * @param dam ダメージ量
 * @return 効果があったらTRUEを返す
 */
void seal_of_mirror(player_type *caster_ptr, HIT_POINT dam)
{
	for (POSITION x = 0; x < caster_ptr->current_floor_ptr->width; x++)
	{
		for (POSITION y = 0; y < caster_ptr->current_floor_ptr->height; y++)
		{
			if (!is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[y][x]))
				continue;

			if (!affect_monster(caster_ptr, 0, 0, y, x, dam, GF_GENOCIDE,
				(PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP), TRUE))
				continue;

			if (!caster_ptr->current_floor_ptr->grid_array[y][x].m_idx)
			{
				remove_mirror(caster_ptr, y, x);
			}
		}
	}
}


/*!
 * @brief 領域魔法に応じて技能の名称を返す。
 * @param tval 魔法書のtval
 * @return 領域魔法の技能名称を保管した文字列ポインタ
 */
concptr spell_category_name(OBJECT_TYPE_VALUE tval)
{
	switch (tval)
	{
	case TV_HISSATSU_BOOK:
		return _("必殺技", "art");
	case TV_LIFE_BOOK:
		return _("祈り", "prayer");
	case TV_MUSIC_BOOK:
		return _("歌", "song");
	default:
		return _("呪文", "spell");
	}
}
