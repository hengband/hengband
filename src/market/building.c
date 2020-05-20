/*!
 * @file building.c
 * @brief 町の施設処理 / Building commands
 * @date 2013/12/23
 * @author
 * Created by Ken Wigle for Kangband - a variant of Angband 2.8.3
 * -KMW-
 * 
 * Rewritten for Kangband 2.8.3i using Kamband's version of
 * building.c as written by Ivan Tkatchev
 * 
 * Changed for ZAngband by Robert Ruehlmann
*/

#include "system/angband.h"
#include "util/util.h"
#include "main/music-definitions-table.h"
#include "term/gameterm.h"

#include "system/system-variables.h"
#include "core/stuff-handler.h"
#include "core/show-file.h"
#include "core/special-internal-keys.h"
#include "cmd/cmd-dump.h"
#include "cmd/cmd-inn.h"
#include "floor/floor.h"
#include "floor/floor-events.h"
#include "floor/floor-save.h"
#include "autopick/autopick.h"
#include "object/object-kind.h"
#include "object/object-boost.h"
#include "object/object-flavor.h"
#include "object/object-hook.h"
#include "monster/monster.h"
#include "monster/monsterrace-hook.h"
#include "melee.h"
#include "floor/wild.h"
#include "world/world.h"
#include "core/sort.h"

#include "player/avatar.h"
#include "market/building.h"
#include "dungeon/dungeon.h"
#include "mutation/mutation.h"
#include "dungeon/quest.h"
#include "object/artifact.h"
#include "cmd-spell.h"
#include "spell/spells3.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "realm/realm-hex.h"
#include "dungeon/dungeon-file.h"

#include "io/files-util.h"
#include "player/player-status.h"
#include "player/player-effects.h"
#include "player/player-class.h"
#include "player/player-personalities-table.h"
#include "inventory/player-inventory.h"
#include "core/scores.h"
#include "shoot.h"
#include "view/display-main-window.h"
#include "monster/monster-race.h"
#include "autopick/autopick.h"

#include "market/poker.h"
#include "market/building-util.h"
#include "market/play-gamble.h"
#include "view/display-fruit.h"
#include "market/arena.h"

building_type building[MAX_BLDG];

MONRACE_IDX battle_mon[4];
u32b mon_odds[4];
int battle_odds;
PRICE kakekin;
int sel_monster;

bool reinit_wilderness = FALSE;
MONSTER_IDX today_mon;

/*!
 * @brief 施設毎に設定された種族、職業、魔法領域フラグがプレイヤーと一致するかを判定する。
 * @details 各種ギルドや寺院など、特定の職業ならば優遇措置を得られる施設、
 * あるいは食堂など特定の種族では利用できない施設の判定処理を行う。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param bldg 施設構造体の参照ポインタ
 * @return 種族、職業、魔法領域のいずれかが一致しているかの是非。
 */
static bool is_owner(player_type *player_ptr, building_type *bldg)
{
	if (bldg->member_class[player_ptr->pclass] == BUILDING_OWNER)
	{
		return TRUE;
	}

	if (bldg->member_race[player_ptr->prace] == BUILDING_OWNER)
	{
		return TRUE;
	}

	REALM_IDX realm1 = player_ptr->realm1;
	REALM_IDX realm2 = player_ptr->realm2;
	if ((is_magic(realm1) && (bldg->member_realm[realm1] == BUILDING_OWNER)) ||
		(is_magic(realm2) && (bldg->member_realm[realm2] == BUILDING_OWNER)))
	{
		return TRUE;
	}

	return FALSE;
}


/*!
 * @brief 施設毎に設定された種族、職業、魔法領域フラグがプレイヤーと一致するかを判定する。
 （スペルマスターの特別判定つき）
 * @details 各種ギルドや寺院など、特定の職業ならば優遇措置を得られる施設、
 * あるいは食堂など特定の種族では利用できない施設の判定処理を行う。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param bldg 施設構造体の参照ポインタ
 * @return 種族、職業、魔法領域のいずれかが一致しているかの是非。
 * @todo is_owner()との実質的な多重実装なので、リファクタリングを行うべきである。
 */
static bool is_member(player_type *player_ptr, building_type *bldg)
{
	if (bldg->member_class[player_ptr->pclass])
	{
		return TRUE;
	}

	if (bldg->member_race[player_ptr->prace])
	{
		return TRUE;
	}

	REALM_IDX realm1 = player_ptr->realm1;
	REALM_IDX realm2 = player_ptr->realm2;
	if ((is_magic(realm1) && bldg->member_realm[realm1]) ||
		(is_magic(realm2) && bldg->member_realm[realm2]))
	{
		return TRUE;
	}

	if (player_ptr->pclass != CLASS_SORCERER) return FALSE;

	for (int i = 0; i < MAX_MAGIC; i++)
	{
		if (bldg->member_realm[i + 1]) return TRUE;
	}

	return FALSE;
}


/*!
 * @brief 所持金を表示する。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void building_prt_gold(player_type *player_ptr)
{
	char tmp_str[80];
	prt(_("手持ちのお金: ", "Gold Remaining: "), 23, 53);
	sprintf(tmp_str, "%9ld", (long)player_ptr->au);
	prt(tmp_str, 23, 68);
}


/*!
 * @brief 施設のサービス一覧を表示する / Display a building.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param bldg 施設構造体の参照ポインタ
 * @return なし
 */
static void show_building(player_type *player_ptr, building_type* bldg)
{
	char buff[20];
	byte action_color;
	char tmp_str[80];

	Term_clear();
	sprintf(tmp_str, "%s (%s) %35s", bldg->owner_name, bldg->owner_race, bldg->name);
	prt(tmp_str, 2, 1);

	for (int i = 0; i < 8; i++)
	{
		if (!bldg->letters[i]) continue;

		if (bldg->action_restr[i] == 0)
		{
			if ((is_owner(player_ptr, bldg) && (bldg->member_costs[i] == 0)) ||
				(!is_owner(player_ptr, bldg) && (bldg->other_costs[i] == 0)))
			{
				action_color = TERM_WHITE;
				buff[0] = '\0';
			}
			else if (is_owner(player_ptr, bldg))
			{
				action_color = TERM_YELLOW;
				sprintf(buff, _("($%ld)", "(%ldgp)"), (long int)bldg->member_costs[i]);
			}
			else
			{
				action_color = TERM_YELLOW;
				sprintf(buff, _("($%ld)", "(%ldgp)"), (long int)bldg->other_costs[i]);
			}

			sprintf(tmp_str, " %c) %s %s", bldg->letters[i], bldg->act_names[i], buff);
			c_put_str(action_color, tmp_str, 19 + (i / 2), 35 * (i % 2));
			continue;
		}

		if (bldg->action_restr[i] == 1)
		{
			if (!is_member(player_ptr, bldg))
			{
				action_color = TERM_L_DARK;
				strcpy(buff, _("(閉店)", "(closed)"));
			}
			else if ((is_owner(player_ptr, bldg) && (bldg->member_costs[i] == 0)) ||
				(is_member(player_ptr, bldg) && (bldg->other_costs[i] == 0)))
			{
				action_color = TERM_WHITE;
				buff[0] = '\0';
			}
			else if (is_owner(player_ptr, bldg))
			{
				action_color = TERM_YELLOW;
				sprintf(buff, _("($%ld)", "(%ldgp)"), (long int)bldg->member_costs[i]);
			}
			else
			{
				action_color = TERM_YELLOW;
				sprintf(buff, _("($%ld)", "(%ldgp)"), (long int)bldg->other_costs[i]);
			}

			sprintf(tmp_str, " %c) %s %s", bldg->letters[i], bldg->act_names[i], buff);
			c_put_str(action_color, tmp_str, 19 + (i / 2), 35 * (i % 2));
			continue;
		}

		if (!is_owner(player_ptr, bldg))
		{
			action_color = TERM_L_DARK;
			strcpy(buff, _("(閉店)", "(closed)"));
		}
		else if (bldg->member_costs[i] != 0)
		{
			action_color = TERM_YELLOW;
			sprintf(buff, _("($%ld)", "(%ldgp)"), (long int)bldg->member_costs[i]);
		}
		else
		{
			action_color = TERM_WHITE;
			buff[0] = '\0';
		}

		sprintf(tmp_str, " %c) %s %s", bldg->letters[i], bldg->act_names[i], buff);
		c_put_str(action_color, tmp_str, 19 + (i / 2), 35 * (i % 2));
	}

	prt(_(" ESC) 建物を出る", " ESC) Exit building"), 23, 0);
}


/*!
 * @brief モンスター闘技場のメインルーチン
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 賭けを開始したか否か
 */
static bool monster_arena_comm(player_type *player_ptr)
{
	PRICE maxbet;
	PRICE wager;
	char out_val[160], tmp_str[80];
	concptr p;

	if ((current_world_ptr->game_turn - current_world_ptr->arena_start_turn) > TURNS_PER_TICK * 250)
	{
		update_gambling_monsters(player_ptr);
		current_world_ptr->arena_start_turn = current_world_ptr->game_turn;
	}

	screen_save();

	/* No money */
	if (player_ptr->au <= 1)
	{
		msg_print(_("おい！おまえ一文なしじゃないか！こっから出ていけ！", "Hey! You don't have gold - get out of here!"));
		msg_print(NULL);
		screen_load();
		return FALSE;
	}

	clear_bldg(4, 10);

	prt(_("モンスター                                                     倍率",
		"Monsters                                                       Odds"), 4, 4);
	for (int i = 0; i < 4; i++)
	{
		char buf[80];
		monster_race *r_ptr = &r_info[battle_mon[i]];

		sprintf(buf, _("%d) %-58s  %4ld.%02ld倍", "%d) %-58s  %4ld.%02ld"), i + 1,
			_(format("%s%s", r_name + r_ptr->name, (r_ptr->flags1 & RF1_UNIQUE) ? "もどき" : "      "),
				format("%s%s", (r_ptr->flags1 & RF1_UNIQUE) ? "Fake " : "", r_name + r_ptr->name)),
				(long int)mon_odds[i] / 100, (long int)mon_odds[i] % 100);
		prt(buf, 5 + i, 1);
	}

	prt(_("どれに賭けますか:", "Which monster: "), 0, 0);
	while (TRUE)
	{
		int i = inkey();

		if (i == ESCAPE)
		{
			screen_load();
			return FALSE;
		}

		if (i >= '1' && i <= '4')
		{
			sel_monster = i - '1';
			battle_odds = mon_odds[sel_monster];
			break;
		}

		else bell();
	}

	clear_bldg(4, 4);
	for (int i = 0; i < 4; i++)
		if (i != sel_monster) clear_bldg(i + 5, i + 5);

	maxbet = player_ptr->lev * 200;

	/* We can't bet more than we have */
	maxbet = MIN(maxbet, player_ptr->au);

	/* Get the wager */
	strcpy(out_val, "");
	sprintf(tmp_str, _("賭け金 (1-%ld)？", "Your wager (1-%ld) ? "), (long int)maxbet);

	/*
	 * Use get_string() because we may need more than
	 * the s16b value returned by get_quantity().
	 */
	if (!get_string(tmp_str, out_val, 32))
	{
		screen_load();
		return FALSE;
	}

	for (p = out_val; *p == ' '; p++);

	wager = atol(p);
	if (wager > player_ptr->au)
	{
		msg_print(_("おい！金が足りないじゃないか！出ていけ！", "Hey! You don't have the gold - get out of here!"));

		msg_print(NULL);
		screen_load();
		return FALSE;
	}
	else if (wager > maxbet)
	{
		msg_format(_("%ldゴールドだけ受けよう。残りは取っときな。", "I'll take %ld gold of that. Keep the rest."), (long int)maxbet);

		wager = maxbet;
	}
	else if (wager < 1)
	{
		msg_print(_("ＯＫ、１ゴールドでいこう。", "Ok, we'll start with 1 gold."));
		wager = 1;
	}

	msg_print(NULL);
	battle_odds = MAX(wager + 1, wager * battle_odds / 100);
	kakekin = wager;
	player_ptr->au -= wager;
	reset_tim_flags(player_ptr);

	prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS);

	player_ptr->phase_out = TRUE;
	player_ptr->leaving = TRUE;
	player_ptr->leave_bldg = TRUE;

	screen_load();
	return TRUE;
}


/*!
 * @brief 本日の賞金首情報を表示する。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void today_target(player_type *player_ptr)
{
	char buf[160];
	monster_race *r_ptr = &r_info[today_mon];

	clear_bldg(4, 18);
	c_put_str(TERM_YELLOW, _("本日の賞金首", "Wanted monster that changes from day to day"), 5, 10);
	sprintf(buf, _("ターゲット： %s", "target: %s"), r_name + r_ptr->name);
	c_put_str(TERM_YELLOW, buf, 6, 10);
	sprintf(buf, _("死体 ---- $%d", "corpse   ---- $%d"), (int)r_ptr->level * 50 + 100);
	prt(buf, 8, 10);
	sprintf(buf, _("骨   ---- $%d", "skeleton ---- $%d"), (int)r_ptr->level * 30 + 60);
	prt(buf, 9, 10);
	player_ptr->today_mon = today_mon;
}


/*!
 * @brief ツチノコの賞金首情報を表示する。
 * @return なし
 */
static void tsuchinoko(void)
{
	clear_bldg(4, 18);
	c_put_str(TERM_YELLOW, _("一獲千金の大チャンス！！！", "Big chance for quick money!!!"), 5, 10);
	c_put_str(TERM_YELLOW, _("ターゲット：幻の珍獣「ツチノコ」", "target: the rarest animal 'Tsuchinoko'"), 6, 10);
	c_put_str(TERM_WHITE, _("生け捕り ---- $1,000,000", "catch alive ---- $1,000,000"), 8, 10);
	c_put_str(TERM_WHITE, _("死体     ----   $200,000", "corpse      ----   $200,000"), 9, 10);
	c_put_str(TERM_WHITE, _("骨       ----   $100,000", "bones       ----   $100,000"), 10, 10);
}


/*!
 * @brief 通常の賞金首情報を表示する。
 * @return なし
 */
static void show_bounty(void)
{
	TERM_LEN y = 0;

	clear_bldg(4, 18);
	prt(_("死体を持ち帰れば報酬を差し上げます。", "Offer a prize when you bring a wanted monster's corpse"), 4, 10);
	c_put_str(TERM_YELLOW, _("現在の賞金首", "Wanted monsters"), 6, 10);

	for (int i = 0; i < MAX_BOUNTY; i++)
	{
		byte color;
		concptr done_mark;
		monster_race *r_ptr = &r_info[(current_world_ptr->bounty_r_idx[i] > 10000 ? current_world_ptr->bounty_r_idx[i] - 10000 : current_world_ptr->bounty_r_idx[i])];

		if (current_world_ptr->bounty_r_idx[i] > 10000)
		{
			color = TERM_RED;
			done_mark = _("(済)", "(done)");
		}
		else
		{
			color = TERM_WHITE;
			done_mark = "";
		}

		c_prt(color, format("%s %s", r_name + r_ptr->name, done_mark), y + 7, 10);

		y = (y + 1) % 10;
		if (!y && (i < MAX_BOUNTY - 1))
		{
			prt(_("何かキーを押してください", "Hit any key."), 0, 0);
			(void)inkey();
			prt("", 0, 0);
			clear_bldg(7, 18);
		}
	}
}


/*!
 * 賞金首の報酬テーブル / List of prize object
 */
static struct {
	OBJECT_TYPE_VALUE tval; /*!< ベースアイテムのメイン種別ID */
	OBJECT_SUBTYPE_VALUE sval; /*!< ベースアイテムのサブ種別ID */
} prize_list[MAX_BOUNTY] =
{
	{TV_POTION, SV_POTION_CURING},
	{TV_POTION, SV_POTION_SPEED},
	{TV_POTION, SV_POTION_SPEED},
	{TV_POTION, SV_POTION_RESISTANCE},
	{TV_POTION, SV_POTION_ENLIGHTENMENT},

	{TV_POTION, SV_POTION_HEALING},
	{TV_POTION, SV_POTION_RESTORE_MANA},
	{TV_SCROLL, SV_SCROLL_STAR_DESTRUCTION},
	{TV_POTION, SV_POTION_STAR_ENLIGHTENMENT},
	{TV_SCROLL, SV_SCROLL_SUMMON_PET},

	{TV_SCROLL, SV_SCROLL_GENOCIDE},
	{TV_POTION, SV_POTION_STAR_HEALING},
	{TV_POTION, SV_POTION_STAR_HEALING},
	{TV_POTION, SV_POTION_NEW_LIFE},
	{TV_SCROLL, SV_SCROLL_MASS_GENOCIDE},

	{TV_POTION, SV_POTION_LIFE},
	{TV_POTION, SV_POTION_LIFE},
	{TV_POTION, SV_POTION_AUGMENTATION},
	{TV_POTION, SV_POTION_INVULNERABILITY},
	{TV_SCROLL, SV_SCROLL_ARTIFACT},
};


/*!
 * @brief 賞金首の引き換え処理 / Get prize
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 各種賞金首のいずれかでも換金が行われたか否か。
 */
static bool kankin(player_type *player_ptr)
{
	bool change = FALSE;
	GAME_TEXT o_name[MAX_NLEN];
	object_type *o_ptr;

	for (INVENTORY_IDX i = 0; i <= INVEN_LARM; i++)
	{
		o_ptr = &player_ptr->inventory_list[i];
		if ((o_ptr->tval == TV_CAPTURE) && (o_ptr->pval == MON_TSUCHINOKO))
		{
			char buf[MAX_NLEN + 20];
			object_desc(player_ptr, o_name, o_ptr, 0);
			sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "), o_name);
			if (get_check(buf))
			{
				msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)(1000000L * o_ptr->number));
				player_ptr->au += 1000000L * o_ptr->number;
				player_ptr->redraw |= (PR_GOLD);
				vary_item(player_ptr, i, -o_ptr->number);
			}

			change = TRUE;
		}
	}

	for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &player_ptr->inventory_list[i];
		if ((o_ptr->tval == TV_CORPSE) && (o_ptr->sval == SV_CORPSE) && (o_ptr->pval == MON_TSUCHINOKO))
		{
			char buf[MAX_NLEN + 20];
			object_desc(player_ptr, o_name, o_ptr, 0);
			sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "), o_name);
			if (get_check(buf))
			{
				msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)(200000L * o_ptr->number));
				player_ptr->au += 200000L * o_ptr->number;
				player_ptr->redraw |= (PR_GOLD);
				vary_item(player_ptr, i, -o_ptr->number);
			}

			change = TRUE;
		}
	}

	for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &player_ptr->inventory_list[i];
		if ((o_ptr->tval == TV_CORPSE) && (o_ptr->sval == SV_SKELETON) && (o_ptr->pval == MON_TSUCHINOKO))
		{
			char buf[MAX_NLEN + 20];
			object_desc(player_ptr, o_name, o_ptr, 0);
			sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "), o_name);
			if (get_check(buf))
			{
				msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)(100000L * o_ptr->number));
				player_ptr->au += 100000L * o_ptr->number;
				player_ptr->redraw |= (PR_GOLD);
				vary_item(player_ptr, i, -o_ptr->number);
			}
			change = TRUE;
		}
	}

	for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &player_ptr->inventory_list[i];
		if ((o_ptr->tval == TV_CORPSE) && (o_ptr->sval == SV_CORPSE) && (streq(r_name + r_info[o_ptr->pval].name, r_name + r_info[today_mon].name)))
		{
			char buf[MAX_NLEN + 20];
			object_desc(player_ptr, o_name, o_ptr, 0);
			sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "), o_name);
			if (get_check(buf))
			{
				msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)((r_info[today_mon].level * 50 + 100) * o_ptr->number));
				player_ptr->au += (r_info[today_mon].level * 50 + 100) * o_ptr->number;
				player_ptr->redraw |= (PR_GOLD);
				vary_item(player_ptr, i, -o_ptr->number);
			}
			change = TRUE;
		}
	}

	for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &player_ptr->inventory_list[i];

		if ((o_ptr->tval == TV_CORPSE) && (o_ptr->sval == SV_SKELETON) && (streq(r_name + r_info[o_ptr->pval].name, r_name + r_info[today_mon].name)))
		{
			char buf[MAX_NLEN + 20];
			object_desc(player_ptr, o_name, o_ptr, 0);
			sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "), o_name);
			if (get_check(buf))
			{
				msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)((r_info[today_mon].level * 30 + 60) * o_ptr->number));
				player_ptr->au += (r_info[today_mon].level * 30 + 60) * o_ptr->number;
				player_ptr->redraw |= (PR_GOLD);
				vary_item(player_ptr, i, -o_ptr->number);
			}

			change = TRUE;
		}
	}

	for (int j = 0; j < MAX_BOUNTY; j++)
	{
		for (INVENTORY_IDX i = INVEN_PACK - 1; i >= 0; i--)
		{
			o_ptr = &player_ptr->inventory_list[i];
			if ((o_ptr->tval != TV_CORPSE) || (o_ptr->pval != current_world_ptr->bounty_r_idx[j])) continue;

			char buf[MAX_NLEN + 20];
			int num, k;
			INVENTORY_IDX item_new;
			object_type forge;

			object_desc(player_ptr, o_name, o_ptr, 0);
			sprintf(buf, _("%sを渡しますか？", "Hand %s over? "), o_name);
			if (!get_check(buf)) continue;

			vary_item(player_ptr, i, -o_ptr->number);
			chg_virtue(player_ptr, V_JUSTICE, 5);
			current_world_ptr->bounty_r_idx[j] += 10000;

			for (num = 0, k = 0; k < MAX_BOUNTY; k++)
			{
				if (current_world_ptr->bounty_r_idx[k] >= 10000) num++;
			}

			msg_format(_("これで合計 %d ポイント獲得しました。", "You earned %d point%s total."), num, (num > 1 ? "s" : ""));

			object_prep(&forge, lookup_kind(prize_list[num - 1].tval, prize_list[num - 1].sval));
			apply_magic(player_ptr, &forge, player_ptr->current_floor_ptr->object_level, AM_NO_FIXED_ART);

			object_aware(player_ptr, &forge);
			object_known(&forge);

			/*
			 * Hand it --- Assume there is an empty slot.
			 * Since a corpse is handed at first,
			 * there is at least one empty slot.
			 */
			item_new = inven_carry(player_ptr, &forge);
			object_desc(player_ptr, o_name, &forge, 0);
			msg_format(_("%s(%c)を貰った。", "You get %s (%c). "), o_name, index_to_label(item_new));

			autopick_alter_item(player_ptr, item_new, FALSE);
			handle_stuff(player_ptr);
			change = TRUE;
		}
	}

	if (change) return TRUE;

	msg_print(_("賞金を得られそうなものは持っていなかった。", "You have nothing."));
	msg_print(NULL);
	return FALSE;
}


/*!
 * @brief クエスト情報を表示しつつ処理する。/ Display quest information
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param questnum クエストのID
 * @param do_init クエストの開始処理(TRUE)、結果処理か(FALSE)
 * @return なし
 */
static void get_questinfo(player_type *player_ptr, IDX questnum, bool do_init)
{
	for (int i = 0; i < 10; i++)
	{
		quest_text[i][0] = '\0';
	}

	quest_text_line = 0;

	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	QUEST_IDX old_quest = floor_ptr->inside_quest;
	floor_ptr->inside_quest = questnum;

	init_flags = INIT_SHOW_TEXT;
	if (do_init) init_flags |= INIT_ASSIGN;

	process_dungeon_file(player_ptr, "q_info.txt", 0, 0, 0, 0);
	floor_ptr->inside_quest = old_quest;

	GAME_TEXT tmp_str[80];
	sprintf(tmp_str, _("クエスト情報 (危険度: %d 階相当)", "Quest Information (Danger level: %d)"), (int)quest[questnum].level);
	prt(tmp_str, 5, 0);
	prt(quest[questnum].name, 7, 0);

	for (int i = 0; i < 10; i++)
	{
		c_put_str(TERM_YELLOW, quest_text[i], i + 8, 0);
	}
}

/*!
 * @brief クエスト処理のメインルーチン / Request a quest from the Lord.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void castle_quest(player_type *player_ptr)
{
	clear_bldg(4, 18);
	QUEST_IDX q_index = player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].special;

	if (!q_index)
	{
		put_str(_("今のところクエストはありません。", "I don't have a quest for you at the moment."), 8, 0);
		return;
	}

	quest_type *q_ptr;
	q_ptr = &quest[q_index];
	if (q_ptr->status == QUEST_STATUS_COMPLETED)
	{
		q_ptr->status = QUEST_STATUS_REWARDED;
		get_questinfo(player_ptr, q_index, FALSE);
		reinit_wilderness = TRUE;
		return;
	}

	if (q_ptr->status == QUEST_STATUS_FAILED)
	{
		get_questinfo(player_ptr, q_index, FALSE);
		q_ptr->status = QUEST_STATUS_FAILED_DONE;
		reinit_wilderness = TRUE;
		return;
	}

	if (q_ptr->status == QUEST_STATUS_TAKEN)
	{
		put_str(_("あなたは現在のクエストを終了させていません！", "You have not completed your current quest yet!"), 8, 0);
		put_str(_("CTRL-Qを使えばクエストの状態がチェックできます。", "Use CTRL-Q to check the status of your quest."), 9, 0);
		put_str(_("クエストを終わらせたら戻って来て下さい。", "Return when you have completed your quest."), 12, 0);
		return;
	}

	if (q_ptr->status != QUEST_STATUS_UNTAKEN) return;

	q_ptr->status = QUEST_STATUS_TAKEN;
	reinit_wilderness = TRUE;
	if (q_ptr->type != QUEST_TYPE_KILL_ANY_LEVEL)
	{
		get_questinfo(player_ptr, q_index, TRUE);
		return;
	}

	if (q_ptr->r_idx == 0)
	{
		q_ptr->r_idx = get_mon_num(player_ptr, q_ptr->level + 4 + randint1(6), 0);
	}

	monster_race *r_ptr;
	r_ptr = &r_info[q_ptr->r_idx];
	while ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->rarity != 1))
	{
		q_ptr->r_idx = get_mon_num(player_ptr, q_ptr->level + 4 + randint1(6), 0);
		r_ptr = &r_info[q_ptr->r_idx];
	}

	if (q_ptr->max_num == 0)
	{
		if (randint1(10) > 7)
			q_ptr->max_num = 1;
		else
			q_ptr->max_num = randint1(3) + 1;
	}

	q_ptr->cur_num = 0;
	concptr name = (r_name + r_ptr->name);
#ifdef JP
	msg_format("クエスト: %sを %d体倒す", name, q_ptr->max_num);
#else
	msg_format("Your quest: kill %d %s", q_ptr->max_num, name);
#endif
	get_questinfo(player_ptr, q_index, TRUE);
}


/*!
 * @brief 町に関するヘルプを表示する / Display town history
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void town_history(player_type *player_ptr)
{
	screen_save();
	(void)show_file(player_ptr, TRUE, _("jbldg.txt", "bldg.txt"), NULL, 0, 0);
	screen_load();
}


/*!
 * @brief 攻撃時スレイによるダメージ期待値修正計算 / critical happens at i / 10000
 * @param dam 基本ダメージ
 * @param mult スレイ倍率（掛け算部分）
 * @param div スレイ倍率（割り算部分）
 * @param force 理力特別計算フラグ
 * @return ダメージ期待値
 */
static HIT_POINT calc_slaydam(HIT_POINT dam, int mult, int div, bool force)
{
	int tmp;
	if (force)
	{
		tmp = dam * 60;
		tmp *= mult * 3;
		tmp /= div * 2;
		tmp += dam * 60 * 2;
		tmp /= 60;
		return tmp;
	}

	tmp = dam * 60;
	tmp *= mult;
	tmp /= div;
	tmp /= 60;
	return tmp;
}


/*!
 * @brief 攻撃時の期待値計算（スレイ→重量クリティカル→切れ味効果）
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param dam 基本ダメージ
 * @param mult スレイ倍率（掛け算部分）
 * @param div スレイ倍率（割り算部分）
 * @param force 理力特別計算フラグ
 * @param weight 重量
 * @param plus 武器ダメージ修正
 * @param meichuu 命中値
 * @param dokubari 毒針処理か否か
 * @param vorpal_mult 切れ味倍率（掛け算部分）
 * @param vorpal_div 切れ味倍率（割り算部分）
 * @return ダメージ期待値
 */
static u32b calc_expect_dice(player_type *owner_ptr, u32b dam, int mult, int div, bool force, WEIGHT weight, int plus, s16b meichuu, bool dokubari, int vorpal_mult, int vorpal_div)
{
	dam = calc_slaydam(dam, mult, div, force);
	dam = calc_expect_crit(owner_ptr, weight, plus, dam, meichuu, dokubari);
	dam = calc_slaydam(dam, vorpal_mult, vorpal_div, FALSE);
	return dam;
}


/*!
 * @brief 武器の各条件毎のダメージ期待値を表示する。
 * @param r 表示行
 * @param c 表示列
 * @param mindice ダイス部分最小値
 * @param maxdice ダイス部分最大値
 * @param blows 攻撃回数
 * @param dam_bonus ダメージ修正値
 * @param attr 条件内容
 * @param color 条件内容の表示色
 * @details
 * Display the damage figure of an object\n
 * (used by compare_weapon_aux)\n
 * \n
 * Only accurate for the current weapon, because it includes\n
 * the current +dam of the player.\n
 * @return なし
 */
static void show_weapon_dmg(int r, int c, int mindice, int maxdice, int blows, int dam_bonus, concptr attr, byte color)
{
	c_put_str(color, attr, r, c);
	GAME_TEXT tmp_str[80];
	int mindam = blows * (mindice + dam_bonus);
	int maxdam = blows * (maxdice + dam_bonus);
	sprintf(tmp_str, _("１ターン: %d-%d ダメージ", "Attack: %d-%d damage"), mindam, maxdam);
	put_str(tmp_str, r, c + 8);
}


/*!
 * @brief 武器一つ毎のダメージ情報を表示する。
 * @param o_ptr オブジェクトの構造体の参照ポインタ。
 * @param col 表示する行の上端
 * @param r 表示する列の左端
 * @details
 * Show the damage figures for the various monster types\n
 * \n
 * Only accurate for the current weapon, because it includes\n
 * the current number of blows for the player.\n
 * @return なし
 */
static void compare_weapon_aux(player_type *owner_ptr, object_type *o_ptr, int col, int r)
{
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	int blow = owner_ptr->num_blow[0];
	bool force = FALSE;
	bool dokubari = FALSE;

	int eff_dd = o_ptr->dd + owner_ptr->to_dd[0];
	int eff_ds = o_ptr->ds + owner_ptr->to_ds[0];

	int mindice = eff_dd;
	int maxdice = eff_ds * eff_dd;
	int mindam = 0;
	int maxdam = 0;
	int vorpal_mult = 1;
	int vorpal_div = 1;
	int dmg_bonus = o_ptr->to_d + owner_ptr->to_d[0];

	object_flags(o_ptr, flgs);
	if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE)) dokubari = TRUE;

	mindam = calc_expect_crit(owner_ptr, o_ptr->weight, o_ptr->to_h, mindice, owner_ptr->to_h[0], dokubari);
	maxdam = calc_expect_crit(owner_ptr, o_ptr->weight, o_ptr->to_h, maxdice, owner_ptr->to_h[0], dokubari);
	show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("会心:", "Critical:"), TERM_L_RED);
	if ((have_flag(flgs, TR_VORPAL) || hex_spelling(owner_ptr, HEX_RUNESWORD)))
	{
		if ((o_ptr->name1 == ART_VORPAL_BLADE) || (o_ptr->name1 == ART_CHAINSWORD))
		{
			vorpal_mult = 5;
			vorpal_div = 3;
		}
		else
		{
			vorpal_mult = 11;
			vorpal_div = 9;
		}

		mindam = calc_expect_dice(owner_ptr, mindice, 1, 1, FALSE, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 1, 1, FALSE, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("切れ味:", "Vorpal:"), TERM_L_RED);
	}

	if ((owner_ptr->pclass != CLASS_SAMURAI) && have_flag(flgs, TR_FORCE_WEAPON) && (owner_ptr->csp > (o_ptr->dd * o_ptr->ds / 5)))
	{
		force = TRUE;

		mindam = calc_expect_dice(owner_ptr, mindice, 1, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 1, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("理力:", "Force  :"), TERM_L_BLUE);
	}

	if (have_flag(flgs, TR_KILL_ANIMAL))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 4, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 4, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("動物:", "Animals:"), TERM_YELLOW);
	}
	else if (have_flag(flgs, TR_SLAY_ANIMAL))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("動物:", "Animals:"), TERM_YELLOW);
	}

	if (have_flag(flgs, TR_KILL_EVIL))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 7, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 7, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("邪悪:", "Evil:"), TERM_YELLOW);
	}
	else if (have_flag(flgs, TR_SLAY_EVIL))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 2, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 2, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("邪悪:", "Evil:"), TERM_YELLOW);
	}

	if (have_flag(flgs, TR_KILL_HUMAN))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 4, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 4, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("人間:", "Human:"), TERM_YELLOW);
	}
	else if (have_flag(flgs, TR_SLAY_HUMAN))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("人間:", "Human:"), TERM_YELLOW);
	}

	if (have_flag(flgs, TR_KILL_UNDEAD))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("不死:", "Undead:"), TERM_YELLOW);
	}
	else if (have_flag(flgs, TR_SLAY_UNDEAD))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("不死:", "Undead:"), TERM_YELLOW);
	}

	if (have_flag(flgs, TR_KILL_DEMON))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("悪魔:", "Demons:"), TERM_YELLOW);
	}
	else if (have_flag(flgs, TR_SLAY_DEMON))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("悪魔:", "Demons:"), TERM_YELLOW);
	}

	if (have_flag(flgs, TR_KILL_ORC))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("オーク:", "Orcs:"), TERM_YELLOW);
	}
	else if (have_flag(flgs, TR_SLAY_ORC))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("オーク:", "Orcs:"), TERM_YELLOW);
	}

	if (have_flag(flgs, TR_KILL_TROLL))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("トロル:", "Trolls:"), TERM_YELLOW);
	}
	else if (have_flag(flgs, TR_SLAY_TROLL))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("トロル:", "Trolls:"), TERM_YELLOW);
	}

	if (have_flag(flgs, TR_KILL_GIANT))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("巨人:", "Giants:"), TERM_YELLOW);
	}
	else if (have_flag(flgs, TR_SLAY_GIANT))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("巨人:", "Giants:"), TERM_YELLOW);
	}

	if (have_flag(flgs, TR_KILL_DRAGON))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("竜:", "Dragons:"), TERM_YELLOW);
	}
	else if (have_flag(flgs, TR_SLAY_DRAGON))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("竜:", "Dragons:"), TERM_YELLOW);
	}

	if (have_flag(flgs, TR_BRAND_ACID))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("酸属性:", "Acid:"), TERM_RED);
	}

	if (have_flag(flgs, TR_BRAND_ELEC))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("電属性:", "Elec:"), TERM_RED);
	}

	if (have_flag(flgs, TR_BRAND_FIRE))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("炎属性:", "Fire:"), TERM_RED);
	}

	if (have_flag(flgs, TR_BRAND_COLD))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("冷属性:", "Cold:"), TERM_RED);
	}

	if (have_flag(flgs, TR_BRAND_POIS))
	{
		mindam = calc_expect_dice(owner_ptr, mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(owner_ptr, maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("毒属性:", "Poison:"), TERM_RED);
	}
}


/*!
 * @brief 武器匠における武器一つ毎の完全情報を表示する。
 * @param player_type プレーヤーへの参照ポインタ
 * @param o_ptr オブジェクトの構造体の参照ポインタ。
 * @param row 表示する列の左端
 * @param col 表示する行の上端
 * @details
 * Displays all info about a weapon
 *
 * Only accurate for the current weapon, because it includes
 * various info about the player's +to_dam and number of blows.
 * @return なし
 */
static void list_weapon(player_type *player_ptr, object_type *o_ptr, TERM_LEN row, TERM_LEN col)
{
	GAME_TEXT o_name[MAX_NLEN];
	GAME_TEXT tmp_str[80];

	DICE_NUMBER eff_dd = o_ptr->dd + player_ptr->to_dd[0];
	DICE_SID eff_ds = o_ptr->ds + player_ptr->to_ds[0];
	HIT_RELIABILITY reli = player_ptr->skill_thn + (player_ptr->to_h[0] + o_ptr->to_h) * BTH_PLUS_ADJ;

	object_desc(player_ptr, o_name, o_ptr, OD_NAME_ONLY);
	c_put_str(TERM_YELLOW, o_name, row, col);
	sprintf(tmp_str, _("攻撃回数: %d", "Number of Blows: %d"), player_ptr->num_blow[0]);
	put_str(tmp_str, row + 1, col);

	sprintf(tmp_str, _("命中率:  0  50 100 150 200 (敵のAC)", "To Hit:  0  50 100 150 200 (AC)"));
	put_str(tmp_str, row + 2, col);
	sprintf(tmp_str, "        %2d  %2d  %2d  %2d  %2d (%%)",
		(int)hit_chance(player_ptr, reli, 0),
		(int)hit_chance(player_ptr, reli, 50),
		(int)hit_chance(player_ptr, reli, 100),
		(int)hit_chance(player_ptr, reli, 150),
		(int)hit_chance(player_ptr, reli, 200));
	put_str(tmp_str, row + 3, col);
	c_put_str(TERM_YELLOW, _("可能なダメージ:", "Possible Damage:"), row + 5, col);

	sprintf(tmp_str, _("攻撃一回につき %d-%d", "One Strike: %d-%d damage"),
		(int)(eff_dd + o_ptr->to_d + player_ptr->to_d[0]),
		(int)(eff_ds * eff_dd + o_ptr->to_d + player_ptr->to_d[0]));
	put_str(tmp_str, row + 6, col + 1);

	sprintf(tmp_str, _("１ターンにつき %d-%d", "One Attack: %d-%d damage"),
		(int)(player_ptr->num_blow[0] * (eff_dd + o_ptr->to_d + player_ptr->to_d[0])),
		(int)(player_ptr->num_blow[0] * (eff_ds * eff_dd + o_ptr->to_d + player_ptr->to_d[0])));
	put_str(tmp_str, row + 7, col + 1);
}


/*!
 * @brief 武器匠鑑定1回分（オブジェクト2種）の処理。/ Compare weapons
 * @details
 * Copies the weapons to compare into the weapon-slot and\n
 * compares the values for both weapons.\n
 * 武器1つだけで比較をしないなら費用は半額になる。
 * @param bcost 基本鑑定費用
 * @return 最終的にかかった費用
 */
static PRICE compare_weapons(player_type *customer_ptr, PRICE bcost)
{
	object_type *o_ptr[2];
	object_type orig_weapon;
	object_type *i_ptr;
	TERM_LEN row = 2;
	TERM_LEN wid = 38, mgn = 2;
	bool old_character_xtra = current_world_ptr->character_xtra;
	char ch;
	PRICE total = 0;
	PRICE cost = 0; /* First time no price */

	screen_save();
	clear_bldg(0, 22);
	i_ptr = &customer_ptr->inventory_list[INVEN_RARM];
	object_copy(&orig_weapon, i_ptr);

	item_tester_hook = item_tester_hook_orthodox_melee_weapons;
	concptr q = _("第一の武器は？", "What is your first weapon? ");
	concptr s = _("比べるものがありません。", "You have nothing to compare.");

	OBJECT_IDX item;
	o_ptr[0] = choose_object(customer_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | IGNORE_BOTHHAND_SLOT), 0);
	if (!o_ptr[0])
	{
		screen_load();
		return 0;
	}

	int n = 1;
	total = bcost;

	while (TRUE)
	{
		clear_bldg(0, 22);
		item_tester_hook = item_tester_hook_orthodox_melee_weapons;
		current_world_ptr->character_xtra = TRUE;
		for (int i = 0; i < n; i++)
		{
			int col = (wid * i + mgn);
			if (o_ptr[i] != i_ptr) object_copy(i_ptr, o_ptr[i]);

			customer_ptr->update |= PU_BONUS;
			handle_stuff(customer_ptr);

			list_weapon(customer_ptr, o_ptr[i], row, col);
			compare_weapon_aux(customer_ptr, o_ptr[i], col, row + 8);
			object_copy(i_ptr, &orig_weapon);
		}

		customer_ptr->update |= PU_BONUS;
		handle_stuff(customer_ptr);

		current_world_ptr->character_xtra = old_character_xtra;
#ifdef JP
		put_str(format("[ 比較対象: 's'で変更 ($%d) ]", cost), 1, (wid + mgn));
		put_str("(一番高いダメージが適用されます。複数の倍打効果は足し算されません。)", row + 4, 0);
		prt("現在の技量から判断すると、あなたの武器は以下のような威力を発揮します:", 0, 0);
#else
		put_str(format("[ 's' Select secondary weapon($%d) ]", cost), 1, (wid + mgn));
		put_str("(Only highest damage applies per monster. Special damage not cumulative.)", row + 4, 0);
		prt("Based on your current abilities, here is what your weapons will do", 0, 0);
#endif

		flush();
		ch = inkey();
		if (ch != 's') break;

		if (total + cost > customer_ptr->au)
		{
			msg_print(_("お金が足りません！", "You don't have enough money!"));
			msg_print(NULL);
			continue;
		}

		q = _("第二の武器は？", "What is your second weapon? ");
		s = _("比べるものがありません。", "You have nothing to compare.");
		OBJECT_IDX item2;
		o_ptr[1] = choose_object(customer_ptr, &item2, q, s, (USE_EQUIP | USE_INVEN | IGNORE_BOTHHAND_SLOT), 0);
		if (!o_ptr[1]) continue;

		total += cost;
		cost = bcost / 2;
		n = 2;
	}

	screen_load();
	return total;
}


/*!
 * @brief ACから回避率、ダメージ減少率を計算し表示する。 / Evaluate AC
 * @details
 * Calculate and display the dodge-rate and the protection-rate
 * based on AC
 * @param iAC プレイヤーのAC。
 * @return 常にTRUEを返す。
 */
static bool eval_ac(ARMOUR_CLASS iAC)
{
#ifdef JP
	const char memo[] =
		"ダメージ軽減率とは、敵の攻撃が当たった時そのダメージを\n"
		"何パーセント軽減するかを示します。\n"
		"ダメージ軽減は通常の直接攻撃(種類が「攻撃する」と「粉砕する」の物)\n"
		"に対してのみ効果があります。\n \n"
		"敵のレベルとは、その敵が通常何階に現れるかを示します。\n \n"
		"回避率は敵の直接攻撃を何パーセントの確率で避けるかを示し、\n"
		"敵のレベルとあなたのACによって決定されます。\n \n"
		"ダメージ期待値とは、敵の１００ポイントの通常攻撃に対し、\n"
		"回避率とダメージ軽減率を考慮したダメージの期待値を示します。\n";
#else
	const char memo[] =
		"'Protection Rate' means how much damage is reduced by your armor.\n"
		"Note that the Protection rate is effective only against normal "
		"'attack' and 'shatter' type melee attacks, "
		"and has no effect against any other types such as 'poison'.\n \n"
		"'Dodge Rate' indicates the success rate on dodging the "
		"monster's melee attacks.  "
		"It is depend on the level of the monster and your AC.\n \n"
		"'Average Damage' indicates the expected amount of damage "
		"when you are attacked by normal melee attacks with power=100.";
#endif

	int protection;
	TERM_LEN col, row = 2;
	DEPTH lvl;
	char buf[80 * 20], *t;

	if (iAC < 0) iAC = 0;

	protection = 100 * MIN(iAC, 150) / 250;
	screen_save();
	clear_bldg(0, 22);

	put_str(format(_("あなたの現在のAC: %3d", "Your current AC : %3d"), iAC), row++, 0);
	put_str(format(_("ダメージ軽減率  : %3d%%", "Protection rate : %3d%%"), protection), row++, 0);
	row++;

	put_str(_("敵のレベル      :", "Level of Monster:"), row + 0, 0);
	put_str(_("回避率          :", "Dodge Rate      :"), row + 1, 0);
	put_str(_("ダメージ期待値  :", "Average Damage  :"), row + 2, 0);

	for (col = 17 + 1, lvl = 0; lvl <= 100; lvl += 10, col += 5)
	{
		int quality = 60 + lvl * 3; /* attack quality with power 60 */
		int dodge;   /* 回避率(%) */
		int average; /* ダメージ期待値 */

		put_str(format("%3d", lvl), row + 0, col);

		/* 回避率を計算 */
		dodge = 5 + (MIN(100, 100 * (iAC * 3 / 4) / quality) * 9 + 5) / 10;
		put_str(format("%3d%%", dodge), row + 1, col);

		/* 100点の攻撃に対してのダメージ期待値を計算 */
		average = (100 - dodge) * (100 - protection) / 100;
		put_str(format("%3d", average), row + 2, col);
	}

	roff_to_buf(memo, 70, buf, sizeof(buf));
	for (t = buf; t[0]; t += strlen(t) + 1)
		put_str(t, (row++) + 4, 4);

	prt(_("現在のあなたの装備からすると、あなたの防御力はこれくらいです:", "Defense abilities from your current Armor Class are evaluated below."), 0, 0);

	flush();
	(void)inkey();
	screen_load();

	return TRUE;
}


/*!
 * @brief 修復材料のオブジェクトから修復対象に特性を移植する。
 * @param to_ptr 修復対象オブジェクトの構造体の参照ポインタ。
 * @param from_ptr 修復材料オブジェクトの構造体の参照ポインタ。
 * @return 修復対象になるならTRUEを返す。
 */
static void give_one_ability_of_object(object_type *to_ptr, object_type *from_ptr)
{
	BIT_FLAGS to_flgs[TR_FLAG_SIZE];
	BIT_FLAGS from_flgs[TR_FLAG_SIZE];
	object_flags(to_ptr, to_flgs);
	object_flags(from_ptr, from_flgs);

	int n = 0;
	int cand[TR_FLAG_MAX];
	for (int i = 0; i < TR_FLAG_MAX; i++)
	{
		switch (i)
		{
		case TR_IGNORE_ACID:
		case TR_IGNORE_ELEC:
		case TR_IGNORE_FIRE:
		case TR_IGNORE_COLD:
		case TR_ACTIVATE:
		case TR_RIDING:
		case TR_THROW:
		case TR_SHOW_MODS:
		case TR_HIDE_TYPE:
		case TR_ES_ATTACK:
		case TR_ES_AC:
		case TR_FULL_NAME:
		case TR_FIXED_FLAVOR:
			break;
		default:
			if (have_flag(from_flgs, i) && !have_flag(to_flgs, i))
			{
				if (!(is_pval_flag(i) && (from_ptr->pval < 1))) cand[n++] = i;
			}
		}
	}

	if (n <= 0) return;

	int tr_idx = cand[randint0(n)];
	add_flag(to_ptr->art_flags, tr_idx);
	if (is_pval_flag(tr_idx)) to_ptr->pval = MAX(to_ptr->pval, 1);
	int bmax = MIN(3, MAX(1, 40 / (to_ptr->dd * to_ptr->ds)));
	if (tr_idx == TR_BLOWS) to_ptr->pval = MIN(to_ptr->pval, bmax);
	if (tr_idx == TR_SPEED) to_ptr->pval = MIN(to_ptr->pval, 4);
}


/*!
 * @brief アイテム修復処理のメインルーチン / Repair broken weapon
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param bcost 基本修復費用
 * @return 実際にかかった費用
 */
static PRICE repair_broken_weapon_aux(player_type *player_ptr, PRICE bcost)
{
	clear_bldg(0, 22);
	int row = 7;
	prt(_("修復には材料となるもう1つの武器が必要です。", "Hand one material weapon to repair a broken weapon."), row, 2);
	prt(_("材料に使用した武器はなくなります！", "The material weapon will disappear after repairing!!"), row + 1, 2);

	concptr q = _("どの折れた武器を修復しますか？", "Repair which broken weapon? ");
	concptr s = _("修復できる折れた武器がありません。", "You have no broken weapon to repair.");
	item_tester_hook = item_tester_hook_broken_weapon;

	OBJECT_IDX item;
	object_type *o_ptr;
	o_ptr = choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_EQUIP), 0);
	if (!o_ptr) return 0;

	if (!object_is_ego(o_ptr) && !object_is_artifact(o_ptr))
	{
		msg_format(_("それは直してもしょうがないぜ。", "It is worthless to repair."));
		return 0;
	}

	if (o_ptr->number > 1)
	{
		msg_format(_("一度に複数を修復することはできません！", "They are too many to repair at once!"));
		return 0;
	}

	char basenm[MAX_NLEN];
	object_desc(player_ptr, basenm, o_ptr, OD_NAME_ONLY);
	prt(format(_("修復する武器　： %s", "Repairing: %s"), basenm), row + 3, 2);

	q = _("材料となる武器は？", "Which weapon for material? ");
	s = _("材料となる武器がありません。", "You have no material to repair.");

	item_tester_hook = item_tester_hook_orthodox_melee_weapons;
	OBJECT_IDX mater;
	object_type *mo_ptr;
	mo_ptr = choose_object(player_ptr, &mater, q, s, (USE_INVEN | USE_EQUIP), 0);
	if (!mo_ptr) return 0;
	if (mater == item)
	{
		msg_print(_("クラインの壷じゃない！", "This is not a klein bottle!"));
		return 0;
	}

	object_desc(player_ptr, basenm, mo_ptr, OD_NAME_ONLY);
	prt(format(_("材料とする武器： %s", "Material : %s"), basenm), row + 4, 2);
	PRICE cost = bcost + object_value_real(o_ptr) * 2;
	if (!get_check(format(_("＄%dかかりますがよろしいですか？ ", "Costs %d gold, okay? "), cost))) return 0;

	if (player_ptr->au < cost)
	{
		object_desc(player_ptr, basenm, o_ptr, OD_NAME_ONLY);
		msg_format(_("%sを修復するだけのゴールドがありません！", "You do not have the gold to repair %s!"), basenm);
		msg_print(NULL);
		return 0;
	}

	player_ptr->total_weight -= o_ptr->weight;
	KIND_OBJECT_IDX k_idx;
	if (o_ptr->sval == SV_BROKEN_DAGGER)
	{
		int n = 1;
		k_idx = 0;
		for (KIND_OBJECT_IDX j = 1; j < max_k_idx; j++)
		{
			object_kind *k_aux_ptr = &k_info[j];

			if (k_aux_ptr->tval != TV_SWORD) continue;
			if ((k_aux_ptr->sval == SV_BROKEN_DAGGER) ||
				(k_aux_ptr->sval == SV_BROKEN_SWORD) ||
				(k_aux_ptr->sval == SV_POISON_NEEDLE)) continue;
			if (k_aux_ptr->weight > 99) continue;

			if (one_in_(n))
			{
				k_idx = j;
				n++;
			}
		}
	}
	else
	{
		OBJECT_TYPE_VALUE tval = (one_in_(5) ? mo_ptr->tval : TV_SWORD);
		while (TRUE)
		{
			object_kind *ck_ptr;
			k_idx = lookup_kind(tval, SV_ANY);
			ck_ptr = &k_info[k_idx];

			if (tval == TV_SWORD)
			{
				if ((ck_ptr->sval == SV_BROKEN_DAGGER) ||
					(ck_ptr->sval == SV_BROKEN_SWORD) ||
					(ck_ptr->sval == SV_DIAMOND_EDGE) ||
					(ck_ptr->sval == SV_POISON_NEEDLE)) continue;
			}
			if (tval == TV_POLEARM)
			{
				if ((ck_ptr->sval == SV_DEATH_SCYTHE) ||
					(ck_ptr->sval == SV_TSURIZAO)) continue;
			}
			if (tval == TV_HAFTED)
			{
				if ((ck_ptr->sval == SV_GROND) ||
					(ck_ptr->sval == SV_WIZSTAFF) ||
					(ck_ptr->sval == SV_NAMAKE_HAMMER)) continue;
			}

			break;
		}
	}

	int dd_bonus = o_ptr->dd - k_info[o_ptr->k_idx].dd;
	int ds_bonus = o_ptr->ds - k_info[o_ptr->k_idx].ds;
	dd_bonus += mo_ptr->dd - k_info[mo_ptr->k_idx].dd;
	ds_bonus += mo_ptr->ds - k_info[mo_ptr->k_idx].ds;

	object_kind *k_ptr;
	k_ptr = &k_info[k_idx];
	o_ptr->k_idx = k_idx;
	o_ptr->weight = k_ptr->weight;
	o_ptr->tval = k_ptr->tval;
	o_ptr->sval = k_ptr->sval;
	o_ptr->dd = k_ptr->dd;
	o_ptr->ds = k_ptr->ds;

	for (int i = 0; i < TR_FLAG_SIZE; i++) o_ptr->art_flags[i] |= k_ptr->flags[i];
	if (k_ptr->pval) o_ptr->pval = MAX(o_ptr->pval, randint1(k_ptr->pval));
	if (have_flag(k_ptr->flags, TR_ACTIVATE)) o_ptr->xtra2 = (byte)k_ptr->act_idx;

	if (dd_bonus > 0)
	{
		o_ptr->dd++;
		for (int i = 1; i < dd_bonus; i++)
		{
			if (one_in_(o_ptr->dd + i)) o_ptr->dd++;
		}
	}

	if (ds_bonus > 0)
	{
		o_ptr->ds++;
		for (int i = 1; i < ds_bonus; i++)
		{
			if (one_in_(o_ptr->ds + i)) o_ptr->ds++;
		}
	}

	if (have_flag(k_ptr->flags, TR_BLOWS))
	{
		int bmax = MIN(3, MAX(1, 40 / (o_ptr->dd * o_ptr->ds)));
		o_ptr->pval = MIN(o_ptr->pval, bmax);
	}

	give_one_ability_of_object(o_ptr, mo_ptr);
	o_ptr->to_d += MAX(0, (mo_ptr->to_d / 3));
	o_ptr->to_h += MAX(0, (mo_ptr->to_h / 3));
	o_ptr->to_a += MAX(0, (mo_ptr->to_a));

	if ((o_ptr->name1 == ART_NARSIL) ||
		(object_is_random_artifact(o_ptr) && one_in_(1)) ||
		(object_is_ego(o_ptr) && one_in_(7)))
	{
		if (object_is_ego(o_ptr))
		{
			add_flag(o_ptr->art_flags, TR_IGNORE_FIRE);
			add_flag(o_ptr->art_flags, TR_IGNORE_ACID);
		}

		give_one_ability_of_object(o_ptr, mo_ptr);
		if (!activation_index(o_ptr)) one_activation(o_ptr);

		if (o_ptr->name1 == ART_NARSIL)
		{
			one_high_resistance(o_ptr);
			one_ability(o_ptr);
		}

		msg_print(_("これはかなりの業物だったようだ。", "This blade seems to be exceptional."));
	}

	object_desc(player_ptr, basenm, o_ptr, OD_NAME_ONLY);
#ifdef JP
	msg_format("＄%dで%sに修復しました。", cost, basenm);
#else
	msg_format("Repaired into %s for %d gold.", basenm, cost);
#endif
	msg_print(NULL);
	o_ptr->ident &= ~(IDENT_BROKEN);
	o_ptr->discount = 99;

	player_ptr->total_weight += o_ptr->weight;
	calc_android_exp(player_ptr);
	inven_item_increase(player_ptr, mater, -1);
	inven_item_optimize(player_ptr, mater);

	player_ptr->update |= PU_BONUS;
	handle_stuff(player_ptr);
	return (cost);
}


/*!
 * @brief アイテム修復処理の過渡ルーチン / Repair broken weapon
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param bcost 基本鑑定費用
 * @return 実際にかかった費用
 */
static int repair_broken_weapon(player_type *player_ptr, PRICE bcost)
{
	PRICE cost;
	screen_save();
	cost = repair_broken_weapon_aux(player_ptr, bcost);
	screen_load();
	return cost;
}


/*!
 * @brief アイテムの強化を行う。 / Enchant item
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param cost 1回毎の費用
 * @param to_hit 命中をアップさせる量
 * @param to_dam ダメージをアップさせる量
 * @param to_ac ＡＣをアップさせる量
 * @return 実際に行ったらTRUE
 */
static bool enchant_item(player_type *player_ptr, PRICE cost, HIT_PROB to_hit, HIT_POINT to_dam, ARMOUR_CLASS to_ac, OBJECT_TYPE_VALUE item_tester_tval)
{
	clear_bldg(4, 18);
	int maxenchant = (player_ptr->lev / 5);
	prt(format(_("現在のあなたの技量だと、+%d まで改良できます。", "  Based on your skill, we can improve up to +%d."), maxenchant), 5, 0);
	prt(format(_(" 改良の料金は一個につき＄%d です。", "  The price for the service is %d gold per item."), cost), 7, 0);

	concptr q = _("どのアイテムを改良しますか？", "Improve which item? ");
	concptr s = _("改良できるものがありません。", "You have nothing to improve.");

	OBJECT_IDX item;
	object_type *o_ptr;
	o_ptr = choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_EQUIP | IGNORE_BOTHHAND_SLOT), item_tester_tval);
	if (!o_ptr) return FALSE;

	char tmp_str[MAX_NLEN];
	if (player_ptr->au < (cost * o_ptr->number))
	{
		object_desc(player_ptr, tmp_str, o_ptr, OD_NAME_ONLY);
		msg_format(_("%sを改良するだけのゴールドがありません！", "You do not have the gold to improve %s!"), tmp_str);
		return FALSE;
	}

	bool okay = FALSE;
	for (int i = 0; i < to_hit; i++)
	{
		if ((o_ptr->to_h < maxenchant) && enchant(player_ptr, o_ptr, 1, (ENCH_TOHIT | ENCH_FORCE)))
		{
			okay = TRUE;
			break;
		}
	}

	for (int i = 0; i < to_dam; i++)
	{
		if ((o_ptr->to_d < maxenchant) && enchant(player_ptr, o_ptr, 1, (ENCH_TODAM | ENCH_FORCE)))
		{
			okay = TRUE;
			break;
		}
	}

	for (int i = 0; i < to_ac; i++)
	{
		if ((o_ptr->to_a < maxenchant) && enchant(player_ptr, o_ptr, 1, (ENCH_TOAC | ENCH_FORCE)))
		{
			okay = TRUE;
			break;
		}
	}

	if (!okay)
	{
		if (flush_failure) flush();
		msg_print(_("改良に失敗した。", "The improvement failed."));
		return FALSE;
	}

	object_desc(player_ptr, tmp_str, o_ptr, OD_NAME_AND_ENCHANT);
#ifdef JP
	msg_format("＄%dで%sに改良しました。", cost * o_ptr->number, tmp_str);
#else
	msg_format("Improved into %s for %d gold.", tmp_str, cost * o_ptr->number);
#endif

	player_ptr->au -= (cost * o_ptr->number);
	if (item >= INVEN_RARM) calc_android_exp(player_ptr);
	return TRUE;
}


/*!
 * @brief 魔道具の使用回数を回復させる施設のメインルーチン / Recharge rods, wands and staffs
 * @details
 * The player can select the number of charges to add\n
 * (up to a limit), and the recharge never fails.\n
 *\n
 * The cost for rods depends on the level of the rod. The prices\n
 * for recharging wands and staffs are dependent on the cost of\n
 * the base-item.\n
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void building_recharge(player_type *player_ptr)
{
	msg_flag = FALSE;
	clear_bldg(4, 18);
	prt(_("  再充填の費用はアイテムの種類によります。", "  The prices of recharge depend on the type."), 6, 0);
	item_tester_hook = item_tester_hook_recharge;

	concptr q = _("どのアイテムに魔力を充填しますか? ", "Recharge which item? ");
	concptr s = _("魔力を充填すべきアイテムがない。", "You have nothing to recharge.");

	OBJECT_IDX item;
	object_type *o_ptr;
	o_ptr = choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), 0);
	if (!o_ptr) return;

	object_kind *k_ptr;
	k_ptr = &k_info[o_ptr->k_idx];

	/*
	 * We don't want to give the player free info about
	 * the level of the item or the number of charges.
	 */
	 /* The item must be "known" */
	char tmp_str[MAX_NLEN];
	if (!object_is_known(o_ptr))
	{
		msg_format(_("充填する前に鑑定されている必要があります！", "The item must be identified first!"));
		msg_print(NULL);

		if ((player_ptr->au >= 50) &&
			get_check(_("＄50で鑑定しますか？ ", "Identify for 50 gold? ")))

		{
			player_ptr->au -= 50;
			identify_item(player_ptr, o_ptr);
			object_desc(player_ptr, tmp_str, o_ptr, 0);
			msg_format(_("%s です。", "You have: %s."), tmp_str);

			autopick_alter_item(player_ptr, item, FALSE);
			building_prt_gold(player_ptr);
		}

		return;
	}

	DEPTH lev = k_info[o_ptr->k_idx].level;
	PRICE price;
	if (o_ptr->tval == TV_ROD)
	{
		if (o_ptr->timeout > 0)
		{
			price = (lev * 50 * o_ptr->timeout) / k_ptr->pval;
		}
		else
		{
			price = 0;
			msg_format(_("それは再充填する必要はありません。", "That doesn't need to be recharged."));
			return;
		}
	}
	else if (o_ptr->tval == TV_STAFF)
	{
		price = (k_info[o_ptr->k_idx].cost / 10) * o_ptr->number;
		price = MAX(10, price);
	}
	else
	{
		price = (k_info[o_ptr->k_idx].cost / 10);
		price = MAX(10, price);
	}

	if (o_ptr->tval == TV_WAND
		&& (o_ptr->pval / o_ptr->number >= k_ptr->pval))
	{
		if (o_ptr->number > 1)
		{
			msg_print(_("この魔法棒はもう充分に充填されています。", "These wands are already fully charged."));
		}
		else
		{
			msg_print(_("この魔法棒はもう充分に充填されています。", "This wand is already fully charged."));
		}

		return;
	}
	else if (o_ptr->tval == TV_STAFF && o_ptr->pval >= k_ptr->pval)
	{
		if (o_ptr->number > 1)
		{
			msg_print(_("この杖はもう充分に充填されています。", "These staffs are already fully charged."));
		}
		else
		{
			msg_print(_("この杖はもう充分に充填されています。", "This staff is already fully charged."));
		}

		return;
	}

	if (player_ptr->au < price)
	{
		object_desc(player_ptr, tmp_str, o_ptr, OD_NAME_ONLY);
#ifdef JP
		msg_format("%sを再充填するには＄%d 必要です！", tmp_str, price);
#else
		msg_format("You need %d gold to recharge %s!", price, tmp_str);
#endif
		return;
	}

	PARAMETER_VALUE charges;
	if (o_ptr->tval == TV_ROD)
	{
#ifdef JP
		if (get_check(format("そのロッドを＄%d で再充填しますか？", price)))
#else
		if (get_check(format("Recharge the %s for %d gold? ",
			((o_ptr->number > 1) ? "rods" : "rod"), price)))
#endif

		{
			o_ptr->timeout = 0;
		}
		else
		{
			return;
		}
	}
	else
	{
		int max_charges;
		if (o_ptr->tval == TV_STAFF)
			max_charges = k_ptr->pval - o_ptr->pval;
		else
			max_charges = o_ptr->number * k_ptr->pval - o_ptr->pval;

		charges = (PARAMETER_VALUE)get_quantity(format(_("一回分＄%d で何回分充填しますか？", "Add how many charges for %d gold? "), price),
			MIN(player_ptr->au / price, max_charges));

		if (charges < 1) return;

		price *= charges;
		o_ptr->pval += charges;
		o_ptr->ident &= ~(IDENT_EMPTY);
	}

	object_desc(player_ptr, tmp_str, o_ptr, 0);
#ifdef JP
	msg_format("%sを＄%d で再充填しました。", tmp_str, price);
#else
	msg_format("%^s %s recharged for %d gold.", tmp_str, ((o_ptr->number > 1) ? "were" : "was"), price);
#endif
	player_ptr->update |= (PU_COMBINE | PU_REORDER);
	player_ptr->window |= (PW_INVEN);
	player_ptr->au -= price;
}


/*!
 * @brief 魔道具の使用回数を回復させる施設の一括処理向けサブルーチン / Recharge rods, wands and staffs
 * @details
 * The player can select the number of charges to add\n
 * (up to a limit), and the recharge never fails.\n
 *\n
 * The cost for rods depends on the level of the rod. The prices\n
 * for recharging wands and staffs are dependent on the cost of\n
 * the base-item.\n
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void building_recharge_all(player_type *player_ptr)
{
	msg_flag = FALSE;
	clear_bldg(4, 18);
	prt(_("  再充填の費用はアイテムの種類によります。", "  The prices of recharge depend on the type."), 6, 0);

	PRICE price = 0;
	PRICE total_cost = 0;
	for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++)
	{
		object_type *o_ptr;
		o_ptr = &player_ptr->inventory_list[i];

		if (o_ptr->tval < TV_STAFF || o_ptr->tval > TV_ROD) continue;
		if (!object_is_known(o_ptr)) total_cost += 50;

		DEPTH lev = k_info[o_ptr->k_idx].level;
		object_kind *k_ptr;
		k_ptr = &k_info[o_ptr->k_idx];

		switch (o_ptr->tval)
		{
		case TV_ROD:
			price = (lev * 50 * o_ptr->timeout) / k_ptr->pval;
			break;

		case TV_STAFF:
			price = (k_info[o_ptr->k_idx].cost / 10) * o_ptr->number;
			price = MAX(10, price);
			price = (k_ptr->pval - o_ptr->pval) * price;
			break;

		case TV_WAND:
			price = (k_info[o_ptr->k_idx].cost / 10);
			price = MAX(10, price);
			price = (o_ptr->number * k_ptr->pval - o_ptr->pval) * price;
			break;
		}

		if (price > 0) total_cost += price;
	}

	if (!total_cost)
	{
		msg_print(_("充填する必要はありません。", "No need to recharge."));
		msg_print(NULL);
		return;
	}

	if (player_ptr->au < total_cost)
	{
		msg_format(_("すべてのアイテムを再充填するには＄%d 必要です！", "You need %d gold to recharge all items!"), total_cost);
		msg_print(NULL);
		return;
	}

	if (!get_check(format(_("すべてのアイテムを ＄%d で再充填しますか？", "Recharge all items for %d gold? "), total_cost))) return;

	for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++)
	{
		object_type *o_ptr;
		o_ptr = &player_ptr->inventory_list[i];
		object_kind *k_ptr;
		k_ptr = &k_info[o_ptr->k_idx];

		if (o_ptr->tval < TV_STAFF || o_ptr->tval > TV_ROD) continue;

		if (!object_is_known(o_ptr))
		{
			identify_item(player_ptr, o_ptr);
			autopick_alter_item(player_ptr, i, FALSE);
		}

		switch (o_ptr->tval)
		{
		case TV_ROD:
			o_ptr->timeout = 0;
			break;
		case TV_STAFF:
			if (o_ptr->pval < k_ptr->pval) o_ptr->pval = k_ptr->pval;

			o_ptr->ident &= ~(IDENT_EMPTY);
			break;
		case TV_WAND:
			if (o_ptr->pval < o_ptr->number * k_ptr->pval)
				o_ptr->pval = o_ptr->number * k_ptr->pval;

			o_ptr->ident &= ~(IDENT_EMPTY);
			break;
		}
	}

	msg_format(_("＄%d で再充填しました。", "You pay %d gold."), total_cost);
	msg_print(NULL);
	player_ptr->update |= (PU_COMBINE | PU_REORDER);
	player_ptr->window |= (PW_INVEN);
	player_ptr->au -= total_cost;
}


/*!
 * @brief 施設でモンスターの情報を知るメインルーチン / research_mon -KMW-
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 常にTRUEを返す。
 * @todo 返り値が意味不明なので直した方が良いかもしれない。
 */
static bool research_mon(player_type *player_ptr)
{
	char buf[128];
	bool notpicked;
	bool recall = FALSE;
	u16b why = 0;
	MONSTER_IDX *who;

	bool all = FALSE;
	bool uniq = FALSE;
	bool norm = FALSE;
	char temp[80] = "";

	static int old_sym = '\0';
	static IDX old_i = 0;
	screen_save();

	char sym;
	if (!get_com(_("モンスターの文字を入力して下さい(記号 or ^A全,^Uユ,^N非ユ,^M名前):",
		"Enter character to be identified(^A:All,^U:Uniqs,^N:Non uniqs,^M:Name): "), &sym, FALSE))

	{
		screen_load();
		return FALSE;
	}

	IDX i;
	for (i = 0; ident_info[i]; ++i)
	{
		if (sym == ident_info[i][0]) break;
	}

	/* XTRA HACK WHATSEARCH */
	if (sym == KTRL('A'))
	{
		all = TRUE;
		strcpy(buf, _("全モンスターのリスト", "Full monster list."));
	}
	else if (sym == KTRL('U'))
	{
		all = uniq = TRUE;
		strcpy(buf, _("ユニーク・モンスターのリスト", "Unique monster list."));
	}
	else if (sym == KTRL('N'))
	{
		all = norm = TRUE;
		strcpy(buf, _("ユニーク外モンスターのリスト", "Non-unique monster list."));
	}
	else if (sym == KTRL('M'))
	{
		all = TRUE;
		if (!get_string(_("名前(英語の場合小文字で可)", "Enter name:"), temp, 70))
		{
			temp[0] = 0;
			screen_load();

			return FALSE;
		}

		sprintf(buf, _("名前:%sにマッチ", "Monsters with a name \"%s\""), temp);
	}
	else if (ident_info[i])
	{
		sprintf(buf, "%c - %s.", sym, ident_info[i] + 2);
	}
	else
	{
		sprintf(buf, "%c - %s", sym, _("無効な文字", "Unknown Symbol"));
	}

	/* Display the result */
	prt(buf, 16, 10);

	/* Allocate the "who" array */
	C_MAKE(who, max_r_idx, MONRACE_IDX);

	/* Collect matching monsters */
	int n = 0;
	for (i = 1; i < max_r_idx; i++)
	{
		monster_race *r_ptr = &r_info[i];

		/* Empty monster */
		if (!r_ptr->name) continue;

		/* XTRA HACK WHATSEARCH */
		/* Require non-unique monsters if needed */
		if (norm && (r_ptr->flags1 & (RF1_UNIQUE))) continue;

		/* Require unique monsters if needed */
		if (uniq && !(r_ptr->flags1 & (RF1_UNIQUE))) continue;

		/* 名前検索 */
		if (temp[0])
		{
			for (int xx = 0; temp[xx] && xx < 80; xx++)
			{
#ifdef JP
				if (iskanji(temp[xx]))
				{
					xx++;
					continue;
				}
#endif
				if (isupper(temp[xx])) temp[xx] = (char)tolower(temp[xx]);
			}

			char temp2[80];
#ifdef JP
			strcpy(temp2, r_name + r_ptr->E_name);
#else
			strcpy(temp2, r_name + r_ptr->name);
#endif
			for (int xx = 0; temp2[xx] && xx < 80; xx++)
			{
				if (isupper(temp2[xx])) temp2[xx] = (char)tolower(temp2[xx]);
			}

#ifdef JP
			if (my_strstr(temp2, temp) || my_strstr(r_name + r_ptr->name, temp))
#else
			if (my_strstr(temp2, temp))
#endif
				who[n++] = i;
		}
		else if (all || (r_ptr->d_char == sym))
		{
			who[n++] = i;
		}
	}

	if (n == 0)
	{
		C_KILL(who, max_r_idx, MONRACE_IDX);
		screen_load();

		return FALSE;
	}

	why = 2;
	char query = 'y';

	if (why)
	{
		ang_sort(who, &why, n, ang_sort_comp_hook, ang_sort_swap_hook);
	}

	if (old_sym == sym && old_i < n) i = old_i;
	else i = n - 1;

	notpicked = TRUE;
	MONRACE_IDX r_idx;
	while (notpicked)
	{
		r_idx = who[i];
		roff_top(r_idx);
		Term_addstr(-1, TERM_WHITE, _(" ['r'思い出, ' 'で続行, ESC]", " [(r)ecall, ESC, space to continue]"));
		while (TRUE)
		{
			if (recall)
			{
				lore_do_probe(player_ptr, r_idx);
				monster_race_track(player_ptr, r_idx);
				handle_stuff(player_ptr);
				screen_roff(player_ptr, r_idx, 0x01);
				notpicked = FALSE;
				old_sym = sym;
				old_i = i;
			}

			query = inkey();
			if (query != 'r') break;

			recall = !recall;
		}

		if (query == ESCAPE) break;

		if (query == '-')
		{
			if (++i == n)
			{
				i = 0;
				if (!expand_list) break;
			}

			continue;
		}

		if (i-- == 0)
		{
			i = n - 1;
			if (!expand_list) break;
		}
	}

	C_KILL(who, max_r_idx, MONRACE_IDX);
	screen_load();
	return !notpicked;
}


/*!
 * @brief 施設の処理実行メインルーチン / Execute a building command
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param bldg 施設構造体の参照ポインタ
 * @param i 実行したい施設のサービステーブルの添字
 * @return なし
 */
static void bldg_process_command(player_type *player_ptr, building_type *bldg, int i)
{
	msg_flag = FALSE;
	msg_erase();

	PRICE bcost;
	if (is_owner(player_ptr, bldg))
		bcost = bldg->member_costs[i];
	else
		bcost = bldg->other_costs[i];

	/* action restrictions */
	if (((bldg->action_restr[i] == 1) && !is_member(player_ptr, bldg)) ||
		((bldg->action_restr[i] == 2) && !is_owner(player_ptr, bldg)))
	{
		msg_print(_("それを選択する権利はありません！", "You have no right to choose that!"));
		return;
	}

	BACT_IDX bact = bldg->actions[i];
	if ((bact != BACT_RECHARGE) &&
		(((bldg->member_costs[i] > player_ptr->au) && is_owner(player_ptr, bldg)) ||
		((bldg->other_costs[i] > player_ptr->au) && !is_owner(player_ptr, bldg))))
	{
		msg_print(_("お金が足りません！", "You do not have the gold!"));
		return;
	}

	bool paid = FALSE;
	switch (bact)
	{
	case BACT_NOTHING:
		/* Do nothing */
		break;
	case BACT_RESEARCH_ITEM:
		paid = identify_fully(player_ptr, FALSE, 0);
		break;
	case BACT_TOWN_HISTORY:
		town_history(player_ptr);
		break;
	case BACT_RACE_LEGENDS:
		race_legends(player_ptr);
		break;
	case BACT_QUEST:
		castle_quest(player_ptr);
		break;
	case BACT_KING_LEGENDS:
	case BACT_ARENA_LEGENDS:
	case BACT_LEGENDS:
		show_highclass(player_ptr);
		break;
	case BACT_POSTER:
	case BACT_ARENA_RULES:
	case BACT_ARENA:
		arena_comm(player_ptr, bact);
		break;
	case BACT_IN_BETWEEN:
	case BACT_CRAPS:
	case BACT_SPIN_WHEEL:
	case BACT_DICE_SLOTS:
	case BACT_GAMBLE_RULES:
	case BACT_POKER:
		gamble_comm(player_ptr, bact);
		break;
	case BACT_REST:
	case BACT_RUMORS:
	case BACT_FOOD:
		paid = inn_comm(player_ptr, bact);
		break;
	case BACT_RESEARCH_MONSTER:
		paid = research_mon(player_ptr);
		break;
	case BACT_COMPARE_WEAPONS:
		paid = TRUE;
		bcost = compare_weapons(player_ptr, bcost);
		break;
	case BACT_ENCHANT_WEAPON:
		item_tester_hook = object_allow_enchant_melee_weapon;
		enchant_item(player_ptr, bcost, 1, 1, 0, 0);
		break;
	case BACT_ENCHANT_ARMOR:
		item_tester_hook = object_is_armour;
		enchant_item(player_ptr, bcost, 0, 0, 1, 0);
		break;
	case BACT_RECHARGE:
		building_recharge(player_ptr);
		break;
	case BACT_RECHARGE_ALL:
		building_recharge_all(player_ptr);
		break;
	case BACT_IDENTS:
		if (!get_check(_("持ち物を全て鑑定してよろしいですか？", "Do you pay for identify all your possession? "))) break;
		identify_pack(player_ptr);
		msg_print(_(" 持ち物全てが鑑定されました。", "Your possessions have been identified."));
		paid = TRUE;
		break;
	case BACT_IDENT_ONE:
		paid = ident_spell(player_ptr, FALSE, 0);
		break;
	case BACT_LEARN:
		do_cmd_study(player_ptr);
		break;
	case BACT_HEALING:
		paid = cure_critical_wounds(player_ptr, 200);
		break;
	case BACT_RESTORE:
		paid = restore_all_status(player_ptr);
		break;
	case BACT_ENCHANT_ARROWS:
		item_tester_hook = item_tester_hook_ammo;
		enchant_item(player_ptr, bcost, 1, 1, 0, 0);
		break;
	case BACT_ENCHANT_BOW:
		enchant_item(player_ptr, bcost, 1, 1, 0, TV_BOW);
		break;

	case BACT_RECALL:
		if (recall_player(player_ptr, 1)) paid = TRUE;
		break;

	case BACT_TELEPORT_LEVEL:
		clear_bldg(4, 20);
		paid = free_level_recall(player_ptr);
		break;

	case BACT_LOSE_MUTATION:
		if (player_ptr->muta1 || player_ptr->muta2 || (player_ptr->muta3 & ~MUT3_GOOD_LUCK) ||
			(player_ptr->pseikaku != PERSONALITY_LUCKY && (player_ptr->muta3 & MUT3_GOOD_LUCK)))
		{
			while (!lose_mutation(player_ptr, 0));
			paid = TRUE;
			break;
		}

		msg_print(_("治すべき突然変異が無い。", "You have no mutations."));
		msg_print(NULL);
		break;

	case BACT_BATTLE:
		monster_arena_comm(player_ptr);
		break;

	case BACT_TSUCHINOKO:
		tsuchinoko();
		break;

	case BACT_BOUNTY:
		show_bounty();
		break;

	case BACT_TARGET:
		today_target(player_ptr);
		break;

	case BACT_KANKIN:
		kankin(player_ptr);
		break;

	case BACT_HEIKOUKA:
		msg_print(_("平衡化の儀式を行なった。", "You received an equalization ritual."));
		set_virtue(player_ptr, V_COMPASSION, 0);
		set_virtue(player_ptr, V_HONOUR, 0);
		set_virtue(player_ptr, V_JUSTICE, 0);
		set_virtue(player_ptr, V_SACRIFICE, 0);
		set_virtue(player_ptr, V_KNOWLEDGE, 0);
		set_virtue(player_ptr, V_FAITH, 0);
		set_virtue(player_ptr, V_ENLIGHTEN, 0);
		set_virtue(player_ptr, V_ENCHANT, 0);
		set_virtue(player_ptr, V_CHANCE, 0);
		set_virtue(player_ptr, V_NATURE, 0);
		set_virtue(player_ptr, V_HARMONY, 0);
		set_virtue(player_ptr, V_VITALITY, 0);
		set_virtue(player_ptr, V_UNLIFE, 0);
		set_virtue(player_ptr, V_PATIENCE, 0);
		set_virtue(player_ptr, V_TEMPERANCE, 0);
		set_virtue(player_ptr, V_DILIGENCE, 0);
		set_virtue(player_ptr, V_VALOUR, 0);
		set_virtue(player_ptr, V_INDIVIDUALISM, 0);
		get_virtues(player_ptr);
		paid = TRUE;
		break;

	case BACT_TELE_TOWN:
		paid = tele_town(player_ptr);
		break;

	case BACT_EVAL_AC:
		paid = eval_ac(player_ptr->dis_ac + player_ptr->dis_to_a);
		break;

	case BACT_BROKEN_WEAPON:
		paid = TRUE;
		bcost = repair_broken_weapon(player_ptr, bcost);
		break;
	}

	if (paid) player_ptr->au -= bcost;
}


/*!
 * @brief 施設入り口にプレイヤーが乗った際の処理 / Do building commands
 * @param プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_bldg(player_type *player_ptr)
{
	if (player_ptr->wild_mode) return;

	take_turn(player_ptr, 100);

	if (!cave_have_flag_bold(player_ptr->current_floor_ptr, player_ptr->y, player_ptr->x, FF_BLDG))
	{
		msg_print(_("ここには建物はない。", "You see no building here."));
		return;
	}

	int which = f_info[player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].feat].subtype;

	building_type *bldg;
	bldg = &building[which];

	reinit_wilderness = FALSE;

	if ((which == 2) && (player_ptr->arena_number < 0))
	{
		msg_print(_("「敗者に用はない。」", "'There's no place here for a LOSER like you!'"));
		return;
	}
	else if ((which == 2) && player_ptr->current_floor_ptr->inside_arena)
	{
		if (!player_ptr->exit_bldg && player_ptr->current_floor_ptr->m_cnt > 0)
		{
			prt(_("ゲートは閉まっている。モンスターがあなたを待っている！", "The gates are closed.  The monster awaits!"), 0, 0);
		}
		else
		{
			prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_NO_RETURN);
			player_ptr->current_floor_ptr->inside_arena = FALSE;
			player_ptr->leaving = TRUE;
			command_new = SPECIAL_KEY_BUILDING;
			free_turn(player_ptr);
		}

		return;
	}
	else if (player_ptr->phase_out)
	{
		prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_NO_RETURN);
		player_ptr->leaving = TRUE;
		player_ptr->phase_out = FALSE;
		command_new = SPECIAL_KEY_BUILDING;
		free_turn(player_ptr);
		return;
	}
	else
	{
		player_ptr->oldpy = player_ptr->y;
		player_ptr->oldpx = player_ptr->x;
	}

	forget_lite(player_ptr->current_floor_ptr);
	forget_view(player_ptr->current_floor_ptr);
	current_world_ptr->character_icky++;

	command_arg = 0;
	command_rep = 0;
	command_new = 0;

	show_building(player_ptr, bldg);
	player_ptr->leave_bldg = FALSE;
	play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_BUILD);

	bool validcmd;
	while (!player_ptr->leave_bldg)
	{
		validcmd = FALSE;
		prt("", 1, 0);

		building_prt_gold(player_ptr);

		char command = inkey();

		if (command == ESCAPE)
		{
			player_ptr->leave_bldg = TRUE;
			player_ptr->current_floor_ptr->inside_arena = FALSE;
			player_ptr->phase_out = FALSE;
			break;
		}

		int i;
		for (i = 0; i < 8; i++)
		{
			if (bldg->letters[i] && (bldg->letters[i] == command))
			{
				validcmd = TRUE;
				break;
			}
		}

		if (validcmd) bldg_process_command(player_ptr, bldg, i);

		handle_stuff(player_ptr);
	}

	select_floor_music(player_ptr);

	msg_flag = FALSE;
	msg_erase();

	if (reinit_wilderness) player_ptr->leaving = TRUE;

	current_world_ptr->character_icky--;
	Term_clear();

	player_ptr->update |= (PU_VIEW | PU_MONSTERS | PU_BONUS | PU_LITE | PU_MON_LITE);
	player_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_EQUIPPY | PR_MAP);
	player_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
}


/*!
 * @brief 今日の賞金首を確定する / Determine today's bounty monster
 * @param player_type プレーヤーへの参照ポインタ
 * @return なし
 * @note conv_old is used if loaded 0.0.3 or older save file
 */
void determine_daily_bounty(player_type *player_ptr, bool conv_old)
{
	int max_dl = 3, i;
	if (!conv_old)
	{
		for (i = 0; i < current_world_ptr->max_d_idx; i++)
		{
			if (max_dlv[i] < d_info[i].mindepth) continue;
			if (max_dl < max_dlv[i]) max_dl = max_dlv[i];
		}
	}
	else
	{
		max_dl = MAX(max_dlv[DUNGEON_ANGBAND], 3);
	}

	get_mon_num_prep(player_ptr, NULL, NULL);

	while (TRUE)
	{
		today_mon = get_mon_num(player_ptr, max_dl, GMN_ARENA);
		monster_race *r_ptr;
		r_ptr = &r_info[today_mon];

		if (r_ptr->flags1 & RF1_UNIQUE) continue;
		if (r_ptr->flags7 & (RF7_NAZGUL | RF7_UNIQUE2)) continue;
		if (r_ptr->flags2 & RF2_MULTIPLY) continue;
		if ((r_ptr->flags9 & (RF9_DROP_CORPSE | RF9_DROP_SKELETON)) != (RF9_DROP_CORPSE | RF9_DROP_SKELETON)) continue;
		if (r_ptr->level < MIN(max_dl / 2, 40)) continue;
		if (r_ptr->rarity > 10) continue;
		break;
	}
}


/*!
 * @brief 賞金首となるユニークを確定する / Determine bounty uniques
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void determine_bounty_uniques(player_type *player_ptr)
{
	get_mon_num_prep(player_ptr, NULL, NULL);
	for (int i = 0; i < MAX_BOUNTY; i++)
	{
		while (TRUE)
		{
			current_world_ptr->bounty_r_idx[i] = get_mon_num(player_ptr, MAX_DEPTH - 1, GMN_ARENA);
			monster_race *r_ptr;
			r_ptr = &r_info[current_world_ptr->bounty_r_idx[i]];

			if (!(r_ptr->flags1 & RF1_UNIQUE)) continue;

			if (!(r_ptr->flags9 & (RF9_DROP_CORPSE | RF9_DROP_SKELETON)))
				continue;

			if (r_ptr->rarity > 100) continue;

			if (no_questor_or_bounty_uniques(current_world_ptr->bounty_r_idx[i]))
				continue;

			int j;
			for (j = 0; j < i; j++)
			{
				if (current_world_ptr->bounty_r_idx[i] == current_world_ptr->bounty_r_idx[j])
					break;
			}

			if (j == i) break;
		}
	}

	for (int i = 0; i < MAX_BOUNTY - 1; i++)
	{
		for (int j = i; j < MAX_BOUNTY; j++)
		{
			MONRACE_IDX tmp;
			if (r_info[current_world_ptr->bounty_r_idx[i]].level > r_info[current_world_ptr->bounty_r_idx[j]].level)
			{
				tmp = current_world_ptr->bounty_r_idx[i];
				current_world_ptr->bounty_r_idx[i] = current_world_ptr->bounty_r_idx[j];
				current_world_ptr->bounty_r_idx[j] = tmp;
			}
		}
	}
}
