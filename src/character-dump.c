#include "character-dump.h"
#include "core.h"
#include "world.h"
#include "floor-town.h"
#include "dungeon.h"
#include "store.h"
#include "cmd/cmd-pet.h"
#include "cmd/cmd-magiceat.h"
#include "objectkind.h"
#include "monster-spell.h"
#include "monster-status.h"
#include "sort.h"
#include "quest.h"
#include "init.h"
#include "mutation.h"
#include "object-flavor.h"
#include "cmd/cmd-dump.h"
#include "avatar.h"

/*!
 * @brief プレイヤーのステータス表示をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_display_player(player_type *creature_ptr, FILE *fff, void(*display_player)(player_type*, int))
{
	TERM_COLOR a;
	char c;
	char buf[1024];
	display_player(creature_ptr, 0);

	for (TERM_LEN y = 1; y < 22; y++)
	{
		TERM_LEN x;
		for (x = 0; x < 79; x++)
		{
			(void)(Term_what(x, y, &a, &c));
			buf[x] = c;
		}

		buf[x] = '\0';
		while ((x > 0) && (buf[x - 1] == ' '))
			buf[--x] = '\0';

		fprintf(fff, _("%s\n", "%s\n"), buf);
	}

	display_player(creature_ptr, 1);
	for (TERM_LEN y = 10; y < 19; y++)
	{
		TERM_LEN x;
		for (x = 0; x < 79; x++)
		{
			(void)(Term_what(x, y, &a, &c));
			buf[x] = c;
		}

		buf[x] = '\0';
		while ((x > 0) && (buf[x - 1] == ' '))
			buf[--x] = '\0';

		fprintf(fff, "%s\n", buf);
	}

	fprintf(fff, "\n");
	display_player(creature_ptr, 2);
	for (TERM_LEN y = 2; y < 22; y++)
	{
		TERM_LEN x;
		for (x = 0; x < 79; x++)
		{
			(void)(Term_what(x, y, &a, &c));
			if (a < 128)
				buf[x] = c;
			else
				buf[x] = ' ';
		}

		buf[x] = '\0';
		while ((x > 0) && (buf[x - 1] == ' '))
			buf[--x] = '\0';

		fprintf(fff, "%s\n", buf);
	}

	fprintf(fff, "\n");
	display_player(creature_ptr, 3);
	for (TERM_LEN y = 1; y < 22; y++)
	{
		TERM_LEN x;
		for (x = 0; x < 79; x++)
		{
			(void)(Term_what(x, y, &a, &c));
			if (a < 128)
				buf[x] = c;
			else
				buf[x] = ' ';
		}

		buf[x] = '\0';
		while ((x > 0) && (buf[x - 1] == ' '))
			buf[--x] = '\0';

		fprintf(fff, "%s\n", buf);
	}

	fprintf(fff, "\n");
}


/*!
 * @brief プレイヤーのペット情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_pet(player_type *master_ptr, FILE *fff)
{
	bool pet = FALSE;
	bool pet_settings = FALSE;
	for (int i = master_ptr->current_floor_ptr->m_max - 1; i >= 1; i--)
	{
		monster_type *m_ptr = &master_ptr->current_floor_ptr->m_list[i];

		if (!monster_is_valid(m_ptr)) continue;
		if (!is_pet(m_ptr)) continue;
		pet_settings = TRUE;
		if (!m_ptr->nickname && (master_ptr->riding != i)) continue;
		if (!pet)
		{
			fprintf(fff, _("\n\n  [主なペット]\n\n", "\n\n  [Leading Pets]\n\n"));
			pet = TRUE;
		}

		GAME_TEXT pet_name[MAX_NLEN];
		monster_desc(master_ptr, pet_name, m_ptr, MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE);
		fprintf(fff, "%s\n", pet_name);
	}

	if (!pet_settings) return;

	fprintf(fff, _("\n\n  [ペットへの命令]\n", "\n\n  [Command for Pets]\n"));

	fprintf(fff, _("\n ドアを開ける:                       %s", "\n Pets open doors:                    %s"),
		(master_ptr->pet_extra_flags & PF_OPEN_DOORS) ? "ON" : "OFF");

	fprintf(fff, _("\n アイテムを拾う:                     %s", "\n Pets pick up items:                 %s"),
		(master_ptr->pet_extra_flags & PF_PICKUP_ITEMS) ? "ON" : "OFF");

	fprintf(fff, _("\n テレポート系魔法を使う:             %s", "\n Allow teleport:                     %s"),
		(master_ptr->pet_extra_flags & PF_TELEPORT) ? "ON" : "OFF");

	fprintf(fff, _("\n 攻撃魔法を使う:                     %s", "\n Allow cast attack spell:            %s"),
		(master_ptr->pet_extra_flags & PF_ATTACK_SPELL) ? "ON" : "OFF");

	fprintf(fff, _("\n 召喚魔法を使う:                     %s", "\n Allow cast summon spell:            %s"),
		(master_ptr->pet_extra_flags & PF_SUMMON_SPELL) ? "ON" : "OFF");

	fprintf(fff, _("\n プレイヤーを巻き込む範囲魔法を使う: %s", "\n Allow involve player in area spell: %s"),
		(master_ptr->pet_extra_flags & PF_BALL_SPELL) ? "ON" : "OFF");

	fputc('\n', fff);
}


/*!
 * todo ここはenum/switchで扱いたい
 * @brief プレイヤーの職業能力情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_class_special(player_type *creature_ptr, FILE *fff)
{
	bool is_special_class = creature_ptr->pclass == CLASS_MAGIC_EATER;
	is_special_class |= creature_ptr->pclass == CLASS_SMITH;
	is_special_class |= creature_ptr->pclass != CLASS_BLUE_MAGE;
	if (!is_special_class) return;

	if (creature_ptr->pclass == CLASS_MAGIC_EATER)
	{
		char s[EATER_EXT][MAX_NLEN];
		OBJECT_TYPE_VALUE tval = 0;
		fprintf(fff, _("\n\n  [取り込んだ魔法道具]\n", "\n\n  [Magic devices eaten]\n"));

		for (int ext = 0; ext < 3; ext++)
		{
			int eat_num = 0;

			/* Dump an extent name */
			switch (ext)
			{
			case 0:
				tval = TV_STAFF;
				fprintf(fff, _("\n[杖]\n", "\n[Staffs]\n"));
				break;
			case 1:
				tval = TV_WAND;
				fprintf(fff, _("\n[魔法棒]\n", "\n[Wands]\n"));
				break;
			case 2:
				tval = TV_ROD;
				fprintf(fff, _("\n[ロッド]\n", "\n[Rods]\n"));
				break;
			}

			/* Get magic device names that were eaten */
			for (OBJECT_SUBTYPE_VALUE i = 0; i < EATER_EXT; i++)
			{
				int idx = EATER_EXT * ext + i;
				int magic_num = creature_ptr->magic_num2[idx];
				if (!magic_num) continue;

				KIND_OBJECT_IDX k_idx = lookup_kind(tval, i);
				if (!k_idx) continue;
				sprintf(s[eat_num], "%23s (%2d)", (k_name + k_info[k_idx].name), magic_num);
				eat_num++;
			}

			/* Dump magic devices in this extent */
			if (eat_num <= 0)
			{
				fputs(_("  (なし)\n", "  (none)\n"), fff);
				continue;
			}

			OBJECT_SUBTYPE_VALUE i;
			for (i = 0; i < eat_num; i++)
			{
				fputs(s[i], fff);
				if (i % 3 < 2) fputs("    ", fff);
				else fputs("\n", fff);
			}

			if (i % 3 > 0) fputs("\n", fff);
		}

		return;
	}

	if (creature_ptr->pclass == CLASS_SMITH)
	{
		int i, id[250], n = 0, row;

		fprintf(fff, _("\n\n  [手に入れたエッセンス]\n\n", "\n\n  [Get Essence]\n\n"));
		fprintf(fff, _("エッセンス   個数     エッセンス   個数     エッセンス   個数",
			"Essence      Num      Essence      Num      Essence      Num "));
		for (i = 0; essence_name[i]; i++)
		{
			if (!essence_name[i][0]) continue;
			id[n] = i;
			n++;
		}

		row = n / 3 + 1;

		for (i = 0; i < row; i++)
		{
			fprintf(fff, "\n");
			fprintf(fff, "%-11s %5d     ", essence_name[id[i]], (int)creature_ptr->magic_num1[id[i]]);
			if (i + row < n) fprintf(fff, "%-11s %5d     ", essence_name[id[i + row]], (int)creature_ptr->magic_num1[id[i + row]]);
			if (i + row * 2 < n) fprintf(fff, "%-11s %5d", essence_name[id[i + row * 2]], (int)creature_ptr->magic_num1[id[i + row * 2]]);
		}

		fputs("\n", fff);
		return;
	}

	// Blue mage
	int l1 = 0;
	int l2 = 0;
	int spellnum[MAX_MONSPELLS];
	BIT_FLAGS f4 = 0, f5 = 0, f6 = 0;
	char p[60][80];
	int col = 0;
	bool pcol = FALSE;

	for (int i = 0; i < 60; i++)
	{
		p[i][0] = '\0';
	}

	strcat(p[col], _("\n\n  [学習済みの青魔法]\n", "\n\n  [Learned Blue Magic]\n"));

	for (int j = 1; j < 6; j++)
	{
		col++;
		set_rf_masks(&f4, &f5, &f6, j);
		switch (j)
		{
		case MONSPELL_TYPE_BOLT:
			strcat(p[col], _("\n     [ボルト型]\n", "\n     [Bolt  Type]\n"));
			break;

		case MONSPELL_TYPE_BALL:
			strcat(p[col], _("\n     [ボール型]\n", "\n     [Ball  Type]\n"));
			break;

		case MONSPELL_TYPE_BREATH:
			strcat(p[col], _("\n     [ブレス型]\n", "\n     [  Breath  ]\n"));
			break;

		case MONSPELL_TYPE_SUMMON:
			strcat(p[col], _("\n     [召喚魔法]\n", "\n     [Summonning]\n"));
			break;

		case MONSPELL_TYPE_OTHER:
			strcat(p[col], _("\n     [ その他 ]\n", "\n     [Other Type]\n"));
			break;
		}

		int num = 0;
		for (int i = 0; i < 32; i++)
		{
			if ((0x00000001 << i) & f4) spellnum[num++] = i;
		}

		for (int i = 32; i < 64; i++)
		{
			if ((0x00000001 << (i - 32)) & f5) spellnum[num++] = i;
		}

		for (int i = 64; i < 96; i++)
		{
			if ((0x00000001 << (i - 64)) & f6) spellnum[num++] = i;
		}

		col++;
		pcol = FALSE;
		strcat(p[col], "       ");

		for (int i = 0; i < num; i++)
		{
			if (creature_ptr->magic_num2[spellnum[i]] == 0) continue;

			pcol = TRUE;
			/* Dump blue magic */
			l1 = strlen(p[col]);
			l2 = strlen(monster_powers_short[spellnum[i]]);
			if ((l1 + l2) >= 75)
			{
				strcat(p[col], "\n");
				col++;
				strcat(p[col], "       ");
			}

			strcat(p[col], monster_powers_short[spellnum[i]]);
			strcat(p[col], ", ");
		}

		if (!pcol)
		{
			strcat(p[col], _("なし", "None"));
			strcat(p[col], "\n");
			continue;
		}

		if (p[col][strlen(p[col]) - 2] == ',')
		{
			p[col][strlen(p[col]) - 2] = '\0';
		}
		else
		{
			p[col][strlen(p[col]) - 10] = '\0';
		}

		strcat(p[col], "\n");
	}

	for (int i = 0; i <= col; i++)
	{
		fputs(p[i], fff);
	}
}


/*!
 * @brief クエスト情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_quest(player_type *creature_ptr, FILE *fff)
{
	fprintf(fff, _("\n\n  [クエスト情報]\n", "\n\n  [Quest Information]\n"));
	QUEST_IDX *quest_num;
	C_MAKE(quest_num, max_q_idx, QUEST_IDX);

	for (QUEST_IDX i = 1; i < max_q_idx; i++)
		quest_num[i] = i;
	int dummy;
	ang_sort(quest_num, &dummy, max_q_idx, ang_sort_comp_quest_num, ang_sort_swap_quest_num);

	fputc('\n', fff);
	do_cmd_knowledge_quests_completed(creature_ptr, fff, quest_num);
	fputc('\n', fff);
	do_cmd_knowledge_quests_failed(creature_ptr, fff, quest_num);
	fputc('\n', fff);

	C_KILL(quest_num, max_q_idx, QUEST_IDX);
}


/*!
 * @brief 死の直前メッセージ並びに遺言をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_last_message(player_type *creature_ptr, FILE *fff)
{
	if (!creature_ptr->is_dead) return;

	if (!current_world_ptr->total_winner)
	{
		fprintf(fff, _("\n  [死ぬ直前のメッセージ]\n\n", "\n  [Last Messages]\n\n"));
		for (int i = MIN(message_num(), 30); i >= 0; i--)
		{
			fprintf(fff, "> %s\n", message_str((s16b)i));
		}

		fputc('\n', fff);
		return;
	}

	if (creature_ptr->last_message)
	{
		fprintf(fff, _("\n  [*勝利*メッセージ]\n\n", "\n  [*Winning* Message]\n\n"));
		fprintf(fff, "  %s\n", creature_ptr->last_message);
		fputc('\n', fff);
	}
}


/*!
 * @brief 帰還場所情報をファイルにダンプする
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_recall(FILE *fff)
{
	fprintf(fff, _("\n  [帰還場所]\n\n", "\n  [Recall Depth]\n\n"));
	for (int y = 1; y < current_world_ptr->max_d_idx; y++)
	{
		bool seiha = FALSE;

		if (!d_info[y].maxdepth) continue;
		if (!max_dlv[y]) continue;
		if (d_info[y].final_guardian)
		{
			if (!r_info[d_info[y].final_guardian].max_num) seiha = TRUE;
		}
		else if (max_dlv[y] == d_info[y].maxdepth) seiha = TRUE;

		fprintf(fff, _("   %c%-12s: %3d 階\n", "   %c%-16s: level %3d\n"),
			seiha ? '!' : ' ', d_name + d_info[y].name, (int)max_dlv[y]);
	}
}


/*!
 * @brief オプション情報をファイルにダンプする
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_options(FILE *fff)
{
	fprintf(fff, _("\n  [オプション設定]\n", "\n  [Option Settings]\n"));
	if (preserve_mode)
		fprintf(fff, _("\n 保存モード:         ON", "\n Preserve Mode:      ON"));

	else
		fprintf(fff, _("\n 保存モード:         OFF", "\n Preserve Mode:      OFF"));

	if (ironman_small_levels)
		fprintf(fff, _("\n 小さいダンジョン:   ALWAYS", "\n Small Levels:       ALWAYS"));
	else if (always_small_levels)
		fprintf(fff, _("\n 小さいダンジョン:   ON", "\n Small Levels:       ON"));
	else if (small_levels)
		fprintf(fff, _("\n 小さいダンジョン:   ENABLED", "\n Small Levels:       ENABLED"));
	else
		fprintf(fff, _("\n 小さいダンジョン:   OFF", "\n Small Levels:       OFF"));

	if (vanilla_town)
		fprintf(fff, _("\n 元祖の町のみ:       ON", "\n Vanilla Town:       ON"));
	else if (lite_town)
		fprintf(fff, _("\n 小規模な町:         ON", "\n Lite Town:          ON"));

	if (ironman_shops)
		fprintf(fff, _("\n 店なし:             ON", "\n No Shops:           ON"));

	if (ironman_downward)
		fprintf(fff, _("\n 階段を上がれない:   ON", "\n Diving Only:        ON"));

	if (ironman_rooms)
		fprintf(fff, _("\n 普通でない部屋:     ON", "\n Unusual Rooms:      ON"));

	if (ironman_nightmare)
		fprintf(fff, _("\n 悪夢モード:         ON", "\n Nightmare Mode:     ON"));

	if (ironman_empty_levels)
		fprintf(fff, _("\n アリーナ:           ALWAYS", "\n Arena Levels:       ALWAYS"));
	else if (empty_levels)
		fprintf(fff, _("\n アリーナ:           ENABLED", "\n Arena Levels:       ENABLED"));
	else
		fprintf(fff, _("\n アリーナ:           OFF", "\n Arena Levels:       OFF"));

	fputc('\n', fff);

	if (current_world_ptr->noscore)
		fprintf(fff, _("\n 何か不正なことをしてしまっています。\n", "\n You have done something illegal.\n"));

	fputc('\n', fff);
}


/*!
 * @brief 闘技場の情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_arena(player_type *creature_ptr, FILE *fff)
{
	if (lite_town || vanilla_town) return;

	if (creature_ptr->arena_number < 0)
	{
		if (creature_ptr->arena_number <= ARENA_DEFEATED_OLD_VER)
		{
			fprintf(fff, _("\n 闘技場: 敗北\n", "\n Arena: Defeated\n"));
		}
		else
		{
#ifdef JP
			fprintf(fff, "\n 闘技場: %d回戦で%sの前に敗北\n", -creature_ptr->arena_number,
				r_name + r_info[arena_info[-1 - creature_ptr->arena_number].r_idx].name);
#else
			fprintf(fff, "\n Arena: Defeated by %s in the %d%s fight\n",
				r_name + r_info[arena_info[-1 - creature_ptr->arena_number].r_idx].name,
				-creature_ptr->arena_number, get_ordinal_number_suffix(-creature_ptr->arena_number));
#endif
		}

		fprintf(fff, "\n");
		return;
	}

	if (creature_ptr->arena_number > MAX_ARENA_MONS + 2)
	{
		fprintf(fff, _("\n 闘技場: 真のチャンピオン\n", "\n Arena: True Champion\n"));
		fprintf(fff, "\n");
		return;
	}

	if (creature_ptr->arena_number > MAX_ARENA_MONS - 1)
	{
		fprintf(fff, _("\n 闘技場: チャンピオン\n", "\n Arena: Champion\n"));
		fprintf(fff, "\n");
		return;
	}

#ifdef JP
	fprintf(fff, "\n 闘技場: %2d勝\n", (creature_ptr->arena_number > MAX_ARENA_MONS ? MAX_ARENA_MONS : creature_ptr->arena_number));
#else
	fprintf(fff, "\n Arena: %2d Victor%s\n", (creature_ptr->arena_number > MAX_ARENA_MONS ? MAX_ARENA_MONS : creature_ptr->arena_number), (creature_ptr->arena_number > 1) ? "ies" : "y");
#endif
	fprintf(fff, "\n");
}


/*!
 * @brief 撃破モンスターの情報をファイルにダンプする
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_monsters(FILE *fff)
{
	fprintf(fff, _("\n  [倒したモンスター]\n\n", "\n  [Defeated Monsters]\n\n"));

	/* Allocate the "who" array */
	MONRACE_IDX *who;
	u16b why = 2;
	C_MAKE(who, max_r_idx, MONRACE_IDX);

	/* Count monster kills */
	long uniq_total = 0;
	long norm_total = 0;
	for (IDX k = 1; k < max_r_idx; k++)
	{
		/* Ignore unused index */
		monster_race *r_ptr = &r_info[k];
		if (!r_ptr->name) continue;

		if (r_ptr->flags1 & RF1_UNIQUE)
		{
			bool dead = (r_ptr->max_num == 0);
			if (dead)
			{
				norm_total++;

				/* Add a unique monster to the list */
				who[uniq_total++] = k;
			}

			continue;
		}

		if (r_ptr->r_pkills > 0)
		{
			norm_total += r_ptr->r_pkills;
		}
	}

	/* No monsters is defeated */
	if (norm_total < 1)
	{
		fprintf(fff, _("まだ敵を倒していません。\n", "You have defeated no enemies yet.\n"));
		C_KILL(who, max_r_idx, s16b);
		return;
	}

	/* Defeated more than one normal monsters */
	if (uniq_total == 0)
	{
#ifdef JP
		fprintf(fff, "%ld体の敵を倒しています。\n", norm_total);
#else
		fprintf(fff, "You have defeated %ld %s.\n", norm_total, norm_total == 1 ? "enemy" : "enemies");
#endif
		C_KILL(who, max_r_idx, s16b);
		return;
	}

	/* Defeated more than one unique monsters */
#ifdef JP
	fprintf(fff, "%ld体のユニーク・モンスターを含む、合計%ld体の敵を倒しています。\n", uniq_total, norm_total);
#else
	fprintf(fff, "You have defeated %ld %s including %ld unique monster%s in total.\n", norm_total, norm_total == 1 ? "enemy" : "enemies", uniq_total, (uniq_total == 1 ? "" : "s"));
#endif

	/* Sort the array by dungeon depth of monsters */
	ang_sort(who, &why, uniq_total, ang_sort_comp_hook, ang_sort_swap_hook);
	fprintf(fff, _("\n《上位%ld体のユニーク・モンスター》\n", "\n< Unique monsters top %ld >\n"), MIN(uniq_total, 10));

	for (IDX k = uniq_total - 1; k >= 0 && k >= uniq_total - 10; k--)
	{
		monster_race *r_ptr = &r_info[who[k]];
		fprintf(fff, _("  %-40s (レベル%3d)\n", "  %-40s (level %3d)\n"), (r_name + r_ptr->name), (int)r_ptr->level);
	}

	C_KILL(who, max_r_idx, s16b);
}


/*!
 * @brief 元種族情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_race_history(player_type *creature_ptr, FILE *fff)
{
	if (!creature_ptr->old_race1 && !creature_ptr->old_race2) return;

	fprintf(fff, _("\n\n あなたは%sとして生まれた。", "\n\n You were born as %s."), race_info[creature_ptr->start_race].title);
	for (int i = 0; i < MAX_RACES; i++)
	{
		if (creature_ptr->start_race == i) continue;
		if (i < 32)
		{
			if (!(creature_ptr->old_race1 & 1L << i)) continue;
		}
		else
		{
			if (!(creature_ptr->old_race2 & 1L << (i - 32))) continue;
		}

		fprintf(fff, _("\n あなたはかつて%sだった。", "\n You were a %s before."), race_info[i].title);
	}

	fputc('\n', fff);
}


/*!
 * @brief 元魔法領域情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_realm_history(player_type *creature_ptr, FILE *fff)
{
	if (creature_ptr->old_realm) return;

	fputc('\n', fff);
	for (int i = 0; i < MAX_MAGIC; i++)
	{
		if (!(creature_ptr->old_realm & 1L << i)) continue;
		fprintf(fff, _("\n あなたはかつて%s魔法を使えた。", "\n You were able to use %s magic before."), realm_names[i + 1]);
	}

	fputc('\n', fff);
}


/*!
 * @brief 徳の情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_virtues(player_type *creature_ptr, FILE *fff)
{
	fprintf(fff, _("\n\n  [自分に関する情報]\n\n", "\n\n  [HP-rate & Max stat & Virtues]\n\n"));

	int percent = (int)(((long)creature_ptr->player_hp[PY_MAX_LEVEL - 1] * 200L) /
		(2 * creature_ptr->hitdie +
		((PY_MAX_LEVEL - 1 + 3) * (creature_ptr->hitdie + 1))));

#ifdef JP
	if (creature_ptr->knowledge & KNOW_HPRATE) fprintf(fff, "現在の体力ランク : %d/100\n\n", percent);
	else fprintf(fff, "現在の体力ランク : ???\n\n");
	fprintf(fff, "能力の最大値\n");
#else
	if (creature_ptr->knowledge & KNOW_HPRATE) fprintf(fff, "Your current Life Rating is %d/100.\n\n", percent);
	else fprintf(fff, "Your current Life Rating is ???.\n\n");
	fprintf(fff, "Limits of maximum stats\n");
#endif
	for (int v_nr = 0; v_nr < A_MAX; v_nr++)
	{
		if ((creature_ptr->knowledge & KNOW_STAT) || creature_ptr->stat_max[v_nr] == creature_ptr->stat_max_max[v_nr]) fprintf(fff, "%s 18/%d\n", stat_names[v_nr], creature_ptr->stat_max_max[v_nr] - 18);
		else fprintf(fff, "%s ???\n", stat_names[v_nr]);
	}

	fprintf(fff, _("\n属性 : %s\n", "\nYour alignment : %s\n"), your_alignment(creature_ptr));
	fprintf(fff, "\n");
	dump_virtues(creature_ptr, fff);
}


/*!
 * @brief 突然変異の情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_mutations(player_type *creature_ptr, FILE *fff)
{
	if (creature_ptr->muta1 || creature_ptr->muta2 || creature_ptr->muta3)
	{
		fprintf(fff, _("\n\n  [突然変異]\n\n", "\n\n  [Mutations]\n\n"));
		dump_mutations(creature_ptr, fff);
	}
}


/*!
 * @brief 所持品の情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_equipment_inventory(player_type *creature_ptr, FILE *fff)
{
	GAME_TEXT o_name[MAX_NLEN];
	if (creature_ptr->equip_cnt)
	{
		fprintf(fff, _("  [キャラクタの装備]\n\n", "  [Character Equipment]\n\n"));
		for (int i = INVEN_RARM; i < INVEN_TOTAL; i++)
		{
			object_desc(creature_ptr, o_name, &creature_ptr->inventory_list[i], 0);
			if ((((i == INVEN_RARM) && creature_ptr->hidarite) || ((i == INVEN_LARM) && creature_ptr->migite)) && creature_ptr->ryoute)
				strcpy(o_name, _("(武器を両手持ち)", "(wielding with two-hands)"));

			fprintf(fff, "%c) %s\n",
				index_to_label(i), o_name);
		}

		fprintf(fff, "\n\n");
	}

	fprintf(fff, _("  [キャラクタの持ち物]\n\n", "  [Character Inventory]\n\n"));

	for (int i = 0; i < INVEN_PACK; i++)
	{
		if (!creature_ptr->inventory_list[i].k_idx) break;
		object_desc(creature_ptr, o_name, &creature_ptr->inventory_list[i], 0);
		fprintf(fff, "%c) %s\n", index_to_label(i), o_name);
	}

	fprintf(fff, "\n\n");
}


/*!
 * @brief 我が家と博物館のオブジェクト情報をファイルにダンプする
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_home_museum(player_type *creature_ptr, FILE *fff)
{
	store_type  *st_ptr;
	st_ptr = &town_info[1].store[STORE_HOME];

	GAME_TEXT o_name[MAX_NLEN];
	if (st_ptr->stock_num)
	{
		fprintf(fff, _("  [我が家のアイテム]\n", "  [Home Inventory]\n"));

		TERM_LEN x = 1;
		for (int i = 0; i < st_ptr->stock_num; i++)
		{
			if ((i % 12) == 0)
				fprintf(fff, _("\n ( %d ページ )\n", "\n ( page %d )\n"), x++);
			object_desc(creature_ptr, o_name, &st_ptr->stock[i], 0);
			fprintf(fff, "%c) %s\n", I2A(i % 12), o_name);
		}

		fprintf(fff, "\n\n");
	}

	st_ptr = &town_info[1].store[STORE_MUSEUM];

	if (st_ptr->stock_num == 0) return;

	fprintf(fff, _("  [博物館のアイテム]\n", "  [Museum]\n"));

	TERM_LEN x = 1;
	for (int i = 0; i < st_ptr->stock_num; i++)
	{
#ifdef JP
		if ((i % 12) == 0) fprintf(fff, "\n ( %d ページ )\n", x++);
		object_desc(creature_ptr, o_name, &st_ptr->stock[i], 0);
		fprintf(fff, "%c) %s\n", I2A(i % 12), o_name);
#else
		if ((i % 12) == 0) fprintf(fff, "\n ( page %d )\n", x++);
		object_desc(creature_ptr, o_name, &st_ptr->stock[i], 0);
		fprintf(fff, "%c) %s\n", I2A(i % 12), o_name);
#endif
	}

	fprintf(fff, "\n\n");
}


/*!
 * @brief ダンプ出力のメインルーチン
 * Output the character dump to a file
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return エラーコード
 */
errr make_character_dump(player_type *creature_ptr, FILE *fff, void(*update_playtime)(void), void(*display_player)(player_type*, int))
{
#ifdef JP
	fprintf(fff, "  [変愚蛮怒 %d.%d.%d キャラクタ情報]\n\n",
		FAKE_VER_MAJOR - 10, FAKE_VER_MINOR, FAKE_VER_PATCH);
#else
	fprintf(fff, "  [Hengband %d.%d.%d Character Dump]\n\n",
		FAKE_VER_MAJOR - 10, FAKE_VER_MINOR, FAKE_VER_PATCH);
#endif
	(*update_playtime)();

	dump_aux_display_player(creature_ptr, fff, display_player);
	dump_aux_last_message(creature_ptr, fff);
	dump_aux_options(fff);
	dump_aux_recall(fff);
	dump_aux_quest(creature_ptr, fff);
	dump_aux_arena(creature_ptr, fff);
	dump_aux_monsters(fff);
	dump_aux_virtues(creature_ptr, fff);
	dump_aux_race_history(creature_ptr, fff);
	dump_aux_realm_history(creature_ptr, fff);
	dump_aux_class_special(creature_ptr, fff);
	dump_aux_mutations(creature_ptr, fff);
	dump_aux_pet(creature_ptr, fff);
	fputs("\n\n", fff);
	dump_aux_equipment_inventory(creature_ptr, fff);
	dump_aux_home_museum(creature_ptr, fff);

	fprintf(fff, _("  [チェックサム: \"%s\"]\n\n", "  [Check Sum: \"%s\"]\n\n"), get_check_sum());
	return 0;
}
