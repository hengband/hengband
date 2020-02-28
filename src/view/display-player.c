/*!
 * @brief プレーヤーのステータス表示メインルーチン群
 * @date 2020/02/25
 * @author Hourier
 * @details
 * ここにこれ以上関数を引っ越してくるのは禁止
 */

#include "display-player.h"
#include "player-personality.h"
#include "term.h"
#include "status-first-page.h"
#include "player-sex.h"
#include "patron.h"
#include "world.h"
#include "quest.h"
#include "core.h" // 暫定。後で消す
#include "mutation.h"
#include "dungeon-file.h"
#include "objectkind.h"
#include "view/display-util.h"
#include "view/display-characteristic.h"
#include "view/display-player-stat-info.h"
#include "view/display-player-misc-info.h"
#include "view/display-player-middle.h"

/*!
 * @brief
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param mode ステータス表示モード
 * @return どれかの処理をこなしたらTRUE、何もしなかったらFALSE
 */
static bool display_player_info(player_type *creature_ptr, int mode)
{
	if (mode == 2)
	{
		display_player_misc_info(creature_ptr);
		display_player_stat_info(creature_ptr);
		display_player_flag_info_1(creature_ptr, display_player_equippy);
		return TRUE;
	}

	if (mode == 3)
	{
		display_player_flag_info_2(creature_ptr, display_player_equippy);
		return TRUE;
	}

	if (mode == 4)
	{
		do_cmd_knowledge_mutations(creature_ptr);
		return TRUE;
	}

	return FALSE;
}


/*!
 * @brief 名前、性別、種族、職業を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
static void display_player_basic_info(player_type *creature_ptr)
{
	char tmp[64];
#ifdef JP
	sprintf(tmp, "%s%s%s", ap_ptr->title, ap_ptr->no == 1 ? "の" : "", creature_ptr->name);
#else
	sprintf(tmp, "%s %s", ap_ptr->title, creature_ptr->name);
#endif

	display_player_one_line(ENTRY_NAME, tmp, TERM_L_BLUE);
	display_player_one_line(ENTRY_SEX, sp_ptr->title, TERM_L_BLUE);
	display_player_one_line(ENTRY_RACE, (creature_ptr->mimic_form ? mimic_info[creature_ptr->mimic_form].title : rp_ptr->title), TERM_L_BLUE);
	display_player_one_line(ENTRY_CLASS, cp_ptr->title, TERM_L_BLUE);
}


/*!
 * @brief 魔法領域を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void display_magic_realms(player_type *creature_ptr)
{
	if (creature_ptr->realm1 == 0) return;

	char tmp[64];
	if (creature_ptr->realm2)
		sprintf(tmp, "%s, %s", realm_names[creature_ptr->realm1], realm_names[creature_ptr->realm2]);
	else
		strcpy(tmp, realm_names[creature_ptr->realm1]);

	display_player_one_line(ENTRY_REALM, tmp, TERM_L_BLUE);
}


/*!
 * @ brief 年齢、身長、体重、社会的地位を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * 日本語版では、身長はcmに、体重はkgに変更してある
 */
static void display_phisique(player_type *creature_ptr)
{
#ifdef JP
	display_player_one_line(ENTRY_AGE, format("%d才", (int)creature_ptr->age), TERM_L_BLUE);
	display_player_one_line(ENTRY_HEIGHT, format("%dcm", (int)((creature_ptr->ht * 254) / 100)), TERM_L_BLUE);
	display_player_one_line(ENTRY_WEIGHT, format("%dkg", (int)((creature_ptr->wt * 4536) / 10000)), TERM_L_BLUE);
	display_player_one_line(ENTRY_SOCIAL, format("%d  ", (int)creature_ptr->sc), TERM_L_BLUE);
#else
	display_player_one_line(ENTRY_AGE, format("%d", (int)creature_ptr->age), TERM_L_BLUE);
	display_player_one_line(ENTRY_HEIGHT, format("%d", (int)creature_ptr->ht), TERM_L_BLUE);
	display_player_one_line(ENTRY_WEIGHT, format("%d", (int)creature_ptr->wt), TERM_L_BLUE);
	display_player_one_line(ENTRY_SOCIAL, format("%d", (int)creature_ptr->sc), TERM_L_BLUE);
#endif
	display_player_one_line(ENTRY_ALIGN, format("%s", your_alignment(creature_ptr)), TERM_L_BLUE);
}


/*!
 * @brief プレイヤーのステータス表示メイン処理
 * Display the character on the screen (various modes)
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param mode 表示モードID
 * @return なし
 * @details
 * <pre>
 * The top one and bottom two lines are left blank.
 * Mode 0 = standard display with skills
 * Mode 1 = standard display with history
 * Mode 2 = summary of various things
 * Mode 3 = summary of various things (part 2)
 * Mode 4 = mutations
 * </pre>
 */
void display_player(player_type *creature_ptr, int mode, map_name_pf map_name)
{
	if ((creature_ptr->muta1 || creature_ptr->muta2 || creature_ptr->muta3) && display_mutations)
		mode = (mode % 5);
	else
		mode = (mode % 4);

	clear_from(0);
	if (display_player_info(creature_ptr, mode)) return;

	display_player_basic_info(creature_ptr);
	display_magic_realms(creature_ptr);

	if ((creature_ptr->pclass == CLASS_CHAOS_WARRIOR) || (creature_ptr->muta2 & MUT2_CHAOS_GIFT))
		display_player_one_line(ENTRY_PATRON, chaos_patrons[creature_ptr->chaos_patron], TERM_L_BLUE);

	display_phisique(creature_ptr);

	char buf[80];
	for (int i = 0; i < A_MAX; i++)
	{
		if (creature_ptr->stat_cur[i] < creature_ptr->stat_max[i])
		{
			put_str(stat_names_reduced[i], 3 + i, 53);
			int value = creature_ptr->stat_use[i];
			cnv_stat(value, buf);
			c_put_str(TERM_YELLOW, buf, 3 + i, 60);
			value = creature_ptr->stat_top[i];
			cnv_stat(value, buf);
			c_put_str(TERM_L_GREEN, buf, 3 + i, 67);
		}
		else
		{
			put_str(stat_names[i], 3 + i, 53);
			cnv_stat(creature_ptr->stat_use[i], buf);
			c_put_str(TERM_L_GREEN, buf, 3 + i, 60);
		}

		if (creature_ptr->stat_max[i] == creature_ptr->stat_max_max[i])
		{
			c_put_str(TERM_WHITE, "!", 3 + i, _(58, 58 - 2));
		}
	}

	floor_type *floor_ptr = creature_ptr->current_floor_ptr;
	if (mode == 0)
	{
		display_player_middle(creature_ptr);
		display_player_various(creature_ptr);
		return;
	}

	char statmsg[1000];
	put_str(_("(キャラクターの生い立ち)", "(Character Background)"), 11, 25);

	for (int i = 0; i < 4; i++)
	{
		put_str(creature_ptr->history[i], i + 12, 10);
	}

	*statmsg = '\0';

	if (creature_ptr->is_dead)
	{
		if (current_world_ptr->total_winner)
		{
#ifdef JP
			sprintf(statmsg, "…あなたは勝利の後%sした。", streq(creature_ptr->died_from, "Seppuku") ? "切腹" : "引退");
#else
			sprintf(statmsg, "...You %s after winning.", streq(creature_ptr->died_from, "Seppuku") ? "committed seppuku" : "retired from the adventure");
#endif
		}
		else if (!floor_ptr->dun_level)
		{
#ifdef JP
			sprintf(statmsg, "…あなたは%sで%sに殺された。", (*map_name)(creature_ptr), creature_ptr->died_from);
#else
			sprintf(statmsg, "...You were killed by %s in %s.", creature_ptr->died_from, map_name(creature_ptr));
#endif
		}
		else if (floor_ptr->inside_quest && is_fixed_quest_idx(floor_ptr->inside_quest))
		{
			/* Get the quest text */
			/* Bewere that INIT_ASSIGN resets the cur_num. */
			init_flags = INIT_NAME_ONLY;

			process_dungeon_file(creature_ptr, "q_info.txt", 0, 0, 0, 0);

#ifdef JP
			sprintf(statmsg, "…あなたは、クエスト「%s」で%sに殺された。", quest[floor_ptr->inside_quest].name, creature_ptr->died_from);
#else
			sprintf(statmsg, "...You were killed by %s in the quest '%s'.", creature_ptr->died_from, quest[floor_ptr->inside_quest].name);
#endif
		}
		else
		{
#ifdef JP
			sprintf(statmsg, "…あなたは、%sの%d階で%sに殺された。", (*map_name)(creature_ptr), (int)floor_ptr->dun_level, creature_ptr->died_from);
#else
			sprintf(statmsg, "...You were killed by %s on level %d of %s.", creature_ptr->died_from, floor_ptr->dun_level, map_name(creature_ptr));
#endif
		}
	}
	else if (current_world_ptr->character_dungeon)
	{
		if (!floor_ptr->dun_level)
		{
			sprintf(statmsg, _("…あなたは現在、 %s にいる。", "...Now, you are in %s."), map_name(creature_ptr));
		}
		else if (floor_ptr->inside_quest && is_fixed_quest_idx(floor_ptr->inside_quest))
		{
			/* Clear the text */
			/* Must be done before doing INIT_SHOW_TEXT */
			for (int i = 0; i < 10; i++)
			{
				quest_text[i][0] = '\0';
			}

			quest_text_line = 0;
			init_flags = INIT_NAME_ONLY;
			process_dungeon_file(creature_ptr, "q_info.txt", 0, 0, 0, 0);
			sprintf(statmsg, _("…あなたは現在、 クエスト「%s」を遂行中だ。", "...Now, you are in the quest '%s'."), quest[floor_ptr->inside_quest].name);
		}
		else
		{
#ifdef JP
			sprintf(statmsg, "…あなたは現在、 %s の %d 階で探索している。", map_name(creature_ptr), (int)floor_ptr->dun_level);
#else
			sprintf(statmsg, "...Now, you are exploring level %d of %s.", floor_ptr->dun_level, map_name(creature_ptr));
#endif
		}
	}

	if (!*statmsg) return;

	char temp[64 * 2];
	roff_to_buf(statmsg, 60, temp, sizeof(temp));
	char  *t;
	t = temp;
	for (int i = 0; i < 2; i++)
	{
		if (t[0] == 0) return;

		put_str(t, i + 5 + 12, 10);
		t += strlen(t) + 1;
	}
}


/*!
 * todo y = 6、x = 0、mode = 0で固定。何とかする
 * @brief プレイヤーの装備一覧をシンボルで並べる
 * Equippy chars
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param y 表示するコンソールの行
 * @param x 表示するコンソールの列
 * @param mode オプション
 * @return なし
 */
void display_player_equippy(player_type *creature_ptr, TERM_LEN y, TERM_LEN x, BIT_FLAGS16 mode)
{
	int max_i = (mode & DP_WP) ? INVEN_LARM + 1 : INVEN_TOTAL;
	for (int i = INVEN_RARM; i < max_i; i++)
	{
		object_type *o_ptr;
		o_ptr = &creature_ptr->inventory_list[i];

		TERM_COLOR a = object_attr(o_ptr);
		char c = object_char(o_ptr);

		if (!equippy_chars || !o_ptr->k_idx)
		{
			c = ' ';
			a = TERM_DARK;
		}

		Term_putch(x + i - INVEN_RARM, y, a, c);
	}
}
