/*!
 * @file grid.c
 * @brief ダンジョンの生成処理の基幹部分 / low-level dungeon creation primitives
 * @date 2014/01/04
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 *\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * \n
 * 2014 Deskull Doxygen向けのコメント整理\n
 */

#include "angband.h"
#include "generate.h"
#include "grid.h"


/*!
 * @brief 新規フロアに入りたてのプレイヤーをランダムな場所に配置する / Returns random co-ordinates for player/monster/object
 * @return 配置に成功したらTRUEを返す
 */
bool new_player_spot(void)
{
	int	y, x;
	int max_attempts = 10000;

	cave_type *c_ptr;
	feature_type *f_ptr;

	/* Place the player */
	while (max_attempts--)
	{
		/* Pick a legal spot */
		y = rand_range(1, cur_hgt - 2);
		x = rand_range(1, cur_wid - 2);

		c_ptr = &cave[y][x];

		/* Must be a "naked" floor grid */
		if (c_ptr->m_idx) continue;
		if (dun_level)
		{
			f_ptr = &f_info[c_ptr->feat];

			if (max_attempts > 5000) /* Rule 1 */
			{
				if (!have_flag(f_ptr->flags, FF_FLOOR)) continue;
			}
			else /* Rule 2 */
			{
				if (!have_flag(f_ptr->flags, FF_MOVE)) continue;
				if (have_flag(f_ptr->flags, FF_HIT_TRAP)) continue;
			}

			/* Refuse to start on anti-teleport grids in dungeon */
			if (!have_flag(f_ptr->flags, FF_TELEPORTABLE)) continue;
		}
		if (!player_can_enter(c_ptr->feat, 0)) continue;
		if (!in_bounds(y, x)) continue;

		/* Refuse to start on anti-teleport grids */
		if (c_ptr->info & (CAVE_ICKY)) continue;

		/* Done */
		break;
	}

	if (max_attempts < 1) /* Should be -1, actually if we failed... */
		return FALSE;

	/* Save the new player grid */
	py = y;
	px = x;

	return TRUE;
}



/*!
 * @brief 所定の位置に上り階段か下り階段を配置する / Place an up/down staircase at given location
 * @param y 配置を試みたいマスのY座標
 * @param x 配置を試みたいマスのX座標
 * @return なし
 */
void place_random_stairs(int y, int x)
{
	bool up_stairs = TRUE;
	bool down_stairs = TRUE;
	cave_type *c_ptr;

	/* Paranoia */
	c_ptr = &cave[y][x];
	if (!is_floor_grid(c_ptr) || c_ptr->o_idx) return;

	/* Town */
	if (!dun_level)
		up_stairs = FALSE;

	/* Ironman */
	if (ironman_downward)
		up_stairs = FALSE;

	/* Bottom */
	if (dun_level >= d_info[dungeon_type].maxdepth)
		down_stairs = FALSE;

	/* Quest-level */
	if (quest_number(dun_level) && (dun_level > 1))
		down_stairs = FALSE;

	/* We can't place both */
	if (down_stairs && up_stairs)
	{
		/* Choose a staircase randomly */
		if (randint0(100) < 50)
			up_stairs = FALSE;
		else
			down_stairs = FALSE;
	}

	/* Place the stairs */
	if (up_stairs)
		place_up_stairs(y, x);
	else if (down_stairs)
		place_down_stairs(y, x);
}

/*!
 * @brief 所定の位置にさまざまな状態や種類のドアを配置する / Place a random type of door at the given location
 * @param y ドアの配置を試みたいマスのY座標
 * @param x ドアの配置を試みたいマスのX座標
 * @param room 部屋に接している場合向けのドア生成か否か
 * @return なし
 */
void place_random_door(int y, int x, bool room)
{
	int tmp, type;
	s16b feat = feat_none;
	cave_type *c_ptr = &cave[y][x];

	/* Initialize mimic info */
	c_ptr->mimic = 0;

	if (d_info[dungeon_type].flags1 & DF1_NO_DOORS)
	{
		place_floor_bold(y, x);
		return;
	}

	type = ((d_info[dungeon_type].flags1 & DF1_CURTAIN) &&
		one_in_((d_info[dungeon_type].flags1 & DF1_NO_CAVE) ? 16 : 256)) ? DOOR_CURTAIN :
		((d_info[dungeon_type].flags1 & DF1_GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR);

	/* Choose an object */
	tmp = randint0(1000);

	/* Open doors (300/1000) */
	if (tmp < 300)
	{
		/* Create open door */
		feat = feat_door[type].open;
	}

	/* Broken doors (100/1000) */
	else if (tmp < 400)
	{
		/* Create broken door */
		feat = feat_door[type].broken;
	}

	/* Secret doors (200/1000) */
	else if (tmp < 600)
	{
		/* Create secret door */
		place_closed_door(y, x, type);

		if (type != DOOR_CURTAIN)
		{
			/* Hide. If on the edge of room, use outer wall. */
			c_ptr->mimic = room ? feat_wall_outer : fill_type[randint0(100)];

			/* Floor type terrain cannot hide a door */
			if (feat_supports_los(c_ptr->mimic) && !feat_supports_los(c_ptr->feat))
			{
				if (have_flag(f_info[c_ptr->mimic].flags, FF_MOVE) || have_flag(f_info[c_ptr->mimic].flags, FF_CAN_FLY))
				{
					c_ptr->feat = one_in_(2) ? c_ptr->mimic : floor_type[randint0(100)];
				}
				c_ptr->mimic = 0;
			}
		}
	}

	/* Closed, locked, or stuck doors (400/1000) */
	else place_closed_door(y, x, type);

	if (tmp < 400)
	{
		if (feat != feat_none)
		{
			set_cave_feat(y, x, feat);
		}
		else
		{
			place_floor_bold(y, x);
		}
	}

	delete_monster(y, x);
}

/*!
 * @brief 所定の位置に各種の閉じたドアを配置する / Place a random type of normal door at the given location.
 * @param y ドアの配置を試みたいマスのY座標
 * @param x ドアの配置を試みたいマスのX座標
 * @param type ドアの地形ID
 * @return なし
 */
void place_closed_door(int y, int x, int type)
{
	int tmp;
	s16b feat = feat_none;

	if (d_info[dungeon_type].flags1 & DF1_NO_DOORS)
	{
		place_floor_bold(y, x);
		return;
	}

	/* Choose an object */
	tmp = randint0(400);

	/* Closed doors (300/400) */
	if (tmp < 300)
	{
		/* Create closed door */
		feat = feat_door[type].closed;
	}

	/* Locked doors (99/400) */
	else if (tmp < 399)
	{
		/* Create locked door */
		feat = feat_locked_door_random(type);
	}

	/* Stuck doors (1/400) */
	else
	{
		/* Create jammed door */
		feat = feat_jammed_door_random(type);
	}

	if (feat != feat_none)
	{
		cave_set_feat(y, x, feat);

		/* Now it is not floor */
		cave[y][x].info &= ~(CAVE_MASK);
	}
	else
	{
		place_floor_bold(y, x);
	}
}

/*!
 * @brief 長方形の空洞を生成する / Make an empty square floor, for the middle of rooms
 * @param x1 長方形の左端X座標(-1)
 * @param x2 長方形の右端X座標(+1)
 * @param y1 長方形の上端Y座標(-1)
 * @param y2 長方形の下端Y座標(+1)
 * @param light 照明の有無
 * @return なし
 */
void place_floor(int x1, int x2, int y1, int y2, bool light)
{
	int x, y;

	/* Place a full floor under the room */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
			place_floor_bold(y, x);
			add_cave_info(y, x, CAVE_ROOM);
			if (light) add_cave_info(y, x, CAVE_GLOW);
		}
	}
}


/*!
 * @brief 長方形の部屋を生成する / Make an empty square room, only floor and wall grids
 * @param x1 長方形の左端X座標(-1)
 * @param x2 長方形の右端X座標(+1)
 * @param y1 長方形の上端Y座標(-1)
 * @param y2 長方形の下端Y座標(+1)
 * @param light 照明の有無
 * @return なし
 */
void place_room(int x1, int x2, int y1, int y2, bool light)
{
	int y, x;

	place_floor(x1, x2, y1, y2, light);

	/* Walls around the room */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		place_outer_bold(y, x1 - 1);
		place_outer_bold(y, x2 + 1);
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
		place_outer_bold(y1 - 1, x);
		place_outer_bold(y2 + 1, x);
	}
}


/*!
 * @brief 特殊な部屋向けに各種アイテムを配置する / Create up to "num" objects near the given coordinates
 * @param y 配置したい中心マスのY座標
 * @param x 配置したい中心マスのX座標
 * @param num 配置したい数
 * @return なし
 * @details
 * Only really called by some of the "vault" routines.
 */
void vault_objects(int y, int x, int num)
{
	int dummy = 0;
	int i = 0, j = y, k = x;

	cave_type *c_ptr;


	/* Attempt to place 'num' objects */
	for (; num > 0; --num)
	{
		/* Try up to 11 spots looking for empty space */
		for (i = 0; i < 11; ++i)
		{
			/* Pick a random location */
			while (dummy < SAFE_MAX_ATTEMPTS)
			{
				j = rand_spread(y, 2);
				k = rand_spread(x, 3);
				dummy++;
				if (!in_bounds(j, k)) continue;
				break;
			}


			if (dummy >= SAFE_MAX_ATTEMPTS)
			{
				if (cheat_room)
				{
#ifdef JP
msg_print("警告！地下室のアイテムを配置できません！");
#else
					msg_print("Warning! Could not place vault object!");
#endif

				}
			}


			/* Require "clean" floor space */
			c_ptr = &cave[j][k];
			if (!is_floor_grid(c_ptr) || c_ptr->o_idx) continue;

			/* Place an item */
			if (randint0(100) < 75)
			{
				place_object(j, k, 0L);
			}

			/* Place gold */
			else
			{
				place_gold(j, k);
			}

			/* Placement accomplished */
			break;
		}
	}
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
void vault_trap_aux(int y, int x, int yd, int xd)
{
	int count = 0, y1 = y, x1 = x;
	int dummy = 0;

	cave_type *c_ptr;

	/* Place traps */
	for (count = 0; count <= 5; count++)
	{
		/* Get a location */
		while (dummy < SAFE_MAX_ATTEMPTS)
		{
			y1 = rand_spread(y, yd);
			x1 = rand_spread(x, xd);
			dummy++;
			if (!in_bounds(y1, x1)) continue;
			break;
		}

		if (dummy >= SAFE_MAX_ATTEMPTS)
		{
			if (cheat_room)
			{
#ifdef JP
msg_print("警告！地下室のトラップを配置できません！");
#else
				msg_print("Warning! Could not place vault trap!");
#endif

			}
		}

		/* Require "naked" floor grids */
		c_ptr = &cave[y1][x1];
		if (!is_floor_grid(c_ptr) || c_ptr->o_idx || c_ptr->m_idx) continue;

		/* Place the trap */
		place_trap(y1, x1);

		/* Done */
		break;
	}
}

/*!
 * @brief 特殊な部屋向けに各種アイテムを配置する(メインルーチン) / Place some traps with a given displacement of given location
 * @param y トラップを配置したいマスの中心Y座標
 * @param x トラップを配置したいマスの中心X座標
 * @param yd Y方向の配置分散マス数
 * @param xd X方向の配置分散マス数
 * @param num 配置したいトラップの数
 * @return なし
 * @details
 * Only really called by some of the "vault" routines.
 */
void vault_traps(int y, int x, int yd, int xd, int num)
{
	int i;

	for (i = 0; i < num; i++)
	{
		vault_trap_aux(y, x, yd, xd);
	}
}

/*!
 * @brief 特殊な部屋地形向けにモンスターを配置する / Hack -- Place some sleeping monsters near the given location
 * @param y1 モンスターを配置したいマスの中心Y座標
 * @param x1 モンスターを配置したいマスの中心X座標
 * @param num 配置したいモンスターの数
 * @return なし
 * @details
 * Only really called by some of the "vault" routines.
 */
void vault_monsters(int y1, int x1, int num)
{
	int k, i, y, x;
	cave_type *c_ptr;

	/* Try to summon "num" monsters "near" the given location */
	for (k = 0; k < num; k++)
	{
		/* Try nine locations */
		for (i = 0; i < 9; i++)
		{
			int d = 1;

			/* Pick a nearby location */
			scatter(&y, &x, y1, x1, d, 0);

			/* Require "empty" floor grids */
			c_ptr = &cave[y][x];
			if (!cave_empty_grid(c_ptr)) continue;

			/* Place the monster (allow groups) */
			monster_level = base_level + 2;
			(void)place_monster(y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
			monster_level = base_level;
		}
	}
}


/*!
 * @brief build_tunnel用に通路を掘るための方向を位置関係通りに決める / Always picks a correct direction
 * @param rdir Y方向に取るべきベクトル値を返す参照ポインタ
 * @param cdir X方向に取るべきベクトル値を返す参照ポインタ
 * @param y1 始点Y座標
 * @param x1 始点X座標
 * @param y2 終点Y座標
 * @param x2 終点X座標
 * @return なし
 */
void correct_dir(int *rdir, int *cdir, int y1, int x1, int y2, int x2)
{
	/* Extract vertical and horizontal directions */
	*rdir = (y1 == y2) ? 0 : (y1 < y2) ? 1 : -1;
	*cdir = (x1 == x2) ? 0 : (x1 < x2) ? 1 : -1;

	/* Never move diagonally */
	if (*rdir && *cdir)
	{
		if (randint0(100) < 50)
			*rdir = 0;
		else
			*cdir = 0;
	}
}

/*!
 * @brief build_tunnel用に通路を掘るための方向をランダムに決める / Pick a random direction
 * @param rdir Y方向に取るべきベクトル値を返す参照ポインタ
 * @param cdir X方向に取るべきベクトル値を返す参照ポインタ
 * @return なし
 */
void rand_dir(int *rdir, int *cdir)
{
	/* Pick a random direction */
	int i = randint0(4);

	/* Extract the dy/dx components */
	*rdir = ddy_ddd[i];
	*cdir = ddx_ddd[i];
}

/*!
 * @brief 指定のマスが床系地形であるかを返す / Function that sees if a square is a floor.  (Includes range checking.)
 * @param x チェックするマスのX座標
 * @param y チェックするマスのY座標
 * @return 床系地形ならばTRUE
 */
bool get_is_floor(int x, int y)
{
	if (!in_bounds(y, x))
	{
		/* Out of bounds */
		return (FALSE);
	}

	/* Do the real check */
	if (is_floor_bold(y, x)) return (TRUE);

	return (FALSE);
}

/*!
 * @brief 指定のマスを床地形に変える / Set a square to be floor.  (Includes range checking.)
 * @param x 地形を変えたいマスのX座標
 * @param y 地形を変えたいマスのY座標
 * @return なし
 */
void set_floor(int x, int y)
{
	if (!in_bounds(y, x))
	{
		/* Out of bounds */
		return;
	}

	if (cave[y][x].info & CAVE_ROOM)
	{
		/* A room border don't touch. */
		return;
	}

	/* Set to be floor if is a wall (don't touch lakes). */
	if (is_extra_bold(y, x))
		place_floor_bold(y, x);
}


/*!
 * @brief 部屋間のトンネルを生成する / Constructs a tunnel between two points
 * @param row1 始点Y座標
 * @param col1 始点X座標
 * @param row2 終点Y座標
 * @param col2 終点X座標
 * @return 生成に成功したらTRUEを返す
 * @details
 * This function must be called BEFORE any streamers are created,\n
 * since we use the special "granite wall" sub-types to keep track\n
 * of legal places for corridors to pierce rooms.\n
 *\n
 * We use "door_flag" to prevent excessive construction of doors\n
 * along overlapping corridors.\n
 *\n
 * We queue the tunnel grids to prevent door creation along a corridor\n
 * which intersects itself.\n
 *\n
 * We queue the wall piercing grids to prevent a corridor from leaving\n
 * a room and then coming back in through the same entrance.\n
 *\n
 * We "pierce" grids which are "outer" walls of rooms, and when we\n
 * do so, we change all adjacent "outer" walls of rooms into "solid"\n
 * walls so that no two corridors may use adjacent grids for exits.\n
 *\n
 * The "solid" wall check prevents corridors from "chopping" the\n
 * corners of rooms off, as well as "silly" door placement, and\n
 * "excessively wide" room entrances.\n
 *\n
 * Kind of walls:\n
 *   extra -- walls\n
 *   inner -- inner room walls\n
 *   outer -- outer room walls\n
 *   solid -- solid room walls\n
 */
bool build_tunnel(int row1, int col1, int row2, int col2)
{
	int y, x;
	int tmp_row, tmp_col;
	int row_dir, col_dir;
	int start_row, start_col;
	int main_loop_count = 0;

	bool door_flag = FALSE;

	cave_type *c_ptr;

	/* Save the starting location */
	start_row = row1;
	start_col = col1;

	/* Start out in the correct direction */
	correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

	/* Keep going until done (or bored) */
	while ((row1 != row2) || (col1 != col2))
	{
		/* Mega-Hack -- Paranoia -- prevent infinite loops */
		if (main_loop_count++ > 2000) return FALSE;

		/* Allow bends in the tunnel */
		if (randint0(100) < dun_tun_chg)
		{
			/* Acquire the correct direction */
			correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

			/* Random direction */
			if (randint0(100) < dun_tun_rnd)
			{
				rand_dir(&row_dir, &col_dir);
			}
		}

		/* Get the next location */
		tmp_row = row1 + row_dir;
		tmp_col = col1 + col_dir;


		/* Extremely Important -- do not leave the dungeon */
		while (!in_bounds(tmp_row, tmp_col))
		{
			/* Acquire the correct direction */
			correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

			/* Random direction */
			if (randint0(100) < dun_tun_rnd)
			{
				rand_dir(&row_dir, &col_dir);
			}

			/* Get the next location */
			tmp_row = row1 + row_dir;
			tmp_col = col1 + col_dir;
		}


		/* Access the location */
		c_ptr = &cave[tmp_row][tmp_col];

		/* Avoid "solid" walls */
		if (is_solid_grid(c_ptr)) continue;

		/* Pierce "outer" walls of rooms */
		if (is_outer_grid(c_ptr))
		{
			/* Acquire the "next" location */
			y = tmp_row + row_dir;
			x = tmp_col + col_dir;

			/* Hack -- Avoid outer/solid walls */
			if (is_outer_bold(y, x)) continue;
			if (is_solid_bold(y, x)) continue;

			/* Accept this location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Save the wall location */
			if (dun->wall_n < WALL_MAX)
			{
				dun->wall[dun->wall_n].y = row1;
				dun->wall[dun->wall_n].x = col1;
				dun->wall_n++;
			}
			else return FALSE;

			/* Forbid re-entry near this piercing */
			for (y = row1 - 1; y <= row1 + 1; y++)
			{
				for (x = col1 - 1; x <= col1 + 1; x++)
				{
					/* Convert adjacent "outer" walls as "solid" walls */
					if (is_outer_bold(y, x))
					{
						/* Change the wall to a "solid" wall */
						place_solid_noperm_bold(y, x);
					}
				}
			}
		}

		/* Travel quickly through rooms */
		else if (c_ptr->info & (CAVE_ROOM))
		{
			/* Accept the location */
			row1 = tmp_row;
			col1 = tmp_col;
		}

		/* Tunnel through all other walls */
		else if (is_extra_grid(c_ptr) || is_inner_grid(c_ptr) || is_solid_grid(c_ptr))
		{
			/* Accept this location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Save the tunnel location */
			if (dun->tunn_n < TUNN_MAX)
			{
				dun->tunn[dun->tunn_n].y = row1;
				dun->tunn[dun->tunn_n].x = col1;
				dun->tunn_n++;
			}
			else return FALSE;

			/* Allow door in next grid */
			door_flag = FALSE;
		}

		/* Handle corridor intersections or overlaps */
		else
		{
			/* Accept the location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Collect legal door locations */
			if (!door_flag)
			{
				/* Save the door location */
				if (dun->door_n < DOOR_MAX)
				{
					dun->door[dun->door_n].y = row1;
					dun->door[dun->door_n].x = col1;
					dun->door_n++;
				}
				else return FALSE;

				/* No door in next grid */
				door_flag = TRUE;
			}

			/* Hack -- allow pre-emptive tunnel termination */
			if (randint0(100) >= dun_tun_con)
			{
				/* Distance between row1 and start_row */
				tmp_row = row1 - start_row;
				if (tmp_row < 0) tmp_row = (-tmp_row);

				/* Distance between col1 and start_col */
				tmp_col = col1 - start_col;
				if (tmp_col < 0) tmp_col = (-tmp_col);

				/* Terminate the tunnel */
				if ((tmp_row > 10) || (tmp_col > 10)) break;
			}
		}
	}

	return TRUE;
}


/*!
 * @brief トンネル生成のための基準点を指定する。
 * @param x 基準点を指定するX座標の参照ポインタ、適時値が修正される。
 * @param y 基準点を指定するY座標の参照ポインタ、適時値が修正される。
 * @param affectwall (調査中)
 * @return なし
 * @details
 * This routine adds the square to the tunnel\n
 * It also checks for SOLID walls - and returns a nearby\n
 * non-SOLID square in (x,y) so that a simple avoiding\n
 * routine can be used. The returned boolean value reflects\n
 * whether or not this routine hit a SOLID wall.\n
 *\n
 * "affectwall" toggles whether or not this new square affects\n
 * the boundaries of rooms. - This is used by the catacomb\n
 * routine.\n
 * @todo 特に詳細な処理の意味を調査すべし
 */
static bool set_tunnel(int *x, int *y, bool affectwall)
{
	int i, j, dx, dy;

	cave_type *c_ptr = &cave[*y][*x];

	if (!in_bounds(*y, *x)) return TRUE;

	if (is_inner_grid(c_ptr))
	{
		return TRUE;
	}

	if (is_extra_bold(*y,*x))
	{
		/* Save the tunnel location */
		if (dun->tunn_n < TUNN_MAX)
		{
			dun->tunn[dun->tunn_n].y = *y;
			dun->tunn[dun->tunn_n].x = *x;
			dun->tunn_n++;

			return TRUE;
		}
		else return FALSE;
	}

	if (is_floor_bold(*y, *x))
	{
		/* Don't do anything */
		return TRUE;
	}

	if (is_outer_grid(c_ptr) && affectwall)
	{
		/* Save the wall location */
		if (dun->wall_n < WALL_MAX)
		{
			dun->wall[dun->wall_n].y = *y;
			dun->wall[dun->wall_n].x = *x;
			dun->wall_n++;
		}
		else return FALSE;

		/* Forbid re-entry near this piercing */
		for (j = *y - 1; j <= *y + 1; j++)
		{
			for (i = *x - 1; i <= *x + 1; i++)
			{
				/* Convert adjacent "outer" walls as "solid" walls */
				if (is_outer_bold(j, i))
				{
					/* Change the wall to a "solid" wall */
					place_solid_noperm_bold(j, i);
				}
			}
		}

		/* Clear mimic type */
		cave[*y][*x].mimic = 0;

		place_floor_bold(*y, *x);

		return TRUE;
	}

	if (is_solid_grid(c_ptr) && affectwall)
	{
		/* cannot place tunnel here - use a square to the side */

		/* find usable square and return value in (x,y) */

		i = 50;

		dy = 0;
		dx = 0;
		while ((i > 0) && is_solid_bold(*y + dy, *x + dx))
		{
			dy = randint0(3) - 1;
			dx = randint0(3) - 1;

			if (!in_bounds(*y + dy, *x + dx))
			{
				dx = 0;
				dy = 0;
			}

			i--;
		}

		if (i == 0)
		{
			/* Failed for some reason: hack - ignore the solidness */
			place_outer_grid(c_ptr);
			dx = 0;
			dy = 0;
		}

		/* Give new, acceptable coordinate. */
		*x = *x + dx;
		*y = *y + dy;

		return FALSE;
	}

	return TRUE;
}


/*!
 * @brief 外壁を削って「カタコンベ状」の通路を作成する / This routine creates the catacomb-like tunnels by removing extra rock.
 * @param x 基準点のX座標
 * @param y 基準点のY座標
 * @return なし
 * @details
 * Note that this routine is only called on "even" squares - so it gives
 * a natural checkerboard pattern.
 */
static void create_cata_tunnel(int x, int y)
{
	int x1, y1;

	/* Build tunnel */
	x1 = x - 1;
	y1 = y;
	set_tunnel(&x1, &y1, FALSE);

	x1 = x + 1;
	y1 = y;
	set_tunnel(&x1, &y1, FALSE);

	x1 = x;
	y1 = y - 1;
	set_tunnel(&x1, &y1, FALSE);

	x1 = x;
	y1 = y + 1;
	set_tunnel(&x1, &y1, FALSE);
}


/*!
 * @brief トンネル生成処理（詳細調査中）/ This routine does the bulk of the work in creating the new types of tunnels.
 * @return なし
 * @todo 詳細用調査
 * @details
 * It is designed to use very simple algorithms to go from (x1,y1) to (x2,y2)\n
 * It doesn't need to add any complexity - straight lines are fine.\n
 * The SOLID walls are avoided by a recursive algorithm which tries random ways\n
 * around the obstical until it works.  The number of itterations is counted, and it\n
 * this gets too large the routine exits. This should stop any crashes - but may leave\n
 * small gaps in the tunnel where there are too many SOLID walls.\n
 *\n
 * Type 1 tunnels are extremely simple - straight line from A to B.  This is only used\n
 * as a part of the dodge SOLID walls algorithm.\n
 *\n
 * Type 2 tunnels are made of two straight lines at right angles. When this is used with\n
 * short line segments it gives the "cavelike" tunnels seen deeper in the dungeon.\n
 *\n
 * Type 3 tunnels are made of two straight lines like type 2, but with extra rock removed.\n
 * This, when used with longer line segments gives the "catacomb-like" tunnels seen near\n
 * the surface.\n
 */
static void short_seg_hack(int x1, int y1, int x2, int y2, int type, int count, bool *fail)
{
	int i, x, y;
	int length;

	/* Check for early exit */
	if (!(*fail)) return;

	length = distance(x1, y1, x2, y2);

	count++;

	if ((type == 1) && (length != 0))
	{

		for (i = 0; i <= length; i++)
		{
			x = x1 + i * (x2 - x1) / length;
			y = y1 + i * (y2 - y1) / length;
			if (!set_tunnel(&x, &y, TRUE))
			{
				if (count > 50)
				{
					/* This isn't working - probably have an infinite loop */
					*fail = FALSE;
					return;
				}

				/* solid wall - so try to go around */
				short_seg_hack(x, y, x1 + (i - 1) * (x2 - x1) / length, y1 + (i - 1) * (y2 - y1) / length, 1, count, fail);
				short_seg_hack(x, y, x1 + (i + 1) * (x2 - x1) / length, y1 + (i + 1) * (y2 - y1) / length, 1, count, fail);
			}
		}
	}
	else if ((type == 2) || (type == 3))
	{
		if (x1 < x2)
		{
			for (i = x1; i <= x2; i++)
			{
				x = i;
				y = y1;
				if (!set_tunnel(&x, &y, TRUE))
				{
					/* solid wall - so try to go around */
					short_seg_hack(x, y, i - 1, y1, 1, count, fail);
					short_seg_hack(x, y, i + 1, y1, 1, count, fail);
				}
				if ((type == 3) && ((x + y) % 2))
				{
					create_cata_tunnel(i, y1);
				}
			}
		}
		else
		{
			for (i = x2; i <= x1; i++)
			{
				x = i;
				y = y1;
				if (!set_tunnel(&x, &y, TRUE))
				{
					/* solid wall - so try to go around */
					short_seg_hack(x, y, i - 1, y1, 1, count, fail);
					short_seg_hack(x, y, i + 1, y1, 1, count, fail);
				}
				if ((type == 3) && ((x + y) % 2))
				{
					create_cata_tunnel(i, y1);
				}
			}

		}
		if (y1 < y2)
		{
			for (i = y1; i <= y2; i++)
			{
				x = x2;
				y = i;
				if (!set_tunnel(&x, &y, TRUE))
				{
					/* solid wall - so try to go around */
					short_seg_hack(x, y, x2, i - 1, 1, count, fail);
					short_seg_hack(x, y, x2, i + 1, 1, count, fail);
				}
				if ((type == 3) && ((x + y) % 2))
				{
					create_cata_tunnel(x2, i);
				}
			}
		}
		else
		{
			for (i = y2; i <= y1; i++)
			{
				x = x2;
				y = i;
				if (!set_tunnel(&x, &y, TRUE))
				{
					/* solid wall - so try to go around */
					short_seg_hack(x, y, x2, i - 1, 1, count, fail);
					short_seg_hack(x, y, x2, i + 1, 1, count, fail);
				}
				if ((type == 3) && ((x + y) % 2))
				{
					create_cata_tunnel(x2, i);
				}
			}
		}
	}
}


/*!
 * @brief 特定の壁(永久壁など)を避けながら部屋間の通路を作成する / This routine maps a path from (x1, y1) to (x2, y2) avoiding SOLID walls.
 * @return なし
 * @todo 詳細用調査
 * @details
 * Permanent rock is ignored in this path finding- sometimes there is no\n
 * path around anyway -so there will be a crash if we try to find one.\n
 * This routine is much like the river creation routine in Zangband.\n
 * It works by dividing a line segment into two.  The segments are divided\n
 * until they are less than "cutoff" - when the corresponding routine from\n
 * "short_seg_hack" is called.\n
 * Note it is VERY important that the "stop if hit another passage" logic\n
 * stays as is.  Without this the dungeon turns into Swiss Cheese...\n
 */
bool build_tunnel2(int x1, int y1, int x2, int y2, int type, int cutoff)
{
	int x3, y3, dx, dy;
	int changex, changey;
	int length;
	int i;
	bool retval, firstsuccede;
	cave_type *c_ptr;

	length = distance(x1, y1, x2, y2);

	if (length > cutoff)
	{
		/*
		* Divide path in half and call routine twice.
		 */
		dx = (x2 - x1) / 2;
		dy = (y2 - y1) / 2;

		/* perturbation perpendicular to path */
		changex = (randint0(abs(dy) + 2) * 2 - abs(dy) - 1) / 2;

		/* perturbation perpendicular to path */
		changey = (randint0(abs(dx) + 2) * 2 - abs(dx) - 1) / 2;

		/* Work out "mid" ponit */
		x3 = x1 + dx + changex;
		y3 = y1 + dy + changey;

		/* See if in bounds - if not - do not perturb point */
		if (!in_bounds(y3, x3))
		{
			x3 = (x1 + x2) / 2;
			y3 = (y1 + y2) / 2;
		}
		/* cache c_ptr */
		c_ptr = &cave[y3][x3];
		if (is_solid_grid(c_ptr))
		{
			/* move midpoint a bit to avoid problem. */

			i = 50;

			dy = 0;
			dx = 0;
			while ((i > 0) && is_solid_bold(y3 + dy, x3 + dx))
			{
				dy = randint0(3) - 1;
				dx = randint0(3) - 1;
				if (!in_bounds(y3 + dy, x3 + dx))
				{
					dx = 0;
					dy = 0;
				}
				i--;
			}

			if (i == 0)
			{
				/* Failed for some reason: hack - ignore the solidness */
				place_outer_bold(y3, x3);
				dx = 0;
				dy = 0;
			}
			y3 += dy;
			x3 += dx;
			c_ptr = &cave[y3][x3];
		}

		if (is_floor_grid(c_ptr))
		{
			if (build_tunnel2(x1, y1, x3, y3, type, cutoff))
			{
				if ((cave[y3][x3].info & CAVE_ROOM) || (randint1(100) > 95))
				{
					/* do second half only if works + if have hit a room */
					retval = build_tunnel2(x3, y3, x2, y2, type, cutoff);
				}
				else
				{
					/* have hit another tunnel - make a set of doors here */
					retval = FALSE;

					/* Save the door location */
					if (dun->door_n < DOOR_MAX)
					{
						dun->door[dun->door_n].y = y3;
						dun->door[dun->door_n].x = x3;
						dun->door_n++;
					}
					else return FALSE;
				}
				firstsuccede = TRUE;
			}
			else
			{
				/* false- didn't work all the way */
				retval = FALSE;
				firstsuccede = FALSE;
			}
		}
		else
		{
			/* tunnel through walls */
			if (build_tunnel2(x1, y1, x3, y3, type, cutoff))
			{
				retval = build_tunnel2(x3, y3, x2, y2, type, cutoff);
				firstsuccede = TRUE;
			}
			else
			{
				/* false- didn't work all the way */
				retval = FALSE;
				firstsuccede = FALSE;
			}
		}
		if (firstsuccede)
		{
			/* only do this if the first half has worked */
			set_tunnel(&x3, &y3, TRUE);
		}
		/* return value calculated above */
		return retval;
	}
	else
	{
		/* Do a short segment */
		retval = TRUE;
		short_seg_hack(x1, y1, x2, y2, type, 0, &retval);

		/* Hack - ignore return value so avoid infinite loops */
		return TRUE;
	}
}

