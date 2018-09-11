#include "angband.h"
#include "grid.h"
#include "rooms.h"
#include "generate.h"



/*!
* @brief Vault地形を回転、上下左右反転するための座標変換を返す / coordinate translation code
* @param x 変換したい点のX座標参照ポインタ
* @param y 変換したい点のY座標参照ポインタ
* @param xoffset Vault生成時の基準X座標
* @param yoffset Vault生成時の基準Y座標
* @param transno 処理ID
* @return なし
*/
static void coord_trans(POSITION *x, POSITION *y, POSITION xoffset, POSITION yoffset, int transno)
{
	int i;
	int temp;

	/*
	* transno specifies what transformation is required. (0-7)
	* The lower two bits indicate by how much the vault is rotated,
	* and the upper bit indicates a reflection.
	* This is done by using rotation matrices... however since
	* these are mostly zeros for rotations by 90 degrees this can
	* be expressed simply in terms of swapping and inverting the
	* x and y coordinates.
	*/
	for (i = 0; i < transno % 4; i++)
	{
		/* rotate by 90 degrees */
		temp = *x;
		*x = -(*y);
		*y = temp;
	}

	if (transno / 4)
	{
		/* Reflect depending on status of 3rd bit. */
		*x = -(*x);
	}

	/* Add offsets so vault stays in the first quadrant */
	*x += xoffset;
	*y += yoffset;
}


/*!
* @brief Vaultをフロアに配置する / Hack -- fill in "vault" rooms
* @param yval 生成基準Y座標
* @param xval 生成基準X座標
* @param ymax VaultのYサイズ
* @param xmax VaultのXサイズ
* @param data Vaultのデータ文字列
* @param xoffset 変換基準X座標
* @param yoffset 変換基準Y座標
* @param transno 変換ID
* @return なし
*/
static void build_vault(POSITION yval, POSITION xval, POSITION ymax, POSITION xmax, cptr data,
	POSITION xoffset, POSITION yoffset, int transno)
{
	POSITION dx, dy, x, y, i, j;
	cptr t;
	cave_type *c_ptr;

	/* Place dungeon features and objects */
	for (t = data, dy = 0; dy < ymax; dy++)
	{
		for (dx = 0; dx < xmax; dx++, t++)
		{
			/* prevent loop counter from being overwritten */
			i = dx;
			j = dy;

			/* Flip / rotate */
			coord_trans(&i, &j, xoffset, yoffset, transno);

			/* Extract the location */
			if (transno % 2 == 0)
			{
				/* no swap of x/y */
				x = xval - (xmax / 2) + i;
				y = yval - (ymax / 2) + j;
			}
			else
			{
				/* swap of x/y */
				x = xval - (ymax / 2) + i;
				y = yval - (xmax / 2) + j;
			}

			/* Hack -- skip "non-grids" */
			if (*t == ' ') continue;

			/* Access the grid */
			c_ptr = &cave[y][x];

			/* Lay down a floor */
			place_floor_grid(c_ptr);

			/* Remove any mimic */
			c_ptr->mimic = 0;

			/* Part of a vault */
			c_ptr->info |= (CAVE_ROOM | CAVE_ICKY);

			/* Analyze the grid */
			switch (*t)
			{
				/* Granite wall (outer) */
			case '%':
				place_outer_noperm_grid(c_ptr);
				break;

				/* Granite wall (inner) */
			case '#':
				place_inner_grid(c_ptr);
				break;

				/* Glass wall (inner) */
			case '$':
				place_inner_grid(c_ptr);
				c_ptr->feat = feat_glass_wall;
				break;

				/* Permanent wall (inner) */
			case 'X':
				place_inner_perm_grid(c_ptr);
				break;

				/* Permanent glass wall (inner) */
			case 'Y':
				place_inner_perm_grid(c_ptr);
				c_ptr->feat = feat_permanent_glass_wall;
				break;

				/* Treasure/trap */
			case '*':
				if (randint0(100) < 75)
				{
					place_object(y, x, 0L);
				}
				else
				{
					place_trap(y, x);
				}
				break;

				/* Secret doors */
			case '+':
				place_secret_door(y, x, DOOR_DEFAULT);
				break;

				/* Secret glass doors */
			case '-':
				place_secret_door(y, x, DOOR_GLASS_DOOR);
				if (is_closed_door(c_ptr->feat)) c_ptr->mimic = feat_glass_wall;
				break;

				/* Curtains */
			case '\'':
				place_secret_door(y, x, DOOR_CURTAIN);
				break;

				/* Trap */
			case '^':
				place_trap(y, x);
				break;

				/* Black market in a dungeon */
			case 'S':
				set_cave_feat(y, x, feat_black_market);
				store_init(NO_TOWN, STORE_BLACK);
				break;

				/* The Pattern */
			case 'p':
				set_cave_feat(y, x, feat_pattern_start);
				break;

			case 'a':
				set_cave_feat(y, x, feat_pattern_1);
				break;

			case 'b':
				set_cave_feat(y, x, feat_pattern_2);
				break;

			case 'c':
				set_cave_feat(y, x, feat_pattern_3);
				break;

			case 'd':
				set_cave_feat(y, x, feat_pattern_4);
				break;

			case 'P':
				set_cave_feat(y, x, feat_pattern_end);
				break;

			case 'B':
				set_cave_feat(y, x, feat_pattern_exit);
				break;

			case 'A':
				/* Reward for Pattern walk */
				object_level = base_level + 12;
				place_object(y, x, AM_GOOD | AM_GREAT);
				object_level = base_level;
				break;
			}
		}
	}


	/* Place dungeon monsters and objects */
	for (t = data, dy = 0; dy < ymax; dy++)
	{
		for (dx = 0; dx < xmax; dx++, t++)
		{
			/* prevent loop counter from being overwritten */
			i = dx;
			j = dy;

			/* Flip / rotate */
			coord_trans(&i, &j, xoffset, yoffset, transno);

			/* Extract the location */
			if (transno % 2 == 0)
			{
				/* no swap of x/y */
				x = xval - (xmax / 2) + i;
				y = yval - (ymax / 2) + j;
			}
			else
			{
				/* swap of x/y */
				x = xval - (ymax / 2) + i;
				y = yval - (xmax / 2) + j;
			}

			/* Hack -- skip "non-grids" */
			if (*t == ' ') continue;

			/* Analyze the symbol */
			switch (*t)
			{
				/* Monster */
			case '&':
			{
				monster_level = base_level + 5;
				place_monster(y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
				monster_level = base_level;
				break;
			}

			/* Meaner monster */
			case '@':
			{
				monster_level = base_level + 11;
				place_monster(y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
				monster_level = base_level;
				break;
			}

			/* Meaner monster, plus treasure */
			case '9':
			{
				monster_level = base_level + 9;
				place_monster(y, x, PM_ALLOW_SLEEP);
				monster_level = base_level;
				object_level = base_level + 7;
				place_object(y, x, AM_GOOD);
				object_level = base_level;
				break;
			}

			/* Nasty monster and treasure */
			case '8':
			{
				monster_level = base_level + 40;
				place_monster(y, x, PM_ALLOW_SLEEP);
				monster_level = base_level;
				object_level = base_level + 20;
				place_object(y, x, AM_GOOD | AM_GREAT);
				object_level = base_level;
				break;
			}

			/* Monster and/or object */
			case ',':
			{
				if (randint0(100) < 50)
				{
					monster_level = base_level + 3;
					place_monster(y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
					monster_level = base_level;
				}
				if (randint0(100) < 50)
				{
					object_level = base_level + 7;
					place_object(y, x, 0L);
					object_level = base_level;
				}
				break;
			}

			}
		}
	}
}



/*!
* @brief タイプ7の部屋…v_info.txtより小型vaultを生成する / Type 7 -- simple vaults (see "v_info.txt")
* @return なし
*/
bool build_type7(void)
{
	vault_type *v_ptr = NULL;
	int dummy;
	POSITION x, y;
	POSITION xval, yval;
	POSITION xoffset, yoffset;
	int transno;

	/* Pick a lesser vault */
	for (dummy = 0; dummy < SAFE_MAX_ATTEMPTS; dummy++)
	{
		/* Access a random vault record */
		v_ptr = &v_info[randint0(max_v_idx)];

		/* Accept the first lesser vault */
		if (v_ptr->typ == 7) break;
	}

	/* No lesser vault found */
	if (dummy >= SAFE_MAX_ATTEMPTS)
	{
		msg_print_wizard(CHEAT_DUNGEON, _("小型固定Vaultを配置できませんでした。", "Could not place lesser vault."));
		return FALSE;
	}

	/* pick type of transformation (0-7) */
	transno = randint0(8);

	/* calculate offsets */
	x = v_ptr->wid;
	y = v_ptr->hgt;

	/* Some huge vault cannot be ratated to fit in the dungeon */
	if (x + 2 > cur_hgt - 2)
	{
		/* Forbid 90 or 270 degree ratation */
		transno &= ~1;
	}

	coord_trans(&x, &y, 0, 0, transno);

	if (x < 0)
	{
		xoffset = -x - 1;
	}
	else
	{
		xoffset = 0;
	}

	if (y < 0)
	{
		yoffset = -y - 1;
	}
	else
	{
		yoffset = 0;
	}

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&yval, &xval, abs(y), abs(x))) return FALSE;

#ifdef FORCE_V_IDX
	v_ptr = &v_info[2];
#endif

	/* Message */
	msg_format_wizard(CHEAT_DUNGEON, _("小型Vault(%s)を生成しました。", "Lesser vault (%s)."), v_name + v_ptr->name);

	/* Hack -- Build the vault */
	build_vault(yval, xval, v_ptr->hgt, v_ptr->wid,
		v_text + v_ptr->text, xoffset, yoffset, transno);

	return TRUE;
}

/*!
* @brief タイプ8の部屋…v_info.txtより大型vaultを生成する / Type 8 -- greater vaults (see "v_info.txt")
* @return なし
*/
bool build_type8(void)
{
	vault_type *v_ptr;
	int dummy;
	POSITION xval, yval;
	POSITION x, y;
	int transno;
	int xoffset, yoffset;

	/* Pick a greater vault */
	for (dummy = 0; dummy < SAFE_MAX_ATTEMPTS; dummy++)
	{
		/* Access a random vault record */
		v_ptr = &v_info[randint0(max_v_idx)];

		/* Accept the first greater vault */
		if (v_ptr->typ == 8) break;
	}

	/* No greater vault found */
	if (dummy >= SAFE_MAX_ATTEMPTS)
	{
		msg_print_wizard(CHEAT_DUNGEON, _("大型固定Vaultを配置できませんでした。", "Could not place greater vault."));
		return FALSE;
	}

	/* pick type of transformation (0-7) */
	transno = randint0(8);

	/* calculate offsets */
	x = v_ptr->wid;
	y = v_ptr->hgt;

	/* Some huge vault cannot be ratated to fit in the dungeon */
	if (x + 2 > cur_hgt - 2)
	{
		/* Forbid 90 or 270 degree ratation */
		transno &= ~1;
	}

	coord_trans(&x, &y, 0, 0, transno);

	if (x < 0)
	{
		xoffset = -x - 1;
	}
	else
	{
		xoffset = 0;
	}

	if (y < 0)
	{
		yoffset = -y - 1;
	}
	else
	{
		yoffset = 0;
	}

	/*
	* Try to allocate space for room.  If fails, exit
	*
	* Hack -- Prepare a bit larger space (+2, +2) to
	* prevent generation of vaults with no-entrance.
	*/
	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&yval, &xval, (POSITION)(abs(y) + 2), (POSITION)(abs(x) + 2))) return FALSE;

#ifdef FORCE_V_IDX
	v_ptr = &v_info[76 + randint1(3)];
#endif

	msg_format_wizard(CHEAT_DUNGEON, _("大型固定Vault(%s)を生成しました。", "Greater vault (%s)."), v_name + v_ptr->name);

	/* Hack -- Build the vault */
	build_vault(yval, xval, v_ptr->hgt, v_ptr->wid,
		v_text + v_ptr->text, xoffset, yoffset, transno);

	return TRUE;
}
