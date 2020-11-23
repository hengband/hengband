#include "room/rooms-pit-nest.h"
#include "dungeon/dungeon.h"
#include "floor/floor-generator.h"
#include "game-option/cheat-options.h"
#include "game-option/cheat-types.h"
#include "grid/door.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags4.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "room/pit-nest-kinds-table.h"
#include "room/space-finder.h"
#include "system/floor-type-definition.h"
#include "util/sort.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"

/*!
* @brief ダンジョン毎に指定されたピット配列を基準にランダムなpit/nestタイプを決める
* @param l_ptr 選択されたpit/nest情報を返す参照ポインタ
* @param allow_flag_mask 生成が許されるpit/nestのビット配列
* @return 選択されたpit/nestのID、選択失敗した場合-1を返す。
*/
static int pick_vault_type(floor_type *floor_ptr, vault_aux_type *l_ptr, BIT_FLAGS16 allow_flag_mask)
{
	int tmp, total, count;

	vault_aux_type *n_ptr;

	/* Calculate the total possibilities */
	for (n_ptr = l_ptr, total = 0, count = 0; TRUE; n_ptr++, count++)
	{
		/* Note end */
		if (!n_ptr->name) break;

		/* Ignore excessive depth */
		if (n_ptr->level > floor_ptr->dun_level) continue;

		/* Not matched with pit/nest flag */
		if (!(allow_flag_mask & (1L << count))) continue;

		/* Count this possibility */
		total += n_ptr->chance * MAX_DEPTH / (MIN(floor_ptr->dun_level, MAX_DEPTH - 1) - n_ptr->level + 5);
	}

	/* Pick a random type */
	tmp = randint0(total);

	/* Find this type */
	for (n_ptr = l_ptr, total = 0, count = 0; TRUE; n_ptr++, count++)
	{
		/* Note end */
		if (!n_ptr->name) break;

		/* Ignore excessive depth */
		if (n_ptr->level > floor_ptr->dun_level) continue;

		/* Not matched with pit/nest flag */
		if (!(allow_flag_mask & (1L << count))) continue;

		/* Count this possibility */
		total += n_ptr->chance * MAX_DEPTH / (MIN(floor_ptr->dun_level, MAX_DEPTH - 1) - n_ptr->level + 5);

		/* Found the type */
		if (tmp < total) break;
	}

	return n_ptr->name ? count : -1;
}


/*!
* @brief デバッグ時に生成されたpit/nestの型を出力する処理
* @param type pit/nestの型ID
* @param nest TRUEならばnest、FALSEならばpit
* @return デバッグ表示文字列の参照ポインタ
* @details
* Hack -- Get the string describing subtype of pit/nest
* Determined in prepare function (some pit/nest only)
*/
static concptr pit_subtype_string(int type, bool nest)
{
	static char inner_buf[256] = "";

	inner_buf[0] = '\0'; /* Init string */

	if (nest) /* Nests */
	{
		switch (type)
		{
		case NEST_TYPE_CLONE:
			sprintf(inner_buf, "(%s)", r_name + r_info[vault_aux_race].name);
			break;
		case NEST_TYPE_SYMBOL_GOOD:
		case NEST_TYPE_SYMBOL_EVIL:
			sprintf(inner_buf, "(%c)", vault_aux_char);
			break;
		}
	}
	else /* Pits */
	{
		switch (type)
		{
		case PIT_TYPE_SYMBOL_GOOD:
		case PIT_TYPE_SYMBOL_EVIL:
			sprintf(inner_buf, "(%c)", vault_aux_char);
			break;
		case PIT_TYPE_DRAGON:
			switch (vault_aux_dragon_mask4)
			{
			case RF4_BR_ACID: strcpy(inner_buf, _("(酸)", "(acid)"));   break;
			case RF4_BR_ELEC: strcpy(inner_buf, _("(稲妻)", "(lightning)")); break;
			case RF4_BR_FIRE: strcpy(inner_buf, _("(火炎)", "(fire)")); break;
			case RF4_BR_COLD: strcpy(inner_buf, _("(冷気)", "(frost)")); break;
			case RF4_BR_POIS: strcpy(inner_buf, _("(毒)", "(poison)"));   break;
			case (RF4_BR_ACID | RF4_BR_ELEC | RF4_BR_FIRE | RF4_BR_COLD | RF4_BR_POIS) :
				strcpy(inner_buf, _("(万色)", "(multi-hued)")); break;
			default: strcpy(inner_buf, _("(未定義)", "(undefined)")); break;
			}
			break;
		}
	}

	return inner_buf;
}


/*
*! @brief nestのモンスターリストをソートするための関数 /
*  Comp function for sorting nest monster information
*  @param u ソート処理対象配列ポインタ
*  @param v 未使用
*  @param a 比較対象参照ID1
*  @param b 比較対象参照ID2
*  TODO: to sort.c
*/
static bool ang_sort_comp_nest_mon_info(player_type *player_ptr, vptr u, vptr v, int a, int b)
{
    /* Unused */
    (void)player_ptr;
    (void)v;

    nest_mon_info_type *nest_mon_info = (nest_mon_info_type *)u;
	MONSTER_IDX w1 = nest_mon_info[a].r_idx;
	MONSTER_IDX w2 = nest_mon_info[b].r_idx;
	monster_race *r1_ptr = &r_info[w1];
	monster_race *r2_ptr = &r_info[w2];
	int z1, z2;

	/* Extract used info */
	z1 = nest_mon_info[a].used;
	z2 = nest_mon_info[b].used;

	/* Compare used status */
	if (z1 < z2) return FALSE;
	if (z1 > z2) return TRUE;

	/* Compare levels */
	if (r1_ptr->level < r2_ptr->level) return TRUE;
	if (r1_ptr->level > r2_ptr->level) return FALSE;

	/* Compare experience */
	if (r1_ptr->mexp < r2_ptr->mexp) return TRUE;
	if (r1_ptr->mexp > r2_ptr->mexp) return FALSE;

	/* Compare indexes */
	return w1 <= w2;
}


/*!
* @brief nestのモンスターリストをスワップするための関数 /
* Swap function for sorting nest monster information
* @param u スワップ処理対象配列ポインタ
* @param v 未使用
* @param a スワップ対象参照ID1
* @param b スワップ対象参照ID2
* TODO: to sort.c
*/
static void ang_sort_swap_nest_mon_info(player_type *player_ptr, vptr u, vptr v, int a, int b)
{
    /* Unused */
    (void)player_ptr;
    (void)v;

    nest_mon_info_type *nest_mon_info = (nest_mon_info_type *)u;
	nest_mon_info_type holder;

	/* Swap */
	holder = nest_mon_info[a];
	nest_mon_info[a] = nest_mon_info[b];
	nest_mon_info[b] = holder;
}


/*!
* @brief タイプ5の部屋…nestを生成する / Type 5 -- Monster nests
* @param player_ptr プレーヤーへの参照ポインタ
* @return なし
* @details
* A monster nest is a "big" room, with an "inner" room, containing\n
* a "collection" of monsters of a given type strewn about the room.\n
*\n
* The monsters are chosen from a set of 64 randomly selected monster\n
* races, to allow the nest creation to fail instead of having "holes".\n
*\n
* Note the use of the "get_mon_num_prep()" function, and the special\n
* "get_mon_num_hook()" restriction function, to prepare the "monster\n
* allocation table" in such a way as to optimize the selection of\n
* "appropriate" non-unique monsters for the nest.\n
*\n
* Note that the "get_mon_num()" function may (rarely) fail, in which\n
* case the nest will be empty.\n
*\n
* Note that "monster nests" will never contain "unique" monsters.\n
*/
bool build_type5(player_type *player_ptr, dun_data_type *dd_ptr)
{
	POSITION y, x, y1, x1, y2, x2, xval, yval;
	int i;
	nest_mon_info_type nest_mon_info[NUM_NEST_MON_TYPE];

	monster_type align;

	grid_type *g_ptr;

	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	int cur_nest_type = pick_vault_type(floor_ptr, nest_types, d_info[floor_ptr->dungeon_idx].nest);
	vault_aux_type *n_ptr;

	/* No type available */
	if (cur_nest_type < 0) return FALSE;

	n_ptr = &nest_types[cur_nest_type];

	/* Process a preparation function if necessary */
	if (n_ptr->prep_func) (*(n_ptr->prep_func))(player_ptr);
	get_mon_num_prep(player_ptr, n_ptr->hook_func, NULL);

	align.sub_align = SUB_ALIGN_NEUTRAL;

	/* Pick some monster types */
	for (i = 0; i < NUM_NEST_MON_TYPE; i++)
	{
		MONRACE_IDX r_idx = 0;
		int attempts = 100;
		monster_race *r_ptr = NULL;

		while (attempts--)
		{
			/* Get a (hard) monster type */
			r_idx = get_mon_num(player_ptr, floor_ptr->dun_level + 11, 0);
			r_ptr = &r_info[r_idx];

			/* Decline incorrect alignment */
			if (monster_has_hostile_align(player_ptr, &align, 0, 0, r_ptr)) continue;

			/* Accept this monster */
			break;
		}

		/* Notice failure */
		if (!r_idx || !attempts) return FALSE;

		/* Note the alignment */
		if (r_ptr->flags3 & RF3_EVIL) align.sub_align |= SUB_ALIGN_EVIL;
		if (r_ptr->flags3 & RF3_GOOD) align.sub_align |= SUB_ALIGN_GOOD;

		nest_mon_info[i].r_idx = (s16b)r_idx;
		nest_mon_info[i].used = FALSE;
	}

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(player_ptr, dd_ptr, &yval, &xval, 11, 25)) return FALSE;

	/* Large room */
	y1 = yval - 4;
	y2 = yval + 4;
	x1 = xval - 11;
	x2 = xval + 11;

	/* Place the floor area */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
			g_ptr = &floor_ptr->grid_array[y][x];
			place_grid(player_ptr, g_ptr, GB_FLOOR);
			g_ptr->info |= (CAVE_ROOM);
		}
	}

	/* Place the outer walls */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		g_ptr = &floor_ptr->grid_array[y][x1 - 1];
		place_grid(player_ptr, g_ptr, GB_OUTER);
		g_ptr = &floor_ptr->grid_array[y][x2 + 1];
		place_grid(player_ptr, g_ptr, GB_OUTER);
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
		g_ptr = &floor_ptr->grid_array[y1 - 1][x];
		place_grid(player_ptr, g_ptr, GB_OUTER);
		g_ptr = &floor_ptr->grid_array[y2 + 1][x];
		place_grid(player_ptr, g_ptr, GB_OUTER);
	}


	/* Advance to the center room */
	y1 = y1 + 2;
	y2 = y2 - 2;
	x1 = x1 + 2;
	x2 = x2 - 2;

	/* The inner walls */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		g_ptr = &floor_ptr->grid_array[y][x1 - 1];
		place_grid(player_ptr, g_ptr, GB_INNER);
		g_ptr = &floor_ptr->grid_array[y][x2 + 1];
		place_grid(player_ptr, g_ptr, GB_INNER);
	}

	for (x = x1 - 1; x <= x2 + 1; x++)
	{
		g_ptr = &floor_ptr->grid_array[y1 - 1][x];
		place_grid(player_ptr, g_ptr, GB_INNER);
		g_ptr = &floor_ptr->grid_array[y2 + 1][x];
		place_grid(player_ptr, g_ptr, GB_INNER);
	}
	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			add_cave_info(floor_ptr, y, x, CAVE_ICKY);
		}
	}

	/* Place a secret door */
	switch (randint1(4))
	{
	case 1: place_secret_door(player_ptr, y1 - 1, xval, DOOR_DEFAULT); break;
	case 2: place_secret_door(player_ptr, y2 + 1, xval, DOOR_DEFAULT); break;
	case 3: place_secret_door(player_ptr, yval, x1 - 1, DOOR_DEFAULT); break;
	case 4: place_secret_door(player_ptr, yval, x2 + 1, DOOR_DEFAULT); break;
	}

	msg_format_wizard(player_ptr, CHEAT_DUNGEON, _("モンスター部屋(nest)(%s%s)を生成します。", "Monster nest (%s%s)"), n_ptr->name,
            pit_subtype_string(cur_nest_type, TRUE));

	/* Place some monsters */
	for (y = yval - 2; y <= yval + 2; y++)
	{
		for (x = xval - 9; x <= xval + 9; x++)
		{
			MONRACE_IDX r_idx;

			i = randint0(NUM_NEST_MON_TYPE);
			r_idx = nest_mon_info[i].r_idx;

			/* Place that "random" monster (no groups) */
			(void)place_monster_aux(player_ptr, 0, y, x, r_idx, 0L);

			nest_mon_info[i].used = TRUE;
		}
	}

	if (cheat_room)
	{
		ang_sort(player_ptr, nest_mon_info, NULL, NUM_NEST_MON_TYPE, ang_sort_comp_nest_mon_info, ang_sort_swap_nest_mon_info);

		/* Dump the entries (prevent multi-printing) */
		for (i = 0; i < NUM_NEST_MON_TYPE; i++)
		{
			if (!nest_mon_info[i].used) break;
			for (; i < NUM_NEST_MON_TYPE - 1; i++)
			{
				if (nest_mon_info[i].r_idx != nest_mon_info[i + 1].r_idx) break;
				if (!nest_mon_info[i + 1].used) break;
			}

			msg_format_wizard(player_ptr, CHEAT_DUNGEON, "Nest構成モンスターNo.%d:%s", i, r_name + r_info[nest_mon_info[i].r_idx].name);
		}
	}

	return TRUE;
}


/*!
* @brief タイプ6の部屋…pitを生成する / Type 6 -- Monster pits
* @return なし
* @details
* A monster pit is a "big" room, with an "inner" room, containing\n
* a "collection" of monsters of a given type organized in the room.\n
*\n
* The inside room in a monster pit appears as shown below, where the\n
* actual monsters in each location depend on the type of the pit\n
*\n
*   XXXXXXXXXXXXXXXXXXXXX\n
*   X0000000000000000000X\n
*   X0112233455543322110X\n
*   X0112233467643322110X\n
*   X0112233455543322110X\n
*   X0000000000000000000X\n
*   XXXXXXXXXXXXXXXXXXXXX\n
*\n
* Note that the monsters in the pit are now chosen by using "get_mon_num()"\n
* to request 16 "appropriate" monsters, sorting them by level, and using\n
* the "even" entries in this sorted list for the contents of the pit.\n
*\n
* Hack -- all of the "dragons" in a "dragon" pit must be the same "color",\n
* which is handled by requiring a specific "breath" attack for all of the\n
* dragons.  This may include "multi-hued" breath.  Note that "wyrms" may\n
* be present in many of the dragon pits, if they have the proper breath.\n
*\n
* Note the use of the "get_mon_num_prep()" function, and the special\n
* "get_mon_num_hook()" restriction function, to prepare the "monster\n
* allocation table" in such a way as to optimize the selection of\n
* "appropriate" non-unique monsters for the pit.\n
*\n
* Note that the "get_mon_num()" function may (rarely) fail, in which case\n
* the pit will be empty.\n
*\n
* Note that "monster pits" will never contain "unique" monsters.\n
*/
bool build_type6(player_type *player_ptr, dun_data_type *dd_ptr)
{
	POSITION y, x, y1, x1, y2, x2, xval, yval;
	int i, j;

	MONRACE_IDX what[16];

	monster_type align;

	grid_type *g_ptr;

	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	int cur_pit_type = pick_vault_type(floor_ptr, pit_types, d_info[floor_ptr->dungeon_idx].pit);
	vault_aux_type *n_ptr;

	/* No type available */
	if (cur_pit_type < 0) return FALSE;

	n_ptr = &pit_types[cur_pit_type];

	/* Process a preparation function if necessary */
	if (n_ptr->prep_func) (*(n_ptr->prep_func))(player_ptr);
	get_mon_num_prep(player_ptr, n_ptr->hook_func, NULL);

	align.sub_align = SUB_ALIGN_NEUTRAL;

	/* Pick some monster types */
	for (i = 0; i < 16; i++)
	{
		MONRACE_IDX r_idx = 0;
		int attempts = 100;
		monster_race *r_ptr = NULL;

		while (attempts--)
		{
			/* Get a (hard) monster type */
			r_idx = get_mon_num(player_ptr, floor_ptr->dun_level + 11, 0);
			r_ptr = &r_info[r_idx];

			/* Decline incorrect alignment */
			if (monster_has_hostile_align(player_ptr, &align, 0, 0, r_ptr)) continue;

			/* Accept this monster */
			break;
		}

		/* Notice failure */
		if (!r_idx || !attempts) return FALSE;

		/* Note the alignment */
		if (r_ptr->flags3 & RF3_EVIL) align.sub_align |= SUB_ALIGN_EVIL;
		if (r_ptr->flags3 & RF3_GOOD) align.sub_align |= SUB_ALIGN_GOOD;

		what[i] = r_idx;
	}

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(player_ptr, dd_ptr, &yval, &xval, 11, 25)) return FALSE;

	/* Large room */
	y1 = yval - 4;
	y2 = yval + 4;
	x1 = xval - 11;
	x2 = xval + 11;

	/* Place the floor area */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
			g_ptr = &floor_ptr->grid_array[y][x];
			place_grid(player_ptr, g_ptr, GB_FLOOR);
			g_ptr->info |= (CAVE_ROOM);
		}
	}

	/* Place the outer walls */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		g_ptr = &floor_ptr->grid_array[y][x1 - 1];
		place_grid(player_ptr, g_ptr, GB_OUTER);
		g_ptr = &floor_ptr->grid_array[y][x2 + 1];
		place_grid(player_ptr, g_ptr, GB_OUTER);
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
		g_ptr = &floor_ptr->grid_array[y1 - 1][x];
		place_grid(player_ptr, g_ptr, GB_OUTER);
		g_ptr = &floor_ptr->grid_array[y2 + 1][x];
		place_grid(player_ptr, g_ptr, GB_OUTER);
	}

	/* Advance to the center room */
	y1 = y1 + 2;
	y2 = y2 - 2;
	x1 = x1 + 2;
	x2 = x2 - 2;

	/* The inner walls */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		g_ptr = &floor_ptr->grid_array[y][x1 - 1];
		place_grid(player_ptr, g_ptr, GB_INNER);
		g_ptr = &floor_ptr->grid_array[y][x2 + 1];
		place_grid(player_ptr, g_ptr, GB_INNER);
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
		g_ptr = &floor_ptr->grid_array[y1 - 1][x];
		place_grid(player_ptr, g_ptr, GB_INNER);
		g_ptr = &floor_ptr->grid_array[y2 + 1][x];
		place_grid(player_ptr, g_ptr, GB_INNER);
	}
	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			add_cave_info(floor_ptr, y, x, CAVE_ICKY);
		}
	}

	/* Place a secret door */
	switch (randint1(4))
	{
	case 1: place_secret_door(player_ptr, y1 - 1, xval, DOOR_DEFAULT); break;
	case 2: place_secret_door(player_ptr, y2 + 1, xval, DOOR_DEFAULT); break;
	case 3: place_secret_door(player_ptr, yval, x1 - 1, DOOR_DEFAULT); break;
	case 4: place_secret_door(player_ptr, yval, x2 + 1, DOOR_DEFAULT); break;
	}

	/* Sort the entries */
	for (i = 0; i < 16 - 1; i++)
	{
		/* Sort the entries */
		for (j = 0; j < 16 - 1; j++)
		{
			int i1 = j;
			int i2 = j + 1;

			int p1 = r_info[what[i1]].level;
			int p2 = r_info[what[i2]].level;

			/* Bubble */
			if (p1 > p2)
			{
				MONRACE_IDX tmp = what[i1];
				what[i1] = what[i2];
				what[i2] = tmp;
			}
		}
	}

	msg_format_wizard(player_ptr, CHEAT_DUNGEON, _("モンスター部屋(pit)(%s%s)を生成します。", "Monster pit (%s%s)"), n_ptr->name,
            pit_subtype_string(cur_pit_type, FALSE));

	/* Select the entries */
	for (i = 0; i < 8; i++)
	{
		/* Every other entry */
		what[i] = what[i * 2];
		msg_format_wizard(player_ptr, CHEAT_DUNGEON, _("Nest構成モンスター選択No.%d:%s", "Nest Monster Select No.%d:%s"), i, r_name + r_info[what[i]].name);
	}

	/* Top and bottom rows */
	for (x = xval - 9; x <= xval + 9; x++)
	{
		place_monster_aux(player_ptr, 0, yval - 2, x, what[0], PM_NO_KAGE);
		place_monster_aux(player_ptr, 0, yval + 2, x, what[0], PM_NO_KAGE);
	}

	/* Middle columns */
	for (y = yval - 1; y <= yval + 1; y++)
	{
		place_monster_aux(player_ptr, 0, y, xval - 9, what[0], PM_NO_KAGE);
		place_monster_aux(player_ptr, 0, y, xval + 9, what[0], PM_NO_KAGE);

		place_monster_aux(player_ptr, 0, y, xval - 8, what[1], PM_NO_KAGE);
		place_monster_aux(player_ptr, 0, y, xval + 8, what[1], PM_NO_KAGE);

		place_monster_aux(player_ptr, 0, y, xval - 7, what[1], PM_NO_KAGE);
		place_monster_aux(player_ptr, 0, y, xval + 7, what[1], PM_NO_KAGE);

		place_monster_aux(player_ptr, 0, y, xval - 6, what[2], PM_NO_KAGE);
		place_monster_aux(player_ptr, 0, y, xval + 6, what[2], PM_NO_KAGE);

		place_monster_aux(player_ptr, 0, y, xval - 5, what[2], PM_NO_KAGE);
		place_monster_aux(player_ptr, 0, y, xval + 5, what[2], PM_NO_KAGE);

		place_monster_aux(player_ptr, 0, y, xval - 4, what[3], PM_NO_KAGE);
		place_monster_aux(player_ptr, 0, y, xval + 4, what[3], PM_NO_KAGE);

		place_monster_aux(player_ptr, 0, y, xval - 3, what[3], PM_NO_KAGE);
		place_monster_aux(player_ptr, 0, y, xval + 3, what[3], PM_NO_KAGE);

		place_monster_aux(player_ptr, 0, y, xval - 2, what[4], PM_NO_KAGE);
		place_monster_aux(player_ptr, 0, y, xval + 2, what[4], PM_NO_KAGE);
	}

	/* Above/Below the center monster */
	for (x = xval - 1; x <= xval + 1; x++)
	{
		place_monster_aux(player_ptr, 0, yval + 1, x, what[5], PM_NO_KAGE);
		place_monster_aux(player_ptr, 0, yval - 1, x, what[5], PM_NO_KAGE);
	}

	/* Next to the center monster */
	place_monster_aux(player_ptr, 0, yval, xval + 1, what[6], PM_NO_KAGE);
	place_monster_aux(player_ptr, 0, yval, xval - 1, what[6], PM_NO_KAGE);

	/* Center monster */
	place_monster_aux(player_ptr, 0, yval, xval, what[7], PM_NO_KAGE);

	return TRUE;
}

/*
 * Helper function for "trapped monster pit"
 */
static bool vault_aux_trapped_pit(player_type *player_ptr, MONRACE_IDX r_idx)
{
    /* Unused */
    (void)player_ptr;

    monster_race *r_ptr = &r_info[r_idx];

	if (!vault_monster_okay(player_ptr, r_idx)) return FALSE;

	/* No wall passing monster */
	if (r_ptr->flags2 & (RF2_PASS_WALL | RF2_KILL_WALL)) return FALSE;

	return TRUE;
}


/*!
* @brief タイプ13の部屋…トラップpitの生成 / Type 13 -- Trapped monster pits
* @return なし
* @details
* A trapped monster pit is a "big" room with a straight corridor in\n
* which wall opening traps are placed, and with two "inner" rooms\n
* containing a "collection" of monsters of a given type organized in\n
* the room.\n
*\n
* The trapped monster pit appears as shown below, where the actual\n
* monsters in each location depend on the type of the pit\n
*\n
*  XXXXXXXXXXXXXXXXXXXXXXXXX\n
*  X                       X\n
*  XXXXXXXXXXXXXXXXXXXXXXX X\n
*  XXXXX001123454321100XXX X\n
*  XXX0012234567654322100X X\n
*  XXXXXXXXXXXXXXXXXXXXXXX X\n
*  X           ^           X\n
*  X XXXXXXXXXXXXXXXXXXXXXXX\n
*  X X0012234567654322100XXX\n
*  X XXX001123454321100XXXXX\n
*  X XXXXXXXXXXXXXXXXXXXXXXX\n
*  X                       X\n
*  XXXXXXXXXXXXXXXXXXXXXXXXX\n
*\n
* Note that the monsters in the pit are now chosen by using "get_mon_num()"\n
* to request 16 "appropriate" monsters, sorting them by level, and using\n
* the "even" entries in this sorted list for the contents of the pit.\n
*\n
* Hack -- all of the "dragons" in a "dragon" pit must be the same "color",\n
* which is handled by requiring a specific "breath" attack for all of the\n
* dragons.  This may include "multi-hued" breath.  Note that "wyrms" may\n
* be present in many of the dragon pits, if they have the proper breath.\n
*\n
* Note the use of the "get_mon_num_prep()" function, and the special\n
* "get_mon_num_hook()" restriction function, to prepare the "monster\n
* allocation table" in such a way as to optimize the selection of\n
* "appropriate" non-unique monsters for the pit.\n
*\n
* Note that the "get_mon_num()" function may (rarely) fail, in which case\n
* the pit will be empty.\n
*\n
* Note that "monster pits" will never contain "unique" monsters.\n
*/
bool build_type13(player_type *player_ptr, dun_data_type *dd_ptr)
{
	POSITION y, x, y1, x1, y2, x2, xval, yval;
	int i, j;

	MONRACE_IDX what[16];

	monster_type align;

	grid_type *g_ptr;

	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	int cur_pit_type = pick_vault_type(floor_ptr, pit_types, d_info[floor_ptr->dungeon_idx].pit);
	vault_aux_type *n_ptr;

	/* Only in Angband */
	if (floor_ptr->dungeon_idx != DUNGEON_ANGBAND) return FALSE;

	/* No type available */
	if (cur_pit_type < 0) return FALSE;

	n_ptr = &pit_types[cur_pit_type];

	/* Process a preparation function if necessary */
	if (n_ptr->prep_func) (*(n_ptr->prep_func))(player_ptr);
	get_mon_num_prep(player_ptr, n_ptr->hook_func, vault_aux_trapped_pit);

	align.sub_align = SUB_ALIGN_NEUTRAL;

	/* Pick some monster types */
	for (i = 0; i < 16; i++)
	{
		MONRACE_IDX r_idx = 0;
		int attempts = 100;
		monster_race *r_ptr = NULL;

		while (attempts--)
		{
			/* Get a (hard) monster type */
			r_idx = get_mon_num(player_ptr, floor_ptr->dun_level + 0, 0);
			r_ptr = &r_info[r_idx];

			/* Decline incorrect alignment */
			if (monster_has_hostile_align(player_ptr, &align, 0, 0, r_ptr)) continue;

			/* Accept this monster */
			break;
		}

		/* Notice failure */
		if (!r_idx || !attempts) return FALSE;

		/* Note the alignment */
		if (r_ptr->flags3 & RF3_EVIL) align.sub_align |= SUB_ALIGN_EVIL;
		if (r_ptr->flags3 & RF3_GOOD) align.sub_align |= SUB_ALIGN_GOOD;

		what[i] = r_idx;
	}

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(player_ptr, dd_ptr, &yval, &xval, 13, 25)) return FALSE;

	/* Large room */
	y1 = yval - 5;
	y2 = yval + 5;
	x1 = xval - 11;
	x2 = xval + 11;

	/* Fill with inner walls */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
			g_ptr = &floor_ptr->grid_array[y][x];
			place_grid(player_ptr, g_ptr, GB_INNER);
			g_ptr->info |= (CAVE_ROOM);
		}
	}

	/* Place the floor area 1 */
	for (x = x1 + 3; x <= x2 - 3; x++)
	{
		g_ptr = &floor_ptr->grid_array[yval - 2][x];
		place_grid(player_ptr, g_ptr, GB_FLOOR);
		add_cave_info(floor_ptr, yval - 2, x, CAVE_ICKY);

		g_ptr = &floor_ptr->grid_array[yval + 2][x];
		place_grid(player_ptr, g_ptr, GB_FLOOR);
		add_cave_info(floor_ptr, yval + 2, x, CAVE_ICKY);
	}

	/* Place the floor area 2 */
	for (x = x1 + 5; x <= x2 - 5; x++)
	{
		g_ptr = &floor_ptr->grid_array[yval - 3][x];
		place_grid(player_ptr, g_ptr, GB_FLOOR);
		add_cave_info(floor_ptr, yval - 3, x, CAVE_ICKY);

		g_ptr = &floor_ptr->grid_array[yval + 3][x];
		place_grid(player_ptr, g_ptr, GB_FLOOR);
		add_cave_info(floor_ptr, yval + 3, x, CAVE_ICKY);
	}

	/* Corridor */
	for (x = x1; x <= x2; x++)
	{
		g_ptr = &floor_ptr->grid_array[yval][x];
		place_grid(player_ptr, g_ptr, GB_FLOOR);
		g_ptr = &floor_ptr->grid_array[y1][x];
		place_grid(player_ptr, g_ptr, GB_FLOOR);
		g_ptr = &floor_ptr->grid_array[y2][x];
		place_grid(player_ptr, g_ptr, GB_FLOOR);
	}

	/* Place the outer walls */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		g_ptr = &floor_ptr->grid_array[y][x1 - 1];
		place_grid(player_ptr, g_ptr, GB_OUTER);
		g_ptr = &floor_ptr->grid_array[y][x2 + 1];
		place_grid(player_ptr, g_ptr, GB_OUTER);
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
		g_ptr = &floor_ptr->grid_array[y1 - 1][x];
		place_grid(player_ptr, g_ptr, GB_OUTER);
		g_ptr = &floor_ptr->grid_array[y2 + 1][x];
		place_grid(player_ptr, g_ptr, GB_OUTER);
	}

	/* Random corridor */
	if (one_in_(2))
	{
		for (y = y1; y <= yval; y++)
		{
			place_bold(player_ptr, y, x2, GB_FLOOR);
			place_bold(player_ptr, y, x1 - 1, GB_SOLID);
		}
		for (y = yval; y <= y2 + 1; y++)
		{
			place_bold(player_ptr, y, x1, GB_FLOOR);
			place_bold(player_ptr, y, x2 + 1, GB_SOLID);
		}
	}
	else
	{
		for (y = yval; y <= y2 + 1; y++)
		{
			place_bold(player_ptr, y, x1, GB_FLOOR);
			place_bold(player_ptr, y, x2 + 1, GB_SOLID);
		}
		for (y = y1; y <= yval; y++)
		{
			place_bold(player_ptr, y, x2, GB_FLOOR);
			place_bold(player_ptr, y, x1 - 1, GB_SOLID);
		}
	}

	/* Place the wall open trap */
	floor_ptr->grid_array[yval][xval].mimic = floor_ptr->grid_array[yval][xval].feat;
	floor_ptr->grid_array[yval][xval].feat = feat_trap_open;

	/* Sort the entries */
	for (i = 0; i < 16 - 1; i++)
	{
		/* Sort the entries */
		for (j = 0; j < 16 - 1; j++)
		{
			int i1 = j;
			int i2 = j + 1;

			int p1 = r_info[what[i1]].level;
			int p2 = r_info[what[i2]].level;

			/* Bubble */
			if (p1 > p2)
			{
				MONRACE_IDX tmp = what[i1];
				what[i1] = what[i2];
				what[i2] = tmp;
			}
		}
	}

	msg_format_wizard(player_ptr, CHEAT_DUNGEON, _("%s%sの罠ピットが生成されました。", "Trapped monster pit (%s%s)"),
		n_ptr->name, pit_subtype_string(cur_pit_type, FALSE));

	/* Select the entries */
	for (i = 0; i < 8; i++)
	{
		/* Every other entry */
		what[i] = what[i * 2];

		if (cheat_hear)
		{
			msg_print(r_name + r_info[what[i]].name);
		}
	}

	for (i = 0; placing[i][2] >= 0; i++)
	{
		y = yval + placing[i][0];
		x = xval + placing[i][1];
		place_monster_aux(player_ptr, 0, y, x, what[placing[i][2]], PM_NO_KAGE);
	}

	return TRUE;
}
