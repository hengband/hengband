/*!
 * @file wizard2.c
 * @brief ウィザードモードの処理(特別処理中心) / Wizard commands
 * @date 2014/09/07
 * @author
 * Copyright (c) 1997 Ben Harrison, and others<br>
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.<br>
 * 2014 Deskull rearranged comment for Doxygen.<br>
 */

#include "system/angband.h"
#include "wizard/wizard-special-process.h"
#include "wizard/wizard-spoiler.h"
#include "system/angband-version.h"
#include "core/stuff-handler.h"
#include "term/gameterm.h"

#include "dungeon/dungeon.h"
#include "io/write-diary.h"
#include "cmd/cmd-draw.h"
#include "cmd/cmd-dump.h"
#include "cmd/cmd-help.h"
#include "cmd/cmd-save.h"
#include "util/util.h"
#include "birth/inventory-initializer.h"
#include "player/selfinfo.h"
#include "player/patron.h"
#include "mutation/mutation.h"
#include "dungeon/quest.h"
#include "object/artifact.h"
#include "player/player-status.h"
#include "player/player-effects.h"
#include "player/player-skill.h"
#include "player/player-class.h"
#include "inventory/player-inventory.h"

#include "spell/spells-util.h"
#include "spell/spells-object.h"
#include "spell/spells-summon.h"
#include "spell/spells-status.h"
#include "spell/spells-world.h"
#include "spell/spells-floor.h"

#include "object/object-flavor.h"
#include "object/object-hook.h"
#include "monster/monster-status.h"

#include "floor/floor.h"
#include "floor/floor-save.h"
#include "grid/grid.h"
#include "dungeon/dungeon-file.h"
#include "io/files-util.h"
#include "mspell/monster-spell.h"
#include "market/arena.h"
#include "object/object-kind.h"
#include "io/targeting.h"
#include "view/display-main-window.h"
#include "world/world.h"
#include "spell/spells2.h"
#include "spell/spells3.h"
#include "spell/spells-detection.h"
#include "player/player-races-table.h"

#define NUM_O_SET 8
#define NUM_O_BIT 32

typedef union spell_functions {
	struct debug_spell_type1 { bool(*spell_function)(player_type *, floor_type *); } spell1;
	struct debug_spell_type2 { bool(*spell_function)(player_type *); } spell2;
	struct debug_spell_type3 { bool(*spell_function)(player_type *, HIT_POINT); } spell3;
} spell_functions;

typedef struct debug_spell_command
{
	int type;
	char *command_name;
	spell_functions command_function;
} debug_spell_command;

#define SPELL_MAX 3
debug_spell_command debug_spell_commands_list[SPELL_MAX] =
{
	{ 2, "vanish dungeon", {.spell2 = { vanish_dungeon } } },
	{ 3, "true healing", {.spell3 = { true_healing } } },
	{ 2, "drop weapons", {.spell2 = { drop_weapons } } }
};

/*!
 * @brief コマンド入力により任意にスペル効果を起こす / Wizard spells
 * @return 実際にテレポートを行ったらTRUEを返す
 */
static bool do_cmd_debug_spell(player_type *creature_ptr)
{
	char tmp_val[50] = "\0";
	int tmp_int;

	if (!get_string("SPELL: ", tmp_val, 32)) return FALSE;

	for (int i = 0; i < SPELL_MAX; i++)
	{
		if (strcmp(tmp_val, debug_spell_commands_list[i].command_name) != 0)
			continue;
		switch (debug_spell_commands_list[i].type)
		{
		case 2:
			(*(debug_spell_commands_list[i].command_function.spell2.spell_function))(creature_ptr);
			return TRUE;
			break;
		case 3:
			tmp_val[0] = '\0';
			if (!get_string("POWER:", tmp_val, 32)) return FALSE;
			tmp_int = atoi(tmp_val);
			(*(debug_spell_commands_list[i].command_function.spell3.spell_function))(creature_ptr, tmp_int);
			return TRUE;
			break;
		default:
			break;
		}
	}

	msg_format("Command not found.");

	return FALSE;
}


/*!
 * @brief 必ず成功するウィザードモード用次元の扉処理 / Wizard Dimension Door
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 実際にテレポートを行ったらTRUEを返す
 */
static bool wiz_dimension_door(player_type *caster_ptr)
{
	POSITION x = 0, y = 0;
	if (!tgt_pt(caster_ptr, &x, &y)) return FALSE;
	teleport_player_to(caster_ptr, y, x, TELEPORT_NONMAGICAL);
	return TRUE;
}

/*!
 * @brief 指定されたIDの固定アーティファクトを生成する / Create the artifact of the specified number
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void wiz_create_named_art(player_type *caster_ptr)
{
	char tmp_val[10] = "";
	ARTIFACT_IDX a_idx;

	/* Query */
	if (!get_string("Artifact ID:", tmp_val, 3)) return;

	/* Extract */
	a_idx = (ARTIFACT_IDX)atoi(tmp_val);
	if (a_idx < 0) a_idx = 0;
	if (a_idx >= max_a_idx) a_idx = 0;

	(void)create_named_art(caster_ptr, a_idx, caster_ptr->y, caster_ptr->x);

	/* All done */
	msg_print("Allocated.");
}

/*!
 * @brief ウィザードモード用モンスターの群れ生成 / Summon a horde of monsters
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void do_cmd_summon_horde(player_type *caster_ptr)
{
	POSITION wy = caster_ptr->y, wx = caster_ptr->x;
	int attempts = 1000;

	while (--attempts)
	{
		scatter(caster_ptr, &wy, &wx, caster_ptr->y, caster_ptr->x, 3, 0);
		if (is_cave_empty_bold(caster_ptr, wy, wx)) break;
	}

	(void)alloc_horde(caster_ptr, wy, wx);
}

/*!
 * @brief 32ビット変数のビット配列を並べて描画する / Output a long int in binary format.
 * @return なし
 */
static void prt_binary(BIT_FLAGS flags, int row, int col)
{
	int i;
	u32b bitmask;

	/* Scan the flags */
	for (i = bitmask = 1; i <= 32; i++, bitmask *= 2)
	{
		/* Dump set bits */
		if (flags & bitmask)
		{
			Term_putch(col++, row, TERM_BLUE, '*');
		}

		/* Dump unset bits */
		else
		{
			Term_putch(col++, row, TERM_WHITE, '-');
		}
	}
}


#define K_MAX_DEPTH 110 /*!< アイテムの階層毎生成率を表示する最大階 */

/*!
 * @brief アイテムの階層毎生成率を表示する / Output a rarity graph for a type of object.
 * @param tval ベースアイテムの大項目ID
 * @param sval ベースアイテムの小項目ID
 * @param row 表示列
 * @param col 表示行
 * @return なし
 */
static void prt_alloc(OBJECT_TYPE_VALUE tval, OBJECT_SUBTYPE_VALUE sval, TERM_LEN row, TERM_LEN col)
{
	u32b rarity[K_MAX_DEPTH];
	(void)C_WIPE(rarity, K_MAX_DEPTH, u32b);
	u32b total[K_MAX_DEPTH];
	(void)C_WIPE(total, K_MAX_DEPTH, u32b);
	s32b display[22];
	(void)C_WIPE(display, 22, s32b);

	/* Scan all entries */
	int home = 0;
	for (int i = 0; i < K_MAX_DEPTH; i++)
	{
		int total_frac = 0;
		object_kind *k_ptr;
		alloc_entry *table = alloc_kind_table;
		for (int j = 0; j < alloc_kind_size; j++)
		{
			PERCENTAGE prob = 0;

			if (table[j].level <= i)
			{
				prob = table[j].prob1 * GREAT_OBJ * K_MAX_DEPTH;
			}
			else if (table[j].level - 1 > 0)
			{
				prob = table[j].prob1 * i * K_MAX_DEPTH / (table[j].level - 1);
			}

			/* Acquire this kind */
			k_ptr = &k_info[table[j].index];

			/* Accumulate probabilities */
			total[i] += prob / (GREAT_OBJ * K_MAX_DEPTH);
			total_frac += prob % (GREAT_OBJ * K_MAX_DEPTH);

			/* Accumulate probabilities */
			if ((k_ptr->tval == tval) && (k_ptr->sval == sval))
			{
				home = k_ptr->level;
				rarity[i] += prob / (GREAT_OBJ * K_MAX_DEPTH);
			}
		}
		total[i] += total_frac / (GREAT_OBJ * K_MAX_DEPTH);
	}

	/* Calculate probabilities for each range */
	for (int i = 0; i < 22; i++)
	{
		/* Shift the values into view */
		int possibility = 0;
		for (int j = i * K_MAX_DEPTH / 22; j < (i + 1) * K_MAX_DEPTH / 22; j++)
			possibility += rarity[j] * 100000 / total[j];
		display[i] = possibility / 5;
	}

	/* Graph the rarities */
	for (int i = 0; i < 22; i++)
	{
		Term_putch(col, row + i + 1, TERM_WHITE, '|');

		prt(format("%2dF", (i * 5)), row + i + 1, col);


		/* Note the level */
		if ((i * K_MAX_DEPTH / 22 <= home) && (home < (i + 1) * K_MAX_DEPTH / 22))
		{
			c_prt(TERM_RED, format("%3d.%04d%%", display[i] / 1000, display[i] % 1000), row + i + 1, col + 3);
		}
		else
		{
			c_prt(TERM_WHITE, format("%3d.%04d%%", display[i] / 1000, display[i] % 1000), row + i + 1, col + 3);
		}
	}

	/* Make it look nice */
	concptr r = "+---Rate---+";
	prt(r, row, col);
}

/*!
 * @brief プレイヤーの職業を変更する
 * @return なし
 * @todo 魔法領域の再選択などがまだ不完全、要実装。
 */
static void do_cmd_wiz_reset_class(player_type *creature_ptr)
{
	/* Prompt */
	char ppp[80];
	sprintf(ppp, "Class (0-%d): ", MAX_CLASS - 1);

	/* Default */
	char tmp_val[160];
	sprintf(tmp_val, "%d", creature_ptr->pclass);

	/* Query */
	if (!get_string(ppp, tmp_val, 2)) return;

	/* Extract */
	int tmp_int = atoi(tmp_val);

	/* Verify */
	if (tmp_int < 0 || tmp_int >= MAX_CLASS) return;

	/* Save it */
	creature_ptr->pclass = (byte)tmp_int;

	/* Redraw inscription */
	creature_ptr->window |= (PW_PLAYER);

	/* {.} and {$} effect creature_ptr->warning and TRC_TELEPORT_SELF */
	creature_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);

	handle_stuff(creature_ptr);
}


/*!
 * @brief ウィザードモード用処理としてターゲット中の相手をテレポートバックする / Hack -- Teleport to the target
 * @return なし
 */
static void do_cmd_wiz_bamf(player_type *caster_ptr)
{
	/* Must have a target */
	if (!target_who) return;

	/* Teleport to the target */
	teleport_player_to(caster_ptr, target_row, target_col, TELEPORT_NONMAGICAL);
}


/*!
 * @brief プレイヤーの現能力値を調整する
 * Aux function for "do_cmd_wiz_change()".	-RAK-
 * @return なし
 */
static void do_cmd_wiz_change_aux(player_type *creature_ptr)
{
	int tmp_int;
	long tmp_long;
	s16b tmp_s16b;
	char tmp_val[160];
	char ppp[80];

	/* Query the stats */
	for (int i = 0; i < A_MAX; i++)
	{
		/* Prompt */
		sprintf(ppp, "%s (3-%d): ", stat_names[i], creature_ptr->stat_max_max[i]);

		/* Default */
		sprintf(tmp_val, "%d", creature_ptr->stat_max[i]);

		/* Query */
		if (!get_string(ppp, tmp_val, 3)) return;

		/* Extract */
		tmp_int = atoi(tmp_val);

		/* Verify */
		if (tmp_int > creature_ptr->stat_max_max[i]) tmp_int = creature_ptr->stat_max_max[i];
		else if (tmp_int < 3) tmp_int = 3;

		/* Save it */
		creature_ptr->stat_cur[i] = creature_ptr->stat_max[i] = (BASE_STATUS)tmp_int;
	}


	/* Default */
	sprintf(tmp_val, "%d", WEAPON_EXP_MASTER);

	/* Query */
	if (!get_string(_("熟練度: ", "Proficiency: "), tmp_val, 9)) return;

	/* Extract */
	tmp_s16b = (s16b)atoi(tmp_val);

	/* Verify */
	if (tmp_s16b < WEAPON_EXP_UNSKILLED) tmp_s16b = WEAPON_EXP_UNSKILLED;
	if (tmp_s16b > WEAPON_EXP_MASTER) tmp_s16b = WEAPON_EXP_MASTER;

	for (int j = 0; j <= TV_WEAPON_END - TV_WEAPON_BEGIN; j++)
	{
		for (int i = 0; i < 64; i++)
		{
			creature_ptr->weapon_exp[j][i] = tmp_s16b;
			if (creature_ptr->weapon_exp[j][i] > s_info[creature_ptr->pclass].w_max[j][i]) creature_ptr->weapon_exp[j][i] = s_info[creature_ptr->pclass].w_max[j][i];
		}
	}

	for (int j = 0; j < 10; j++)
	{
		creature_ptr->skill_exp[j] = tmp_s16b;
		if (creature_ptr->skill_exp[j] > s_info[creature_ptr->pclass].s_max[j]) creature_ptr->skill_exp[j] = s_info[creature_ptr->pclass].s_max[j];
	}

	int k;
	for (k = 0; k < 32; k++)
		creature_ptr->spell_exp[k] = (tmp_s16b > SPELL_EXP_MASTER ? SPELL_EXP_MASTER : tmp_s16b);
	for (; k < 64; k++)
		creature_ptr->spell_exp[k] = (tmp_s16b > SPELL_EXP_EXPERT ? SPELL_EXP_EXPERT : tmp_s16b);

	/* Default */
	sprintf(tmp_val, "%ld", (long)(creature_ptr->au));

	/* Query */
	if (!get_string("Gold: ", tmp_val, 9)) return;

	/* Extract */
	tmp_long = atol(tmp_val);

	/* Verify */
	if (tmp_long < 0) tmp_long = 0L;

	/* Save */
	creature_ptr->au = tmp_long;

	/* Default */
	sprintf(tmp_val, "%ld", (long)(creature_ptr->max_exp));

	/* Query */
	if (!get_string("Experience: ", tmp_val, 9)) return;

	/* Extract */
	tmp_long = atol(tmp_val);

	/* Verify */
	if (tmp_long < 0) tmp_long = 0L;

	if (creature_ptr->prace == RACE_ANDROID) return;

	/* Save */
	creature_ptr->max_exp = tmp_long;
	creature_ptr->exp = tmp_long;

	/* Update */
	check_experience(creature_ptr);
}


/*!
 * @brief プレイヤーの現能力値を調整する(メインルーチン)
 * Change various "permanent" player variables.
 * @return なし
 */
static void do_cmd_wiz_change(player_type *creature_ptr)
{
	/* Interact */
	do_cmd_wiz_change_aux(creature_ptr);
	do_cmd_redraw(creature_ptr);
}


/*!
 * @brief アイテムの詳細ステータスを表示する /
 * Change various "permanent" player variables.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 詳細を表示するアイテム情報の参照ポインタ
 * @return なし
 * @details
 * Wizard routines for creating objects		-RAK-
 * And for manipulating them!                   -Bernd-
 *
 * This has been rewritten to make the whole procedure
 * of debugging objects much easier and more comfortable.
 *
 * The following functions are meant to play with objects:
 * Create, modify, roll for them (for statistic purposes) and more.
 * The original functions were by RAK.
 * The function to show an item's debug information was written
 * by David Reeve Sward <sward+@CMU.EDU>.
 *                             Bernd (wiebelt@mathematik.hu-berlin.de)
 *
 * Here are the low-level functions
 * - wiz_display_item()
 *     display an item's debug-info
 * - wiz_create_itemtype()
 *     specify tval and sval (type and subtype of object)
 * - wiz_tweak_item()
 *     specify pval, +AC, +tohit, +todam
 *     Note that the wizard can leave this function anytime,
 *     thus accepting the default-values for the remaining values.
 *     pval comes first now, since it is most important.
 * - wiz_reroll_item()
 *     apply some magic to the item or turn it into an artifact.
 * - wiz_roll_item()
 *     Get some statistics about the rarity of an item:
 *     We create a lot of fake items and see if they are of the
 *     same type (tval and sval), then we compare pval and +AC.
 *     If the fake-item is better or equal it is counted.
 *     Note that cursed items that are better or equal (absolute values)
 *     are counted, too.
 *     HINT: This is *very* useful for balancing the game!
 * - wiz_quantity_item()
 *     change the quantity of an item, but be sane about it.
 *
 * And now the high-level functions
 * - do_cmd_wiz_play()
 *     play with an existing object
 * - wiz_create_item()
 *     create a new object
 *
 * Note -- You do not have to specify "pval" and other item-properties
 * directly. Just apply magic until you are satisfied with the item.
 *
 * Note -- For some items (such as wands, staffs, some rings, etc), you
 * must apply magic, or you will get "broken" or "uncharged" objects.
 *
 * Note -- Redefining artifacts via "do_cmd_wiz_play()" may destroy
 * the artifact.  Be careful.
 *
 * Hack -- this function will allow you to create multiple artifacts.
 * This "feature" may induce crashes or other nasty effects.
 * Just display an item's properties (debug-info)
 * Originally by David Reeve Sward <sward+@CMU.EDU>
 * Verbose item flags by -Bernd-
 */
static void wiz_display_item(player_type *player_ptr, object_type *o_ptr)
{
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	object_flags(o_ptr, flgs);

	/* Clear the screen */
	int j = 13;
	for (int i = 1; i <= 23; i++) prt("", i, j - 2);

	prt_alloc(o_ptr->tval, o_ptr->sval, 1, 0);

	/* Describe fully */
	char buf[256];
	object_desc(player_ptr, buf, o_ptr, OD_STORE);

	prt(buf, 2, j);

	prt(format("kind = %-5d  level = %-4d  tval = %-5d  sval = %-5d",
		o_ptr->k_idx, k_info[o_ptr->k_idx].level,
		o_ptr->tval, o_ptr->sval), 4, j);

	prt(format("number = %-3d  wgt = %-6d  ac = %-5d    damage = %dd%d",
		o_ptr->number, o_ptr->weight,
		o_ptr->ac, o_ptr->dd, o_ptr->ds), 5, j);

	prt(format("pval = %-5d  toac = %-5d  tohit = %-4d  todam = %-4d",
		o_ptr->pval, o_ptr->to_a, o_ptr->to_h, o_ptr->to_d), 6, j);

	prt(format("name1 = %-4d  name2 = %-4d  cost = %ld",
		o_ptr->name1, o_ptr->name2, (long)object_value_real(o_ptr)), 7, j);

	prt(format("ident = %04x  xtra1 = %-4d  xtra2 = %-4d  timeout = %-d",
		o_ptr->ident, o_ptr->xtra1, o_ptr->xtra2, o_ptr->timeout), 8, j);

	prt(format("xtra3 = %-4d  xtra4 = %-4d  xtra5 = %-4d  cursed  = %-d",
		o_ptr->xtra3, o_ptr->xtra4, o_ptr->xtra5, o_ptr->curse_flags), 9, j);

	prt("+------------FLAGS1------------+", 10, j);
	prt("AFFECT........SLAY........BRAND.", 11, j);
	prt("      mf      cvae      xsqpaefc", 12, j);
	prt("siwdccsossidsahanvudotgddhuoclio", 13, j);
	prt("tnieohtctrnipttmiinmrrnrrraiierl", 14, j);
	prt("rtsxnarelcfgdkcpmldncltggpksdced", 15, j);
	prt_binary(flgs[0], 16, j);

	prt("+------------FLAGS2------------+", 17, j);
	prt("SUST....IMMUN.RESIST............", 18, j);
	prt("      reaefctrpsaefcpfldbc sn   ", 19, j);
	prt("siwdcciaclioheatcliooeialoshtncd", 20, j);
	prt("tnieohdsierlrfraierliatrnnnrhehi", 21, j);
	prt("rtsxnaeydcedwlatdcedsrekdfddrxss", 22, j);
	prt_binary(flgs[1], 23, j);

	prt("+------------FLAGS3------------+", 10, j + 32);
	prt("fe cnn t      stdrmsiiii d ab   ", 11, j + 32);
	prt("aa aoomywhs lleeieihgggg rtgl   ", 12, j + 32);
	prt("uu utmacaih eielgggonnnnaaere   ", 13, j + 32);
	prt("rr reanurdo vtieeehtrrrrcilas   ", 14, j + 32);
	prt("aa algarnew ienpsntsaefctnevs   ", 15, j + 32);
	prt_binary(flgs[2], 16, j + 32);

	prt("+------------FLAGS4------------+", 17, j + 32);
	prt("KILL....ESP.........            ", 18, j + 32);
	prt("aeud tghaud tgdhegnu            ", 19, j + 32);
	prt("nvneoriunneoriruvoon            ", 20, j + 32);
	prt("iidmroamidmroagmionq            ", 21, j + 32);
	prt("mlenclnmmenclnnnldlu            ", 22, j + 32);
	prt_binary(flgs[3], 23, j + 32);
}


/*!
 * ベースアイテムの大項目IDの種別名をまとめる構造体 / A structure to hold a tval and its description
 */
typedef struct tval_desc
{
	int        tval; /*!< 大項目のID */
	concptr       desc; /*!< 大項目名 */
} tval_desc;

/*!
 * ベースアイテムの大項目IDの種別名定義 / A list of tvals and their textual names
 */
static tval_desc tvals[] =
{
	{ TV_SWORD,             "Sword"                },
	{ TV_POLEARM,           "Polearm"              },
	{ TV_HAFTED,            "Hafted Weapon"        },
	{ TV_BOW,               "Bow"                  },
	{ TV_ARROW,             "Arrows"               },
	{ TV_BOLT,              "Bolts"                },
	{ TV_SHOT,              "Shots"                },
	{ TV_SHIELD,            "Shield"               },
	{ TV_CROWN,             "Crown"                },
	{ TV_HELM,              "Helm"                 },
	{ TV_GLOVES,            "Gloves"               },
	{ TV_BOOTS,             "Boots"                },
	{ TV_CLOAK,             "Cloak"                },
	{ TV_DRAG_ARMOR,        "Dragon Scale Mail"    },
	{ TV_HARD_ARMOR,        "Hard Armor"           },
	{ TV_SOFT_ARMOR,        "Soft Armor"           },
	{ TV_RING,              "Ring"                 },
	{ TV_AMULET,            "Amulet"               },
	{ TV_LITE,              "Lite"                 },
	{ TV_POTION,            "Potion"               },
	{ TV_SCROLL,            "Scroll"               },
	{ TV_WAND,              "Wand"                 },
	{ TV_STAFF,             "Staff"                },
	{ TV_ROD,               "Rod"                  },
	{ TV_LIFE_BOOK,         "Life Spellbook"       },
	{ TV_SORCERY_BOOK,      "Sorcery Spellbook"    },
	{ TV_NATURE_BOOK,       "Nature Spellbook"     },
	{ TV_CHAOS_BOOK,        "Chaos Spellbook"      },
	{ TV_DEATH_BOOK,        "Death Spellbook"      },
	{ TV_TRUMP_BOOK,        "Trump Spellbook"      },
	{ TV_ARCANE_BOOK,       "Arcane Spellbook"     },
	{ TV_CRAFT_BOOK,      "Craft Spellbook"},
	{ TV_DAEMON_BOOK,       "Daemon Spellbook"},
	{ TV_CRUSADE_BOOK,      "Crusade Spellbook"},
	{ TV_MUSIC_BOOK,        "Music Spellbook"      },
	{ TV_HISSATSU_BOOK,     "Book of Kendo" },
	{ TV_HEX_BOOK,          "Hex Spellbook"        },
	{ TV_PARCHMENT,         "Parchment" },
	{ TV_WHISTLE,           "Whistle"	},
	{ TV_SPIKE,             "Spikes"               },
	{ TV_DIGGING,           "Digger"               },
	{ TV_CHEST,             "Chest"                },
	{ TV_CAPTURE,           "Capture Ball"         },
	{ TV_CARD,              "Express Card"         },
	{ TV_FIGURINE,          "Magical Figurine"     },
	{ TV_STATUE,            "Statue"               },
	{ TV_CORPSE,            "Corpse"               },
	{ TV_FOOD,              "Food"                 },
	{ TV_FLASK,             "Flask"                },
	{ TV_JUNK,              "Junk"                 },
	{ TV_SKELETON,          "Skeleton"             },
	{ 0,                    NULL                   }
};


/*!
 * 選択処理用キーコード /
 * Global array for converting numbers to a logical list symbol
 */
static const char listsym[] =
{
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'\0'
};

/*!
 * @brief ベースアイテムのウィザード生成のために大項目IDと小項目IDを取得する /
 * Specify tval and sval (type and subtype of object) originally
 * @return ベースアイテムID
 * @details
 * by RAK, heavily modified by -Bernd-
 * This function returns the k_idx of an object type, or zero if failed
 * List up to 50 choices in three columns
 */
static KIND_OBJECT_IDX wiz_create_itemtype(void)
{
	KIND_OBJECT_IDX i;
	int num, max_num;
	TERM_LEN col, row;
	OBJECT_TYPE_VALUE tval;

	concptr tval_desc;
	char ch;

	KIND_OBJECT_IDX choice[80];

	char buf[160];

	Term_clear();

	/* Print all tval's and their descriptions */
	for (num = 0; (num < 80) && tvals[num].tval; num++)
	{
		row = 2 + (num % 20);
		col = 20 * (num / 20);
		ch = listsym[num];
		prt(format("[%c] %s", ch, tvals[num].desc), row, col);
	}

	/* Me need to know the maximal possible tval_index */
	max_num = num;

	/* Choose! */
	if (!get_com("Get what type of object? ", &ch, FALSE)) return 0;

	/* Analyze choice */
	for (num = 0; num < max_num; num++)
	{
		if (listsym[num] == ch) break;
	}

	/* Bail out if choice is illegal */
	if ((num < 0) || (num >= max_num)) return 0;

	/* Base object type chosen, fill in tval */
	tval = tvals[num].tval;
	tval_desc = tvals[num].desc;

	/*** And now we go for k_idx ***/
	Term_clear();

	/* We have to search the whole itemlist. */
	for (num = 0, i = 1; (num < 80) && (i < max_k_idx); i++)
	{
		object_kind *k_ptr = &k_info[i];

		/* Analyze matching items */
		if (k_ptr->tval != tval) continue;

		/* Prepare it */
		row = 2 + (num % 20);
		col = 20 * (num / 20);
		ch = listsym[num];
		strcpy(buf, "                    ");

		/* Acquire the "name" of object "i" */
		strip_name(buf, i);

		/* Print it */
		prt(format("[%c] %s", ch, buf), row, col);

		/* Remember the object index */
		choice[num++] = i;
	}

	/* Me need to know the maximal possible remembered object_index */
	max_num = num;

	/* Choose! */
	if (!get_com(format("What Kind of %s? ", tval_desc), &ch, FALSE)) return 0;

	/* Analyze choice */
	for (num = 0; num < max_num; num++)
	{
		if (listsym[num] == ch) break;
	}

	/* Bail out if choice is "illegal" */
	if ((num < 0) || (num >= max_num)) return 0;

	/* And return successful */
	return (choice[num]);
}


/*!
 * @briefアイテムの基礎能力値を調整する / Tweak an item
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 調整するアイテムの参照ポインタ
 * @return なし
 */
static void wiz_tweak_item(player_type *player_ptr, object_type *o_ptr)
{
	if (object_is_artifact(o_ptr)) return;

	concptr p = "Enter new 'pval' setting: ";
	char tmp_val[80];
	sprintf(tmp_val, "%d", o_ptr->pval);
	if (!get_string(p, tmp_val, 5)) return;
	o_ptr->pval = (s16b)atoi(tmp_val);
	wiz_display_item(player_ptr, o_ptr);

	p = "Enter new 'to_a' setting: ";
	sprintf(tmp_val, "%d", o_ptr->to_a);
	if (!get_string(p, tmp_val, 5)) return;
	o_ptr->to_a = (s16b)atoi(tmp_val);
	wiz_display_item(player_ptr, o_ptr);

	p = "Enter new 'to_h' setting: ";
	sprintf(tmp_val, "%d", o_ptr->to_h);
	if (!get_string(p, tmp_val, 5)) return;
	o_ptr->to_h = (s16b)atoi(tmp_val);
	wiz_display_item(player_ptr, o_ptr);

	p = "Enter new 'to_d' setting: ";
	sprintf(tmp_val, "%d", (int)o_ptr->to_d);
	if (!get_string(p, tmp_val, 5)) return;
	o_ptr->to_d = (s16b)atoi(tmp_val);
	wiz_display_item(player_ptr, o_ptr);
}


/*!
 * @brief アイテムの質を選択して再生成する /
 * Apply magic to an item or turn it into an artifact. -Bernd-
 * @param o_ptr 再生成の対象となるアイテム情報の参照ポインタ
 * @return なし
 */
static void wiz_reroll_item(player_type *owner_ptr, object_type *o_ptr)
{
	if (object_is_artifact(o_ptr)) return;

	object_type forge;
	object_type *q_ptr;
	q_ptr = &forge;
	object_copy(q_ptr, o_ptr);

	/* Main loop. Ask for magification and artifactification */
	char ch;
	bool changed = FALSE;
	while (TRUE)
	{
		/* Display full item debug information */
		wiz_display_item(owner_ptr, q_ptr);

		/* Ask wizard what to do. */
		if (!get_com("[a]ccept, [w]orthless, [c]ursed, [n]ormal, [g]ood, [e]xcellent, [s]pecial? ", &ch, FALSE))
		{
			/* Preserve wizard-generated artifacts */
			if (object_is_fixed_artifact(q_ptr))
			{
				a_info[q_ptr->name1].cur_num = 0;
				q_ptr->name1 = 0;
			}

			changed = FALSE;
			break;
		}

		/* Create/change it! */
		if (ch == 'A' || ch == 'a')
		{
			changed = TRUE;
			break;
		}

		/* Preserve wizard-generated artifacts */
		if (object_is_fixed_artifact(q_ptr))
		{
			a_info[q_ptr->name1].cur_num = 0;
			q_ptr->name1 = 0;
		}

		switch (ch)
		{
			/* Apply bad magic, but first clear object */
		case 'w': case 'W':
		{
			object_prep(q_ptr, o_ptr->k_idx);
			apply_magic(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD | AM_GREAT | AM_CURSED);
			break;
		}
		/* Apply bad magic, but first clear object */
		case 'c': case 'C':
		{
			object_prep(q_ptr, o_ptr->k_idx);
			apply_magic(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD | AM_CURSED);
			break;
		}
		/* Apply normal magic, but first clear object */
		case 'n': case 'N':
		{
			object_prep(q_ptr, o_ptr->k_idx);
			apply_magic(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART);
			break;
		}
		/* Apply good magic, but first clear object */
		case 'g': case 'G':
		{
			object_prep(q_ptr, o_ptr->k_idx);
			apply_magic(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD);
			break;
		}
		/* Apply great magic, but first clear object */
		case 'e': case 'E':
		{
			object_prep(q_ptr, o_ptr->k_idx);
			apply_magic(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD | AM_GREAT);
			break;
		}
		/* Apply special magic, but first clear object */
		case 's': case 'S':
		{
			object_prep(q_ptr, o_ptr->k_idx);
			apply_magic(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_GOOD | AM_GREAT | AM_SPECIAL);

			/* Failed to create artifact; make a random one */
			if (!object_is_artifact(q_ptr)) become_random_artifact(owner_ptr, q_ptr, FALSE);
			break;
		}
		}

		q_ptr->iy = o_ptr->iy;
		q_ptr->ix = o_ptr->ix;
		q_ptr->next_o_idx = o_ptr->next_o_idx;
		q_ptr->marked = o_ptr->marked;
	}

	/* Notice change */
	if (changed)
	{
		object_copy(o_ptr, q_ptr);
		owner_ptr->update |= (PU_BONUS);
		owner_ptr->update |= (PU_COMBINE | PU_REORDER);
		owner_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);
	}
}


/*!
 * @brief 検査対象のアイテムを基準とした生成テストを行う /
 * Try to create an item again. Output some statistics.    -Bernd-
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 生成テストの基準となるアイテム情報の参照ポインタ
 * @return なし
 * The statistics are correct now.  We acquire a clean grid, and then
 * repeatedly place an object in this grid, copying it into an item
 * holder, and then deleting the object.  We fiddle with the artifact
 * counter flags to prevent weirdness.  We use the items to collect
 * statistics on item creation relative to the initial item.
 */
static void wiz_statistics(player_type *caster_ptr, object_type *o_ptr)
{
	object_type forge;
	object_type	*q_ptr;

	concptr q = "Rolls: %ld  Correct: %ld  Matches: %ld  Better: %ld  Worse: %ld  Other: %ld";
	concptr p = "Enter number of items to roll: ";
	char tmp_val[80];

	/* Mega-Hack -- allow multiple artifacts */
	if (object_is_fixed_artifact(o_ptr)) a_info[o_ptr->name1].cur_num = 0;

	/* Interact */
	u32b i, matches, better, worse, other, correct;
	u32b test_roll = 1000000;
	char ch;
	concptr quality;
	BIT_FLAGS mode;
	while (TRUE)
	{
		concptr pmt = "Roll for [n]ormal, [g]ood, or [e]xcellent treasure? ";

		/* Display item */
		wiz_display_item(caster_ptr, o_ptr);

		/* Get choices */
		if (!get_com(pmt, &ch, FALSE)) break;

		if (ch == 'n' || ch == 'N')
		{
			mode = 0L;
			quality = "normal";
		}
		else if (ch == 'g' || ch == 'G')
		{
			mode = AM_GOOD;
			quality = "good";
		}
		else if (ch == 'e' || ch == 'E')
		{
			mode = AM_GOOD | AM_GREAT;
			quality = "excellent";
		}
		else
		{
			break;
		}

		sprintf(tmp_val, "%ld", (long int)test_roll);
		if (get_string(p, tmp_val, 10)) test_roll = atol(tmp_val);
		test_roll = MAX(1, test_roll);

		/* Let us know what we are doing */
		msg_format("Creating a lot of %s items. Base level = %d.",
			quality, caster_ptr->current_floor_ptr->dun_level);
		msg_print(NULL);

		/* Set counters to zero */
		correct = matches = better = worse = other = 0;

		/* Let's rock and roll */
		for (i = 0; i <= test_roll; i++)
		{
			/* Output every few rolls */
			if ((i < 100) || (i % 100 == 0))
			{
				/* Do not wait */
				inkey_scan = TRUE;

				/* Allow interupt */
				if (inkey())
				{
					flush();
					break; // stop rolling
				}

				/* Dump the stats */
				prt(format(q, i, correct, matches, better, worse, other), 0, 0);
				Term_fresh();
			}
			q_ptr = &forge;
			object_wipe(q_ptr);

			/* Create an object */
			make_object(caster_ptr, q_ptr, mode);


			/* Mega-Hack -- allow multiple artifacts */
			if (object_is_fixed_artifact(q_ptr)) a_info[q_ptr->name1].cur_num = 0;


			/* Test for the same tval and sval. */
			if ((o_ptr->tval) != (q_ptr->tval)) continue;
			if ((o_ptr->sval) != (q_ptr->sval)) continue;

			/* One more correct item */
			correct++;

			/* Check for match */
			if ((q_ptr->pval == o_ptr->pval) &&
				(q_ptr->to_a == o_ptr->to_a) &&
				(q_ptr->to_h == o_ptr->to_h) &&
				(q_ptr->to_d == o_ptr->to_d) &&
				(q_ptr->name1 == o_ptr->name1))
			{
				matches++;
			}

			/* Check for better */
			else if ((q_ptr->pval >= o_ptr->pval) &&
				(q_ptr->to_a >= o_ptr->to_a) &&
				(q_ptr->to_h >= o_ptr->to_h) &&
				(q_ptr->to_d >= o_ptr->to_d))
			{
				better++;
			}

			/* Check for worse */
			else if ((q_ptr->pval <= o_ptr->pval) &&
				(q_ptr->to_a <= o_ptr->to_a) &&
				(q_ptr->to_h <= o_ptr->to_h) &&
				(q_ptr->to_d <= o_ptr->to_d))
			{
				worse++;
			}

			/* Assume different */
			else
			{
				other++;
			}
		}

		/* Final dump */
		msg_format(q, i, correct, matches, better, worse, other);
		msg_print(NULL);
	}

	/* Hack -- Normally only make a single artifact */
	if (object_is_fixed_artifact(o_ptr)) a_info[o_ptr->name1].cur_num = 1;
}


/*!
 * @brief 検査対象のアイテムの数を変更する /
 * Change the quantity of a the item
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 変更するアイテム情報構造体の参照ポインタ
 * @return なし
 */
static void wiz_quantity_item(object_type *o_ptr)
{
	/* Never duplicate artifacts */
	if (object_is_artifact(o_ptr)) return;

	/* Store old quantity. -LM- */
	int tmp_qnt = o_ptr->number;

	/* Default */
	char tmp_val[100];
	sprintf(tmp_val, "%d", (int)o_ptr->number);

	/* Query */
	if (get_string("Quantity: ", tmp_val, 2))
	{
		/* Extract */
		int tmp_int = atoi(tmp_val);
		if (tmp_int < 1) tmp_int = 1;
		if (tmp_int > 99) tmp_int = 99;

		/* Accept modifications */
		o_ptr->number = (byte)tmp_int;
	}

	if (o_ptr->tval == TV_ROD)
	{
		o_ptr->pval = o_ptr->pval * o_ptr->number / tmp_qnt;
	}
}


/*!
 * @brief 青魔導師の魔法を全て習得済みにする /
 * debug command for blue mage
 * @return なし
 */
static void do_cmd_wiz_blue_mage(player_type *caster_ptr)
{
	BIT_FLAGS f4 = 0L, f5 = 0L, f6 = 0L;
	for (int j = 1; j < A_MAX; j++)
	{
		set_rf_masks(&f4, &f5, &f6, j);

		int i;
		for (i = 0; i < 32; i++)
		{
			if ((0x00000001 << i) & f4) caster_ptr->magic_num2[i] = 1;
		}

		for (; i < 64; i++)
		{
			if ((0x00000001 << (i - 32)) & f5) caster_ptr->magic_num2[i] = 1;
		}

		for (; i < 96; i++)
		{
			if ((0x00000001 << (i - 64)) & f6) caster_ptr->magic_num2[i] = 1;
		}
	}
}


/*!
 * @brief アイテム検査のメインルーチン /
 * Play with an item. Options include:
 * @return なし
 * @details
 *   - Output statistics (via wiz_roll_item)<br>
 *   - Reroll item (via wiz_reroll_item)<br>
 *   - Change properties (via wiz_tweak_item)<br>
 *   - Change the number of items (via wiz_quantity_item)<br>
 */
static void do_cmd_wiz_play(player_type *creature_ptr)
{
	concptr q = "Play with which object? ";
	concptr s = "You have nothing to play with.";

	OBJECT_IDX item;
	object_type *o_ptr;
	o_ptr = choose_object(creature_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);

	if (!o_ptr) return;

	screen_save(creature_ptr);

	object_type	forge;
	object_type *q_ptr;
	q_ptr = &forge;
	object_copy(q_ptr, o_ptr);

	/* The main loop */
	char ch;
	bool changed = FALSE;
	while (TRUE)
	{
		/* Display the item */
		wiz_display_item(creature_ptr, q_ptr);

		/* Get choice */
		if (!get_com("[a]ccept [s]tatistics [r]eroll [t]weak [q]uantity? ", &ch, FALSE))
		{
			changed = FALSE;
			break;
		}

		if (ch == 'A' || ch == 'a')
		{
			changed = TRUE;
			break;
		}

		if (ch == 's' || ch == 'S')
		{
			wiz_statistics(creature_ptr, q_ptr);
		}

		if (ch == 'r' || ch == 'r')
		{
			wiz_reroll_item(creature_ptr, q_ptr);
		}

		if (ch == 't' || ch == 'T')
		{
			wiz_tweak_item(creature_ptr, q_ptr);
		}

		if (ch == 'q' || ch == 'Q')
		{
			wiz_quantity_item(q_ptr);
		}
	}

	screen_load(creature_ptr);

	/* Accept change */
	if (changed)
	{
		msg_print("Changes accepted.");

		/* Recalcurate object's weight */
		if (item >= 0)
		{
			creature_ptr->total_weight += (q_ptr->weight * q_ptr->number)
				- (o_ptr->weight * o_ptr->number);
		}

		/* Change */
		object_copy(o_ptr, q_ptr);

		creature_ptr->update |= (PU_BONUS);
		creature_ptr->update |= (PU_COMBINE | PU_REORDER);

		creature_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);
	}

	/* Ignore change */
	else
	{
		msg_print("Changes ignored.");
	}
}


/*!
 * @brief 任意のベースアイテム生成のメインルーチン /
 * Wizard routine for creating objects		-RAK-
 * @return なし
 * @details
 * Heavily modified to allow magification and artifactification  -Bernd-
 *
 * Note that wizards cannot create objects on top of other objects.
 *
 * Hack -- this routine always makes a "dungeon object", and applies
 * magic to it, and attempts to decline cursed items.
 */
static void wiz_create_item(player_type *caster_ptr)
{
	screen_save(caster_ptr);

	/* Get object base type */
	OBJECT_IDX k_idx = wiz_create_itemtype();

	screen_load(caster_ptr);

	/* Return if failed */
	if (!k_idx) return;

	if (k_info[k_idx].gen_flags & TRG_INSTA_ART)
	{
		ARTIFACT_IDX i;

		/* Artifactify */
		for (i = 1; i < max_a_idx; i++)
		{
			/* Ignore incorrect tval */
			if (a_info[i].tval != k_info[k_idx].tval) continue;

			/* Ignore incorrect sval */
			if (a_info[i].sval != k_info[k_idx].sval) continue;

			/* Create this artifact */
			(void)create_named_art(caster_ptr, i, caster_ptr->y, caster_ptr->x);

			/* All done */
			msg_print("Allocated(INSTA_ART).");

			return;
		}
	}

	object_type	forge;
	object_type *q_ptr;
	q_ptr = &forge;
	object_prep(q_ptr, k_idx);

	apply_magic(caster_ptr, q_ptr, caster_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART);

	/* Drop the object from heaven */
	(void)drop_near(caster_ptr, q_ptr, -1, caster_ptr->y, caster_ptr->x);

	/* All done */
	msg_print("Allocated.");
}


/*!
 * @brief プレイヤーを完全回復する /
 * Cure everything instantly
 * @return なし
 */
static void do_cmd_wiz_cure_all(player_type *creature_ptr)
{
	(void)life_stream(creature_ptr, FALSE, FALSE);
	(void)restore_mana(creature_ptr, TRUE);
	(void)set_food(creature_ptr, PY_FOOD_MAX - 1);
}


/*!
 * @brief 任意のダンジョン及び階層に飛ぶ /
 * Go to any level
 * @return なし
 */
static void do_cmd_wiz_jump(player_type *creature_ptr)
{
	/* Ask for level */
	if (command_arg <= 0)
	{
		char	ppp[80];
		char	tmp_val[160];
		DUNGEON_IDX tmp_dungeon_type;

		/* Prompt */
		sprintf(ppp, "Jump which dungeon : ");

		/* Default */
		sprintf(tmp_val, "%d", creature_ptr->dungeon_idx);

		/* Ask for a level */
		if (!get_string(ppp, tmp_val, 2)) return;

		tmp_dungeon_type = (DUNGEON_IDX)atoi(tmp_val);
		if (!d_info[tmp_dungeon_type].maxdepth || (tmp_dungeon_type > current_world_ptr->max_d_idx)) tmp_dungeon_type = DUNGEON_ANGBAND;

		/* Prompt */
		sprintf(ppp, "Jump to level (0, %d-%d): ",
			(int)d_info[tmp_dungeon_type].mindepth, (int)d_info[tmp_dungeon_type].maxdepth);

		/* Default */
		sprintf(tmp_val, "%d", (int)creature_ptr->current_floor_ptr->dun_level);

		/* Ask for a level */
		if (!get_string(ppp, tmp_val, 10)) return;

		/* Extract request */
		command_arg = (COMMAND_ARG)atoi(tmp_val);

		creature_ptr->dungeon_idx = tmp_dungeon_type;
	}

	if (command_arg < d_info[creature_ptr->dungeon_idx].mindepth) command_arg = 0;
	if (command_arg > d_info[creature_ptr->dungeon_idx].maxdepth) command_arg = (COMMAND_ARG)d_info[creature_ptr->dungeon_idx].maxdepth;

	/* Accept request */
	msg_format("You jump to dungeon level %d.", command_arg);

	if (autosave_l) do_cmd_save_game(creature_ptr, TRUE);

	/* Change level */
	creature_ptr->current_floor_ptr->dun_level = command_arg;

	prepare_change_floor_mode(creature_ptr, CFM_RAND_PLACE);

	if (!creature_ptr->current_floor_ptr->dun_level) creature_ptr->dungeon_idx = 0;
	creature_ptr->current_floor_ptr->inside_arena = FALSE;
	creature_ptr->wild_mode = FALSE;

	leave_quest_check(creature_ptr);

	if (record_stair) exe_write_diary(creature_ptr, DIARY_WIZ_TELE, 0, NULL);

	creature_ptr->current_floor_ptr->inside_quest = 0;
	free_turn(creature_ptr);

	/* Prevent energy_need from being too lower than 0 */
	creature_ptr->energy_need = 0;

	/*
	 * Clear all saved floors
	 * and create a first saved floor
	 */
	prepare_change_floor_mode(creature_ptr, CFM_FIRST_FLOOR);
	creature_ptr->leaving = TRUE;
}


/*!
 * @brief 全ベースアイテムを鑑定済みにする /
 * Become aware of a lot of objects
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void do_cmd_wiz_learn(player_type *caster_ptr)
{
	/* Scan every object */
	object_type forge;
	object_type *q_ptr;
	for (KIND_OBJECT_IDX i = 1; i < max_k_idx; i++)
	{
		object_kind *k_ptr = &k_info[i];

		/* Induce awareness */
		if (k_ptr->level <= command_arg)
		{
			q_ptr = &forge;
			object_prep(q_ptr, i);
			object_aware(caster_ptr, q_ptr);
		}
	}
}


/*!
 * @brief 現在のフロアに合ったモンスターをランダムに召喚する /
 * Summon some creatures
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param num 生成処理回数
 * @return なし
 */
static void do_cmd_wiz_summon(player_type *caster_ptr, int num)
{
	for (int i = 0; i < num; i++)
	{
		(void)summon_specific(caster_ptr, 0, caster_ptr->y, caster_ptr->x, caster_ptr->current_floor_ptr->dun_level, 0, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));
	}
}


/*!
 * @brief モンスターを種族IDを指定して敵対的に召喚する /
 * Summon a creature of the specified type
 * @param r_idx モンスター種族ID
 * @return なし
 * @details
 * This function is rather dangerous
 */
static void do_cmd_wiz_named(player_type *summoner_ptr, MONRACE_IDX r_idx)
{
	(void)summon_named_creature(summoner_ptr, 0, summoner_ptr->y, summoner_ptr->x, r_idx, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
}


/*!
 * @brief モンスターを種族IDを指定してペット召喚する /
 * Summon a creature of the specified type
 * @param r_idx モンスター種族ID
 * @return なし
 * @details
 * This function is rather dangerous
 */
static void do_cmd_wiz_named_friendly(player_type *summoner_ptr, MONRACE_IDX r_idx)
{
	(void)summon_named_creature(summoner_ptr, 0, summoner_ptr->y, summoner_ptr->x, r_idx, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP | PM_FORCE_PET));
}


/*!
 * @brief プレイヤー近辺の全モンスターを消去する /
 * Hack -- Delete all nearby monsters
 * @return なし
 */
static void do_cmd_wiz_zap(player_type *caster_ptr)
{
	/* Genocide everyone nearby */
	for (MONSTER_IDX i = 1; i < caster_ptr->current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
		if (!monster_is_valid(m_ptr)) continue;

		/* Skip the mount */
		if (i == caster_ptr->riding) continue;

		/* Delete nearby monsters */
		if (m_ptr->cdis > MAX_SIGHT) continue;

		if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname)
		{
			GAME_TEXT m_name[MAX_NLEN];

			monster_desc(caster_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
			exe_write_diary(caster_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_WIZ_ZAP, m_name);
		}

		delete_monster_idx(caster_ptr, i);
	}
}


/*!
 * @brief フロアに存在する全モンスターを消去する /
 * Hack -- Delete all monsters
 * @param caster_ptr 術者の参照ポインタ
 * @return なし
 */
static void do_cmd_wiz_zap_all(player_type *caster_ptr)
{
	/* Genocide everyone */
	for (MONSTER_IDX i = 1; i < caster_ptr->current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
		if (!monster_is_valid(m_ptr)) continue;

		/* Skip the mount */
		if (i == caster_ptr->riding) continue;

		if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname)
		{
			GAME_TEXT m_name[MAX_NLEN];

			monster_desc(caster_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
			exe_write_diary(caster_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_WIZ_ZAP, m_name);
		}

		/* Delete this monster */
		delete_monster_idx(caster_ptr, i);
	}
}


/*!
 * @brief 指定された地点の地形IDを変更する /
 * Create desired feature
 * @param creaturer_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void do_cmd_wiz_create_feature(player_type *creature_ptr)
{
	POSITION y, x;
	if (!tgt_pt(creature_ptr, &x, &y)) return;

	grid_type *g_ptr;
	g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

	/* Default */
	static int prev_feat = 0;
	char tmp_val[160];
	sprintf(tmp_val, "%d", prev_feat);

	/* Query */
	if (!get_string(_("地形: ", "Feature: "), tmp_val, 3)) return;

	/* Extract */
	FEAT_IDX tmp_feat = (FEAT_IDX)atoi(tmp_val);
	if (tmp_feat < 0) tmp_feat = 0;
	else if (tmp_feat >= max_f_idx) tmp_feat = max_f_idx - 1;

	/* Default */
	static int prev_mimic = 0;
	sprintf(tmp_val, "%d", prev_mimic);

	/* Query */
	if (!get_string(_("地形 (mimic): ", "Feature (mimic): "), tmp_val, 3)) return;

	/* Extract */
	FEAT_IDX tmp_mimic = (FEAT_IDX)atoi(tmp_val);
	if (tmp_mimic < 0) tmp_mimic = 0;
	else if (tmp_mimic >= max_f_idx) tmp_mimic = max_f_idx - 1;

	cave_set_feat(creature_ptr, y, x, tmp_feat);
	g_ptr->mimic = (s16b)tmp_mimic;

	feature_type *f_ptr;
	f_ptr = &f_info[get_feat_mimic(g_ptr)];

	if (have_flag(f_ptr->flags, FF_GLYPH) ||
		have_flag(f_ptr->flags, FF_MINOR_GLYPH))
		g_ptr->info |= (CAVE_OBJECT);
	else if (have_flag(f_ptr->flags, FF_MIRROR))
		g_ptr->info |= (CAVE_GLOW | CAVE_OBJECT);

	note_spot(creature_ptr, y, x);
	lite_spot(creature_ptr, y, x);
	creature_ptr->update |= (PU_FLOW);

	prev_feat = tmp_feat;
	prev_mimic = tmp_mimic;
}


/*!
 * @brief 現在のオプション設定をダンプ出力する /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * Hack -- Dump option bits usage
 * @return なし
 */
static void do_cmd_dump_options()
{
	char buf[1024];
	path_build(buf, sizeof buf, ANGBAND_DIR_USER, "opt_info.txt");

	/* File type is "TEXT" */
	FILE *fff;
	FILE_TYPE(FILE_TYPE_TEXT);
	fff = my_fopen(buf, "a");

	if (!fff)
	{
		msg_format(_("ファイル %s を開けませんでした。", "Failed to open file %s."), buf);
		msg_print(NULL);
		return;
	}

	/* Allocate the "exist" array (2-dimension) */
	int  **exist;
	C_MAKE(exist, NUM_O_SET, int *);
	C_MAKE(*exist, NUM_O_BIT * NUM_O_SET, int);
	for (int i = 1; i < NUM_O_SET; i++) exist[i] = *exist + i * NUM_O_BIT;

	/* Check for exist option bits */
	for (int i = 0; option_info[i].o_desc; i++)
	{
		const option_type *ot_ptr = &option_info[i];
		if (ot_ptr->o_var) exist[ot_ptr->o_set][ot_ptr->o_bit] = i + 1;
	}

	fprintf(fff, "[Option bits usage on Hengband %d.%d.%d]\n\n",
		FAKE_VER_MAJOR - 10, FAKE_VER_MINOR, FAKE_VER_PATCH);

	fputs("Set - Bit (Page) Option Name\n", fff);
	fputs("------------------------------------------------\n", fff);

	/* Dump option bits usage */
	for (int i = 0; i < NUM_O_SET; i++)
	{
		for (int j = 0; j < NUM_O_BIT; j++)
		{
			if (exist[i][j])
			{
				const option_type *ot_ptr = &option_info[exist[i][j] - 1];
				fprintf(fff, "  %d -  %02d (%4d) %s\n",
					i, j, ot_ptr->o_page, ot_ptr->o_text);
			}
			else
			{
				fprintf(fff, "  %d -  %02d\n", i, j);
			}
		}

		fputc('\n', fff);
	}

	/* Free the "exist" array (2-dimension) */
	C_KILL(*exist, NUM_O_BIT * NUM_O_SET, int);
	C_KILL(exist, NUM_O_SET, int *);
	my_fclose(fff);

	msg_format(_("オプションbit使用状況をファイル %s に書き出しました。", "Option bits usage dump saved to file %s."), buf);
}


/*!
 * @brief デバッグコマンドを選択する処理のメインルーチン /
 * Ask for and parse a "debug command"
 * The "command_arg" may have been set.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_debug(player_type *creature_ptr)
{
	char cmd;
	get_com("Debug Command: ", &cmd, FALSE);

	switch (cmd)
	{
	case ESCAPE:
	case ' ':
	case '\n':
	case '\r':
		break;

		/* Hack -- Generate Spoilers */
	case '"':
		do_cmd_spoilers(creature_ptr);
		break;

		/* Hack -- Help */
	case '?':
		do_cmd_help(creature_ptr);
		break;

		/* Cure all maladies */
	case 'a':
		do_cmd_wiz_cure_all(creature_ptr);
		break;

		/* Know alignment */
	case 'A':
		msg_format("Your alignment is %d.", creature_ptr->align);
		break;

		/* Teleport to target */
	case 'b':
		do_cmd_wiz_bamf(creature_ptr);
		break;

	case 'B':
		update_gambling_monsters(creature_ptr);
		break;

		/* Create any object */
	case 'c':
		wiz_create_item(creature_ptr);
		break;

		/* Create a named artifact */
	case 'C':
		wiz_create_named_art(creature_ptr);
		break;

		/* Detect everything */
	case 'd':
		detect_all(creature_ptr, DETECT_RAD_ALL * 3);
		break;

		/* Dimension_door */
	case 'D':
		wiz_dimension_door(creature_ptr);
		break;

		/* Edit character */
	case 'e':
		do_cmd_wiz_change(creature_ptr);
		break;

		/* Blue Mage Only */
	case 'E':
		if (creature_ptr->pclass == CLASS_BLUE_MAGE)
		{
			do_cmd_wiz_blue_mage(creature_ptr);
		}
		break;

		/* View item info */
	case 'f':
		identify_fully(creature_ptr, FALSE, 0);
		break;

		/* Create desired feature */
	case 'F':
		do_cmd_wiz_create_feature(creature_ptr);
		break;

		/* Good Objects */
	case 'g':
		if (command_arg <= 0) command_arg = 1;
		acquirement(creature_ptr, creature_ptr->y, creature_ptr->x, command_arg, FALSE, FALSE, TRUE);
		break;

		/* Hitpoint rerating */
	case 'h':
		roll_hitdice(creature_ptr, SPOP_DISPLAY_MES | SPOP_DEBUG);
		break;

	case 'H':
		do_cmd_summon_horde(creature_ptr);
		break;

		/* Identify */
	case 'i':
		(void)ident_spell(creature_ptr, FALSE, 0);
		break;

		/* Go up or down in the dungeon */
	case 'j':
		do_cmd_wiz_jump(creature_ptr);
		break;

		/* Self-Knowledge */
	case 'k':
		self_knowledge(creature_ptr);
		break;

		/* Learn about objects */
	case 'l':
		do_cmd_wiz_learn(creature_ptr);
		break;

		/* Magic Mapping */
	case 'm':
		map_area(creature_ptr, DETECT_RAD_ALL * 3);
		break;

		/* Mutation */
	case 'M':
		(void)gain_mutation(creature_ptr, command_arg);
		break;

		/* Reset Class */
	case 'R':
		(void)do_cmd_wiz_reset_class(creature_ptr);
		break;

		/* Specific reward */
	case 'r':
		(void)gain_level_reward(creature_ptr, command_arg);
		break;

		/* Summon _friendly_ named monster */
	case 'N':
		do_cmd_wiz_named_friendly(creature_ptr, command_arg);
		break;

		/* Summon Named Monster */
	case 'n':
		do_cmd_wiz_named(creature_ptr, command_arg);
		break;

		/* Dump option bits usage */
	case 'O':
		do_cmd_dump_options();
		break;

		/* Object playing routines */
	case 'o':
		do_cmd_wiz_play(creature_ptr);
		break;

		/* Phase Door */
	case 'p':
		teleport_player(creature_ptr, 10, TELEPORT_SPONTANEOUS);
		break;

		/* Take a Quests */
	case 'Q':
	{
		char ppp[30];
		char tmp_val[5];
		int tmp_int;
		sprintf(ppp, "QuestID (0-%d):", max_q_idx - 1);
		sprintf(tmp_val, "%d", 0);

		if (!get_string(ppp, tmp_val, 3)) return;
		tmp_int = atoi(tmp_val);

		if (tmp_int < 0) break;
		if (tmp_int >= max_q_idx) break;

		creature_ptr->current_floor_ptr->inside_quest = (QUEST_IDX)tmp_int;
		process_dungeon_file(creature_ptr, "q_info.txt", 0, 0, 0, 0);
		quest[tmp_int].status = QUEST_STATUS_TAKEN;
		creature_ptr->current_floor_ptr->inside_quest = 0;
	}

	break;

	/* Complete a Quest -KMW- */
	case 'q':
		if (creature_ptr->current_floor_ptr->inside_quest)
		{
			if (quest[creature_ptr->current_floor_ptr->inside_quest].status == QUEST_STATUS_TAKEN)
			{
				complete_quest(creature_ptr, creature_ptr->current_floor_ptr->inside_quest);
				break;
			}
		}
		else
		{
			msg_print("No current quest");
			msg_print(NULL);
		}

		break;

		/* Make every dungeon square "known" to test streamers -KMW- */
	case 'u':
		for (int y = 0; y < creature_ptr->current_floor_ptr->height; y++)
		{
			for (int x = 0; x < creature_ptr->current_floor_ptr->width; x++)
			{
				creature_ptr->current_floor_ptr->grid_array[y][x].info |= (CAVE_GLOW | CAVE_MARK);
			}
		}

		wiz_lite(creature_ptr, FALSE);
		break;

		/* Summon Random Monster(s) */
	case 's':
		if (command_arg <= 0) command_arg = 1;
		do_cmd_wiz_summon(creature_ptr, command_arg);
		break;

		/* Special(Random Artifact) Objects */
	case 'S':
		if (command_arg <= 0) command_arg = 1;
		acquirement(creature_ptr, creature_ptr->y, creature_ptr->x, command_arg, TRUE, TRUE, TRUE);
		break;

		/* Teleport */
	case 't':
		teleport_player(creature_ptr, 100, TELEPORT_SPONTANEOUS);
		break;

		/* Game Time Setting */
	case 'T':
		set_gametime();
		break;

		/* Very Good Objects */
	case 'v':
		if (command_arg <= 0) command_arg = 1;
		acquirement(creature_ptr, creature_ptr->y, creature_ptr->x, command_arg, TRUE, FALSE, TRUE);
		break;

		/* Wizard Light the Level */
	case 'w':
		wiz_lite(creature_ptr, (bool)(creature_ptr->pclass == CLASS_NINJA));
		break;

		/* Increase Experience */
	case 'x':
		gain_exp(creature_ptr, command_arg ? command_arg : (creature_ptr->exp + 1));
		break;

		/* Zap Monsters (Genocide) */
	case 'z':
		do_cmd_wiz_zap(creature_ptr);
		break;

		/* Zap Monsters (Omnicide) */
	case 'Z':
		do_cmd_wiz_zap_all(creature_ptr);
		break;

		/* Hack -- whatever I desire */
	case '_':
		probing(creature_ptr);
		break;

		/* For temporary test. */
	case 'X':
	{
		INVENTORY_IDX i;
		for (i = INVEN_TOTAL - 1; i >= 0; i--)
		{
			if (creature_ptr->inventory_list[i].k_idx) drop_from_inventory(creature_ptr, i, 999);
		}
		player_outfit(creature_ptr);
		break;
	}

	case 'V':
		do_cmd_wiz_reset_class(creature_ptr);
		break;

	case '@':
		do_cmd_debug_spell(creature_ptr);
		break;

	default:
		msg_print("That is not a valid debug command.");
		break;
	}
}
