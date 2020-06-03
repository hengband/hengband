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

#include "system/angband.h"
#include "floor/floor.h"
#include "system/system-variables.h"
#include "util/util.h"
#include "main/sound-definitions-table.h"
#include "cmd-action/cmd-pet.h"
#include "cmd-io/cmd-dump.h"
#include "player/player-class.h"
#include "monster/monster.h"
#include "spell/spells1.h"
#include "term/gameterm.h"
#include "view/display-main-window.h"
#include "effect/spells-effect-util.h"
#include "effect/effect-feature.h"
#include "effect/effect-item.h"
#include "effect/effect-monster.h"
#include "effect/effect-characteristics.h"

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
 * @brief 領域魔法に応じて技能の名称を返す。
 * @param tval 魔法書のtval
 * @return 領域魔法の技能名称を保管した文字列ポインタ
 */
concptr spell_category_name(tval_type tval)
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
