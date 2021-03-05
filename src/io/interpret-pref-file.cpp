﻿/*!
 * @brief prefファイルの内容を解釈しメモリに展開する
 * @date 2020/03/01
 * @author Hourier
 */

#include "io/interpret-pref-file.h"
#include "birth/character-builder.h"
#include "cmd-io/macro-util.h"
#include "game-option/option-flags.h"
#include "game-option/option-types-table.h"
#include "grid/feature.h"
#include "io/gf-descriptions.h"
#include "io/input-key-requester.h"
#include "io/tokenizer.h"
#include "object/object-kind.h"
#include "monster-race/monster-race.h"
#include "system/game-option-types.h"
#include "term/gameterm.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"

#define MAX_MACRO_CHARS 16128 // 1つのマクロキー押下で実行可能なコマンド最大数 (エスケープシーケンス含む).

char *histpref_buf = NULL;

/*!
 * @brief Rトークンの解釈 / Process "R:<num>:<a>/<c>" -- attr/char for monster races
 * @param buf バッファ
 * @return エラーコード
 */
static errr interpret_r_token(char *buf)
{
	char *zz[16];
	if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) return 1;

	monster_race *r_ptr;
	int i = (int)strtol(zz[0], NULL, 0);
	TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
	SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
	if (i >= max_r_idx) return 1;

	r_ptr = &r_info[i];
	if (n1 || (!(n2 & 0x80) && n2)) r_ptr->x_attr = n1; /* Allow TERM_DARK text */
	if (n2) r_ptr->x_char = n2;

	return 0;
}


/*!
 * @brief Kトークンの解釈 / Process "K:<num>:<a>/<c>"  -- attr/char for object kinds
 * @param buf バッファ
 * @return エラーコード
 */
static errr interpret_k_token(char *buf)
{
	char *zz[16];
	if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) return 1;

	object_kind *k_ptr;
	int i = (int)strtol(zz[0], NULL, 0);
	TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
	SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
	if (i >= max_k_idx) return 1;

	k_ptr = &k_info[i];
	if (n1 || (!(n2 & 0x80) && n2)) k_ptr->x_attr = n1; /* Allow TERM_DARK text */
	if (n2) k_ptr->x_char = n2;

	return 0;
}


/*!
 * @brief トークン数によって地形の文字形と色を決定する
 * @param i 地形種別
 * @param num トークン数
 * @return エラーコード
 */
static errr decide_feature_type(int i, int num, char **zz)
{
	feature_type *f_ptr;
	f_ptr = &f_info[i];

	TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
	SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
	if (n1 || (!(n2 & 0x80) && n2)) f_ptr->x_attr[F_LIT_STANDARD] = n1; /* Allow TERM_DARK text */
	if (n2) f_ptr->x_char[F_LIT_STANDARD] = n2;

	switch (num)
	{
	case 3:
	{
		/* No lighting support */
		n1 = f_ptr->x_attr[F_LIT_STANDARD];
		n2 = f_ptr->x_char[F_LIT_STANDARD];
		for (int j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++)
		{
			f_ptr->x_attr[j] = n1;
			f_ptr->x_char[j] = n2;
		}

		return 0;
	}
	case 4:
	{
		/* Use default lighting */
		apply_default_feat_lighting(f_ptr->x_attr, f_ptr->x_char);
		return 0;
	}
	case F_LIT_MAX * 2 + 1:
	{
		/* Use desired lighting */
		for (int j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++)
		{
			n1 = (TERM_COLOR)strtol(zz[j * 2 + 1], NULL, 0);
			n2 = (SYMBOL_CODE)strtol(zz[j * 2 + 2], NULL, 0);
			if (n1 || (!(n2 & 0x80) && n2)) f_ptr->x_attr[j] = n1; /* Allow TERM_DARK text */
			if (n2) f_ptr->x_char[j] = n2;
		}

		return 0;
	}
	default:
		return 0;
	}
}


/*!
 * @brief Fトークンの解釈 / Process "F:<num>:<a>/<c>" -- attr/char for terrain features
 * @param buf バッファ
 * @return エラーコード
 * @details
 * "F:<num>:<a>/<c>"
 * "F:<num>:<a>/<c>:LIT"
 * "F:<num>:<a>/<c>:<la>/<lc>:<da>/<dc>"
 */
static errr interpret_f_token(char *buf)
{
	char *zz[16];
	int num = tokenize(buf + 2, F_LIT_MAX * 2 + 1, zz, TOKENIZE_CHECKQUOTE);

	if ((num != 3) && (num != 4) && (num != F_LIT_MAX * 2 + 1)) return 1;
	else if ((num == 4) && !streq(zz[3], "LIT")) return 1;

	int i = (int)strtol(zz[0], NULL, 0);
	if (i >= max_f_idx) return 1;

	return decide_feature_type(i, num, zz);
}


/*!
 * @brief Fトークンの解釈 / Process "S:<num>:<a>/<c>" -- attr/char for special things
 * @param buf バッファ
 * @return エラーコード
 */
static errr interpret_s_token(char *buf)
{
	char *zz[16];
	if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) return 1;

	int j = (byte)strtol(zz[0], NULL, 0);
	TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
	SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
	misc_to_attr[j] = n1;
	misc_to_char[j] = n2;
	return 0;
}


/*!
 * @brief Uトークンの解釈 / Process "U:<tv>:<a>/<c>" -- attr/char for unaware items
 * @param buf バッファ
 * @return エラーコード
 */
static errr interpret_u_token(char *buf)
{
	char *zz[16];
	if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) return 1;

	int j = (int)strtol(zz[0], NULL, 0);
	TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
	SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
	for (int i = 1; i < max_k_idx; i++)
	{
		object_kind *k_ptr = &k_info[i];
		if (k_ptr->tval == j)
		{
			if (n1) k_ptr->d_attr = n1;
			if (n2) k_ptr->d_char = n2;
		}
	}

	return 0;
}


/*!
 * @brief Eトークンの解釈 / Process "E:<tv>:<a>" -- attribute for inventory objects
 * @param buf バッファ
 * @return エラーコード
 */
static errr interpret_e_token(char *buf)
{
	char *zz[16];
	if (tokenize(buf + 2, 2, zz, TOKENIZE_CHECKQUOTE) != 2) return 1;

	int j = (byte)strtol(zz[0], NULL, 0) % 128;
	TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
	if (n1) tval_to_attr[j] = n1;
	return 0;
}


/*!
 * @brief Pトークンの解釈 / Process "P:<str>" -- normal macro
 * @param buf バッファ
 * @return エラーコード
 */
static errr interpret_p_token(char *buf)
{
	char tmp[1024];
	text_to_ascii(tmp, buf + 2);
	macro_add(tmp, macro__buf);
	return 0;
}


/*!
 * @brief Cトークンの解釈 / Process "C:<str>" -- create keymap
 * @param buf バッファ
 * @return エラーコード
 */
static errr interpret_c_token(char *buf)
{
	char *zz[16];
	if (tokenize(buf + 2, 2, zz, TOKENIZE_CHECKQUOTE) != 2) return 1;

	int mode = strtol(zz[0], NULL, 0);
	if ((mode < 0) || (mode >= KEYMAP_MODES)) return 1;

	char tmp[1024];
	text_to_ascii(tmp, zz[1]);
	if (!tmp[0] || tmp[1]) return 1;

	int i = (byte)(tmp[0]);
	string_free(keymap_act[mode][i]);
	keymap_act[mode][i] = string_make(macro__buf);
	return 0;
}


/*!
 * @brief Vトークンの解釈 / Process "V:<num>:<kv>:<rv>:<gv>:<bv>" -- visual info
 * @param buf バッファ
 * @return エラーコード
 */
static errr interpret_v_token(char *buf)
{
	char *zz[16];
	if (tokenize(buf + 2, 5, zz, TOKENIZE_CHECKQUOTE) != 5) return 1;

	int i = (byte)strtol(zz[0], NULL, 0);
	angband_color_table[i][0] = (byte)strtol(zz[1], NULL, 0);
	angband_color_table[i][1] = (byte)strtol(zz[2], NULL, 0);
	angband_color_table[i][2] = (byte)strtol(zz[3], NULL, 0);
	angband_color_table[i][3] = (byte)strtol(zz[4], NULL, 0);
	return 0;
}


/*!
 * @brief X/Yトークンの解釈
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param buf バッファ
 * @return エラーコード
 * @details
 * Process "X:<str>" -- turn option off
 * Process "Y:<str>" -- turn option on
 */
static errr interpret_xy_token(player_type *creature_ptr, char *buf)
{
	for (int i = 0; option_info[i].o_desc; i++)
	{
		bool is_option = option_info[i].o_var != NULL;
		is_option &= option_info[i].o_text != NULL;
		is_option &= streq(option_info[i].o_text, buf + 2);
		if (!is_option) continue;

		int os = option_info[i].o_set;
		int ob = option_info[i].o_bit;

		if ((creature_ptr->playing || current_world_ptr->character_xtra) &&
			(OPT_PAGE_BIRTH == option_info[i].o_page) && !current_world_ptr->wizard)
		{
			msg_format(_("初期オプションは変更できません! '%s'", "Birth options can not changed! '%s'"), buf);
			msg_print(NULL);
			return 0;
		}

		if (buf[0] == 'X')
		{
			option_flag[os] &= ~(1UL << ob);
			(*option_info[i].o_var) = FALSE;
			return 0;
		}

		option_flag[os] |= (1UL << ob);
		(*option_info[i].o_var) = TRUE;
		return 0;
	}

	msg_format(_("オプションの名前が正しくありません： %s", "Ignored invalid option: %s"), buf);
	msg_print(NULL);
	return 0;
}


/*!
 * @brief Zトークンの解釈 / Process "Z:<type>:<str>" -- set spell color
 * @param buf バッファ
 * @param zz トークン保管文字列
 * @return エラーコード
 */
static errr interpret_z_token(char *buf)
{
	char *t = angband_strchr(buf + 2, ':');
	if (!t) return 1;

	*(t++) = '\0';
	for (int i = 0; i < MAX_NAMED_NUM; i++)
	{
		if (!streq(gf_desc[i].name, buf + 2)) continue;

		gf_color[gf_desc[i].num] = (TERM_COLOR)quark_add(t);
		return 0;
	}

	return 1;
}


/*!
 * @brief Tトークンの解釈 / Process "T:<template>:<modifier chr>:<modifier name>:..." for 4 tokens
 * @param buf バッファ
 * @param zz トークン保管文字列
 * @return エラーコード
 */
static errr decide_template_modifier(int tok, char **zz)
{
	if (macro_template != NULL)
	{
		int macro_modifier_length = strlen(macro_modifier_chr);
		string_free(macro_template);
		macro_template = NULL;
		string_free(macro_modifier_chr);
		for (int i = 0; i < macro_modifier_length; i++)
		{
			string_free(macro_modifier_name[i]);
		}

		for (int i = 0; i < max_macrotrigger; i++)
		{
			string_free(macro_trigger_name[i]);
			string_free(macro_trigger_keycode[0][i]);
			string_free(macro_trigger_keycode[1][i]);
		}

		max_macrotrigger = 0;
	}

	if (*zz[0] == '\0') return 0;

	int zz_length = strlen(zz[1]);
	zz_length = MIN(MAX_MACRO_MOD, zz_length);
	if (2 + zz_length != tok) return 1;

	macro_template = string_make(zz[0]);
	macro_modifier_chr = string_make(zz[1]);
	for (int i = 0; i < zz_length; i++)
	{
		macro_modifier_name[i] = string_make(zz[2 + i]);
	}

	return 0;
}


/*!
 * @brief Tトークンの解釈 / Process "T:<trigger>:<keycode>:<shift-keycode>" for 2 or 3 tokens
 * @param tok トークン数
 * @param zz トークン保管文字列
 * @return エラーコード
 */
static errr interpret_macro_keycodes(int tok, char **zz)
{
	char buf_aux[MAX_MACRO_CHARS];
	char *t, *s;
	if (max_macrotrigger >= MAX_MACRO_TRIG)
	{
		msg_print(_("マクロトリガーの設定が多すぎます!", "Too many macro triggers!"));
		return 1;
	}

	int m = max_macrotrigger;
	max_macrotrigger++;
	t = buf_aux;
	s = zz[0];
	while (*s)
	{
		if ('\\' == *s) s++;
		*t++ = *s++;
	}

	*t = '\0';
	macro_trigger_name[m] = string_make(buf_aux);
	macro_trigger_keycode[0][m] = string_make(zz[1]);
	if (tok == 3)
	{
		macro_trigger_keycode[1][m] = string_make(zz[2]);
		return 0;
	}

	macro_trigger_keycode[1][m] = string_make(zz[1]);
	return 0;
}


/*!
 * todo 2.2.1r時点のコードからトークン数0～1の場合もエラーコード0だが、1であるべきでは？
 * @brief Tトークンの個数調査 (解釈はサブルーチンで) / Initialize macro trigger names and a template
 * @param buf バッファ
 * @return エラーコード
 */
static errr interpret_t_token(char *buf)
{
	char *zz[16];
	int tok = tokenize(buf + 2, 2 + MAX_MACRO_MOD, zz, 0);
	if (tok >= 4) return decide_template_modifier(tok, zz);
	if (tok < 2) return 0;

	return interpret_macro_keycodes(tok, zz);
}


/*!
 * @brief 設定ファイルの各行から各種テキスト情報を取得する /
 * Parse a sub-file of the "extra info" (format shown below)
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param buf データテキストの参照ポインタ
 * @return エラーコード
 * @details
 * <pre>
 * Each "action" line has an "action symbol" in the first column,
 * followed by a colon, followed by some command specific info,
 * usually in the form of "tokens" separated by colons or slashes.
 * Blank lines, lines starting with white space, and lines starting
 * with pound signs ("#") are ignored (as comments).
 * Note the use of "tokenize()" to allow the use of both colons and
 * slashes as delimeters, while still allowing final tokens which
 * may contain any characters including "delimiters".
 * Note the use of "strtol()" to allow all "integers" to be encoded
 * in decimal, hexidecimal, or octal form.
 * Note that "monster zero" is used for the "player" attr/char, "object
 * zero" will be used for the "stack" attr/char, and "feature zero" is
 * used for the "nothing" attr/char.
 * </pre>
 */
errr interpret_pref_file(player_type *creature_ptr, char *buf)
{
	if (buf[1] != ':') return 1;

	switch (buf[0])
	{
	case 'H':
	{
		/* Process "H:<history>" */
		add_history_from_pref_line(buf + 2);
		return 0;
	}
	case 'R':
		return interpret_r_token(buf);
	case 'K':
		return interpret_k_token(buf);
	case 'F':
		return interpret_f_token(buf);
	case 'S':
		return interpret_s_token(buf);
	case 'U':
		return interpret_u_token(buf);
	case 'E':
		return interpret_e_token(buf);
	case 'A':
	{
		/* Process "A:<str>" -- save an "action" for later */
		text_to_ascii(macro__buf, buf + 2);
		return 0;
	}
	case 'P':
		return interpret_p_token(buf);
	case 'C':
		return interpret_c_token(buf);
	case 'V':
		return interpret_v_token(buf);
	case 'X':
	case 'Y':
		return interpret_xy_token(creature_ptr, buf);
	case 'Z':
		return interpret_z_token(buf);
	case 'T':
		return interpret_t_token(buf);
	default:
		return 1;
	}
}

/*!
 * @brief 生い立ちメッセージの内容をバッファに加える。 / Hook function for reading the histpref.prf file.
 * @return なし
 */
void add_history_from_pref_line(concptr t)
{
    if (!histpref_buf)
        return;

    angband_strcat(histpref_buf, t, HISTPREF_LIMIT);
}
