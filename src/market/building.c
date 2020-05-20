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
#include "core/stuff-handler.h"
#include "core/show-file.h"
#include "core/special-internal-keys.h"
#include "cmd/cmd-dump.h"
#include "cmd/cmd-inn.h"
#include "floor/floor.h"
#include "floor/floor-events.h"
#include "floor/floor-save.h"
#include "object/object-flavor.h"
#include "object/object-hook.h"
#include "monster/monster.h"
#include "floor/wild.h"
#include "world/world.h"
#include "core/sort.h"

#include "player/avatar.h"
#include "market/building.h"
#include "mutation/mutation.h"
#include "cmd-spell.h"
#include "spell/spells3.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "io/files-util.h"
#include "player/player-status.h"
#include "player/player-effects.h"
#include "player/player-personalities-table.h"
#include "inventory/player-inventory.h"
#include "core/scores.h"
#include "view/display-main-window.h"
#include "monster/monster-race.h"
#include "market/poker.h"
#include "market/building-util.h"
#include "market/play-gamble.h"
#include "view/display-fruit.h"
#include "market/arena.h"
#include "market/bounty.h"
#include "market/building-recharger.h"
#include "market/building-quest.h"
#include "market/building-service.h"
#include "market/building-craft-weapon.h"
#include "market/building-craft-armor.h"
#include "market/building-craft-fix.h"

building_type building[MAX_BLDG];

MONRACE_IDX battle_mon[4];
u32b mon_odds[4];
int battle_odds;
PRICE kakekin;
int sel_monster;

bool reinit_wilderness = FALSE;
MONSTER_IDX today_mon;

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
		exchange_cash(player_ptr);
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

	display_buikding_service(player_ptr, bldg);
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
