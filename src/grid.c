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
#include "trap.h"
#include "rooms.h"
#include "monster.h"
#include "quest.h"


/*!
 * @brief 新規フロアに入りたてのプレイヤーをランダムな場所に配置する / Returns random co-ordinates for player/monster/object
 * @return 配置に成功したらTRUEを返す
 */
bool new_player_spot(void)
{
	POSITION y = 0, x = 0;
	int max_attempts = 10000;

	cave_type *c_ptr;
	feature_type *f_ptr;

	/* Place the player */
	while (max_attempts--)
	{
		/* Pick a legal spot */
		y = (POSITION)rand_range(1, cur_hgt - 2);
		x = (POSITION)rand_range(1, cur_wid - 2);

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

		break;
	}

	if (max_attempts < 1) /* Should be -1, actually if we failed... */
		return FALSE;

	/* Save the new player grid */
	p_ptr->y = y;
	p_ptr->x = x;

	return TRUE;
}



/*!
 * @brief 所定の位置に上り階段か下り階段を配置する / Place an up/down staircase at given location
 * @param y 配置を試みたいマスのY座標
 * @param x 配置を試みたいマスのX座標
 * @return なし
 */
void place_random_stairs(POSITION y, POSITION x)
{
	bool up_stairs = TRUE;
	bool down_stairs = TRUE;
	cave_type *c_ptr;

	/* Paranoia */
	c_ptr = &cave[y][x];
	if (!is_floor_grid(c_ptr) || c_ptr->o_idx) return;

	/* Town */
	if (!dun_level) up_stairs = FALSE;

	/* Ironman */
	if (ironman_downward) up_stairs = FALSE;

	/* Bottom */
	if (dun_level >= d_info[dungeon_type].maxdepth) down_stairs = FALSE;

	/* Quest-level */
	if (quest_number(dun_level) && (dun_level > 1)) down_stairs = FALSE;

	/* We can't place both */
	if (down_stairs && up_stairs)
	{
		/* Choose a staircase randomly */
		if (randint0(100) < 50) up_stairs = FALSE;
		else down_stairs = FALSE;
	}

	/* Place the stairs */
	if (up_stairs) place_up_stairs(y, x);
	else if (down_stairs) place_down_stairs(y, x);
}

/*!
 * @brief 所定の位置にさまざまな状態や種類のドアを配置する / Place a random type of door at the given location
 * @param y ドアの配置を試みたいマスのY座標
 * @param x ドアの配置を試みたいマスのX座標
 * @param room 部屋に接している場合向けのドア生成か否か
 * @return なし
 */
void place_random_door(POSITION y, POSITION x, bool room)
{
	int tmp, type;
	FEAT_IDX feat = feat_none;
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
			c_ptr->mimic = room ? feat_wall_outer : feat_wall_type[randint0(100)];

			/* Floor type terrain cannot hide a door */
			if (feat_supports_los(c_ptr->mimic) && !feat_supports_los(c_ptr->feat))
			{
				if (have_flag(f_info[c_ptr->mimic].flags, FF_MOVE) || have_flag(f_info[c_ptr->mimic].flags, FF_CAN_FLY))
				{
					c_ptr->feat = one_in_(2) ? c_ptr->mimic : feat_ground_type[randint0(100)];
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
void place_closed_door(POSITION y, POSITION x, int type)
{
	int tmp;
	FEAT_IDX feat = feat_none;

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
* @brief 鍵のかかったドアを配置する
* @param y 配置したいフロアのY座標
* @param x 配置したいフロアのX座標
* @return なし
*/
void place_locked_door(POSITION y, POSITION x)
{
	if (d_info[dungeon_type].flags1 & DF1_NO_DOORS)
	{
		place_floor_bold(y, x);
	}
	else
	{
		set_cave_feat(y, x, feat_locked_door_random((d_info[dungeon_type].flags1 & DF1_GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR));
		cave[y][x].info &= ~(CAVE_FLOOR);
		delete_monster(y, x);
	}
}


/*!
* @brief 隠しドアを配置する
* @param y 配置したいフロアのY座標
* @param x 配置したいフロアのX座標
* @param type DOOR_DEFAULT / DOOR_DOOR / DOOR_GLASS_DOOR / DOOR_CURTAIN のいずれか
* @return なし
*/
void place_secret_door(POSITION y, POSITION x, int type)
{
	if (d_info[dungeon_type].flags1 & DF1_NO_DOORS)
	{
		place_floor_bold(y, x);
	}
	else
	{
		cave_type *c_ptr = &cave[y][x];

		if (type == DOOR_DEFAULT)
		{
			type = ((d_info[dungeon_type].flags1 & DF1_CURTAIN) &&
				one_in_((d_info[dungeon_type].flags1 & DF1_NO_CAVE) ? 16 : 256)) ? DOOR_CURTAIN :
				((d_info[dungeon_type].flags1 & DF1_GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR);
		}

		/* Create secret door */
		place_closed_door(y, x, type);

		if (type != DOOR_CURTAIN)
		{
			/* Hide by inner wall because this is used in rooms only */
			c_ptr->mimic = feat_wall_inner;

			/* Floor type terrain cannot hide a door */
			if (feat_supports_los(c_ptr->mimic) && !feat_supports_los(c_ptr->feat))
			{
				if (have_flag(f_info[c_ptr->mimic].flags, FF_MOVE) || have_flag(f_info[c_ptr->mimic].flags, FF_CAN_FLY))
				{
					c_ptr->feat = one_in_(2) ? c_ptr->mimic : feat_ground_type[randint0(100)];
				}
				c_ptr->mimic = 0;
			}
		}

		c_ptr->info &= ~(CAVE_FLOOR);
		delete_monster(y, x);
	}
}

/*
 * Routine used by the random vault creators to add a door to a location
 * Note that range checking has to be done in the calling routine.
 *
 * The doors must be INSIDE the allocated region.
 */
void add_door(POSITION x, POSITION y)
{
	/* Need to have a wall in the center square */
	if (!is_outer_bold(y, x)) return;

	/* look at:
	*  x#x
	*  .#.
	*  x#x
	*
	*  where x=don't care
	*  .=floor, #=wall
	*/

	if (is_floor_bold(y - 1, x) && is_floor_bold(y + 1, x) &&
		(is_outer_bold(y, x - 1) && is_outer_bold(y, x + 1)))
	{
		/* secret door */
		place_secret_door(y, x, DOOR_DEFAULT);

		/* set boundarys so don't get wide doors */
		place_solid_bold(y, x - 1);
		place_solid_bold(y, x + 1);
	}


	/* look at:
	*  x#x
	*  .#.
	*  x#x
	*
	*  where x = don't care
	*  .=floor, #=wall
	*/
	if (is_outer_bold(y - 1, x) && is_outer_bold(y + 1, x) &&
		is_floor_bold(y, x - 1) && is_floor_bold(y, x + 1))
	{
		/* secret door */
		place_secret_door(y, x, DOOR_DEFAULT);

		/* set boundarys so don't get wide doors */
		place_solid_bold(y - 1, x);
		place_solid_bold(y + 1, x);
	}
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
static int next_to_corr(POSITION y1, POSITION x1)
{
	int i, k = 0;
	POSITION y, x;

	cave_type *c_ptr;

	/* Scan adjacent grids */
	for (i = 0; i < 4; i++)
	{
		/* Extract the location */
		y = y1 + ddy_ddd[i];
		x = x1 + ddx_ddd[i];
		c_ptr = &cave[y][x];

		/* Skip non floors */
		if (cave_have_flag_grid(c_ptr, FF_WALL)) continue;

		/* Skip non "empty floor" grids */
		if (!is_floor_grid(c_ptr))
			continue;

		/* Skip grids inside rooms */
		if (c_ptr->info & (CAVE_ROOM)) continue;

		/* Count these grids */
		k++;
	}

	/* Return the number of corridors */
	return (k);
}

/*!
* @brief ドアを設置可能な地形かを返す / Determine if the given location is "between" two walls, and "next to" two corridor spaces.
* @param y 判定を行いたいマスのY座標
* @param x 判定を行いたいマスのX座標
* @return ドアを設置可能ならばTRUEを返す
* @note Assumes "in_bounds(y1, x1)"
* @details
* \n
* Assumes "in_bounds(y, x)"\n
*/
static bool possible_doorway(POSITION y, POSITION x)
{
	/* Count the adjacent corridors */
	if (next_to_corr(y, x) >= 2)
	{
		/* Check Vertical */
		if (cave_have_flag_bold(y - 1, x, FF_WALL) &&
			cave_have_flag_bold(y + 1, x, FF_WALL))
		{
			return (TRUE);
		}

		/* Check Horizontal */
		if (cave_have_flag_bold(y, x - 1, FF_WALL) &&
			cave_have_flag_bold(y, x + 1, FF_WALL))
		{
			return (TRUE);
		}
	}

	/* No doorway */
	return (FALSE);
}

/*!
* @brief ドアの設置を試みる / Places door at y, x position if at least 2 walls found
* @param y 設置を行いたいマスのY座標
* @param x 設置を行いたいマスのX座標
* @return なし
*/
void try_door(POSITION y, POSITION x)
{
	/* Paranoia */
	if (!in_bounds(y, x)) return;

	/* Ignore walls */
	if (cave_have_flag_bold(y, x, FF_WALL)) return;

	/* Ignore room grids */
	if (cave[y][x].info & (CAVE_ROOM)) return;

	/* Occasional door (if allowed) */
	if ((randint0(100) < dun_tun_jct) && possible_doorway(y, x) && !(d_info[dungeon_type].flags1 & DF1_NO_DOORS))
	{
		/* Place a door */
		place_random_door(y, x, FALSE);
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
void place_floor(POSITION x1, POSITION x2, POSITION y1, POSITION y2, bool light)
{
	POSITION x, y;

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
void place_room(POSITION x1, POSITION x2, POSITION y1, POSITION y2, bool light)
{
	POSITION y, x;

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
void vault_objects(POSITION y, POSITION x, int num)
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

			if (dummy >= SAFE_MAX_ATTEMPTS && cheat_room)
			{
				msg_print(_("警告！地下室のアイテムを配置できません！", "Warning! Could not place vault object!"));
			}

			/* Require "clean" floor space */
			c_ptr = &cave[j][k];
			if (!is_floor_grid(c_ptr) || c_ptr->o_idx) continue;

			if (randint0(100) < 75)
			{
				place_object(j, k, 0L);
			}
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
void vault_trap_aux(POSITION y, POSITION x, POSITION yd, POSITION xd)
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

		if (dummy >= SAFE_MAX_ATTEMPTS && cheat_room)
		{
			msg_print(_("警告！地下室のトラップを配置できません！", "Warning! Could not place vault trap!"));
		}

		/* Require "naked" floor grids */
		c_ptr = &cave[y1][x1];
		if (!is_floor_grid(c_ptr) || c_ptr->o_idx || c_ptr->m_idx) continue;

		/* Place the trap */
		place_trap(y1, x1);

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
void vault_traps(POSITION y, POSITION x, POSITION yd, POSITION xd, int num)
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
void vault_monsters(POSITION y1, POSITION x1, int num)
{
	int k, i;
	POSITION y, x;
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
void correct_dir(POSITION *rdir, POSITION *cdir, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
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
void rand_dir(POSITION *rdir, POSITION *cdir)
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
bool get_is_floor(POSITION x, POSITION y)
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
void set_floor(POSITION x, POSITION y)
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


