#include "floor/floor.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "floor/floor-generate.h"
#include "floor/floor-object.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/map-screen-options.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "io/targeting.h"
#include "mind/mind-ninja.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-remover.h"
#include "monster/monster-update.h"
#include "monster-floor/place-monster-types.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object/object-generator.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player/special-defense-types.h"
#include "room/rooms.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world-object.h"
#include "world/world.h"

/*
 * The array of floor [MAX_WID][MAX_HGT].
 * Not completely allocated, that would be inefficient
 * Not completely hardcoded, that would overflow memory
 */
floor_type floor_info;

/*
 * Grid based version of "cave_empty_bold()"
 */
bool is_cave_empty_grid(player_type *player_ptr, grid_type *g_ptr)
{
	bool is_empty_grid = cave_have_flag_grid(g_ptr, FF_PLACE);
	is_empty_grid &= g_ptr->m_idx == 0;
	is_empty_grid &= !player_grid(player_ptr, g_ptr);
	return is_empty_grid;
}


bool pattern_tile(floor_type *floor_ptr, POSITION y, POSITION x)
{
	return cave_have_flag_bold(floor_ptr, y, x, FF_PATTERN);
}


/*!
* @brief 鍵のかかったドアを配置する
* @param player_ptr プレーヤーへの参照ポインタ
* @param y 配置したいフロアのY座標
* @param x 配置したいフロアのX座標
* @return なし
*/
void place_locked_door(player_type *player_ptr, POSITION y, POSITION x)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (d_info[floor_ptr->dungeon_idx].flags1 & DF1_NO_DOORS)
	{
		place_bold(player_ptr, y, x, GB_FLOOR);
		return;
	}

	set_cave_feat(floor_ptr, y, x, feat_locked_door_random((d_info[player_ptr->dungeon_idx].flags1 & DF1_GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR));
	floor_ptr->grid_array[y][x].info &= ~(CAVE_FLOOR);
	delete_monster(player_ptr, y, x);
}


/*!
* @brief 隠しドアを配置する
* @param player_ptr プレーヤーへの参照ポインタ
* @param y 配置したいフロアのY座標
* @param x 配置したいフロアのX座標
* @param type DOOR_DEFAULT / DOOR_DOOR / DOOR_GLASS_DOOR / DOOR_CURTAIN のいずれか
* @return なし
*/
void place_secret_door(player_type *player_ptr, POSITION y, POSITION x, int type)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (d_info[floor_ptr->dungeon_idx].flags1 & DF1_NO_DOORS)
	{
		place_bold(player_ptr, y, x, GB_FLOOR);
		return;
	}

	if (type == DOOR_DEFAULT)
	{
		type = ((d_info[floor_ptr->dungeon_idx].flags1 & DF1_CURTAIN) &&
			one_in_((d_info[floor_ptr->dungeon_idx].flags1 & DF1_NO_CAVE) ? 16 : 256)) ? DOOR_CURTAIN :
			((d_info[floor_ptr->dungeon_idx].flags1 & DF1_GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR);
	}

	place_closed_door(player_ptr, y, x, type);
	grid_type *g_ptr = &floor_ptr->grid_array[y][x];
	if (type != DOOR_CURTAIN)
	{
		g_ptr->mimic = feat_wall_inner;
		if (feat_supports_los(g_ptr->mimic) && !feat_supports_los(g_ptr->feat))
		{
			if (have_flag(f_info[g_ptr->mimic].flags, FF_MOVE) || have_flag(f_info[g_ptr->mimic].flags, FF_CAN_FLY))
			{
				g_ptr->feat = one_in_(2) ? g_ptr->mimic : feat_ground_type[randint0(100)];
			}

			g_ptr->mimic = 0;
		}
	}

	g_ptr->info &= ~(CAVE_FLOOR);
	delete_monster(player_ptr, y, x);
}

static int scent_when = 0;

/*
 * Characters leave scent trails for perceptive monsters to track.
 *
 * Smell is rather more limited than sound.  Many creatures cannot use
 * it at all, it doesn't extend very far outwards from the character's
 * current position, and monsters can use it to home in the character,
 * but not to run away from him.
 *
 * Smell is valued according to age.  When a character takes his turn,
 * scent is aged by one, and new scent of the current age is laid down.
 * Speedy characters leave more scent, true, but it also ages faster,
 * which makes it harder to hunt them down.
 *
 * Whenever the age count loops, most of the scent trail is erased and
 * the age of the remainder is recalculated.
 */
void update_smell(floor_type *floor_ptr, player_type *subject_ptr)
{
	/* Create a table that controls the spread of scent */
	const int scent_adjust[5][5] =
	{
		{ -1, 0, 0, 0,-1 },
		{  0, 1, 1, 1, 0 },
		{  0, 1, 2, 1, 0 },
		{  0, 1, 1, 1, 0 },
		{ -1, 0, 0, 0,-1 },
	};

	if (++scent_when == 254)
	{
		for (POSITION y = 0; y < floor_ptr->height; y++)
		{
			for (POSITION x = 0; x < floor_ptr->width; x++)
			{
				int w = floor_ptr->grid_array[y][x].when;
				floor_ptr->grid_array[y][x].when = (w > 128) ? (w - 128) : 0;
			}
		}

		scent_when = 126;
	}

	for (POSITION i = 0; i < 5; i++)
	{
		for (POSITION j = 0; j < 5; j++)
		{
			grid_type *g_ptr;
			POSITION y = i + subject_ptr->y - 2;
			POSITION x = j + subject_ptr->x - 2;
			if (!in_bounds(floor_ptr, y, x)) continue;

			g_ptr = &floor_ptr->grid_array[y][x];
			if (!cave_have_flag_grid(g_ptr, FF_MOVE) && !is_closed_door(subject_ptr, g_ptr->feat)) continue;
			if (!player_has_los_bold(subject_ptr, y, x)) continue;
			if (scent_adjust[i][j] == -1) continue;

			g_ptr->when = scent_when + scent_adjust[i][j];
		}
	}
}


/*
 * Hack -- forget the "flow" information
 */
void forget_flow(floor_type *floor_ptr)
{
	for (POSITION y = 0; y < floor_ptr->height; y++)
	{
		for (POSITION x = 0; x < floor_ptr->width; x++)
		{
			floor_ptr->grid_array[y][x].dist = 0;
			floor_ptr->grid_array[y][x].cost = 0;
			floor_ptr->grid_array[y][x].when = 0;
		}
	}
}


/*
 * Routine used by the random vault creators to add a door to a location
 * Note that range checking has to be done in the calling routine.
 *
 * The doors must be INSIDE the allocated region.
 */
void add_door(player_type *player_ptr, POSITION x, POSITION y)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (!is_outer_bold(floor_ptr, y, x)) return;

	/* look at:
	*  x#x
	*  .#.
	*  x#x
	*
	*  where x=don't care
	*  .=floor, #=wall
	*/

	if (is_floor_bold(floor_ptr, y - 1, x) && is_floor_bold(floor_ptr, y + 1, x) &&
		(is_outer_bold(floor_ptr, y, x - 1) && is_outer_bold(floor_ptr, y, x + 1)))
	{
		place_secret_door(player_ptr, y, x, DOOR_DEFAULT);
		place_bold(player_ptr, y, x - 1, GB_SOLID);
		place_bold(player_ptr, y, x + 1, GB_SOLID);
	}

	/* look at:
	*  x#x
	*  .#.
	*  x#x
	*
	*  where x = don't care
	*  .=floor, #=wall
	*/
	if (is_outer_bold(floor_ptr, y - 1, x) && is_outer_bold(floor_ptr, y + 1, x) &&
		is_floor_bold(floor_ptr, y, x - 1) && is_floor_bold(floor_ptr, y, x + 1))
	{
		place_secret_door(player_ptr, y, x, DOOR_DEFAULT);
		place_bold(player_ptr, y - 1, x, GB_SOLID);
		place_bold(player_ptr, y + 1, x, GB_SOLID);
	}
}


/*!
 * @brief 所定の位置に上り階段か下り階段を配置する / Place an up/down staircase at given location
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 配置を試みたいマスのY座標
 * @param x 配置を試みたいマスのX座標
 * @return なし
 */
void place_random_stairs(player_type *player_ptr, POSITION y, POSITION x)
{
	bool up_stairs = TRUE;
	bool down_stairs = TRUE;
	grid_type *g_ptr;
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	g_ptr = &floor_ptr->grid_array[y][x];
	if (!is_floor_grid(g_ptr) || g_ptr->o_idx) return;

	if (!floor_ptr->dun_level) up_stairs = FALSE;
	if (ironman_downward) up_stairs = FALSE;
	if (floor_ptr->dun_level >= d_info[player_ptr->dungeon_idx].maxdepth) down_stairs = FALSE;
	if (quest_number(player_ptr, floor_ptr->dun_level) && (floor_ptr->dun_level > 1)) down_stairs = FALSE;

	if (down_stairs && up_stairs)
	{
		if (randint0(100) < 50) up_stairs = FALSE;
		else down_stairs = FALSE;
	}

	if (up_stairs) set_cave_feat(floor_ptr, y, x, feat_up_stair);
	else if (down_stairs) set_cave_feat(floor_ptr, y, x, feat_down_stair);
}


/*!
 * @brief LOS(Line Of Sight / 視線が通っているか)の判定を行う。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y1 始点のy座標
 * @param x1 始点のx座標
 * @param y2 終点のy座標
 * @param x2 終点のx座標
 * @return LOSが通っているならTRUEを返す。
 * @details
 * A simple, fast, integer-based line-of-sight algorithm.  By Joseph Hall,\n
 * 4116 Brewster Drive, Raleigh NC 27606.  Email to jnh@ecemwl.ncsu.edu.\n
 *\n
 * Returns TRUE if a line of sight can be traced from (x1,y1) to (x2,y2).\n
 *\n
 * The LOS begins at the center of the tile (x1,y1) and ends at the center of\n
 * the tile (x2,y2).  If los() is to return TRUE, all of the tiles this line\n
 * passes through must be floor tiles, except for (x1,y1) and (x2,y2).\n
 *\n
 * We assume that the "mathematical corner" of a non-floor tile does not\n
 * block line of sight.\n
 *\n
 * Because this function uses (short) ints for all calculations, overflow may\n
 * occur if dx and dy exceed 90.\n
 *\n
 * Once all the degenerate cases are eliminated, the values "qx", "qy", and\n
 * "m" are multiplied by a scale factor "f1 = abs(dx * dy * 2)", so that\n
 * we can use integer arithmetic.\n
 *\n
 * We travel from start to finish along the longer axis, starting at the border\n
 * between the first and second tiles, where the y offset = .5 * slope, taking\n
 * into account the scale factor.  See below.\n
 *\n
 * Also note that this function and the "move towards target" code do NOT\n
 * share the same properties.  Thus, you can see someone, target them, and\n
 * then fire a bolt at them, but the bolt may hit a wall, not them.  However\n,
 * by clever choice of target locations, you can sometimes throw a "curve".\n
 *\n
 * Note that "line of sight" is not "reflexive" in all cases.\n
 *\n
 * Use the "projectable()" routine to test "spell/missile line of sight".\n
 *\n
 * Use the "update_view()" function to determine player line-of-sight.\n
 */
bool los(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
	POSITION dy = y2 - y1;
	POSITION dx = x2 - x1;
	POSITION ay = ABS(dy);
	POSITION ax = ABS(dx);
	if ((ax < 2) && (ay < 2)) return TRUE;

	/* Directly South/North */
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	POSITION tx, ty;
	if (!dx)
	{
		/* South -- check for walls */
		if (dy > 0)
		{
			for (ty = y1 + 1; ty < y2; ty++)
			{
				if (!cave_los_bold(floor_ptr, ty, x1)) return FALSE;
			}
		}

		/* North -- check for walls */
		else
		{
			for (ty = y1 - 1; ty > y2; ty--)
			{
				if (!cave_los_bold(floor_ptr, ty, x1)) return FALSE;
			}
		}

		/* Assume los */
		return TRUE;
	}

	/* Directly East/West */
	if (!dy)
	{
		/* East -- check for walls */
		if (dx > 0)
		{
			for (tx = x1 + 1; tx < x2; tx++)
			{
				if (!cave_los_bold(floor_ptr, y1, tx)) return FALSE;
			}
		}

		/* West -- check for walls */
		else
		{
			for (tx = x1 - 1; tx > x2; tx--)
			{
				if (!cave_los_bold(floor_ptr, y1, tx)) return FALSE;
			}
		}

		return TRUE;
	}

	POSITION sx = (dx < 0) ? -1 : 1;
	POSITION sy = (dy < 0) ? -1 : 1;

	if (ax == 1)
	{
		if (ay == 2)
		{
			if (cave_los_bold(floor_ptr, y1 + sy, x1)) return TRUE;
		}
	}
	else if (ay == 1)
	{
		if (ax == 2)
		{
			if (cave_los_bold(floor_ptr, y1, x1 + sx)) return TRUE;
		}
	}

	POSITION f2 = (ax * ay);
	POSITION f1 = f2 << 1;
	POSITION qy;
	POSITION m;
	if (ax >= ay)
	{
		qy = ay * ay;
		m = qy << 1;
		tx = x1 + sx;
		if (qy == f2)
		{
			ty = y1 + sy;
			qy -= f1;
		}
		else
		{
			ty = y1;
		}

		/* Note (below) the case (qy == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (x2 - tx)
		{
			if (!cave_los_bold(floor_ptr, ty, tx)) return FALSE;

			qy += m;

			if (qy < f2)
			{
				tx += sx;
				continue;
			}
			
			if (qy > f2)
			{
				ty += sy;
				if (!cave_los_bold(floor_ptr, ty, tx)) return FALSE;
				qy -= f1;
				tx += sx;
				continue;
			}

			ty += sy;
			qy -= f1;
			tx += sx;
		}

		return TRUE;
	}

	/* Travel vertically */
	POSITION qx = ax * ax;
	m = qx << 1;
	ty = y1 + sy;
	if (qx == f2)
	{
		tx = x1 + sx;
		qx -= f1;
	}
	else
	{
		tx = x1;
	}

	/* Note (below) the case (qx == f2), where */
	/* the LOS exactly meets the corner of a tile. */
	while (y2 - ty)
	{
		if (!cave_los_bold(floor_ptr, ty, tx)) return FALSE;

		qx += m;

		if (qx < f2)
		{
			ty += sy;
			continue;
		}

		if (qx > f2)
		{
			tx += sx;
			if (!cave_los_bold(floor_ptr, ty, tx)) return FALSE;
			qx -= f1;
			ty += sy;
			continue;
		}

		tx += sx;
		qx -= f1;
		ty += sy;
	}

	return TRUE;
}


/*
 * Determine if a bolt spell cast from (y1,x1) to (y2,x2) will arrive
 * at the final destination, assuming no monster gets in the way.
 *
 * This is slightly (but significantly) different from "los(y1,x1,y2,x2)".
 */
bool projectable(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
	u16b grid_g[512];
    int grid_n = project_path(player_ptr, grid_g, (project_length ? project_length : get_max_range(player_ptr)), y1, x1, y2, x2, 0);
	if (!grid_n) return TRUE;

	POSITION y = GRID_Y(grid_g[grid_n - 1]);
	POSITION x = GRID_X(grid_g[grid_n - 1]);
	if ((y != y2) || (x != x2)) return FALSE;

	return TRUE;
}


/*!
 * @brief 特殊な部屋地形向けにモンスターを配置する / Hack -- Place some sleeping monsters near the given location
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y1 モンスターを配置したいマスの中心Y座標
 * @param x1 モンスターを配置したいマスの中心X座標
 * @param num 配置したいモンスターの数
 * @return なし
 * @details
 * Only really called by some of the "vault" routines.
 */
void vault_monsters(player_type *player_ptr, POSITION y1, POSITION x1, int num)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	for (int k = 0; k < num; k++)
	{
		for (int i = 0; i < 9; i++)
		{
			int d = 1;
			POSITION y, x;
			scatter(player_ptr, &y, &x, y1, x1, d, 0);
			grid_type *g_ptr;
			g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
			if (!is_cave_empty_grid(player_ptr, g_ptr)) continue;

			floor_ptr->monster_level = floor_ptr->base_level + 2;
			(void)place_monster(player_ptr, y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
			floor_ptr->monster_level = floor_ptr->base_level;
		}
	}
}


/*!
 * @brief 指定された座標が地震や階段生成の対象となるマスかを返す。 / Determine if a given location may be "destroyed"
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y y座標
 * @param x x座標
 * @return 各種の変更が可能ならTRUEを返す。
 * @details
 * 条件は永久地形でなく、なおかつ該当のマスにアーティファクトが存在しないか、である。英語の旧コメントに反して＊破壊＊の抑止判定には現在使われていない。
 */
bool cave_valid_bold(floor_type *floor_ptr, POSITION y, POSITION x)
{
	grid_type *g_ptr = &floor_ptr->grid_array[y][x];
	if (cave_perma_grid(g_ptr)) return FALSE;

	OBJECT_IDX next_o_idx = 0;
	for (OBJECT_IDX this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;
		o_ptr = &floor_ptr->o_list[this_o_idx];
		next_o_idx = o_ptr->next_o_idx;
		if (object_is_artifact(o_ptr)) return FALSE;
	}

	return TRUE;
}


/*
 * Change the "feat" flag for a grid, and notice/redraw the grid
 */
void cave_set_feat(player_type *player_ptr, POSITION y, POSITION x, FEAT_IDX feat)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	grid_type *g_ptr = &floor_ptr->grid_array[y][x];
	feature_type *f_ptr = &f_info[feat];
	if (!current_world_ptr->character_dungeon)
	{
		g_ptr->mimic = 0;
		g_ptr->feat = feat;
		if (have_flag(f_ptr->flags, FF_GLOW) && !(d_info[floor_ptr->dungeon_idx].flags1 & DF1_DARKNESS))
		{
			for (DIRECTION i = 0; i < 9; i++)
			{
				POSITION yy = y + ddy_ddd[i];
				POSITION xx = x + ddx_ddd[i];
				if (!in_bounds2(floor_ptr, yy, xx)) continue;
				floor_ptr->grid_array[yy][xx].info |= CAVE_GLOW;
			}
		}

		return;
	}

	bool old_los = cave_have_flag_bold(floor_ptr, y, x, FF_LOS);
	bool old_mirror = is_mirror_grid(g_ptr);

	g_ptr->mimic = 0;
	g_ptr->feat = feat;
	g_ptr->info &= ~(CAVE_OBJECT);
	if (old_mirror && (d_info[floor_ptr->dungeon_idx].flags1 & DF1_DARKNESS))
	{
		g_ptr->info &= ~(CAVE_GLOW);
		if (!view_torch_grids) g_ptr->info &= ~(CAVE_MARK);

		update_local_illumination(player_ptr, y, x);
	}

	if (!have_flag(f_ptr->flags, FF_REMEMBER)) g_ptr->info &= ~(CAVE_MARK);
	if (g_ptr->m_idx) update_monster(player_ptr, g_ptr->m_idx, FALSE);

	note_spot(player_ptr, y, x);
	lite_spot(player_ptr, y, x);
	if (old_los ^ have_flag(f_ptr->flags, FF_LOS))
	{

#ifdef COMPLEX_WALL_ILLUMINATION /* COMPLEX_WALL_ILLUMINATION */

		update_local_illumination(player_ptr, y, x);

#endif /* COMPLEX_WALL_ILLUMINATION */

		player_ptr->update |= (PU_VIEW | PU_LITE | PU_MON_LITE | PU_MONSTERS);
	}

	if (!have_flag(f_ptr->flags, FF_GLOW) || (d_info[player_ptr->dungeon_idx].flags1 & DF1_DARKNESS))
		return;

	for (DIRECTION i = 0; i < 9; i++)
	{
		POSITION yy = y + ddy_ddd[i];
		POSITION xx = x + ddx_ddd[i];
		if (!in_bounds2(floor_ptr, yy, xx)) continue;

		grid_type *cc_ptr;
		cc_ptr = &floor_ptr->grid_array[yy][xx];
		cc_ptr->info |= CAVE_GLOW;

		if (player_has_los_grid(cc_ptr))
		{
			if (cc_ptr->m_idx) update_monster(player_ptr, cc_ptr->m_idx, FALSE);
			note_spot(player_ptr, yy, xx);
			lite_spot(player_ptr, yy, xx);
		}

		update_local_illumination(player_ptr, yy, xx);
	}

	if (player_ptr->special_defense & NINJA_S_STEALTH)
	{
		if (floor_ptr->grid_array[player_ptr->y][player_ptr->x].info & CAVE_GLOW) set_superstealth(player_ptr, FALSE);
	}
}


/*!
 * @brief 所定の位置にさまざまな状態や種類のドアを配置する / Place a random type of door at the given location
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y ドアの配置を試みたいマスのY座標
 * @param x ドアの配置を試みたいマスのX座標
 * @param room 部屋に接している場合向けのドア生成か否か
 * @return なし
 */
void place_random_door(player_type *player_ptr, POSITION y, POSITION x, bool room)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	grid_type *g_ptr = &floor_ptr->grid_array[y][x];
	g_ptr->mimic = 0;

	if (d_info[floor_ptr->dungeon_idx].flags1 & DF1_NO_DOORS)
	{
		place_bold(player_ptr, y, x, GB_FLOOR);
		return;
	}

	int type = ((d_info[floor_ptr->dungeon_idx].flags1 & DF1_CURTAIN) &&
		one_in_((d_info[floor_ptr->dungeon_idx].flags1 & DF1_NO_CAVE) ? 16 : 256)) ? DOOR_CURTAIN :
		((d_info[floor_ptr->dungeon_idx].flags1 & DF1_GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR);

	int tmp = randint0(1000);
	FEAT_IDX feat = feat_none;
	if (tmp < 300)
	{
		feat = feat_door[type].open;
	}
	else if (tmp < 400)
	{
		feat = feat_door[type].broken;
	}
	else if (tmp < 600)
	{
		place_closed_door(player_ptr, y, x, type);

		if (type != DOOR_CURTAIN)
		{
			g_ptr->mimic = room ? feat_wall_outer : feat_wall_type[randint0(100)];
			if (feat_supports_los(g_ptr->mimic) && !feat_supports_los(g_ptr->feat))
			{
				if (have_flag(f_info[g_ptr->mimic].flags, FF_MOVE) || have_flag(f_info[g_ptr->mimic].flags, FF_CAN_FLY))
				{
					g_ptr->feat = one_in_(2) ? g_ptr->mimic : feat_ground_type[randint0(100)];
				}
				g_ptr->mimic = 0;
			}
		}
	}
	else
	{
		place_closed_door(player_ptr, y, x, type);
	}

	if (tmp >= 400)
	{
		delete_monster(player_ptr, y, x);
		return;
	}

	if (feat != feat_none)
	{
		set_cave_feat(floor_ptr, y, x, feat);
	}
	else
	{
		place_bold(player_ptr, y, x, GB_FLOOR);
	}

	delete_monster(player_ptr, y, x);
}


/*!
 * @brief グローバルオブジェクト配列を初期化する /
 * Delete all the items when player leaves the level
 * @note we do NOT visually reflect these (irrelevant) changes
 * @details
 * Hack -- we clear the "g_ptr->o_idx" field for every grid,
 * and the "m_ptr->next_o_idx" field for every monster, since
 * we know we are clearing every object.  Technically, we only
 * clear those fields for grids/monsters containing objects,
 * and we clear it once for every such object.
 * @return なし
 */
void wipe_o_list(floor_type *floor_ptr)
{
	for (int i = 1; i < floor_ptr->o_max; i++)
	{
		object_type *o_ptr = &floor_ptr->o_list[i];
		if (!object_is_valid(o_ptr)) continue;

		if (!current_world_ptr->character_dungeon || preserve_mode)
		{
			if (object_is_fixed_artifact(o_ptr) && !object_is_known(o_ptr))
			{
				a_info[o_ptr->name1].cur_num = 0;
			}
		}

		if (object_is_held_monster(o_ptr))
		{
			monster_type *m_ptr;
			m_ptr = &floor_ptr->m_list[o_ptr->held_m_idx];
			m_ptr->hold_o_idx = 0;
			object_wipe(o_ptr);
			continue;
		}

		grid_type *g_ptr;
		POSITION y = o_ptr->iy;
		POSITION x = o_ptr->ix;

		g_ptr = &floor_ptr->grid_array[y][x];
		g_ptr->o_idx = 0;
		object_wipe(o_ptr);
	}

	floor_ptr->o_max = 1;
	floor_ptr->o_cnt = 0;
}


/*!
 * @brief 所定の位置に各種の閉じたドアを配置する / Place a random type of normal door at the given location.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y ドアの配置を試みたいマスのY座標
 * @param x ドアの配置を試みたいマスのX座標
 * @param type ドアの地形ID
 * @return なし
 */
void place_closed_door(player_type *player_ptr, POSITION y, POSITION x, int type)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (d_info[floor_ptr->dungeon_idx].flags1 & DF1_NO_DOORS)
	{
		place_bold(player_ptr, y, x, GB_FLOOR);
		return;
	}

	int tmp = randint0(400);
	FEAT_IDX feat = feat_none;
	if (tmp < 300)
	{
		/* Create closed door */
		feat = feat_door[type].closed;
	}
	else if (tmp < 399)
	{
		feat = feat_locked_door_random(type);
	}
	else
	{
		feat = feat_jammed_door_random(type);
	}

	if (feat == feat_none)
	{
		place_bold(player_ptr, y, x, GB_FLOOR);
		return;
	}

	cave_set_feat(player_ptr, y, x, feat);
	floor_ptr->grid_array[y][x].info &= ~(CAVE_MASK);
}


/*!
 * @brief 特殊な部屋向けに各種アイテムを配置する(vault_trapのサブセット) / Place a trap with a given displacement of point
 * @param y トラップを配置したいマスの中心Y座標
 * @param x トラップを配置したいマスの中心X座標
 * @param yd Y方向の配置分散マス数
 * @param xd X方向の配置分散マス数
 * @return なし
 * @details
 * Only really called by some of the "vault" routines.
 */
void vault_trap_aux(player_type *player_ptr, POSITION y, POSITION x, POSITION yd, POSITION xd)
{
	grid_type *g_ptr;
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	int y1 = y, x1 = x;
	int dummy = 0;
	for (int count = 0; count <= 5; count++)
	{
		while (dummy < SAFE_MAX_ATTEMPTS)
		{
			y1 = rand_spread(y, yd);
			x1 = rand_spread(x, xd);
			dummy++;
			if (!in_bounds(floor_ptr, y1, x1)) continue;
			break;
		}

		if (dummy >= SAFE_MAX_ATTEMPTS && cheat_room)
		{
			msg_print(_("警告！地下室のトラップを配置できません！", "Warning! Could not place vault trap!"));
		}

		g_ptr = &floor_ptr->grid_array[y1][x1];
		if (!is_floor_grid(g_ptr) || g_ptr->o_idx || g_ptr->m_idx) continue;

		place_trap(player_ptr, y1, x1);
		break;
	}
}


/*!
 * @brief 指定のマスが床系地形であるかを返す / Function that sees if a square is a floor.  (Includes range checking.)
 * @param x チェックするマスのX座標
 * @param y チェックするマスのY座標
 * @return 床系地形ならばTRUE
 */
bool get_is_floor(floor_type *floor_ptr, POSITION x, POSITION y)
{
	if (!in_bounds(floor_ptr, y, x))
	{
		return FALSE;
	}

	if (is_floor_bold(floor_ptr, y, x)) return TRUE;

	return FALSE;
}


/*!
* @brief 隣接4マスに存在する通路の数を返す / Count the number of "corridor" grids adjacent to the given grid.
* @param y1 基準となるマスのY座標
* @param x1 基準となるマスのX座標
* @return 通路の数
* @note Assumes "in_bounds(y1, x1)"
* @details
* XXX XXX This routine currently only counts actual "empty floor"\n
* grids which are not in rooms.  We might want to also count stairs,\n
* open doors, closed doors, etc.
*/
static int next_to_corr(floor_type *floor_ptr, POSITION y1, POSITION x1)
{
	int k = 0;
	for (int i = 0; i < 4; i++)
	{
		POSITION y = y1 + ddy_ddd[i];
		POSITION x = x1 + ddx_ddd[i];
		grid_type *g_ptr;
		g_ptr = &floor_ptr->grid_array[y][x];

		if (cave_have_flag_grid(g_ptr, FF_WALL)) continue;
		if (!is_floor_grid(g_ptr)) continue;
		if (g_ptr->info & (CAVE_ROOM)) continue;

		k++;
	}

	return k;
}

/*!
* @brief ドアを設置可能な地形かを返す / Determine if the given location is "between" two walls, and "next to" two corridor spaces.
* @param y 判定を行いたいマスのY座標
* @param x 判定を行いたいマスのX座標
* @return ドアを設置可能ならばTRUEを返す
* @note Assumes "in_bounds()"
* @details
* \n
* Assumes "in_bounds()"\n
*/
static bool possible_doorway(floor_type *floor_ptr, POSITION y, POSITION x)
{
	if (next_to_corr(floor_ptr, y, x) < 2) return FALSE;

	/* Check Vertical */
	if (cave_have_flag_bold(floor_ptr, y - 1, x, FF_WALL) &&
		cave_have_flag_bold(floor_ptr, y + 1, x, FF_WALL))
	{
		return TRUE;
	}

	/* Check Horizontal */
	if (cave_have_flag_bold(floor_ptr, y, x - 1, FF_WALL) &&
		cave_have_flag_bold(floor_ptr, y, x + 1, FF_WALL))
	{
		return TRUE;
	}

	return FALSE;
}


/*!
* @brief ドアの設置を試みる / Places door at y, x position if at least 2 walls found
* @param player_ptr プレーヤーへの参照ポインタ
* @param y 設置を行いたいマスのY座標
* @param x 設置を行いたいマスのX座標
* @return なし
*/
void try_door(player_type *player_ptr, POSITION y, POSITION x)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (!in_bounds(floor_ptr, y, x)) return;

	if (cave_have_flag_bold(floor_ptr, y, x, FF_WALL)) return;
	if (floor_ptr->grid_array[y][x].info & (CAVE_ROOM)) return;

	bool can_place_door = randint0(100) < dun_tun_jct;
	can_place_door &= possible_doorway(floor_ptr, y, x);
	can_place_door &= (d_info[player_ptr->dungeon_idx].flags1 & DF1_NO_DOORS) == 0;
	if (can_place_door)
	{
		place_random_door(player_ptr, y, x, FALSE);
	}
}


FEAT_IDX conv_dungeon_feat(floor_type *floor_ptr, FEAT_IDX newfeat)
{
	feature_type *f_ptr = &f_info[newfeat];
	if (have_flag(f_ptr->flags, FF_CONVERT))
	{
		switch (f_ptr->subtype)
		{
		case CONVERT_TYPE_FLOOR:
			return feat_ground_type[randint0(100)];
		case CONVERT_TYPE_WALL:
			return feat_wall_type[randint0(100)];
		case CONVERT_TYPE_INNER:
			return feat_wall_inner;
		case CONVERT_TYPE_OUTER:
			return feat_wall_outer;
		case CONVERT_TYPE_SOLID:
			return feat_wall_solid;
		case CONVERT_TYPE_STREAM1:
			return d_info[floor_ptr->dungeon_idx].stream1;
		case CONVERT_TYPE_STREAM2:
			return d_info[floor_ptr->dungeon_idx].stream2;
		default:
			return newfeat;
		}
	}
	else return newfeat;
}


/*!
 * @brief 特殊な部屋向けに各種アイテムを配置する / Create up to "num" objects near the given coordinates
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 配置したい中心マスのY座標
 * @param x 配置したい中心マスのX座標
 * @param num 配置したい数
 * @return なし
 * @details
 * Only really called by some of the "vault" routines.
 */
void vault_objects(player_type *player_ptr, POSITION y, POSITION x, int num)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	for (; num > 0; --num)
	{
		int j = y, k = x;
		int dummy = 0;
		for (int i = 0; i < 11; ++i)
		{
			while (dummy < SAFE_MAX_ATTEMPTS)
			{
				j = rand_spread(y, 2);
				k = rand_spread(x, 3);
				dummy++;
				if (!in_bounds(floor_ptr, j, k)) continue;
				break;
			}

			if (dummy >= SAFE_MAX_ATTEMPTS && cheat_room)
			{
				msg_print(_("警告！地下室のアイテムを配置できません！", "Warning! Could not place vault object!"));
			}

			grid_type *g_ptr;
			g_ptr = &floor_ptr->grid_array[j][k];
			if (!is_floor_grid(g_ptr) || g_ptr->o_idx) continue;

			if (randint0(100) < 75)
			{
				place_object(player_ptr, j, k, 0L);
			}
			else
			{
				place_gold(player_ptr, j, k);
			}

			break;
		}
	}
}


/*!
 * @brief 始点から終点への直線経路を返す /
 * Determine the path taken by a projection.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param gp 経路座標リストを返す参照ポインタ
 * @param range 距離
 * @param y1 始点Y座標
 * @param x1 始点X座標
 * @param y2 終点Y座標
 * @param x2 終点X座標
 * @param flg フラグID
 * @return リストの長さ
 * @details
 * <pre>
 * The projection will always start from the grid (y1,x1), and will travel
 * towards the grid (y2,x2), touching one grid per unit of distance along
 * the major axis, and stopping when it enters the destination grid or a
 * wall grid, or has travelled the maximum legal distance of "range".
 *
 * Note that "distance" in this function (as in the "update_view()" code)
 * is defined as "MAX(dy,dx) + MIN(dy,dx)/2", which means that the player
 * actually has an "octagon of projection" not a "circle of projection".
 *
 * The path grids are saved into the grid array pointed to by "gp", and
 * there should be room for at least "range" grids in "gp".  Note that
 * due to the way in which distance is calculated, this function normally
 * uses fewer than "range" grids for the projection path, so the result
 * of this function should never be compared directly to "range".  Note
 * that the initial grid (y1,x1) is never saved into the grid array, not
 * even if the initial grid is also the final grid.
 *
 * The "flg" flags can be used to modify the behavior of this function.
 *
 * In particular, the "PROJECT_STOP" and "PROJECT_THRU" flags have the same
 * semantics as they do for the "project" function, namely, that the path
 * will stop as soon as it hits a monster, or that the path will continue
 * through the destination grid, respectively.
 *
 * The "PROJECT_JUMP" flag, which for the "project()" function means to
 * start at a special grid (which makes no sense in this function), means
 * that the path should be "angled" slightly if needed to avoid any wall
 * grids, allowing the player to "target" any grid which is in "view".
 * This flag is non-trivial and has not yet been implemented, but could
 * perhaps make use of the "vinfo" array (above).
 *
 * This function returns the number of grids (if any) in the path.  This
 * function will return zero if and only if (y1,x1) and (y2,x2) are equal.
 *
 * This algorithm is similar to, but slightly different from, the one used
 * by "update_view_los()", and very different from the one used by "los()".
 * </pre>
 */
int project_path(player_type *player_ptr, u16b *gp, POSITION range, POSITION y1, POSITION x1, POSITION y2, POSITION x2, BIT_FLAGS flg)
{
	if ((x1 == x2) && (y1 == y2)) return 0;

	POSITION y, x;
	POSITION ay, ax;
	POSITION sy, sx;
	int frac;
	int m;

	if (y2 < y1)
	{
		ay = (y1 - y2);
		sy = -1;
	}
	else
	{
		ay = (y2 - y1);
		sy = 1;
	}

	if (x2 < x1)
	{
		ax = (x1 - x2);
		sx = -1;
	}
	else
	{
		ax = (x2 - x1);
		sx = 1;
	}

	int half = (ay * ax);
	int full = half << 1;

	/* Vertical */
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	int n = 0;
	int k = 0;
	if (ay > ax)
	{
		m = ax * ax * 2;
		y = y1 + sy;
		x = x1;
		frac = m;
		if (frac > half)
		{
			x += sx;
			frac -= full;
			k++;
		}

		while (TRUE)
		{
			gp[n++] = GRID(y, x);
			if ((n + (k >> 1)) >= range) break;

			if (!(flg & (PROJECT_THRU)))
			{
				if ((x == x2) && (y == y2)) break;
			}

			if (flg & (PROJECT_DISI))
			{
				if ((n > 0) && cave_stop_disintegration(floor_ptr, y, x)) break;
			}
			else if (flg & (PROJECT_LOS))
			{
				if ((n > 0) && !cave_los_bold(floor_ptr, y, x)) break;
			}
			else if (!(flg & (PROJECT_PATH)))
			{
				if ((n > 0) && !cave_have_flag_bold(floor_ptr, y, x, FF_PROJECT)) break;
			}

			if (flg & (PROJECT_STOP))
			{
				if ((n > 0) &&
					(player_bold(player_ptr, y, x) || floor_ptr->grid_array[y][x].m_idx != 0))
					break;
			}

			if (!in_bounds(floor_ptr, y, x)) break;

			if (m)
			{
				frac += m;
				if (frac > half)
				{
					x += sx;
					frac -= full;
					k++;
				}
			}

			y += sy;
		}

		return n;
	}

	/* Horizontal */
	if (ax > ay)
	{
		m = ay * ay * 2;
		y = y1;
		x = x1 + sx;
		frac = m;
		if (frac > half)
		{
			y += sy;
			frac -= full;
			k++;
		}

		while (TRUE)
		{
			gp[n++] = GRID(y, x);
			if ((n + (k >> 1)) >= range) break;

			if (!(flg & (PROJECT_THRU)))
			{
				if ((x == x2) && (y == y2)) break;
			}

			if (flg & (PROJECT_DISI))
			{
				if ((n > 0) && cave_stop_disintegration(floor_ptr, y, x)) break;
			}
			else if (flg & (PROJECT_LOS))
			{
				if ((n > 0) && !cave_los_bold(floor_ptr, y, x)) break;
			}
			else if (!(flg & (PROJECT_PATH)))
			{
				if ((n > 0) && !cave_have_flag_bold(floor_ptr, y, x, FF_PROJECT)) break;
			}

			if (flg & (PROJECT_STOP))
			{
				if ((n > 0) &&
					(player_bold(player_ptr, y, x) || floor_ptr->grid_array[y][x].m_idx != 0))
					break;
			}

			if (!in_bounds(floor_ptr, y, x)) break;

			if (m)
			{
				frac += m;
				if (frac > half)
				{
					y += sy;
					frac -= full;
					k++;
				}
			}

			x += sx;
		}

		return n;
	}

	y = y1 + sy;
	x = x1 + sx;

	while (TRUE)
	{
		gp[n++] = GRID(y, x);
		if ((n + (n >> 1)) >= range) break;

		if (!(flg & (PROJECT_THRU)))
		{
			if ((x == x2) && (y == y2)) break;
		}

		if (flg & (PROJECT_DISI))
		{
			if ((n > 0) && cave_stop_disintegration(floor_ptr, y, x)) break;
		}
		else if (flg & (PROJECT_LOS))
		{
			if ((n > 0) && !cave_los_bold(floor_ptr, y, x)) break;
		}
		else if (!(flg & (PROJECT_PATH)))
		{
			if ((n > 0) && !cave_have_flag_bold(floor_ptr, y, x, FF_PROJECT)) break;
		}

		if (flg & (PROJECT_STOP))
		{
			if ((n > 0) &&
				(player_bold(player_ptr, y, x) || floor_ptr->grid_array[y][x].m_idx != 0))
				break;
		}

		if (!in_bounds(floor_ptr, y, x)) break;

		y += sy;
		x += sx;
	}

	return n;
}


/*!
 * @brief 指定のマスを床地形に変える / Set a square to be floor.  (Includes range checking.)
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param x 地形を変えたいマスのX座標
 * @param y 地形を変えたいマスのY座標
 * @return なし
 */
void set_floor(player_type *player_ptr, POSITION x, POSITION y)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (!in_bounds(floor_ptr, y, x))
	{
		return;
	}

	if (floor_ptr->grid_array[y][x].info & CAVE_ROOM)
	{
		return;
	}

	if (is_extra_bold(floor_ptr, y, x))
		place_bold(player_ptr, y, x, GB_FLOOR);
}


/*!
 * @brief フロアの指定位置に生成階に応じたベースアイテムの生成を行う。
 * Attempt to place an object (normal or good/great) at the given location.
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 * @param mode オプションフラグ
 * @return 生成に成功したらTRUEを返す。
 * @details
 * This routine plays nasty games to generate the "special artifacts".\n
 * This routine uses "object_level" for the "generation level".\n
 * This routine requires a clean floor grid destination.\n
 */
void place_object(player_type *owner_ptr, POSITION y, POSITION x, BIT_FLAGS mode)
{
	floor_type *floor_ptr = owner_ptr->current_floor_ptr;
	grid_type *g_ptr = &floor_ptr->grid_array[y][x];
	object_type forge;
	object_type *q_ptr;
	if (!in_bounds(floor_ptr, y, x)) return;
	if (!cave_drop_bold(floor_ptr, y, x)) return;
	if (g_ptr->o_idx) return;

	q_ptr = &forge;
	object_wipe(q_ptr);
	if (!make_object(owner_ptr, q_ptr, mode)) return;

	OBJECT_IDX o_idx = o_pop(floor_ptr);
	if (o_idx == 0)
	{
		if (object_is_fixed_artifact(q_ptr))
		{
			a_info[q_ptr->name1].cur_num = 0;
		}

		return;
	}

	object_type *o_ptr;
	o_ptr = &floor_ptr->o_list[o_idx];
	object_copy(o_ptr, q_ptr);

	o_ptr->iy = y;
	o_ptr->ix = x;
	o_ptr->next_o_idx = g_ptr->o_idx;

	g_ptr->o_idx = o_idx;
	note_spot(owner_ptr, y, x);
	lite_spot(owner_ptr, y, x);
}


/*!
 * @brief フロアの指定位置に生成階に応じた財宝オブジェクトの生成を行う。
 * Places a treasure (Gold or Gems) at given location
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 * @return 生成に成功したらTRUEを返す。
 * @details
 * The location must be a legal, clean, floor grid.
 */
void place_gold(player_type *player_ptr, POSITION y, POSITION x)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	grid_type *g_ptr = &floor_ptr->grid_array[y][x];
	if (!in_bounds(floor_ptr, y, x)) return;
	if (!cave_drop_bold(floor_ptr, y, x)) return;
	if (g_ptr->o_idx) return;

	object_type forge;
	object_type *q_ptr;
	q_ptr = &forge;
	object_wipe(q_ptr);
	if (!make_gold(player_ptr, q_ptr)) return;

	OBJECT_IDX o_idx = o_pop(floor_ptr);
	if (o_idx == 0) return;

	object_type *o_ptr;
	o_ptr = &floor_ptr->o_list[o_idx];
	object_copy(o_ptr, q_ptr);

	o_ptr->iy = y;
	o_ptr->ix = x;
	o_ptr->next_o_idx = g_ptr->o_idx;

	g_ptr->o_idx = o_idx;
	note_spot(player_ptr, y, x);
	lite_spot(player_ptr, y, x);
}


/*!
 * @brief 指定位置に存在するモンスターを削除する / Delete the monster, if any, at a given location
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param x 削除位置x座標
 * @param y 削除位置y座標
 * @return なし
 */
void delete_monster(player_type *player_ptr, POSITION y, POSITION x)
{
	grid_type *g_ptr;
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (!in_bounds(floor_ptr, y, x)) return;

	g_ptr = &floor_ptr->grid_array[y][x];
	if (g_ptr->m_idx) delete_monster_idx(player_ptr, g_ptr->m_idx);
}


/*!
 * @brief グローバルオブジェクト配列に対し指定範囲のオブジェクトを整理してIDの若い順に寄せる /
 * Move an object from index i1 to index i2 in the object list
 * @param i1 整理したい配列の始点
 * @param i2 整理したい配列の終点
 * @return なし
 */
static void compact_objects_aux(floor_type *floor_ptr, OBJECT_IDX i1, OBJECT_IDX i2)
{
	if (i1 == i2) return;

	object_type *o_ptr;
	for (OBJECT_IDX i = 1; i < floor_ptr->o_max; i++)
	{
		o_ptr = &floor_ptr->o_list[i];
		if (!o_ptr->k_idx) continue;

		if (o_ptr->next_o_idx == i1)
		{
			o_ptr->next_o_idx = i2;
		}
	}

	o_ptr = &floor_ptr->o_list[i1];

	if (object_is_held_monster(o_ptr))
	{
		monster_type *m_ptr;
		m_ptr = &floor_ptr->m_list[o_ptr->held_m_idx];
		if (m_ptr->hold_o_idx == i1)
		{
			m_ptr->hold_o_idx = i2;
		}
	}
	else
	{
		POSITION y = o_ptr->iy;
		POSITION x = o_ptr->ix;
		grid_type *g_ptr;
		g_ptr = &floor_ptr->grid_array[y][x];

		if (g_ptr->o_idx == i1)
		{
			g_ptr->o_idx = i2;
		}
	}

	floor_ptr->o_list[i2] = floor_ptr->o_list[i1];
	object_wipe(o_ptr);
}


/*!
 * @brief グローバルオブジェクト配列から優先度の低いものを削除し、データを圧縮する。 /
 * Compact and Reorder the object list.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param size 最低でも減らしたいオブジェクト数の水準
 * @return なし
 * @details
 * （危険なので使用には注意すること）
 * This function can be very dangerous, use with caution!\n
 *\n
 * When actually "compacting" objects, we base the saving throw on a\n
 * combination of object level, distance from player, and current\n
 * "desperation".\n
 *\n
 * After "compacting" (if needed), we "reorder" the objects into a more\n
 * compact order, and we reset the allocation info, and the "live" array.\n
 */
void compact_objects(player_type *player_ptr, int size)
{
	object_type *o_ptr;
	if (size)
	{
		msg_print(_("アイテム情報を圧縮しています...", "Compacting objects..."));
		player_ptr->redraw |= (PR_MAP);
		player_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
	}

	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	for (int num = 0, cnt = 1; num < size; cnt++)
	{
		int cur_lev = 5 * cnt;
		int cur_dis = 5 * (20 - cnt);
		for (OBJECT_IDX i = 1; i < floor_ptr->o_max; i++)
		{
			o_ptr = &floor_ptr->o_list[i];

			if (!object_is_valid(o_ptr)) continue;
			if (k_info[o_ptr->k_idx].level > cur_lev) continue;

			POSITION y, x;
			if (object_is_held_monster(o_ptr))
			{
				monster_type *m_ptr;
				m_ptr = &floor_ptr->m_list[o_ptr->held_m_idx];
				y = m_ptr->fy;
				x = m_ptr->fx;

				if (randint0(100) < 90) continue;
			}
			else
			{
				y = o_ptr->iy;
				x = o_ptr->ix;
			}

			if ((cur_dis > 0) && (distance(player_ptr->y, player_ptr->x, y, x) < cur_dis)) continue;

			int chance = 90;
			if ((object_is_fixed_artifact(o_ptr) || o_ptr->art_name) &&
				(cnt < 1000)) chance = 100;

			if (randint0(100) < chance) continue;

			delete_object_idx(player_ptr, i);
			num++;
		}
	}

	for (OBJECT_IDX i = floor_ptr->o_max - 1; i >= 1; i--)
	{
		o_ptr = &floor_ptr->o_list[i];
		if (o_ptr->k_idx) continue;

		compact_objects_aux(floor_ptr, floor_ptr->o_max - 1, i);
		floor_ptr->o_max--;
	}
}


/*!
 * @brief 特殊な部屋向けに各種アイテムを配置する(メインルーチン) / Place some traps with a given displacement of given location
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y トラップを配置したいマスの中心Y座標
 * @param x トラップを配置したいマスの中心X座標
 * @param yd Y方向の配置分散マス数
 * @param xd X方向の配置分散マス数
 * @param num 配置したいトラップの数
 * @return なし
 * @details
 * Only really called by some of the "vault" routines.
 */
void vault_traps(player_type *player_ptr, POSITION y, POSITION x, POSITION yd, POSITION xd, int num)
{
	for (int i = 0; i < num; i++)
	{
		vault_trap_aux(player_ptr, y, x, yd, xd);
	}
}


/*
 * Standard "find me a location" function
 *
 * Obtains a legal location within the given distance of the initial
 * location, and with "los()" from the source to destination location.
 *
 * This function is often called from inside a loop which searches for
 * locations while increasing the "d" distance.
 *
 * Currently the "m" parameter is unused.
 */
void scatter(player_type *player_ptr, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION d, BIT_FLAGS mode)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	POSITION nx, ny;
	while (TRUE)
	{
		ny = rand_spread(y, d);
		nx = rand_spread(x, d);

		if (!in_bounds(floor_ptr, ny, nx)) continue;
		if ((d > 1) && (distance(y, x, ny, nx) > d)) continue;
		if (mode & PROJECT_LOS)
		{
			if (los(player_ptr, y, x, ny, nx)) break;
			continue;
		}

		if (projectable(player_ptr, y, x, ny, nx)) break;
	}

	*yp = ny;
	*xp = nx;
}
