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
#include "info-reader/kind-info-tokens-table.h"
#include "info-reader/race-info-tokens-table.h"
#include "io/files-util.h"
#include "io/tokenizer.h"
#include "main/init.h"
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

dungeon_grid letter[255];

/*!
 * @brief データの可変文字列情報をテキストとして保管する /
 * Add a text to the text-storage and store offset to it.
 * @param offset 文字列保管ポインタからのオフセット
 * @param head テキスト保管ヘッダ情報の構造体参照ポインタ
 * @param buf 保管文字列
 * @param normal_text テキストの正規化を行う
 * @return
 * 無事保管ができたらTRUEを返す。
 * Returns FALSE when there isn't enough space available to store
 * the text.
 */
static bool add_text(u32b *offset, header *head, concptr buf, bool normal_text)
{
	if (head->text_size + strlen(buf) + 8 > FAKE_TEXT_SIZE)
		return FALSE;

	if (*offset == 0)
	{
		*offset = ++head->text_size;
	}
	else if (normal_text)
	{
		/*
		 * If neither the end of the last line nor
		 * the beginning of current line is not a space,
		 * fill up a space as a correct separator of two words.
		 */
		if (head->text_size > 0 &&
#ifdef JP
		(*(head->text_ptr + head->text_size - 1) != ' ') &&
			((head->text_size == 1) || !iskanji(*(head->text_ptr + head->text_size - 2))) &&
			(buf[0] != ' ') && !iskanji(buf[0])
#else
			(*(head->text_ptr + head->text_size - 1) != ' ') &&
			(buf[0] != ' ')
#endif
			)
		{
			*(head->text_ptr + head->text_size) = ' ';
			head->text_size++;
		}
	}

	strcpy(head->text_ptr + head->text_size, buf);
	head->text_size += strlen(buf);
	return TRUE;
}


/*!
 * @brief データの可変文字列情報を名前として保管する /
 * Add a name to the name-storage and return an offset to it.
 * @param offset 文字列保管ポインタからのオフセット
 * @param head テキスト保管ヘッダ情報の構造体参照ポインタ
 * @param buf 保管文字列
 * @return
 * 無事保管ができたらTRUEを返す。
 * Returns FALSE when there isn't enough space available to store
 * the text.
 */
static bool add_name(u32b *offset, header *head, concptr buf)
{
	if (head->name_size + strlen(buf) + 8 > FAKE_NAME_SIZE)
		return FALSE;

	if (*offset == 0)
	{
		*offset = ++head->name_size;
	}

	strcpy(head->name_ptr + head->name_size, buf);
	head->name_size += strlen(buf);
	return TRUE;
}


/*!
 * @brief データの可変文字列情報をタグとして保管する /
 * Add a tag to the tag-storage and return an offset to it.
 * @param offset 文字列保管ポインタからのオフセット
 * @param head テキスト保管ヘッダ情報の構造体参照ポインタ
 * @param buf 保管文字列
 * @return
 * 無事保管ができたらTRUEを返す。
 * Returns FALSE when there isn't enough space available to store
 * the text.
 */
static bool add_tag(STR_OFFSET *offset, header *head, concptr buf)
{
	u32b i;
	for (i = 1; i < head->tag_size; i += strlen(&head->tag_ptr[i]) + 1)
	{
		if (streq(&head->tag_ptr[i], buf)) break;
	}

	if (i >= head->tag_size)
	{
		if (head->tag_size + strlen(buf) + 8 > FAKE_TAG_SIZE)
			return FALSE;

		strcpy(head->tag_ptr + head->tag_size, buf);
		i = head->tag_size;
		head->tag_size += strlen(buf) + 1;
	}

	*offset = (s16b)i;
	return TRUE;
}


/*!
 * @brief パース関数に基づいてデータファイルからデータを読み取る /
 * Initialize an "*_info" array, by parsing an ascii "template" file
 * @param fp 読み取りに使うファイルポインタ
 * @param buf 読み取りに使うバッファ領域
 * @param head ヘッダ構造体
 * @param parse_info_txt_line パース関数
 * @return エラーコード
 */
errr init_info_txt(FILE *fp, char *buf, header *head, parse_info_txt_func parse_info_txt_line)
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
 * @brief Vault情報(v_info)のパース関数 /
 * Initialize the "v_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_v_info(char *buf, header *head)
{
	char *s;
	static vault_type *v_ptr = NULL;

	if (buf[0] == 'N')
	{
		s = my_strchr(buf + 2, ':');
		if (!s) return 1;

		*s++ = '\0';
		if (!*s) return 1;

		int i = atoi(buf + 2);
		if (i <= error_idx) return 4;
		if (i >= head->info_num) return 2;

		error_idx = i;
		v_ptr = &v_info[i];
		if (!add_name(&v_ptr->name, head, s)) return 7;
	}
	else if (!v_ptr) return 3;
	else if (buf[0] == 'D')
	{
		s = buf + 2;
		if (!add_text(&v_ptr->text, head, s, FALSE)) return 7;
	}
	else if (buf[0] == 'X')
	{
		EFFECT_ID typ, rat, hgt, wid;
		if (4 != sscanf(buf + 2, "%d:%d:%d:%d",
			&typ, &rat, &hgt, &wid)) return 1;

		v_ptr->typ = (ROOM_IDX)typ;
		v_ptr->rat = (PROB)rat;
		v_ptr->hgt = (POSITION)hgt;
		v_ptr->wid = (POSITION)wid;
	}
	else
		return 6;

	return 0;
}


/*!
 * @brief 職業技能情報(s_info)のパース関数 /
 * Initialize the "s_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_s_info(char *buf, header *head)
{
	static skill_table *s_ptr = NULL;
	if (buf[0] == 'N')
	{
		int i = atoi(buf + 2);
		if (i <= error_idx) return 4;
		if (i >= head->info_num) return 2;

		error_idx = i;
		s_ptr = &s_info[i];
	}
	else if (!s_ptr)
	{
		return 3;
	}
	else if (buf[0] == 'W')
	{
		int tval, sval, start, max;
		const s16b exp_conv_table[] =
		{
			WEAPON_EXP_UNSKILLED, WEAPON_EXP_BEGINNER, WEAPON_EXP_SKILLED,
			WEAPON_EXP_EXPERT, WEAPON_EXP_MASTER
		};

		if (4 != sscanf(buf + 2, "%d:%d:%d:%d",
			&tval, &sval, &start, &max)) return 1;

		if (start < EXP_LEVEL_UNSKILLED || start > EXP_LEVEL_MASTER
			|| max < EXP_LEVEL_UNSKILLED || max > EXP_LEVEL_MASTER) return 8;

		s_ptr->w_start[tval][sval] = exp_conv_table[start];
		s_ptr->w_max[tval][sval] = exp_conv_table[max];
	}
	else if (buf[0] == 'S')
	{
		int num, start, max;
		if (3 != sscanf(buf + 2, "%d:%d:%d",
			&num, &start, &max)) return 1;

		if (start < WEAPON_EXP_UNSKILLED || start > WEAPON_EXP_MASTER
			|| max < WEAPON_EXP_UNSKILLED || max > WEAPON_EXP_MASTER) return 8;

		s_ptr->s_start[num] = (SUB_EXP)start;
		s_ptr->s_max[num] = (SUB_EXP)max;
	}
	else
		return 6;

	return 0;
}


/*!
 * @brief 職業魔法情報(m_info)のパース関数 /
 * Initialize the "m_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_m_info(char *buf, header *head)
{
	static player_magic *m_ptr = NULL;
	static int realm, magic_idx = 0, readable = 0;

	if (buf[0] == 'N')
	{
		int i = atoi(buf + 2);

		if (i <= error_idx) return 4;
		if (i >= head->info_num) return 2;

		error_idx = i;
		m_ptr = &m_info[i];
	}
	else if (!m_ptr)
	{
		return 3;
	}
	else if (buf[0] == 'I')
	{
		char *book, *stat;
		int xtra, type, first, weight;
		char *s;
		s = my_strchr(buf + 2, ':');

		/* Verify that colon */
		if (!s) return 1;

		/* Nuke the colon, advance to the name */
		*s++ = '\0';

		book = buf + 2;

		if (streq(book, "SORCERY")) m_ptr->spell_book = TV_SORCERY_BOOK;
		else if (streq(book, "LIFE")) m_ptr->spell_book = TV_LIFE_BOOK;
		else if (streq(book, "MUSIC")) m_ptr->spell_book = TV_MUSIC_BOOK;
		else if (streq(book, "HISSATSU")) m_ptr->spell_book = TV_HISSATSU_BOOK;
		else if (streq(book, "NONE")) m_ptr->spell_book = 0;
		else return 5;

		stat = s;
		s = my_strchr(s, ':');
		if (!s) return 1;
		*s++ = '\0';

		if (streq(stat, "STR")) m_ptr->spell_stat = A_STR;
		else if (streq(stat, "INT")) m_ptr->spell_stat = A_INT;
		else if (streq(stat, "WIS")) m_ptr->spell_stat = A_WIS;
		else if (streq(stat, "DEX")) m_ptr->spell_stat = A_DEX;
		else if (streq(stat, "CON")) m_ptr->spell_stat = A_CON;
		else if (streq(stat, "CHR")) m_ptr->spell_stat = A_CHR;
		else return 5;

		if (4 != sscanf(s, "%x:%d:%d:%d",
			(uint *)&xtra, &type, &first, &weight))	return 1;

		m_ptr->spell_xtra = xtra;
		m_ptr->spell_type = type;
		m_ptr->spell_first = first;
		m_ptr->spell_weight = weight;
	}
	else if (buf[0] == 'R')
	{
		if (2 != sscanf(buf + 2, "%d:%d",
			&realm, &readable)) return 1;

		magic_idx = 0;
	}
	else if (buf[0] == 'T')
	{
		int level, mana, fail, exp;

		if (!readable) return 1;
		if (4 != sscanf(buf + 2, "%d:%d:%d:%d",
			&level, &mana, &fail, &exp)) return 1;

		m_ptr->info[realm][magic_idx].slevel = (PLAYER_LEVEL)level;
		m_ptr->info[realm][magic_idx].smana = (MANA_POINT)mana;
		m_ptr->info[realm][magic_idx].sfail = (PERCENTAGE)fail;
		m_ptr->info[realm][magic_idx].sexp = (EXP)exp;
		magic_idx++;
	}
	else
		return 6;

	return 0;
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る(汎用) /
 * Grab one flag from a textual string
 * @param flags ビットフラグを追加する先の参照ポインタ
 * @param names トークン定義配列
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_flag(u32b *flags, concptr names[], concptr what)
{
	for (int i = 0; i < 32; i++)
	{
		if (streq(what, names[i]))
		{
			*flags |= (1L << i);
			return 0;
		}
	}

	return -1;
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る（地形情報向け） /
 * Grab one flag in an feature_type from a textual string
 * @param f_ptr 地形情報を保管する先の構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_feat_flag(feature_type *f_ptr, concptr what)
{
	for (int i = 0; i < FF_FLAG_MAX; i++)
	{
		if (streq(what, f_info_flags[i]))
		{
			add_flag(f_ptr->flags, i);
			return 0;
		}
	}

	msg_format(_("未知の地形フラグ '%s'。", "Unknown feature flag '%s'."), what);
	return PARSE_ERROR_GENERIC;
}


/*!
 * @brief テキストトークンを走査してフラグ(ステート)を一つ得る（地形情報向け2） /
 * Grab an action in an feature_type from a textual string
 * @param f_ptr 地形情報を保管する先の構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @param count ステートの保存先ID
 * @return エラーコード
 */
static errr grab_one_feat_action(feature_type *f_ptr, concptr what, int count)
{
	for (FF_FLAGS_IDX i = 0; i < FF_FLAG_MAX; i++)
	{
		if (streq(what, f_info_flags[i]))
		{
			f_ptr->state[count].action = i;
			return 0;
		}
	}

	msg_format(_("未知の地形アクション '%s'。", "Unknown feature action '%s'."), what);
	return PARSE_ERROR_GENERIC;
}


/*!
 * @brief 地形情報(f_info)のパース関数 /
 * Initialize the "f_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_f_info(char *buf, header *head)
{
	static feature_type *f_ptr = NULL;
	int i;
	char *s, *t;
	if (buf[0] == 'N')
	{
		s = my_strchr(buf + 2, ':');

		if (s)
		{
			*s++ = '\0';
		}

		i = atoi(buf + 2);
		if (i <= error_idx) return 4;
		if (i >= head->info_num) return 2;

		error_idx = i;
		f_ptr = &f_info[i];
		if (s)
		{
			if (!add_tag(&f_ptr->tag, head, s)) return 7;
		}

		f_ptr->mimic = (FEAT_IDX)i;
		f_ptr->destroyed = (FEAT_IDX)i;
		for (i = 0; i < MAX_FEAT_STATES; i++)
			f_ptr->state[i].action = FF_FLAG_MAX;
	}
	else if (!f_ptr)
	{
		return 3;
	}
#ifdef JP
	else if (buf[0] == 'J')
	{
		if (!add_name(&f_ptr->name, head, buf + 2)) return 7;
	}
	else if (buf[0] == 'E')
	{
	}
#else
	else if (buf[0] == 'J')
	{
	}
	else if (buf[0] == 'E')
	{
		s = buf + 2;
		if (!add_name(&f_ptr->name, head, s)) return 7;
	}
#endif
	else if (buf[0] == 'M')
	{
		STR_OFFSET offset;
		if (!add_tag(&offset, head, buf + 2))
			return PARSE_ERROR_OUT_OF_MEMORY;

		f_ptr->mimic_tag = offset;
	}
	else if (buf[0] == 'G')
	{
		int j;
		byte s_attr;
		char char_tmp[F_LIT_MAX];
		if (buf[1] != ':') return 1;
		if (!buf[2]) return 1;
		if (buf[3] != ':') return 1;
		if (!buf[4]) return 1;

		char_tmp[F_LIT_STANDARD] = buf[2];
		s_attr = color_char_to_attr(buf[4]);
		if (s_attr > 127) return 1;

		f_ptr->d_attr[F_LIT_STANDARD] = s_attr;
		f_ptr->d_char[F_LIT_STANDARD] = char_tmp[F_LIT_STANDARD];
		if (buf[5] == ':')
		{
			apply_default_feat_lighting(f_ptr->d_attr, f_ptr->d_char);
			if (!streq(buf + 6, "LIT"))
			{
				char attr_lite_tmp[F_LIT_MAX - F_LIT_NS_BEGIN];

				if ((F_LIT_MAX - F_LIT_NS_BEGIN) * 2 != sscanf(buf + 6, "%c:%c:%c:%c",
					&char_tmp[F_LIT_LITE], &attr_lite_tmp[F_LIT_LITE - F_LIT_NS_BEGIN],
					&char_tmp[F_LIT_DARK], &attr_lite_tmp[F_LIT_DARK - F_LIT_NS_BEGIN])) return 1;
				if (buf[F_LIT_MAX * 4 + 1]) return 1;

				for (j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++)
				{
					switch (attr_lite_tmp[j - F_LIT_NS_BEGIN])
					{
					case '*':
						/* Use default lighting */
						break;
					case '-':
						/* No lighting support */
						f_ptr->d_attr[j] = f_ptr->d_attr[F_LIT_STANDARD];
						break;
					default:
						/* Extract the color */
						f_ptr->d_attr[j] = color_char_to_attr(attr_lite_tmp[j - F_LIT_NS_BEGIN]);
						if (f_ptr->d_attr[j] > 127) return 1;
						break;
					}
					f_ptr->d_char[j] = char_tmp[j];
				}
			}
		}
		else if (!buf[5])
		{
			for (j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++)
			{
				f_ptr->d_attr[j] = s_attr;
				f_ptr->d_char[j] = char_tmp[F_LIT_STANDARD];
			}
		}
		else return 1;
	}
	else if (buf[0] == 'F')
	{
		for (s = buf + 2; *s; )
		{
			/* loop */
			for (t = s; *t && (*t != ' ') && (*t != '|'); ++t);

			if (*t)
			{
				*t++ = '\0';
				while (*t == ' ' || *t == '|') t++;
			}

			if (1 == sscanf(s, "SUBTYPE_%d", &i))
			{
				f_ptr->subtype = (FEAT_SUBTYPE)i;
				s = t;

				continue;
			}

			if (1 == sscanf(s, "POWER_%d", &i))
			{
				f_ptr->power = (FEAT_POWER)i;
				s = t;
				continue;
			}

			if (0 != grab_one_feat_flag(f_ptr, s))
				return (PARSE_ERROR_INVALID_FLAG);

			s = t;
		}
	}
	else if (buf[0] == 'W')
	{
		int priority;
		if (1 != sscanf(buf + 2, "%d", &priority))
			return (PARSE_ERROR_GENERIC);
		f_ptr->priority = (FEAT_PRIORITY)priority;
	}
	else if (buf[0] == 'K')
	{
		STR_OFFSET offset;
		for (i = 0; i < MAX_FEAT_STATES; i++)
			if (f_ptr->state[i].action == FF_FLAG_MAX) break;

		if (i == MAX_FEAT_STATES) return PARSE_ERROR_GENERIC;

		/* loop */
		for (s = t = buf + 2; *t && (*t != ':'); t++);

		if (*t == ':') *t++ = '\0';

		if (streq(s, "DESTROYED"))
		{
			if (!add_tag(&offset, head, t)) return PARSE_ERROR_OUT_OF_MEMORY;

			f_ptr->destroyed_tag = offset;
		}
		else
		{
			f_ptr->state[i].action = 0;
			if (0 != grab_one_feat_action(f_ptr, s, i)) return PARSE_ERROR_INVALID_FLAG;
			if (!add_tag(&offset, head, t)) return PARSE_ERROR_OUT_OF_MEMORY;

			f_ptr->state[i].result_tag = offset;
		}
	}
	else
	{
		return 6;
	}

	return 0;
}


/*!
 * @brief 地形タグからIDを得る /
 * Convert a fake tag to a real feat index
 * @param str タグ文字列
 * @return 地形ID
 */
s16b f_tag_to_index(concptr str)
{
	for (u16b i = 0; i < f_head.info_num; i++)
	{
		if (streq(f_tag + f_info[i].tag, str))
		{
			return (s16b)i;
		}
	}

	return -1;
}


/*!
 * @brief 地形タグからIDを得る /
 * Search for real index corresponding to this fake tag
 * @param feat タグ文字列のオフセット
 * @return 地形ID。該当がないなら-1
 */
static FEAT_IDX search_real_feat(STR_OFFSET feat)
{
	if (feat <= 0)
	{
		return -1;
	}

	for (FEAT_IDX i = 0; i < f_head.info_num; i++)
	{
		if (feat == f_info[i].tag)
		{
			return i;
		}
	}

	msg_format(_("未定義のタグ '%s'。", "%s is undefined."), f_tag + feat);
	return -1;
}


/*!
 * @brief 地形情報の各種タグからIDへ変換して結果を収める /
 * Retouch fake tags of f_info
 * @param head ヘッダ構造体
 * @return なし
 */
void retouch_f_info(header *head)
{
	for (int i = 0; i < head->info_num; i++)
	{
		feature_type *f_ptr = &f_info[i];
		FEAT_IDX k = search_real_feat(f_ptr->mimic_tag);
		f_ptr->mimic = k < 0 ? f_ptr->mimic : k;
		k = search_real_feat(f_ptr->destroyed_tag);
		f_ptr->destroyed = k < 0 ? f_ptr->destroyed : k;
		for (FEAT_IDX j = 0; j < MAX_FEAT_STATES; j++)
		{
			k = search_real_feat(f_ptr->state[j].result_tag);
			f_ptr->state[j].result = k < 0 ? f_ptr->state[j].result : k;
		}
	}
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る(ベースアイテム用) /
 * Grab one flag in an object_kind from a textual string
 * @param k_ptr 保管先のベースアイテム構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_kind_flag(object_kind *k_ptr, concptr what)
{
	for (int i = 0; i < TR_FLAG_MAX; i++)
	{
		if (streq(what, k_info_flags[i]))
		{
			add_flag(k_ptr->flags, i);
			return 0;
		}
	}

	if (grab_one_flag(&k_ptr->gen_flags, k_info_gen_flags, what) == 0)
		return 0;

	msg_format(_("未知のアイテム・フラグ '%s'。", "Unknown object flag '%s'."), what);
	return 1;
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る(発動能力用) /
 * Grab one activation index flag
 * @param what 参照元の文字列ポインタ
 * @return 発動能力ID
 */
static byte grab_one_activation_flag(concptr what)
{
	for (int i = 0; ; i++)
	{
		if (activation_info[i].flag == NULL) break;

		if (streq(what, activation_info[i].flag))
		{
			return activation_info[i].index;
		}
	}

	int j = atoi(what);
	if (j > 0)
	{
		return ((byte)j);
	}

	msg_format(_("未知の発動・フラグ '%s'。", "Unknown activation flag '%s'."), what);
	return 0;
}


/*!
 * @brief ベースアイテム(k_info)のパース関数 /
 * Initialize the "k_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_k_info(char *buf, header *head)
{
	static object_kind *k_ptr = NULL;

	char *s, *t;
	if (buf[0] == 'N')
	{
#ifdef JP
		char *flavor;
#endif
		s = my_strchr(buf + 2, ':');
		if (!s) return 1;

		*s++ = '\0';
		int i = atoi(buf + 2);

		if (i <= error_idx) return 4;
		if (i >= head->info_num) return 2;

		error_idx = i;
		k_ptr = &k_info[i];

#ifdef JP
		if (!*s) return 1;

		flavor = my_strchr(s, ':');
		if (flavor)
		{
			*flavor++ = '\0';
			if (!add_name(&k_ptr->flavor_name, head, flavor)) return 7;
		}

		if (!add_name(&k_ptr->name, head, s)) return 7;
#endif
	}
	else if (!k_ptr)
	{
		return 3;
	}
#ifdef JP
	/* 英語名を読むルーチンを追加 */
	/* 'E' から始まる行は英語名としている */
	else if (buf[0] == 'E')
	{
		/* nothing to do */
	}
#else
	else if (buf[0] == 'E')
	{
		char *flavor;
		s = buf + 2;
		flavor = my_strchr(s, ':');
		if (flavor)
		{
			*flavor++ = '\0';
			if (!add_name(&k_ptr->flavor_name, head, flavor)) return 7;
		}

		if (!add_name(&k_ptr->name, head, s)) return 7;
	}
#endif
	else if (buf[0] == 'D')
	{
#ifdef JP
		if (buf[2] == '$')
			return 0;
		s = buf + 2;
#else
		if (buf[2] != '$')
			return 0;
		s = buf + 3;
#endif
		if (!add_text(&k_ptr->text, head, s, TRUE)) return 7;
	}
	else if (buf[0] == 'G')
	{
		char sym;
		byte tmp;
		if (buf[1] != ':') return 1;
		if (!buf[2]) return 1;
		if (buf[3] != ':') return 1;
		if (!buf[4]) return 1;

		sym = buf[2];
		tmp = color_char_to_attr(buf[4]);
		if (tmp > 127) return 1;

		k_ptr->d_attr = tmp;
		k_ptr->d_char = sym;
	}
	else if (buf[0] == 'I')
	{
		int tval, sval, pval;
		if (3 != sscanf(buf + 2, "%d:%d:%d",
			&tval, &sval, &pval)) return 1;

		k_ptr->tval = (tval_type)tval;
		k_ptr->sval = (OBJECT_SUBTYPE_VALUE)sval;
		k_ptr->pval = (PARAMETER_VALUE)pval;
	}
	else if (buf[0] == 'W')
	{
		int level, extra, wgt;
		long cost;
		if (4 != sscanf(buf + 2, "%d:%d:%d:%ld",
			&level, &extra, &wgt, &cost)) return 1;

		k_ptr->level = (DEPTH)level;
		k_ptr->extra = (BIT_FLAGS8)extra;
		k_ptr->weight = (WEIGHT)wgt;
		k_ptr->cost = (PRICE)cost;
	}
	else if (buf[0] == 'A')
	{
		int i = 0;
		for (s = buf + 1; s && (s[0] == ':') && s[1]; ++i)
		{
			k_ptr->chance[i] = 1;
			k_ptr->locale[i] = atoi(s + 1);
			t = my_strchr(s + 1, '/');
			s = my_strchr(s + 1, ':');
			if (t && (!s || t < s))
			{
				int chance = atoi(t + 1);
				if (chance > 0) k_ptr->chance[i] = (PROB)chance;
			}
		}
	}
	else if (buf[0] == 'P')
	{
		int ac, hd1, hd2, th, td, ta;
		if (6 != sscanf(buf + 2, "%d:%dd%d:%d:%d:%d",
			&ac, &hd1, &hd2, &th, &td, &ta)) return 1;

		k_ptr->ac = (ARMOUR_CLASS)ac;
		k_ptr->dd = (DICE_NUMBER)hd1;
		k_ptr->ds = (DICE_SID)hd2;
		k_ptr->to_h = (HIT_PROB)th;
		k_ptr->to_d = (HIT_POINT)td;
		k_ptr->to_a = (ARMOUR_CLASS)ta;
	}
	else if (buf[0] == 'U')
	{
		byte n;
		n = grab_one_activation_flag(buf + 2);
		if (n > 0)
		{
			k_ptr->act_idx = n;
		}
		else
		{
			return 5;
		}
	}
	else if (buf[0] == 'F')
	{
		for (s = buf + 2; *s; )
		{
			/* loop */
			for (t = s; *t && (*t != ' ') && (*t != '|'); ++t);

			if (*t)
			{
				*t++ = '\0';
				while (*t == ' ' || *t == '|') t++;
			}

			if (0 != grab_one_kind_flag(k_ptr, s)) return 5;
			s = t;
		}
	}
	else
	{
		return 6;
	}

	return 0;
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る(アーティファクト用) /
 * Grab one activation index flag
 * @param a_ptr 保管先のアーティファクト構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーがあった場合1、エラーがない場合0を返す
 */
static errr grab_one_artifact_flag(artifact_type *a_ptr, concptr what)
{
	for (int i = 0; i < TR_FLAG_MAX; i++)
	{
		if (streq(what, k_info_flags[i]))
		{
			add_flag(a_ptr->flags, i);
			return 0;
		}
	}

	if (grab_one_flag(&a_ptr->gen_flags, k_info_gen_flags, what) == 0)
		return 0;

	msg_format(_("未知の伝説のアイテム・フラグ '%s'。", "Unknown artifact flag '%s'."), what);
	return 1;
}


/*!
 * @brief 固定アーティファクト情報(a_info)のパース関数 /
 * Initialize the "a_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_a_info(char *buf, header *head)
{
	static artifact_type *a_ptr = NULL;
	char *s, *t;
	if (buf[0] == 'N')
	{
		s = my_strchr(buf + 2, ':');
		if (!s) return 1;

		*s++ = '\0';
#ifdef JP
		if (!*s) return 1;
#endif
		int i = atoi(buf + 2);
		if (i < error_idx) return 4;
		if (i >= head->info_num) return 2;

		error_idx = i;
		a_ptr = &a_info[i];
		add_flag(a_ptr->flags, TR_IGNORE_ACID);
		add_flag(a_ptr->flags, TR_IGNORE_ELEC);
		add_flag(a_ptr->flags, TR_IGNORE_FIRE);
		add_flag(a_ptr->flags, TR_IGNORE_COLD);
#ifdef JP
		if (!add_name(&a_ptr->name, head, s)) return 7;
#endif
	}
	else if (!a_ptr)
	{
		return 3;
	}
#ifdef JP
	/* 英語名を読むルーチンを追加 */
	/* 'E' から始まる行は英語名としている */
	else if (buf[0] == 'E')
	{
		/* nothing to do */
	}
#else
	else if (buf[0] == 'E')
	{
		s = buf + 2;
		if (!add_name(&a_ptr->name, head, s)) return 7;
	}
#endif
	else if (buf[0] == 'D')
	{
#ifdef JP
		if (buf[2] == '$')
			return 0;
		s = buf + 2;
#else
		if (buf[2] != '$')
			return 0;
		s = buf + 3;
#endif
		if (!add_text(&a_ptr->text, head, s, TRUE)) return 7;
	}
	else if (buf[0] == 'I')
	{
		int tval, sval, pval;
		if (3 != sscanf(buf + 2, "%d:%d:%d",
			&tval, &sval, &pval)) return 1;

		a_ptr->tval = (tval_type)tval;
		a_ptr->sval = (OBJECT_SUBTYPE_VALUE)sval;
		a_ptr->pval = (PARAMETER_VALUE)pval;
	}
	else if (buf[0] == 'W')
	{
		int level, rarity, wgt;
		long cost;
		if (4 != sscanf(buf + 2, "%d:%d:%d:%ld",
			&level, &rarity, &wgt, &cost)) return 1;

		a_ptr->level = (DEPTH)level;
		a_ptr->rarity = (RARITY)rarity;
		a_ptr->weight = (WEIGHT)wgt;
		a_ptr->cost = (PRICE)cost;
	}
	else if (buf[0] == 'P')
	{
		int ac, hd1, hd2, th, td, ta;
		if (6 != sscanf(buf + 2, "%d:%dd%d:%d:%d:%d",
			&ac, &hd1, &hd2, &th, &td, &ta)) return 1;

		a_ptr->ac = (ARMOUR_CLASS)ac;
		a_ptr->dd = (DICE_NUMBER)hd1;
		a_ptr->ds = (DICE_SID)hd2;
		a_ptr->to_h = (HIT_PROB)th;
		a_ptr->to_d = (HIT_POINT)td;
		a_ptr->to_a = (ARMOUR_CLASS)ta;
	}
	else if (buf[0] == 'U')
	{
		byte n;
		n = grab_one_activation_flag(buf + 2);
		if (n > 0)
		{
			a_ptr->act_idx = n;
		}
		else
		{
			return 5;
		}
	}
	else if (buf[0] == 'F')
	{
		for (s = buf + 2; *s; )
		{
			/* loop */
			for (t = s; *t && (*t != ' ') && (*t != '|'); ++t);

			if (*t)
			{
				*t++ = '\0';
				while ((*t == ' ') || (*t == '|')) t++;
			}

			if (0 != grab_one_artifact_flag(a_ptr, s)) return 5;

			s = t;
		}
	}
	else
	{
		return 6;
	}

	return 0;
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る(アーティファクト用) /
 * Grab one flag in a ego-item_type from a textual string
 * @param e_ptr 保管先のエゴ構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーがあった場合1、エラーがない場合0を返す
 */
static bool grab_one_ego_item_flag(ego_item_type *e_ptr, concptr what)
{
	for (int i = 0; i < TR_FLAG_MAX; i++)
	{
		if (streq(what, k_info_flags[i]))
		{
			add_flag(e_ptr->flags, i);
			return 0;
		}
	}

	if (grab_one_flag(&e_ptr->gen_flags, k_info_gen_flags, what) == 0)
		return 0;

	msg_format(_("未知の名のあるアイテム・フラグ '%s'。", "Unknown ego-item flag '%s'."), what);
	return 1;
}


/*!
 * @brief アイテムエゴ情報(e_info)のパース関数 /
 * Initialize the "e_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_e_info(char *buf, header *head)
{
	static ego_item_type *e_ptr = NULL;
	error_idx = -1;
	error_line = -1;
	char *s, *t;
	if (buf[0] == 'N')
	{
		s = my_strchr(buf + 2, ':');
		if (!s) return 1;

		*s++ = '\0';
#ifdef JP
		if (!*s) return 1;
#endif
		int i = atoi(buf + 2);
		if (i < error_idx) return 4;
		if (i >= head->info_num) return 2;

		error_idx = i;
		e_ptr = &e_info[i];
#ifdef JP
		if (!add_name(&e_ptr->name, head, s)) return 7;
#endif
	}
	else if (!e_ptr)
	{
		return 3;
	}
#ifdef JP
	/* 英語名を読むルーチンを追加 */
	/* 'E' から始まる行は英語名 */
	else if (buf[0] == 'E')
	{
		/* nothing to do */
	}
#else
	else if (buf[0] == 'E')
	{
		s = buf + 2;
		if (!add_name(&e_ptr->name, head, s)) return 7;
	}
#endif
	else if (buf[0] == 'X')
	{
		int slot, rating;
		if (2 != sscanf(buf + 2, "%d:%d",
			&slot, &rating)) return 1;

		e_ptr->slot = (INVENTORY_IDX)slot;
		e_ptr->rating = (PRICE)rating;
	}
	else if (buf[0] == 'W')
	{
		int level, rarity, pad2;
		long cost;

		if (4 != sscanf(buf + 2, "%d:%d:%d:%ld",
			&level, &rarity, &pad2, &cost)) return 1;

		e_ptr->level = level;
		e_ptr->rarity = (RARITY)rarity;
		e_ptr->cost = cost;
	}
	else if (buf[0] == 'C')
	{
		int th, td, ta, pval;

		if (4 != sscanf(buf + 2, "%d:%d:%d:%d",
			&th, &td, &ta, &pval)) return 1;

		e_ptr->max_to_h = (HIT_PROB)th;
		e_ptr->max_to_d = (HIT_POINT)td;
		e_ptr->max_to_a = (ARMOUR_CLASS)ta;
		e_ptr->max_pval = (PARAMETER_VALUE)pval;
	}
	else if (buf[0] == 'U')
	{
		byte n;
		n = grab_one_activation_flag(buf + 2);
		if (n > 0)
		{
			e_ptr->act_idx = n;
		}
		else
		{
			return 5;
		}
	}
	else if (buf[0] == 'F')
	{
		for (s = buf + 2; *s; )
		{
			/* loop */
			for (t = s; *t && (*t != ' ') && (*t != '|'); ++t);

			if (*t)
			{
				*t++ = '\0';
				while ((*t == ' ') || (*t == '|')) t++;
			}

			if (0 != grab_one_ego_item_flag(e_ptr, s)) return 5;

			s = t;
		}
	}
	else
	{
		return 6;
	}

	return 0;
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスター用1) /
 * Grab one (basic) flag in a monster_race from a textual string
 * @param r_ptr 保管先のモンスター種族構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_basic_flag(monster_race *r_ptr, concptr what)
{
	if (grab_one_flag(&r_ptr->flags1, r_info_flags1, what) == 0)
		return 0;

	if (grab_one_flag(&r_ptr->flags2, r_info_flags2, what) == 0)
		return 0;

	if (grab_one_flag(&r_ptr->flags3, r_info_flags3, what) == 0)
		return 0;

	if (grab_one_flag(&r_ptr->flags7, r_info_flags7, what) == 0)
		return 0;

	if (grab_one_flag(&r_ptr->flags8, r_info_flags8, what) == 0)
		return 0;

	if (grab_one_flag(&r_ptr->flags9, r_info_flags9, what) == 0)
		return 0;

	if (grab_one_flag(&r_ptr->flagsr, r_info_flagsr, what) == 0)
		return 0;

	msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what);
	return 1;
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスター用2) /
 * Grab one (spell) flag in a monster_race from a textual string
 * @param r_ptr 保管先のモンスター種族構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_spell_flag(monster_race *r_ptr, concptr what)
{
	if (grab_one_flag(&r_ptr->flags4, r_info_flags4, what) == 0)
		return 0;

	if (grab_one_flag(&r_ptr->a_ability_flags1, r_a_ability_flags1, what) == 0)
		return 0;

	if (grab_one_flag(&r_ptr->a_ability_flags2, r_a_ability_flags2, what) == 0)
		return 0;

	msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what);
	return 1;
}


/*!
 * @brief モンスター種族情報(r_info)のパース関数 /
 * Initialize the "r_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_r_info(char *buf, header *head)
{
	static monster_race *r_ptr = NULL;
	char *s, *t;
	if (buf[0] == 'N')
	{
		s = my_strchr(buf + 2, ':');
		if (!s) return 1;

		*s++ = '\0';
#ifdef JP
		if (!*s) return 1;
#endif

		int i = atoi(buf + 2);
		if (i < error_idx) return 4;
		if (i >= head->info_num) return 2;

		error_idx = i;
		r_ptr = &r_info[i];
#ifdef JP
		if (!add_name(&r_ptr->name, head, s)) return 7;
#endif
	}
	else if (!r_ptr)
	{
		return 3;
	}
#ifdef JP
	/* 英語名を読むルーチンを追加 */
	/* 'E' から始まる行は英語名 */
	else if (buf[0] == 'E')
	{
		s = buf + 2;
		if (!add_name(&r_ptr->E_name, head, s)) return 7;
	}
#else
	else if (buf[0] == 'E')
	{
		s = buf + 2;
		if (!add_name(&r_ptr->name, head, s)) return 7;
	}
#endif
	else if (buf[0] == 'D')
	{
#ifdef JP
		if (buf[2] == '$')
			return 0;
		s = buf + 2;
#else
		if (buf[2] != '$')
			return 0;
		s = buf + 3;
#endif
		if (!add_text(&r_ptr->text, head, s, TRUE)) return 7;
	}
	else if (buf[0] == 'G')
	{
		if (buf[1] != ':') return 1;
		if (!buf[2]) return 1;
		if (buf[3] != ':') return 1;
		if (!buf[4]) return 1;

		char sym = buf[2];
		byte tmp = color_char_to_attr(buf[4]);
		if (tmp > 127) return 1;

		r_ptr->d_char = sym;
		r_ptr->d_attr = tmp;
	}
	else if (buf[0] == 'I')
	{
		int spd, hp1, hp2, aaf, ac, slp;

		if (6 != sscanf(buf + 2, "%d:%dd%d:%d:%d:%d",
			&spd, &hp1, &hp2, &aaf, &ac, &slp)) return 1;

		r_ptr->speed = (SPEED)spd;
		r_ptr->hdice = (DICE_NUMBER)MAX(hp1, 1);
		r_ptr->hside = (DICE_SID)MAX(hp2, 1);
		r_ptr->aaf = (POSITION)aaf;
		r_ptr->ac = (ARMOUR_CLASS)ac;
		r_ptr->sleep = (SLEEP_DEGREE)slp;
	}
	else if (buf[0] == 'W')
	{
		int lev, rar, pad;
		long exp;
		long nextexp;
		int nextmon;
		if (6 != sscanf(buf + 2, "%d:%d:%d:%ld:%ld:%d",
			&lev, &rar, &pad, &exp, &nextexp, &nextmon)) return 1;

		r_ptr->level = (DEPTH)lev;
		r_ptr->rarity = (RARITY)rar;
		r_ptr->extra = (BIT_FLAGS16)pad;
		r_ptr->mexp = (EXP)exp;
		r_ptr->next_exp = (EXP)nextexp;
		r_ptr->next_r_idx = (MONRACE_IDX)nextmon;
	}
	else if (buf[0] == 'R')
	{
		int id, ds, dd;
		int i = 0;
		for (; i < A_MAX; i++) if (r_ptr->reinforce_id[i] == 0) break;

		if (i == 6) return 1;

		if (3 != sscanf(buf + 2, "%d:%dd%d", &id, &dd, &ds)) return 1;
		r_ptr->reinforce_id[i] = (MONRACE_IDX)id;
		r_ptr->reinforce_dd[i] = (DICE_NUMBER)dd;
		r_ptr->reinforce_ds[i] = (DICE_SID)ds;
	}
	else if (buf[0] == 'B')
	{
		int n1, n2;
		int i = 0;
		for (i = 0; i < 4; i++) if (!r_ptr->blow[i].method) break;

		if (i == 4) return 1;

		/* loop */
		for (s = t = buf + 2; *t && (*t != ':'); t++);

		if (*t == ':') *t++ = '\0';

		for (n1 = 0; r_info_blow_method[n1]; n1++)
		{
			if (streq(s, r_info_blow_method[n1])) break;
		}

		if (!r_info_blow_method[n1]) return 1;

		/* loop */
		for (s = t; *t && (*t != ':'); t++);

		if (*t == ':') *t++ = '\0';

		for (n2 = 0; r_info_blow_effect[n2]; n2++)
		{
			if (streq(s, r_info_blow_effect[n2])) break;
		}

		if (!r_info_blow_effect[n2]) return 1;

		/* loop */
		for (s = t; *t && (*t != 'd'); t++);

		if (*t == 'd') *t++ = '\0';

		r_ptr->blow[i].method = (rbm_type)n1;
		r_ptr->blow[i].effect = (rbe_type)n2;
		r_ptr->blow[i].d_dice = atoi(s);
		r_ptr->blow[i].d_side = atoi(t);
	}
	else if (buf[0] == 'F')
	{
		for (s = buf + 2; *s; )
		{
			/* loop */
			for (t = s; *t && (*t != ' ') && (*t != '|'); ++t);

			if (*t)
			{
				*t++ = '\0';
				while (*t == ' ' || *t == '|') t++;
			}

			if (0 != grab_one_basic_flag(r_ptr, s)) return 5;

			s = t;
		}
	}
	else if (buf[0] == 'S')
	{
		for (s = buf + 2; *s; )
		{

			/* loop */
			for (t = s; *t && (*t != ' ') && (*t != '|'); ++t);

			if (*t)
			{
				*t++ = '\0';
				while ((*t == ' ') || (*t == '|')) t++;
			}

			int i;
			if (1 == sscanf(s, "1_IN_%d", &i))
			{
				r_ptr->freq_spell = 100 / i;
				s = t;
				continue;
			}

			if (0 != grab_one_spell_flag(r_ptr, s)) return 5;

			s = t;
		}
	}
	else if (buf[0] == 'A')
	{
		int id, per, rarity;
		int i = 0;
		for (i = 0; i < 4; i++) if (!r_ptr->artifact_id[i]) break;

		if (i == 4) return 1;

		if (3 != sscanf(buf + 2, "%d:%d:%d", &id, &rarity, &per)) return 1;
		r_ptr->artifact_id[i] = (ARTIFACT_IDX)id;
		r_ptr->artifact_rarity[i] = (RARITY)rarity;
		r_ptr->artifact_percent[i] = (PERCENTAGE)per;
	}
	else if (buf[0] == 'V')
	{
		int val;
		if (3 != sscanf(buf + 2, "%d", &val)) return 1;
		r_ptr->arena_ratio = (PERCENTAGE)val;
	}
	else
	{
		return 6;
	}

	return 0;
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る(ダンジョン用) /
 * Grab one flag for a dungeon type from a textual string
 * @param d_ptr 保管先のダンジョン構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_dungeon_flag(dungeon_type *d_ptr, concptr what)
{
	if (grab_one_flag(&d_ptr->flags1, d_info_flags1, what) == 0)
		return 0;

	msg_format(_("未知のダンジョン・フラグ '%s'。", "Unknown dungeon type flag '%s'."), what);
	return 1;
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用1) /
 * Grab one (basic) flag in a monster_race from a textual string
 * @param d_ptr 保管先のダンジョン構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_basic_monster_flag(dungeon_type *d_ptr, concptr what)
{
	if (grab_one_flag(&d_ptr->mflags1, r_info_flags1, what) == 0)
		return 0;

	if (grab_one_flag(&d_ptr->mflags2, r_info_flags2, what) == 0)
		return 0;

	if (grab_one_flag(&d_ptr->mflags3, r_info_flags3, what) == 0)
		return 0;

	if (grab_one_flag(&d_ptr->mflags7, r_info_flags7, what) == 0)
		return 0;

	if (grab_one_flag(&d_ptr->mflags8, r_info_flags8, what) == 0)
		return 0;

	if (grab_one_flag(&d_ptr->mflags9, r_info_flags9, what) == 0)
		return 0;

	if (grab_one_flag(&d_ptr->mflagsr, r_info_flagsr, what) == 0)
		return 0;

	msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what);
	return 1;
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用2) /
 * Grab one (spell) flag in a monster_race from a textual string
 * @param d_ptr 保管先のダンジョン構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_spell_monster_flag(dungeon_type *d_ptr, concptr what)
{
	if (grab_one_flag(&d_ptr->mflags4, r_info_flags4, what) == 0)
		return 0;

	if (grab_one_flag(&d_ptr->m_a_ability_flags1, r_a_ability_flags1, what) == 0)
		return 0;

	if (grab_one_flag(&d_ptr->m_a_ability_flags2, r_a_ability_flags2, what) == 0)
		return 0;

	msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what);
	return 1;
}


/*!
 * @brief ダンジョン情報(d_info)のパース関数 /
 * Initialize the "d_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_d_info(char *buf, header *head)
{
	static dungeon_type *d_ptr = NULL;
	char *s, *t;
	if (buf[0] == 'N')
	{
		s = my_strchr(buf + 2, ':');
		if (!s) return 1;

		*s++ = '\0';
#ifdef JP
		if (!*s) return 1;
#endif

		int i = atoi(buf + 2);
		if (i < error_idx) return 4;
		if (i >= head->info_num) return 2;

		error_idx = i;
		d_ptr = &d_info[i];
#ifdef JP
		if (!add_name(&d_ptr->name, head, s)) return 7;
#endif
	}
#ifdef JP
	else if (buf[0] == 'E') return 0;
#else
	else if (buf[0] == 'E')
	{
		/* Acquire the Text */
		s = buf + 2;

		/* Store the name */
		if (!add_name(&d_ptr->name, head, s)) return 7;
	}
#endif
	else if (buf[0] == 'D')
	{
#ifdef JP
		if (buf[2] == '$')
			return 0;
		s = buf + 2;
#else
		if (buf[2] != '$')
			return 0;
		s = buf + 3;
#endif
		if (!add_text(&d_ptr->text, head, s, TRUE)) return 7;
	}
	else if (buf[0] == 'W')
	{
		int min_lev, max_lev;
		int min_plev, mode;
		int min_alloc, max_chance;
		int obj_good, obj_great;
		int pit, nest;

		if (10 != sscanf(buf + 2, "%d:%d:%d:%d:%d:%d:%d:%d:%x:%x",
			&min_lev, &max_lev, &min_plev, &mode, &min_alloc, &max_chance, &obj_good, &obj_great, (unsigned int *)&pit, (unsigned int *)&nest)) return 1;

		d_ptr->mindepth = (DEPTH)min_lev;
		d_ptr->maxdepth = (DEPTH)max_lev;
		d_ptr->min_plev = (PLAYER_LEVEL)min_plev;
		d_ptr->mode = (BIT_FLAGS8)mode;
		d_ptr->min_m_alloc_level = min_alloc;
		d_ptr->max_m_alloc_chance = max_chance;
		d_ptr->obj_good = obj_good;
		d_ptr->obj_great = obj_great;
		d_ptr->pit = (BIT_FLAGS16)pit;
		d_ptr->nest = (BIT_FLAGS16)nest;
	}
	else if (buf[0] == 'P')
	{
		int dy, dx;
		if (2 != sscanf(buf + 2, "%d:%d", &dy, &dx)) return 1;

		d_ptr->dy = dy;
		d_ptr->dx = dx;
	}
	else if (buf[0] == 'L')
	{
		char *zz[16];
		if (tokenize(buf + 2, DUNGEON_FEAT_PROB_NUM * 2 + 1, zz, 0) != (DUNGEON_FEAT_PROB_NUM * 2 + 1)) return 1;

		for (int i = 0; i < DUNGEON_FEAT_PROB_NUM; i++)
		{
			d_ptr->floor[i].feat = f_tag_to_index(zz[i * 2]);
			if (d_ptr->floor[i].feat < 0) return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

			d_ptr->floor[i].percent = (PERCENTAGE)atoi(zz[i * 2 + 1]);
		}

		d_ptr->tunnel_percent = atoi(zz[DUNGEON_FEAT_PROB_NUM * 2]);
	}
	else if (buf[0] == 'A')
	{
		char *zz[16];
		if (tokenize(buf + 2, DUNGEON_FEAT_PROB_NUM * 2 + 4, zz, 0) != (DUNGEON_FEAT_PROB_NUM * 2 + 4)) return 1;

		for (int i = 0; i < DUNGEON_FEAT_PROB_NUM; i++)
		{
			d_ptr->fill[i].feat = f_tag_to_index(zz[i * 2]);
			if (d_ptr->fill[i].feat < 0) return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

			d_ptr->fill[i].percent = (PERCENTAGE)atoi(zz[i * 2 + 1]);
		}

		d_ptr->outer_wall = f_tag_to_index(zz[DUNGEON_FEAT_PROB_NUM * 2]);
		if (d_ptr->outer_wall < 0) return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

		d_ptr->inner_wall = f_tag_to_index(zz[DUNGEON_FEAT_PROB_NUM * 2 + 1]);
		if (d_ptr->inner_wall < 0) return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

		d_ptr->stream1 = f_tag_to_index(zz[DUNGEON_FEAT_PROB_NUM * 2 + 2]);
		if (d_ptr->stream1 < 0) return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

		d_ptr->stream2 = f_tag_to_index(zz[DUNGEON_FEAT_PROB_NUM * 2 + 3]);
		if (d_ptr->stream2 < 0) return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
	}
	else if (buf[0] == 'F')
	{
		int artif = 0, monst = 0;

		for (s = buf + 2; *s; )
		{
			/* loop */
			for (t = s; *t && (*t != ' ') && (*t != '|'); ++t);

			if (*t)
			{
				*t++ = '\0';
				while (*t == ' ' || *t == '|') t++;
			}

			if (1 == sscanf(s, "FINAL_ARTIFACT_%d", &artif))
			{
				d_ptr->final_artifact = (ARTIFACT_IDX)artif;
				s = t;
				continue;
			}

			if (1 == sscanf(s, "FINAL_OBJECT_%d", &artif))
			{
				d_ptr->final_object = (KIND_OBJECT_IDX)artif;
				s = t;
				continue;
			}

			if (1 == sscanf(s, "FINAL_GUARDIAN_%d", &monst))
			{
				d_ptr->final_guardian = (MONRACE_IDX)monst;
				s = t;
				continue;
			}

			if (1 == sscanf(s, "MONSTER_DIV_%d", &monst))
			{
				d_ptr->special_div = (PROB)monst;
				s = t;
				continue;
			}

			if (0 != grab_one_dungeon_flag(d_ptr, s)) return 5;

			s = t;
		}
	}
	else if (buf[0] == 'M')
	{
		for (s = buf + 2; *s; )
		{
			/* loop */
			for (t = s; *t && (*t != ' ') && (*t != '|'); ++t);

			if (*t)
			{
				*t++ = '\0';
				while (*t == ' ' || *t == '|') t++;
			}

			if (!strncmp(s, "R_CHAR_", 7))
			{
				s += 7;
				strncpy(d_ptr->r_char, s, sizeof(d_ptr->r_char));
				s = t;
				continue;
			}

			if (0 != grab_one_basic_monster_flag(d_ptr, s)) return 5;

			s = t;
		}
	}
	else if (buf[0] == 'S')
	{
		for (s = buf + 2; *s; )
		{
			/* loop */
			for (t = s; *t && (*t != ' ') && (*t != '|'); ++t);

			if (*t)
			{
				*t++ = '\0';
				while ((*t == ' ') || (*t == '|')) t++;
			}

			int i;
			if (1 == sscanf(s, "1_IN_%d", &i))
			{
				s = t;
				continue;
			}

			if (0 != grab_one_spell_monster_flag(d_ptr, s)) return 5;

			s = t;
		}
	}
	else
	{
		return 6;
	}

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
