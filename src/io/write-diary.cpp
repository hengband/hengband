﻿/*!
 * @brief 日記へのメッセージ追加処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "io/write-diary.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "info-reader/fixed-map-parser.h"
#include "io/files-util.h"
#include "market/arena-info-table.h"
#include "monster-race/monster-race.h"
#include "system/floor-type-definition.h"
#include "system/system-variables.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "world/world.h"

// todo *抹殺* したい…
bool write_level;

#ifdef JP
#else
/*!
 * @brief Return suffix of ordinal number
 * @param num number
 * @return pointer of suffix string.
 */
concptr get_ordinal_number_suffix(int num)
{
	num = ABS(num) % 100;
	switch (num % 10)
	{
	case 1:
		return (num == 11) ? "th" : "st";
	case 2:
		return (num == 12) ? "th" : "nd";
	case 3:
		return (num == 13) ? "th" : "rd";
	default:
		return "th";
	}
}
#endif


/*!
 * todo files.c に移すことも検討する？
 * @brief 日記ファイルを開く
 * @param fff ファイルへのポインタ
 * @param disable_diary 日記への追加を無効化する場合TRUE
 * @return ファイルがあったらTRUE、なかったらFALSE
 */
static bool open_diary_file(FILE **fff, bool *disable_diary)
{
	GAME_TEXT file_name[MAX_NLEN];
	sprintf(file_name, _("playrecord-%s.txt", "playrec-%s.txt"), savefile_base);
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, file_name);
	*fff = angband_fopen(buf, "a");
	if (*fff) return TRUE;

	msg_format(_("%s を開くことができませんでした。プレイ記録を一時停止します。", "Failed to open %s. Play-Record is disabled temporarily."), buf);
	msg_format(NULL);
	*disable_diary = TRUE;
	return FALSE;
}


/*!
 * @brief フロア情報を日記に追加する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return クエストID
 */
static QUEST_IDX write_floor(player_type *creature_ptr, concptr *note_level, char *note_level_buf)
{
	floor_type *floor_ptr = creature_ptr->current_floor_ptr;
	QUEST_IDX q_idx = quest_number(creature_ptr, floor_ptr->dun_level);
	if (!write_level) return q_idx;

	if (floor_ptr->inside_arena)
		*note_level = _("アリーナ:", "Arena:");
	else if (!floor_ptr->dun_level)
		*note_level = _("地上:", "Surface:");
	else if (q_idx && (is_fixed_quest_idx(q_idx) && !((q_idx == QUEST_OBERON) || (q_idx == QUEST_SERPENT))))
		*note_level = _("クエスト:", "Quest:");
	else
	{
#ifdef JP
		sprintf(note_level_buf, "%d階(%s):", (int)floor_ptr->dun_level, d_name + d_info[creature_ptr->dungeon_idx].name);
#else
		sprintf(note_level_buf, "%s L%d:", d_name + d_info[creature_ptr->dungeon_idx].name, (int)floor_ptr->dun_level);
#endif
		*note_level = note_level_buf;
	}

	return q_idx;
}


/*!
 * @brief ペットに関する日記を追加する
 * @param fff 日記ファイル
 * @param num 日記へ追加する内容番号
 * @param note 日記内容のIDに応じた文字列参照ポインタ
 * @return なし
 */
static void write_diary_pet(FILE *fff, int num, concptr note)
{
	switch (num)
	{
	case RECORD_NAMED_PET_NAME:
		fprintf(fff, _("%sを旅の友にすることに決めた。\n", "decided to travel together with %s.\n"), note);
		break;
	case RECORD_NAMED_PET_UNNAME:
		fprintf(fff, _("%sの名前を消した。\n", "unnamed %s.\n"), note);
		break;
	case RECORD_NAMED_PET_DISMISS:
		fprintf(fff, _("%sを解放した。\n", "dismissed %s.\n"), note);
		break;
	case RECORD_NAMED_PET_DEATH:
		fprintf(fff, _("%sが死んでしまった。\n", "%s died.\n"), note);
		break;
	case RECORD_NAMED_PET_MOVED:
		fprintf(fff, _("%sをおいて別のマップへ移動した。\n", "moved to another map leaving %s behind.\n"), note);
		break;
	case RECORD_NAMED_PET_LOST_SIGHT:
		fprintf(fff, _("%sとはぐれてしまった。\n", "lost sight of %s.\n"), note);
		break;
	case RECORD_NAMED_PET_DESTROY:
		fprintf(fff, _("%sが*破壊*によって消え去った。\n", "%s was killed by *destruction*.\n"), note);
		break;
	case RECORD_NAMED_PET_EARTHQUAKE:
		fprintf(fff, _("%sが岩石に押し潰された。\n", "%s was crushed by falling rocks.\n"), note);
		break;
	case RECORD_NAMED_PET_GENOCIDE:
		fprintf(fff, _("%sが抹殺によって消え去った。\n", "%s was a victim of genocide.\n"), note);
		break;
	case RECORD_NAMED_PET_WIZ_ZAP:
		fprintf(fff, _("%sがデバッグコマンドによって消え去った。\n", "%s was removed by debug command.\n"), note);
		break;
	case RECORD_NAMED_PET_TELE_LEVEL:
		fprintf(fff, _("%sがテレポート・レベルによって消え去った。\n", "%s was lost after teleporting a level.\n"), note);
		break;
	case RECORD_NAMED_PET_BLAST:
		fprintf(fff, _("%sを爆破した。\n", "blasted %s.\n"), note);
		break;
	case RECORD_NAMED_PET_HEAL_LEPER:
		fprintf(fff, _("%sの病気が治り旅から外れた。\n", "%s was healed and left.\n"), note);
		break;
	case RECORD_NAMED_PET_COMPACT:
		fprintf(fff, _("%sがモンスター情報圧縮によって消え去った。\n", "%s was lost when the monster list was pruned.\n"), note);
		break;
	case RECORD_NAMED_PET_LOSE_PARENT:
		fprintf(fff, _("%sの召喚者が既にいないため消え去った。\n", "%s disappeared because its summoner left.\n"), note);
		break;
	default:
		fprintf(fff, "\n");
		break;
	}
}


/*!
 * @brief 日記にメッセージを追加する /
 * Take note to the diary.
 * @param type 日記内容のID
 * @param num 日記内容のIDに応じた数値
 * @param note 日記内容のIDに応じた文字列参照ポインタ
 * @return エラーコード
 */
errr exe_write_diary(player_type *creature_ptr, int type, int num, concptr note)
{
	static bool disable_diary = FALSE;

	int day, hour, min;
	extract_day_hour_min(creature_ptr, &day, &hour, &min);

	if (disable_diary) return -1;

	if (type == DIARY_FIX_QUEST_C ||
		type == DIARY_FIX_QUEST_F ||
		type == DIARY_RAND_QUEST_C ||
		type == DIARY_RAND_QUEST_F ||
		type == DIARY_TO_QUEST)
	{
		QUEST_IDX old_quest = creature_ptr->current_floor_ptr->inside_quest;
		creature_ptr->current_floor_ptr->inside_quest = (quest[num].type == QUEST_TYPE_RANDOM) ? 0 : num;
		init_flags = INIT_NAME_ONLY;
		parse_fixed_map(creature_ptr, "q_info.txt", 0, 0, 0, 0);
		creature_ptr->current_floor_ptr->inside_quest = old_quest;
	}

	FILE *fff = NULL;
	if (!open_diary_file(&fff, &disable_diary)) return -1;

	concptr note_level = "";
    char note_level_buf[40];
	QUEST_IDX q_idx = write_floor(creature_ptr, &note_level, note_level_buf);

	bool do_level = TRUE;
	switch (type)
	{
	case DIARY_DIALY:
	{
		if (day < MAX_DAYS)
			fprintf(fff, _("%d日目\n", "Day %d\n"), day);
		else
			fputs(_("*****日目\n", "Day *****\n"), fff);

		do_level = FALSE;
		break;
	}
	case DIARY_DESCRIPTION:
	{
		if (num)
		{
			fprintf(fff, "%s\n", note);
			do_level = FALSE;
		}
		else
			fprintf(fff, " %2d:%02d %20s %s\n", hour, min, note_level, note);

		break;
	}
	case DIARY_ART:
	{
		fprintf(fff, _(" %2d:%02d %20s %sを発見した。\n", " %2d:%02d %20s discovered %s.\n"), hour, min, note_level, note);
		break;
	}
	case DIARY_ART_SCROLL:
	{
		fprintf(fff, _(" %2d:%02d %20s 巻物によって%sを生成した。\n", " %2d:%02d %20s created %s by scroll.\n"), hour, min, note_level, note);
		break;
	}
	case DIARY_UNIQUE:
	{
		fprintf(fff, _(" %2d:%02d %20s %sを倒した。\n", " %2d:%02d %20s defeated %s.\n"), hour, min, note_level, note);
		break;
	}
	case DIARY_FIX_QUEST_C:
	{
		if (quest[num].flags & QUEST_FLAG_SILENT) break;

		fprintf(fff, _(" %2d:%02d %20s クエスト「%s」を達成した。\n",
			" %2d:%02d %20s completed quest '%s'.\n"), hour, min, note_level, quest[num].name);
		break;
	}
	case DIARY_FIX_QUEST_F:
	{
		if (quest[num].flags & QUEST_FLAG_SILENT) break;

		fprintf(fff, _(" %2d:%02d %20s クエスト「%s」から命からがら逃げ帰った。\n",
			" %2d:%02d %20s ran away from quest '%s'.\n"), hour, min, note_level, quest[num].name);
		break;
	}
	case DIARY_RAND_QUEST_C:
	{
		GAME_TEXT name[MAX_NLEN];
		strcpy(name, r_name + r_info[quest[num].r_idx].name);
		fprintf(fff, _(" %2d:%02d %20s ランダムクエスト(%s)を達成した。\n",
			" %2d:%02d %20s completed random quest '%s'\n"), hour, min, note_level, name);
		break;
	}
	case DIARY_RAND_QUEST_F:
	{
		GAME_TEXT name[MAX_NLEN];
		strcpy(name, r_name + r_info[quest[num].r_idx].name);
		fprintf(fff, _(" %2d:%02d %20s ランダムクエスト(%s)から逃げ出した。\n",
			" %2d:%02d %20s ran away from quest '%s'.\n"), hour, min, note_level, name);
		break;
	}
	case DIARY_MAXDEAPTH:
	{
		fprintf(fff, _(" %2d:%02d %20s %sの最深階%d階に到達した。\n",
			" %2d:%02d %20s reached level %d of %s for the first time.\n"), hour, min, note_level,
			_(d_name + d_info[creature_ptr->dungeon_idx].name, num),
			_(num, d_name + d_info[creature_ptr->dungeon_idx].name));
		break;
	}
	case DIARY_TRUMP:
	{
		fprintf(fff, _(" %2d:%02d %20s %s%sの最深階を%d階にセットした。\n",
			" %2d:%02d %20s reset recall level of %s to %d %s.\n"), hour, min, note_level, note,
			_(d_name + d_info[num].name, (int)max_dlv[num]),
			_((int)max_dlv[num], d_name + d_info[num].name));
		break;
	}
	case DIARY_STAIR:
	{
		concptr to = q_idx && (is_fixed_quest_idx(q_idx)
			&& !((q_idx == QUEST_OBERON) || (q_idx == QUEST_SERPENT)))
			? _("地上", "the surface")
			: !(creature_ptr->current_floor_ptr->dun_level + num)
			? _("地上", "the surface")
			: format(_("%d階", "level %d"), creature_ptr->current_floor_ptr->dun_level + num);
		fprintf(fff, _(" %2d:%02d %20s %sへ%s。\n", " %2d:%02d %20s %s %s.\n"), hour, min, note_level, _(to, note), _(note, to));
		break;
	}
	case DIARY_RECALL:
	{
		if (!num)
			fprintf(fff, _(" %2d:%02d %20s 帰還を使って%sの%d階へ下りた。\n", " %2d:%02d %20s recalled to dungeon level %d of %s.\n"),
				hour, min, note_level, _(d_name + d_info[creature_ptr->dungeon_idx].name, (int)max_dlv[creature_ptr->dungeon_idx]),
				_((int)max_dlv[creature_ptr->dungeon_idx], d_name + d_info[creature_ptr->dungeon_idx].name));
		else
			fprintf(fff, _(" %2d:%02d %20s 帰還を使って地上へと戻った。\n", " %2d:%02d %20s recalled from dungeon to surface.\n"), hour, min, note_level);

		break;
	}
	case DIARY_TO_QUEST:
	{
		if (quest[num].flags & QUEST_FLAG_SILENT) break;

		fprintf(fff, _(" %2d:%02d %20s クエスト「%s」へと突入した。\n", " %2d:%02d %20s entered the quest '%s'.\n"),
			hour, min, note_level, quest[num].name);
		break;
	}
	case DIARY_TELEPORT_LEVEL:
	{
		fprintf(fff, _(" %2d:%02d %20s レベル・テレポートで脱出した。\n", " %2d:%02d %20s got out using teleport level.\n"),
			hour, min, note_level);
		break;
	}
	case DIARY_BUY:
	{
		fprintf(fff, _(" %2d:%02d %20s %sを購入した。\n", " %2d:%02d %20s bought %s.\n"), hour, min, note_level, note);
		break;
	}
	case DIARY_SELL:
	{
		fprintf(fff, _(" %2d:%02d %20s %sを売却した。\n", " %2d:%02d %20s sold %s.\n"), hour, min, note_level, note);
		break;
	}
	case DIARY_ARENA:
	{
		if (num < 0)
		{
			int n = -num;
			fprintf(fff, _(" %2d:%02d %20s 闘技場の%d%s回戦で、%sの前に敗れ去った。\n", " %2d:%02d %20s beaten by %s in the %d%s fight.\n"),
				hour, min, note_level, _(n, note), _("", n), _(note, get_ordinal_number_suffix(n)));
			break;
		}

		fprintf(fff, _(" %2d:%02d %20s 闘技場の%d%s回戦(%s)に勝利した。\n", " %2d:%02d %20s won the %d%s fight (%s).\n"),
			hour, min, note_level, num, _("", get_ordinal_number_suffix(num)), note);

		if (num == MAX_ARENA_MONS)
		{
			fprintf(fff, _("                 闘技場のすべての敵に勝利し、チャンピオンとなった。\n",
				"                 won all fights to become a Champion.\n"));
			do_level = FALSE;
		}

		break;
	}
	case DIARY_FOUND:
	{
		fprintf(fff, _(" %2d:%02d %20s %sを識別した。\n", " %2d:%02d %20s identified %s.\n"), hour, min, note_level, note);
		break;
	}
	case DIARY_WIZ_TELE:
	{
		concptr to = !creature_ptr->current_floor_ptr->dun_level
			? _("地上", "the surface")
			: format(_("%d階(%s)", "level %d of %s"), creature_ptr->current_floor_ptr->dun_level, d_name + d_info[creature_ptr->dungeon_idx].name);
		fprintf(fff, _(" %2d:%02d %20s %sへとウィザード・テレポートで移動した。\n",
			" %2d:%02d %20s wizard-teleported to %s.\n"), hour, min, note_level, to);
		break;
	}
	case DIARY_PAT_TELE:
	{
		concptr to = !creature_ptr->current_floor_ptr->dun_level
			? _("地上", "the surface")
			: format(_("%d階(%s)", "level %d of %s"), creature_ptr->current_floor_ptr->dun_level, d_name + d_info[creature_ptr->dungeon_idx].name);
		fprintf(fff, _(" %2d:%02d %20s %sへとパターンの力で移動した。\n",
			" %2d:%02d %20s used Pattern to teleport to %s.\n"), hour, min, note_level, to);
		break;
	}
	case DIARY_LEVELUP:
	{
		fprintf(fff, _(" %2d:%02d %20s レベルが%dに上がった。\n", " %2d:%02d %20s reached player level %d.\n"), hour, min, note_level, num);
		break;
	}
	case DIARY_GAMESTART:
	{
		time_t ct = time((time_t*)0);
		do_level = FALSE;
		if (num)
			fprintf(fff, "%s %s", note, ctime(&ct));
		else
			fprintf(fff, " %2d:%02d %20s %s %s", hour, min, note_level, note, ctime(&ct));

		break;
	}
	case DIARY_NAMED_PET:
	{
		fprintf(fff, " %2d:%02d %20s ", hour, min, note_level);
		write_diary_pet(fff, num, note);
		break;
	}
	case DIARY_WIZARD_LOG:
		fprintf(fff, "%s\n", note);
		break;
	default:
		break;
	}

	angband_fclose(fff);
	if (do_level) write_level = FALSE;

	return 0;
}
