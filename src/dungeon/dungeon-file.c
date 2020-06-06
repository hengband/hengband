/*!
 * @file init1.c
 * @brief ゲームデータ初期化1 / Initialization (part 1) -BEN-
 * @date 2014/01/28
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.\n
 * </pre>
 * @details
 * <pre>
 * This file is used to initialize various variables and arrays for the
 * Angband game.  Note the use of "fd_read()" and "fd_write()" to bypass
 * the common limitation of "read()" and "write()" to only 32767 bytes
 * at a time.
 * Several of the arrays for Angband are built from "template" files in
 * the "lib/file" directory, from which quick-load binary "image" files
 * are constructed whenever they are not present in the "lib/data"
 * directory, or if those files become obsolete, if we are allowed.
 * Warning -- the "ascii" file parsers use a minor hack to collect the
 * name and text information in a single pass.  Thus, the game will not
 * be able to load any template file with more than 20K of names or 60K
 * of text, even though technically, up to 64K should be legal.
 *
 * The code could actually be removed and placed into a "stand-alone"
 * program, but that feels a little silly, especially considering some
 * of the platforms that we currently support.
 * </pre>
 */

#include "dungeon/dungeon-file.h"
#include "cmd-item/cmd-activate.h"
#include "cmd-building/cmd-building.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/floor-object.h"
#include "floor/floor-town.h"
#include "floor/floor.h"
#include "floor/wild.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "info-reader/dungeon-info-tokens-table.h"
#include "info-reader/feature-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/kind-info-tokens-table.h"
#include "info-reader/race-info-tokens-table.h"
#include "info-reader/random-grid-effect-types.h"
#include "io/files-util.h"
#include "io/tokenizer.h"
#include "monster/monster-race.h"
#include "monster/monster.h"
#include "object-enchant/apply-magic.h"
#include "object-enchant/artifact.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/object-ego.h"
#include "object/object-generator.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "sv-definition/sv-scroll-types.h"
#include "object-enchant/tr-types.h"
#include "player/player-class.h"
#include "player/player-race.h"
#include "player/player-skill.h"
#include "realm/realm-names-table.h"
#include "room/rooms-vault.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "util/util.h"
#include "view/display-main-window.h"
#include "world/world-object.h"
#include "world/world.h"

typedef struct dungeon_grid {
    FEAT_IDX feature; /* Terrain feature */
    MONSTER_IDX monster; /* Monster */
    OBJECT_IDX object; /* Object */
    EGO_IDX ego; /* Ego-Item */
    ARTIFACT_IDX artifact; /* Artifact */
    IDX trap; /* Trap */
    BIT_FLAGS cave_info; /* Flags for CAVE_MARK, CAVE_GLOW, CAVE_ICKY, CAVE_ROOM */
    s16b special; /* Reserved for special terrain info */
    int random; /* Number of the random effect */
} dungeon_grid;

static dungeon_grid letter[255];

/*!
 * @brief パース関数に基づいてデータファイルからデータを読み取る /
 * Initialize an "*_info" array, by parsing an ascii "template" file
 * @param fp 読み取りに使うファイルポインタ
 * @param buf 読み取りに使うバッファ領域
 * @param head ヘッダ構造体
 * @param parse_info_txt_line パース関数
 * @return エラーコード
 */
errr init_info_txt(FILE *fp, char *buf, angband_header *head, parse_info_txt_func parse_info_txt_line)
{
	error_idx = -1;
	error_line = 0;

	head->name_size = 0;
	head->text_size = 0;
	head->tag_size = 0;

	errr err;
	while (my_fgets(fp, buf, 1024) == 0)
	{
		error_line++;
		if (!buf[0] || (buf[0] == '#')) continue;

		if (buf[1] != ':') return (PARSE_ERROR_GENERIC);

		if (buf[0] == 'V')
		{
			continue;
		}

		if (buf[0] != 'N' && buf[0] != 'D')
		{
			int i;
			for (i = 0; buf[i]; i++)
			{
				head->v_extra += (byte)buf[i];
				head->v_extra ^= (1 << (i % 8));
			}
		}

		if ((err = (*parse_info_txt_line)(buf, head)) != 0)
			return (err);
	}

	if (head->name_size) head->name_size++;
	if (head->text_size) head->text_size++;

	return 0;
}


/*!
 * @brief 地形情報の「F:」情報をパースする
 * Process "F:<letter>:<terrain>:<cave_info>:<monster>:<object>:<ego>:<artifact>:<trap>:<special>" -- info for dungeon grid
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @param buf 解析文字列
 * @return エラーコード
 */
static errr parse_line_feature(floor_type *floor_ptr, char *buf)
{
	if (init_flags & INIT_ONLY_BUILDINGS) return 0;

	char *zz[9];
	int num = tokenize(buf + 2, 9, zz, 0);
	if (num <= 1) return 1;

	int index = zz[0][0];
	letter[index].feature = feat_none;
	letter[index].monster = 0;
	letter[index].object = 0;
	letter[index].ego = 0;
	letter[index].artifact = 0;
	letter[index].trap = feat_none;
	letter[index].cave_info = 0;
	letter[index].special = 0;
	letter[index].random = RANDOM_NONE;

	switch (num)
	{
	case 9:
		letter[index].special = (s16b)atoi(zz[8]);
		/* Fall through */
	case 8:
		if ((zz[7][0] == '*') && !zz[7][1])
		{
			letter[index].random |= RANDOM_TRAP;
		}
		else
		{
			letter[index].trap = f_tag_to_index(zz[7]);
			if (letter[index].trap < 0) return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
		}
		/* Fall through */
	case 7:
		if (zz[6][0] == '*')
		{
			letter[index].random |= RANDOM_ARTIFACT;
			if (zz[6][1]) letter[index].artifact = (ARTIFACT_IDX)atoi(zz[6] + 1);
		}
		else if (zz[6][0] == '!')
		{
			if (floor_ptr->inside_quest)
			{
				letter[index].artifact = quest[floor_ptr->inside_quest].k_idx;
			}
		}
		else
		{
			letter[index].artifact = (ARTIFACT_IDX)atoi(zz[6]);
		}
		/* Fall through */
	case 6:
		if (zz[5][0] == '*')
		{
			letter[index].random |= RANDOM_EGO;
			if (zz[5][1]) letter[index].ego = (EGO_IDX)atoi(zz[5] + 1);
		}
		else
		{
			letter[index].ego = (EGO_IDX)atoi(zz[5]);
		}
		/* Fall through */
	case 5:
		if (zz[4][0] == '*')
		{
			letter[index].random |= RANDOM_OBJECT;
			if (zz[4][1]) letter[index].object = (OBJECT_IDX)atoi(zz[4] + 1);
		}
		else if (zz[4][0] == '!')
		{
			if (floor_ptr->inside_quest)
			{
				ARTIFACT_IDX a_idx = quest[floor_ptr->inside_quest].k_idx;
				if (a_idx)
				{
					artifact_type *a_ptr = &a_info[a_idx];
					if (!(a_ptr->gen_flags & TRG_INSTA_ART))
					{
						letter[index].object = lookup_kind(a_ptr->tval, a_ptr->sval);
					}
				}
			}
		}
		else
		{
			letter[index].object = (IDX)atoi(zz[4]);
		}
		/* Fall through */
	case 4:
		if (zz[3][0] == '*')
		{
			letter[index].random |= RANDOM_MONSTER;
			if (zz[3][1]) letter[index].monster = (IDX)atoi(zz[3] + 1);
		}
		else if (zz[3][0] == 'c')
		{
			if (!zz[3][1]) return PARSE_ERROR_GENERIC;
			letter[index].monster = -atoi(zz[3] + 1);
		}
		else
		{
			letter[index].monster = (IDX)atoi(zz[3]);
		}
		/* Fall through */
	case 3:
		letter[index].cave_info = atoi(zz[2]);
		/* Fall through */
	case 2:
		if ((zz[1][0] == '*') && !zz[1][1])
		{
			letter[index].random |= RANDOM_FEATURE;
		}
		else
		{
			letter[index].feature = f_tag_to_index(zz[1]);
			if (letter[index].feature < 0) return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
		}

		break;
	}

	return 0;
}


/*!
 * @brief 地形情報の「B:」情報をパースする
 * Process "B:<Index>:<Command>:..." -- Building definition
 * @param buf 解析文字列
 * @return エラーコード
 */
static errr parse_line_building(char *buf)
{
	char *zz[1000];
	char *s;

#ifdef JP
	if (buf[2] == '$')
		return 0;
	s = buf + 2;
#else
	if (buf[2] != '$')
		return 0;
	s = buf + 3;
#endif
	int index = atoi(s);
	s = my_strchr(s, ':');
	if (!s) return 1;

	*s++ = '\0';
	if (!*s) return 1;

	switch (s[0])
	{
	case 'N':
	{
		if (tokenize(s + 2, 3, zz, 0) == 3)
		{
			strcpy(building[index].name, zz[0]);
			strcpy(building[index].owner_name, zz[1]);
			strcpy(building[index].owner_race, zz[2]);
			break;
		}

		return (PARSE_ERROR_TOO_FEW_ARGUMENTS);
	}
	case 'A':
	{
		if (tokenize(s + 2, 8, zz, 0) >= 7)
		{
			int action_index = atoi(zz[0]);
			strcpy(building[index].act_names[action_index], zz[1]);
			building[index].member_costs[action_index] = (PRICE)atoi(zz[2]);
			building[index].other_costs[action_index] = (PRICE)atoi(zz[3]);
			building[index].letters[action_index] = zz[4][0];
			building[index].actions[action_index] = (BACT_IDX)atoi(zz[5]);
			building[index].action_restr[action_index] = (BACT_RESTRICT_IDX)atoi(zz[6]);
			break;
		}

		return (PARSE_ERROR_TOO_FEW_ARGUMENTS);
	}
	case 'C':
	{
		int n;
		n = tokenize(s + 2, MAX_CLASS, zz, 0);
		for (int i = 0; i < MAX_CLASS; i++)
		{
			building[index].member_class[i] = ((i < n) ? (player_class_type)atoi(zz[i]) : 1);
		}

		break;
	}
	case 'R':
	{
		int n;
		n = tokenize(s + 2, MAX_RACES, zz, 0);
		for (int i = 0; i < MAX_RACES; i++)
		{
			building[index].member_race[i] = ((i < n) ? (player_race_type)atoi(zz[i]) : 1);
		}

		break;
	}
	case 'M':
	{
		int n;
		n = tokenize(s + 2, MAX_MAGIC, zz, 0);
		for (int i = 0; i < MAX_MAGIC; i++)
		{
			building[index].member_realm[i + 1] = ((i < n) ? (REALM_IDX)atoi(zz[i]) : 1);
		}

		break;
	}
	case 'Z':
	{
		break;
	}
	default:
	{
		return (PARSE_ERROR_UNDEFINED_DIRECTIVE);
	}
	}

	return 0;
}


/*!
 * @brief フロアの所定のマスにオブジェクトを配置する
 * Place the object j_ptr to a grid
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @param j_ptr オブジェクト構造体の参照ポインタ
 * @param y 配置先Y座標
 * @param x 配置先X座標
 * @return エラーコード
 */
static void drop_here(floor_type *floor_ptr, object_type *j_ptr, POSITION y, POSITION x)
{
	OBJECT_IDX o_idx = o_pop(floor_ptr);
	object_type *o_ptr;
	o_ptr = &floor_ptr->o_list[o_idx];
	object_copy(o_ptr, j_ptr);
	o_ptr->iy = y;
	o_ptr->ix = x;
	o_ptr->held_m_idx = 0;
	grid_type *g_ptr = &floor_ptr->grid_array[y][x];
	o_ptr->next_o_idx = g_ptr->o_idx;
	g_ptr->o_idx = o_idx;
}


/*!
 * todo yminとymaxは本当に使われているのか？
 * @brief クエスト用固定ダンジョンをフロアに生成する
 * Parse a sub-file of the "extra info"
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param buf 文字列
 * @param ymin 詳細不明
 * @param xmin 詳細不明
 * @param ymax 詳細不明
 * @param xmax 詳細不明
 * @param y 詳細不明
 * @param x 詳細不明
 * @return エラーコード
 */
static errr process_dungeon_file_aux(player_type *player_ptr, char *buf, int ymin, int xmin, int ymax, int xmax, int *y, int *x)
{
	char *zz[33];

	if (!buf[0]) return 0;
	if (iswspace(buf[0])) return 0;
	if (buf[0] == '#') return 0;
	if (buf[1] != ':') return 1;

	if (buf[0] == '%')
	{
		return process_dungeon_file(player_ptr, buf + 2, ymin, xmin, ymax, xmax);
	}

	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	/* Process "F:<letter>:<terrain>:<cave_info>:<monster>:<object>:<ego>:<artifact>:<trap>:<special>" -- info for dungeon grid */
	if (buf[0] == 'F')
	{
		return parse_line_feature(player_ptr->current_floor_ptr, buf);
	}
	else if (buf[0] == 'D')
	{
		object_type object_type_body;
		char *s = buf + 2;
		int len = strlen(s);
		if (init_flags & INIT_ONLY_BUILDINGS) return 0;

		*x = xmin;
		for (int i = 0; ((*x < xmax) && (i < len)); (*x)++, s++, i++)
		{
			grid_type *g_ptr = &floor_ptr->grid_array[*y][*x];
			int idx = s[0];
			OBJECT_IDX object_index = letter[idx].object;
			MONSTER_IDX monster_index = letter[idx].monster;
			int random = letter[idx].random;
			ARTIFACT_IDX artifact_index = letter[idx].artifact;
			g_ptr->feat = conv_dungeon_feat(floor_ptr, letter[idx].feature);
			if (init_flags & INIT_ONLY_FEATURES) continue;

			g_ptr->info = letter[idx].cave_info;
			if (random & RANDOM_MONSTER)
			{
				floor_ptr->monster_level = floor_ptr->base_level + monster_index;

				place_monster(player_ptr, *y, *x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));

				floor_ptr->monster_level = floor_ptr->base_level;
			}
			else if (monster_index)
			{
				int old_cur_num, old_max_num;
				bool clone = FALSE;

				if (monster_index < 0)
				{
					monster_index = -monster_index;
					clone = TRUE;
				}

				old_cur_num = r_info[monster_index].cur_num;
				old_max_num = r_info[monster_index].max_num;

				if (r_info[monster_index].flags1 & RF1_UNIQUE)
				{
					r_info[monster_index].cur_num = 0;
					r_info[monster_index].max_num = 1;
				}
				else if (r_info[monster_index].flags7 & RF7_NAZGUL)
				{
					if (r_info[monster_index].cur_num == r_info[monster_index].max_num)
					{
						r_info[monster_index].max_num++;
					}
				}

				place_monster_aux(player_ptr, 0, *y, *x, monster_index, (PM_ALLOW_SLEEP | PM_NO_KAGE));
				if (clone)
				{
					floor_ptr->m_list[hack_m_idx_ii].smart |= SM_CLONED;
					r_info[monster_index].cur_num = old_cur_num;
					r_info[monster_index].max_num = old_max_num;
				}
			}

			if ((random & RANDOM_OBJECT) && (random & RANDOM_TRAP))
			{
				floor_ptr->object_level = floor_ptr->base_level + object_index;

				/*
				 * Random trap and random treasure defined
				 * 25% chance for trap and 75% chance for object
				 */
				if (randint0(100) < 75)
				{
					place_object(player_ptr, *y, *x, 0L);
				}
				else
				{
					place_trap(player_ptr, *y, *x);
				}

				floor_ptr->object_level = floor_ptr->base_level;
			}
			else if (random & RANDOM_OBJECT)
			{
				floor_ptr->object_level = floor_ptr->base_level + object_index;
				if (randint0(100) < 75)
					place_object(player_ptr, *y, *x, 0L);
				else if (randint0(100) < 80)
					place_object(player_ptr, *y, *x, AM_GOOD);
				else
					place_object(player_ptr, *y, *x, AM_GOOD | AM_GREAT);

				floor_ptr->object_level = floor_ptr->base_level;
			}
			else if (random & RANDOM_TRAP)
			{
				place_trap(player_ptr, *y, *x);
			}
			else if (letter[idx].trap)
			{
				g_ptr->mimic = g_ptr->feat;
				g_ptr->feat = conv_dungeon_feat(floor_ptr, letter[idx].trap);
			}
			else if (object_index)
			{
				object_type *o_ptr = &object_type_body;
				object_prep(o_ptr, object_index);
				if (o_ptr->tval == TV_GOLD)
				{
					coin_type = object_index - OBJ_GOLD_LIST;
					make_gold(floor_ptr, o_ptr);
					coin_type = 0;
				}

				apply_magic(player_ptr, o_ptr, floor_ptr->base_level, AM_NO_FIXED_ART | AM_GOOD);
				drop_here(floor_ptr, o_ptr, *y, *x);
			}

			if (artifact_index)
			{
				if (a_info[artifact_index].cur_num)
				{
					KIND_OBJECT_IDX k_idx = lookup_kind(TV_SCROLL, SV_SCROLL_ACQUIREMENT);
					object_type forge;
					object_type *q_ptr = &forge;

					object_prep(q_ptr, k_idx);
					drop_here(floor_ptr, q_ptr, *y, *x);
				}
				else
				{
					if (create_named_art(player_ptr, artifact_index, *y, *x))
						a_info[artifact_index].cur_num = 1;
				}
			}

			g_ptr->special = letter[idx].special;
		}

		(*y)++;
		return 0;
	}
	else if (buf[0] == 'Q')
	{
		int num;
		quest_type *q_ptr;
#ifdef JP
		if (buf[2] == '$')
			return 0;
		num = tokenize(buf + 2, 33, zz, 0);
#else
		if (buf[2] != '$')
			return 0;
		num = tokenize(buf + 3, 33, zz, 0);
#endif

		if (num < 3) return (PARSE_ERROR_TOO_FEW_ARGUMENTS);

		q_ptr = &(quest[atoi(zz[0])]);
		if (zz[1][0] == 'Q')
		{
			if (init_flags & INIT_ASSIGN)
			{
				monster_race *r_ptr;
				artifact_type *a_ptr;

				if (num < 9) return (PARSE_ERROR_TOO_FEW_ARGUMENTS);

				q_ptr->type = (QUEST_TYPE)atoi(zz[2]);
				q_ptr->num_mon = (MONSTER_NUMBER)atoi(zz[3]);
				q_ptr->cur_num = (MONSTER_NUMBER)atoi(zz[4]);
				q_ptr->max_num = (MONSTER_NUMBER)atoi(zz[5]);
				q_ptr->level = (DEPTH)atoi(zz[6]);
				q_ptr->r_idx = (IDX)atoi(zz[7]);
				q_ptr->k_idx = (IDX)atoi(zz[8]);
				q_ptr->dungeon = (DUNGEON_IDX)atoi(zz[9]);

				if (num > 10) q_ptr->flags = atoi(zz[10]);

				r_ptr = &r_info[q_ptr->r_idx];
				if (r_ptr->flags1 & RF1_UNIQUE)
					r_ptr->flags1 |= RF1_QUESTOR;

				a_ptr = &a_info[q_ptr->k_idx];
				a_ptr->gen_flags |= TRG_QUESTITEM;
			}

			return 0;
		}
		else if (zz[1][0] == 'R')
		{
			if (init_flags & INIT_ASSIGN)
			{
				int count = 0;
				IDX idx, reward_idx = 0;

				for (idx = 2; idx < num; idx++)
				{
					IDX a_idx = (IDX)atoi(zz[idx]);
					if (a_idx < 1) continue;
					if (a_info[a_idx].cur_num > 0) continue;
					count++;
					if (one_in_(count)) reward_idx = a_idx;
				}

				if (reward_idx)
				{
					q_ptr->k_idx = reward_idx;
					a_info[reward_idx].gen_flags |= TRG_QUESTITEM;
				}
				else
				{
					q_ptr->type = QUEST_TYPE_KILL_ALL;
				}
			}

			return 0;
		}
		else if (zz[1][0] == 'N')
		{
			if (init_flags & (INIT_ASSIGN | INIT_SHOW_TEXT | INIT_NAME_ONLY))
			{
				strcpy(q_ptr->name, zz[2]);
			}

			return 0;
		}
		else if (zz[1][0] == 'T')
		{
			if (init_flags & INIT_SHOW_TEXT)
			{
				strcpy(quest_text[quest_text_line], zz[2]);
				quest_text_line++;
			}

			return 0;
		}
	}
	else if (buf[0] == 'W')
	{
		return parse_line_wilderness(player_ptr, buf, xmin, xmax, y, x);
	}
	else if (buf[0] == 'P')
	{
		if (init_flags & INIT_CREATE_DUNGEON)
		{
			if (tokenize(buf + 2, 2, zz, 0) == 2)
			{
				int panels_x, panels_y;

				panels_y = (*y / SCREEN_HGT);
				if (*y % SCREEN_HGT) panels_y++;
				floor_ptr->height = panels_y * SCREEN_HGT;

				panels_x = (*x / SCREEN_WID);
				if (*x % SCREEN_WID) panels_x++;
				floor_ptr->width = panels_x * SCREEN_WID;

				panel_row_min = floor_ptr->height;
				panel_col_min = floor_ptr->width;

				if (floor_ptr->inside_quest)
				{
					delete_monster(player_ptr, player_ptr->y, player_ptr->x);

					POSITION py = atoi(zz[0]);
					POSITION px = atoi(zz[1]);

					player_ptr->y = py;
					player_ptr->x = px;
				}
				else if (!player_ptr->oldpx && !player_ptr->oldpy)
				{
					player_ptr->oldpy = atoi(zz[0]);
					player_ptr->oldpx = atoi(zz[1]);
				}
			}
		}

		return 0;
	}
	else if (buf[0] == 'B')
	{
		return parse_line_building(buf);
	}
	else if (buf[0] == 'M')
	{
		if (tokenize(buf + 2, 2, zz, 0) == 2)
		{
			if (zz[0][0] == 'T')
			{
				max_towns = (TOWN_IDX)atoi(zz[1]);
			}
			else if (zz[0][0] == 'Q')
			{
				max_q_idx = (QUEST_IDX)atoi(zz[1]);
			}
			else if (zz[0][0] == 'R')
			{
				max_r_idx = (player_race_type)atoi(zz[1]);
			}
			else if (zz[0][0] == 'K')
			{
				max_k_idx = (KIND_OBJECT_IDX)atoi(zz[1]);
			}
			else if (zz[0][0] == 'V')
			{
				max_v_idx = (VAULT_IDX)atoi(zz[1]);
			}
			else if (zz[0][0] == 'F')
			{
				max_f_idx = (FEAT_IDX)atoi(zz[1]);
			}
			else if (zz[0][0] == 'A')
			{
				max_a_idx = (ARTIFACT_IDX)atoi(zz[1]);
			}
			else if (zz[0][0] == 'E')
			{
				max_e_idx = (EGO_IDX)atoi(zz[1]);
			}
			else if (zz[0][0] == 'D')
			{
				current_world_ptr->max_d_idx = (DUNGEON_IDX)atoi(zz[1]);
			}
			else if (zz[0][0] == 'O')
			{
				current_world_ptr->max_o_idx = (OBJECT_IDX)atoi(zz[1]);
			}
			else if (zz[0][0] == 'M')
			{
				current_world_ptr->max_m_idx = (MONSTER_IDX)atoi(zz[1]);
			}
			else if (zz[0][0] == 'W')
			{
				if (zz[0][1] == 'X')
					current_world_ptr->max_wild_x = (POSITION)atoi(zz[1]);

				if (zz[0][1] == 'Y')
					current_world_ptr->max_wild_y = (POSITION)atoi(zz[1]);
			}

			return 0;
		}
	}

	return 1;
}

/*
 * todo ここから先頭に移すとコンパイル警告が出る……
 */
static char tmp[8];
static concptr variant = "ZANGBAND";

/*!
 * @brief クエスト用固定ダンジョン生成時の分岐処理
 * Helper function for "process_dungeon_file()"
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param sp
 * @param fp
 * @return エラーコード
 */
static concptr process_dungeon_file_expr(player_type *player_ptr, char **sp, char *fp)
{
	char b1 = '[';
	char b2 = ']';

	char f = ' ';

	char *s;
	s = (*sp);

	while (iswspace(*s)) s++;

	char *b;
	b = s;
	concptr v = "?o?o?";
	if (*s == b1)
	{
		concptr p;
		concptr t;
		s++;
		t = process_dungeon_file_expr(player_ptr, &s, &f);
		if (!*t)
		{
			/* Nothing */
		}
		else if (streq(t, "IOR"))
		{
			v = "0";
			while (*s && (f != b2))
			{
				t = process_dungeon_file_expr(player_ptr, &s, &f);
				if (*t && !streq(t, "0")) v = "1";
			}
		}
		else if (streq(t, "AND"))
		{
			v = "1";
			while (*s && (f != b2))
			{
				t = process_dungeon_file_expr(player_ptr, &s, &f);
				if (*t && streq(t, "0")) v = "0";
			}
		}
		else if (streq(t, "NOT"))
		{
			v = "1";
			while (*s && (f != b2))
			{
				t = process_dungeon_file_expr(player_ptr, &s, &f);
				if (*t && streq(t, "1")) v = "0";
			}
		}
		else if (streq(t, "EQU"))
		{
			v = "0";
			if (*s && (f != b2))
			{
				t = process_dungeon_file_expr(player_ptr, &s, &f);
			}

			while (*s && (f != b2))
			{
				p = process_dungeon_file_expr(player_ptr, &s, &f);
				if (streq(t, p)) v = "1";
			}
		}
		else if (streq(t, "LEQ"))
		{
			v = "1";
			if (*s && (f != b2))
			{
				t = process_dungeon_file_expr(player_ptr, &s, &f);
			}

			while (*s && (f != b2))
			{
				p = t;
				t = process_dungeon_file_expr(player_ptr, &s, &f);
				if (*t && atoi(p) > atoi(t)) v = "0";
			}
		}
		else if (streq(t, "GEQ"))
		{
			v = "1";
			if (*s && (f != b2))
			{
				t = process_dungeon_file_expr(player_ptr, &s, &f);
			}

			while (*s && (f != b2))
			{
				p = t;
				t = process_dungeon_file_expr(player_ptr, &s, &f);
				if (*t && atoi(p) < atoi(t)) v = "0";
			}
		}
		else
		{
			while (*s && (f != b2))
			{
				t = process_dungeon_file_expr(player_ptr, &s, &f);
			}
		}

		if (f != b2) v = "?x?x?";
		if ((f = *s) != '\0') *s++ = '\0';

		(*fp) = f;
		(*sp) = s;
		return v;
	}

#ifdef JP
	while (iskanji(*s) || (isprint(*s) && !my_strchr(" []", *s)))
	{
		if (iskanji(*s)) s++;
		s++;
	}
#else
	while (isprint(*s) && !my_strchr(" []", *s)) ++s;
#endif
	if ((f = *s) != '\0') *s++ = '\0';

	if (*b != '$')
	{
		v = b;
		(*fp) = f;
		(*sp) = s;
		return v;
	}

	if (streq(b + 1, "SYS"))
	{
		v = ANGBAND_SYS;
	}
	else if (streq(b + 1, "GRAF"))
	{
		v = ANGBAND_GRAF;
	}
	else if (streq(b + 1, "MONOCHROME"))
	{
		if (arg_monochrome)
			v = "ON";
		else
			v = "OFF";
	}
	else if (streq(b + 1, "RACE"))
	{
		v = _(rp_ptr->E_title, rp_ptr->title);
	}
	else if (streq(b + 1, "CLASS"))
	{
		v = _(cp_ptr->E_title, cp_ptr->title);
	}
	else if (streq(b + 1, "REALM1"))
	{
		v = _(E_realm_names[player_ptr->realm1], realm_names[player_ptr->realm1]);
	}
	else if (streq(b + 1, "REALM2"))
	{
		v = _(E_realm_names[player_ptr->realm2], realm_names[player_ptr->realm2]);
	}
	else if (streq(b + 1, "PLAYER"))
	{
		static char tmp_player_name[32];
		char *pn, *tpn;
		for (pn = player_ptr->name, tpn = tmp_player_name; *pn; pn++, tpn++)
		{
#ifdef JP
			if (iskanji(*pn))
			{
				*(tpn++) = *(pn++);
				*tpn = *pn;
				continue;
			}
#endif
			*tpn = my_strchr(" []", *pn) ? '_' : *pn;
		}

		*tpn = '\0';
		v = tmp_player_name;
	}
	else if (streq(b + 1, "TOWN"))
	{
		sprintf(tmp, "%d", player_ptr->town_num);
		v = tmp;
	}
	else if (streq(b + 1, "LEVEL"))
	{
		sprintf(tmp, "%d", player_ptr->lev);
		v = tmp;
	}
	else if (streq(b + 1, "QUEST_NUMBER"))
	{
		sprintf(tmp, "%d", player_ptr->current_floor_ptr->inside_quest);
		v = tmp;
	}
	else if (streq(b + 1, "LEAVING_QUEST"))
	{
		sprintf(tmp, "%d", leaving_quest);
		v = tmp;
	}
	else if (prefix(b + 1, "QUEST_TYPE"))
	{
		sprintf(tmp, "%d", quest[atoi(b + 11)].type);
		v = tmp;
	}
	else if (prefix(b + 1, "QUEST"))
	{
		sprintf(tmp, "%d", quest[atoi(b + 6)].status);
		v = tmp;
	}
	else if (prefix(b + 1, "RANDOM"))
	{
		sprintf(tmp, "%d", (int)(current_world_ptr->seed_town%atoi(b + 7)));
		v = tmp;
	}
	else if (streq(b + 1, "VARIANT"))
	{
		v = variant;
	}
	else if (streq(b + 1, "WILDERNESS"))
	{
		if (vanilla_town)
			sprintf(tmp, "NONE");
		else if (lite_town)
			sprintf(tmp, "LITE");
		else
			sprintf(tmp, "NORMAL");
		v = tmp;
	}

	(*fp) = f;
	(*sp) = s;
	return v;
}


/*!
 * @brief クエスト用固定ダンジョン生成時のメインルーチン
 * Helper function for "process_dungeon_file()"
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param name ファイル名
 * @param ymin 詳細不明
 * @param xmin 詳細不明
 * @param ymax 詳細不明
 * @param xmax 詳細不明
 * @return エラーコード
 */
errr process_dungeon_file(player_type *player_ptr, concptr name, int ymin, int xmin, int ymax, int xmax)
{
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_EDIT, name);
	FILE *fp;
	fp = my_fopen(buf, "r");

	if (!fp) return -1;

	int num = -1;
	errr err = 0;
	bool bypass = FALSE;
	int x = xmin, y = ymin;
	while (my_fgets(fp, buf, sizeof(buf)) == 0)
	{
		num++;
		if (!buf[0]) continue;
		if (iswspace(buf[0])) continue;
		if (buf[0] == '#') continue;
		if ((buf[0] == '?') && (buf[1] == ':'))
		{
			char f;
			char *s;
			s = buf + 2;
			concptr v = process_dungeon_file_expr(player_ptr, &s, &f);
			bypass = (streq(v, "0") ? TRUE : FALSE);
			continue;
		}

		if (bypass) continue;

		err = process_dungeon_file_aux(player_ptr, buf, ymin, xmin, ymax, xmax, &y, &x);
		if (err) break;
	}

	if (err != 0)
	{
		concptr oops = (((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : "unknown");
		msg_format("Error %d (%s) at line %d of '%s'.", err, oops, num, name);
		msg_format(_("'%s'を解析中。", "Parsing '%s'."), buf);
		msg_print(NULL);
	}

	my_fclose(fp);
	return err;
}
