/*!
 *  @file japanese.c
 *  @brief 日本語処理関数
 *  @date 2014/07/07
 */

#include "locale/japanese.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

#ifdef JP

typedef struct convert_key convert_key;

struct convert_key
{
	concptr key1;
	concptr key2;
};

static const convert_key s2j_table[] = {
	{"mb","nb"}, {"mp","np"}, {"mv","nv"}, {"mm","nm"},
	{"x","ks"},
	/* sindar:シンダール  parantir:パランティア  feanor:フェアノール */
	{"ar$","a-ru$"}, {"ir$","ia$"}, {"or$","o-ru$"},
	{"ra","ラ"}, {"ri","リ"}, {"ru","ル"}, {"re","レ"}, {"ro","ロ"},
	{"ir","ia"}, {"ur","ua"}, {"er","ea"}, {"ar","aル"},
	{"sha","シャ"}, {"shi","シ"}, {"shu","シュ"}, {"she","シェ"}, {"sho","ショ"},
	{"tha","サ"}, {"thi","シ"}, {"thu","ス"}, {"the","セ"}, {"tho","ソ"},
	{"cha","ハ"}, {"chi","ヒ"}, {"chu","フ"}, {"che","ヘ"}, {"cho","ホ"},
	{"dha","ザ"}, {"dhi","ジ"}, {"dhu","ズ"}, {"dhe","ゼ"}, {"dho","ゾ"},
	{"ba","バ"}, {"bi","ビ"}, {"bu","ブ"}, {"be","ベ"}, {"bo","ボ"},
	{"ca","カ"}, {"ci","キ"}, {"cu","ク"}, {"ce","ケ"}, {"co","コ"},
	{"da","ダ"}, {"di","ディ"}, {"du","ドゥ"}, {"de","デ"}, {"do","ド"},
	{"fa","ファ"}, {"fi","フィ"}, {"fu","フ"}, {"fe","フェ"}, {"fo","フォ"},
	{"ga","ガ"}, {"gi","ギ"}, {"gu","グ"}, {"ge","ゲ"}, {"go","ゴ"},
	{"ha","ハ"}, {"hi","ヒ"}, {"hu","フ"}, {"he","ヘ"}, {"ho","ホ"},
	{"ja","ジャ"}, {"ji","ジ"}, {"ju","ジュ"}, {"je","ジェ"}, {"jo","ジョ"},
	{"ka","カ"}, {"ki","キ"}, {"ku","ク"}, {"ke","ケ"}, {"ko","コ"},
	{"la","ラ"}, {"li","リ"}, {"lu","ル"}, {"le","レ"}, {"lo","ロ"},
	{"ma","マ"}, {"mi","ミ"}, {"mu","ム"}, {"me","メ"}, {"mo","モ"},
	{"na","ナ"}, {"ni","ニ"}, {"nu","ヌ"}, {"ne","ネ"}, {"no","ノ"},
	{"pa","パ"}, {"pi","ピ"}, {"pu","プ"}, {"pe","ペ"}, {"po","ポ"},
	{"qu","ク"},
	{"sa","サ"}, {"si","シ"}, {"su","ス"}, {"se","セ"}, {"so","ソ"},
	{"ta","タ"}, {"ti","ティ"}, {"tu","トゥ"}, {"te","テ"}, {"to","ト"},
	{"va","ヴァ"}, {"vi","ヴィ"}, {"vu","ヴ"}, {"ve","ヴェ"}, {"vo","ヴォ"},
	{"wa","ワ"}, {"wi","ウィ"}, {"wu","ウ"}, {"we","ウェ"}, {"wo","ウォ"},
	{"ya","ヤ"}, {"yu","ユ"}, {"yo","ヨ"},
	{"za","ザ"}, {"zi","ジ"}, {"zu","ズ"}, {"ze","ゼ"}, {"zo","ゾ"},
	{"dh","ズ"}, {"ch","フ"}, {"th","ス"},
	{"b","ブ"}, {"c","ク"}, {"d","ド"}, {"f","フ"}, {"g","グ"},
	{"h","フ"}, {"j","ジュ"}, {"k","ク"}, {"l","ル"}, {"m","ム"},
	{"n","ン"}, {"p","プ"}, {"q","ク"}, {"r","ル"}, {"s","ス"},
	{"t","ト"}, {"v","ヴ"}, {"w","ウ"}, {"y","イ"},
	{"a","ア"}, {"i","イ"}, {"u","ウ"}, {"e","エ"}, {"o","オ"},
	{"-","ー"},
	{NULL,NULL}
};

/*!
 * @brief シンダリンを日本語の読みに変換する
 * @param kana 変換後の日本語文字列ポインタ
 * @param sindarin 変換前のシンダリン文字列ポインタ
 * @return なし
 * @details
 */
void sindarin_to_kana(char *kana, concptr sindarin)
{
	char buf[256];
	int idx;

	sprintf(kana, "%s$", sindarin);
	for (idx = 0; kana[idx]; idx++)
		if (isupper(kana[idx])) kana[idx] = (char)tolower(kana[idx]);

	for (idx = 0; s2j_table[idx].key1 != NULL; idx++)
	{
		concptr pat1 = s2j_table[idx].key1;
		concptr pat2 = s2j_table[idx].key2;
		int len = strlen(pat1);
		char *src = kana;
		char *dest = buf;

		while (*src)
		{
			if (strncmp(src, pat1, len) == 0)
			{
				strcpy(dest, pat2);
				src += len;
				dest += strlen(pat2);
			}
			else
			{
				if (iskanji(*src))
				{
					*dest = *src;
					src++;
					dest++;
				}
				*dest = *src;
				src++;
				dest++;
			}
		}

		*dest = 0;
		strcpy(kana, buf);
	}

	idx = 0;

	while (kana[idx] != '$') idx++;

	kana[idx] = '\0';
}


/*! 日本語動詞活用 (打つ＞打って,打ち etc)
 * JVERB_AND: 殴る,蹴る > 殴り,蹴る
 * JVERB_TO:  殴る,蹴る > 殴って蹴る
 * JVERB_OR:  殴る,蹴る > 殴ったり蹴ったり */
static const struct jverb_table_t {
	const char* from;
	const char* to[3];
} jverb_table[] = {
	{ "する", {"し", "して", "した"}},
	{ "いる", {"いて", "いて", "いた"}},

	{ "える", {"え", "えて", "えた"}},
	{ "ける", {"け", "けて", "けた"}},
	{ "げる", {"げ", "えて", "げた"}},
	{ "せる", {"せ", "せて", "せた"}},
	{ "ぜる", {"ぜ", "ぜて", "ぜた"}},
	{ "てる", {"て", "てって", "てった"}},
	{ "でる", {"で", "でて", "でた"}},
	{ "ねる", {"ね", "ねて", "ねた"}},
	{ "へる", {"へ", "へて", "へた"}},
	{ "べる", {"べ", "べて", "べた"}},
	{ "める", {"め", "めて", "めた"}},
	{ "れる", {"れ", "れて", "れた"}},

	{ "う", {"い", "って", "った"}},
	{ "く", {"き", "いて", "いた"}},
	{ "ぐ", {"ぎ", "いで", "いだ"}},
	{ "す", {"し", "して", "した"}},
	{ "ず", {"じ", "じて", "じた"}},
	{ "つ", {"ち", "って", "った"}},
	{ "づ", {"ぢ", "って", "った"}},
	{ "ぬ", {"に", "ねて", "ねた"}},
	{ "ふ", {"ひ", "へて", "へた"}},
	{ "ぶ", {"び", "んで", "んだ"}},
	{ "む", {"み", "んで", "んだ"}},
	{ "る", {"り", "って", "った"}},
	{ NULL, {"そして", "ことにより", "ことや"}},
};

/*!
 * @brief jverb_table_tに従って動詞を活用する
 * @param in 変換元文字列ポインタ
 * @param out 変換先文字列ポインタ
 * @param flag 変換種類を指定(JVERB_AND/JVERB_TO/JVERB_OR)
 * @return なし
 * @details
 */
void jverb(concptr in, char *out, int flag)
{
	const struct jverb_table_t * p;
	int in_len = strlen(in);

	strcpy(out, in);

	for (p = jverb_table; p->from; p++) {
		int from_len = strlen(p->from);
		if (strncmp(&in[in_len-from_len], p->from, from_len) == 0) {
			strcpy(&out[in_len - from_len], p->to[flag - 1]);
			break;
		}
	}

	if (p->from == NULL)
		strcpy(&out[in_len], p->to[flag - 1]);
}

/*!
 * @brief 文字コードをSJISからEUCに変換する / Convert SJIS string to EUC string
 * @param str 変換する文字列のポインタ
 * @return なし
 * @details
 */
void sjis2euc(char *str)
{
	int i;
	unsigned char c1, c2;
	unsigned char *tmp;

	int len = strlen(str);

	C_MAKE(tmp, len+1, byte);

	for (i = 0; i < len; i++)
	{
		c1 = str[i];
		if (c1 & 0x80)
		{
			i++;
			c2 = str[i];
			if (c2 >= 0x9f)
			{
				c1 = c1 * 2 - (c1 >= 0xe0 ? 0xe0 : 0x60);
				c2 += 2;
			}
			else
			{
				c1 = c1 * 2 - (c1 >= 0xe0 ? 0xe1 : 0x61);
				c2 += 0x60 + (c2 < 0x7f);
			}
			tmp[i - 1] = c1;
			tmp[i] = c2;
		}
		else
			tmp[i] = c1;
	}
	tmp[len] = 0;
	strcpy(str, (char *)tmp);

	C_KILL(tmp, len+1, byte);
}  


/*!
 * @brief 文字コードをEUCからSJISに変換する / Convert EUC string to SJIS string
 * @param str 変換する文字列のポインタ
 * @return なし
 * @details
 */
void euc2sjis(char *str)
{
	int i;
	unsigned char c1, c2;
	unsigned char *tmp;
	
	int len = strlen(str);

	C_MAKE(tmp, len+1, byte);

	for (i = 0; i < len; i++)
	{
		c1 = str[i];
		if (c1 & 0x80)
		{
			i++;
			c2 = str[i];
			if (c1 % 2)
			{
				c1 = (c1 >> 1) + (c1 < 0xdf ? 0x31 : 0x71);
				c2 -= 0x60 + (c2 < 0xe0);
			}
			else
			{
				c1 = (c1 >> 1) + (c1 < 0xdf ? 0x30 : 0x70);
				c2 -= 2;
			}

			tmp[i - 1] = c1;
			tmp[i] = c2;
		}
		else
			tmp[i] = c1;
	}
	tmp[len] = 0;
	strcpy(str, (char *)tmp);

	C_KILL(tmp, len+1, byte);
}  


/*!
 * @brief strを環境に合った文字コードに変換し、変換前の文字コードを返す。strの長さに制限はない。
 * @param str 変換する文字列のポインタ
 * @return 
 * 0: Unknown<br>
 * 1: ASCII (Never known to be ASCII in this function.)<br>
 * 2: EUC<br>
 * 3: SJIS<br>
 */
byte codeconv(char *str)
{
	byte code = 0;
	int i;

	for (i = 0; str[i]; i++)
	{
		unsigned char c1;
		unsigned char c2;

		/* First byte */
		c1 = str[i];

		/* ASCII? */
		if (!(c1 & 0x80)) continue;

		/* Second byte */
		i++;
		c2 = str[i];

		if (((0xa1 <= c1 && c1 <= 0xdf) || (0xfd <= c1 && c1 <= 0xfe)) &&
		    (0xa1 <= c2 && c2 <= 0xfe))
		{
			/* Only EUC is allowed */
			if (!code)
			{
				/* EUC */
				code = 2;
			}

			/* Broken string? */
			else if (code != 2)
			{
				/* No conversion */
				return 0;
			}
		}

		else if (((0x81 <= c1 && c1 <= 0x9f) &&
			  ((0x40 <= c2 && c2 <= 0x7e) || (0x80 <= c2 && c2 <= 0xfc))) ||
			 ((0xe0 <= c1 && c1 <= 0xfc) &&
			  (0x40 <= c2 && c2 <= 0x7e)))
		{
			/* Only SJIS is allowed */
			if (!code)
			{
				/* SJIS */
				code = 3;
			}

			/* Broken string? */
			else if (code != 3)
			{
				/* No conversion */
				return 0;
			}
		}
	}


	switch (code)
	{
#ifdef EUC
	case 3:
		/* SJIS -> EUC */
		sjis2euc(str);
		break;
#endif

#ifdef SJIS
	case 2:
		/* EUC -> SJIS */
		euc2sjis(str);

		break;
#endif
	}

	/* Return kanji code */
	return code;
}

/*!
 * @brief 文字列sのxバイト目が漢字の1バイト目かどうか判定する
 * @param s 判定する文字列のポインタ
 * @param x 判定する位置(バイト)
 * @return 漢字の1バイト目ならばTRUE
 */
bool iskanji2(concptr s, int x)
{
	int i;

	for (i = 0; i < x; i++)
	{
		if (iskanji(s[i])) i++;
	}
	if ((x == i) && iskanji(s[x])) return TRUE;

	return FALSE;
}

/*!
 * @brief 文字列の文字コードがASCIIかどうかを判定する
 * @param str 判定する文字列へのポインタ
 * @return 文字列の文字コードがASCIIならTRUE、そうでなければFALSE
 */
static bool is_ascii_str(concptr str)
{
	for (;*str; str++) {
		int ch = *str;
		if (!(0x00 < ch && ch <= 0x7f))
			return FALSE;
	}
	return TRUE;
}

/*!
 * @brief 文字列の文字コードがUTF-8かどうかを判定する
 * @param str 判定する文字列へのポインタ
 * @return 文字列の文字コードがUTF-8ならTRUE、そうでなければFALSE
 */
static bool is_utf8_str(concptr str)
{
	const unsigned char* p;
	for (p = (const unsigned char*)str; *p; p++) {
		int subseq_num = 0;
		if (0x00 < *p && *p <= 0x7f) continue;
		
		if ((*p & 0xe0) == 0xc0) subseq_num = 1;
		if ((*p & 0xf0) == 0xe0) subseq_num = 2;
		if ((*p & 0xf8) == 0xf0) subseq_num = 3;

		if (subseq_num == 0) return FALSE;
		while (subseq_num--) {
			p++;
			if (!*p || (*p & 0xc0) != 0x80) return FALSE;
		}
	}
	return TRUE;
}

#if defined(EUC)
#include <iconv.h>

static const struct ms_to_jis_unicode_conv_t {
	char from[3];
	char to[3];
} ms_to_jis_unicode_conv[] = {
	{{0xef, 0xbd, 0x9e}, {0xe3, 0x80, 0x9c}}, /* FULLWIDTH TILDE -> WAVE DASH */
	{{0xef, 0xbc, 0x8d}, {0xe2, 0x88, 0x92}}, /* FULLWIDTH HYPHEN-MINUS -> MINUS SIGN */
};

/*!
 * @brief EUCがシステムコードである環境下向けにUTF-8から変換処理を行うサブルーチン
 * @param str 変換する文字列のポインタ
 * @return なし
 */
static void ms_to_jis_unicode(char* str)
{
	unsigned char* p;
	for (p = (unsigned char*)str; *p; p++) {
		int subseq_num = 0;
		if (0x00 < *p && *p <= 0x7f) continue;

		if ((*p & 0xe0) == 0xc0) subseq_num = 1;
		if ((*p & 0xf0) == 0xe0) {
			size_t i;
			for (i = 0; i < sizeof(ms_to_jis_unicode_conv) / sizeof(ms_to_jis_unicode_conv[0]); ++ i) {
				const struct ms_to_jis_unicode_conv_t *c = &ms_to_jis_unicode_conv[i];
				if (memcmp(p, c->from, 3) == 0) {
					memcpy(p, c->to, 3);
				}
			}
			subseq_num = 2;
                }
		if ((*p & 0xf8) == 0xf0) subseq_num = 3;

		p += subseq_num;
	}
}

#elif defined(SJIS) && defined(WINDOWS)
#include <Windows.h>
#endif
/*!
 * @brief 文字コードがUTF-8の文字列をシステムの文字コードに変換する
 * @param utf8_str 変換するUTF-8の文字列へのポインタ
 * @param sys_str_buffer 変換したシステムの文字コードの文字列を格納するバッファへのポインタ
 * @param sys_str_buflen 変換したシステムの文字コードの文字列を格納するバッファの長さ
 * @return 変換に成功した場合TRUE、失敗した場合FALSEを返す
 */
static bool utf8_to_sys(char* utf8_str, char* sys_str_buffer, size_t sys_str_buflen)
{
#if defined(EUC)

	iconv_t cd = iconv_open("EUC-JP", "UTF-8");
	size_t utf8_len = strlen(utf8_str) + 1; /* include termination character */
	char *from = utf8_str;
	int ret;

	ms_to_jis_unicode(utf8_str);
	ret = iconv(cd, &from, &utf8_len, &sys_str_buffer, &sys_str_buflen);
	iconv_close(cd);
	return (ret >= 0);

#elif defined(SJIS) && defined(WINDOWS)

	LPWSTR utf16buf;
	int input_len = strlen(utf8_str) + 1; /* include termination character */

	C_MAKE(utf16buf, input_len, WCHAR);

	/* UTF-8 -> UTF-16 */
	if (MultiByteToWideChar( CP_UTF8, 0, utf8_str, input_len, utf16buf, input_len) == 0) {
		C_KILL(utf16buf, input_len, WCHAR);
		return FALSE;
	}

	/* UTF-8 -> SJIS(CP932) */
	if (WideCharToMultiByte( CP_ACP, 0, utf16buf, -1, sys_str_buffer, sys_str_buflen, NULL, NULL ) == 0) {
		C_KILL(utf16buf, input_len, WCHAR);
		return FALSE;
	}

	C_KILL(utf16buf, input_len, WCHAR);
	return TRUE;

#endif
}

/*!
 * @brief 受け取った文字列の文字コードを推定し、システムの文字コードへ変換する
 * @param strbuf 変換する文字列を格納したバッファへのポインタ。
 *               バッファは変換した文字列で上書きされる。
 *               UTF-8からSJISもしくはEUCへの変換を想定しているのでバッファの長さが足りなくなることはない。
 * @param buflen バッファの長さ。
 * @return なし
 */
void guess_convert_to_system_encoding(char* strbuf, int buflen)
{
	if (is_ascii_str(strbuf)) return;

	if (is_utf8_str(strbuf)) {
		char* work;
		C_MAKE(work, buflen, char);
		angband_strcpy(work, strbuf, buflen);
		if (!utf8_to_sys(work, strbuf, buflen)) {
			msg_print("警告:文字コードの変換に失敗しました");
			msg_print(NULL);
		}
		C_KILL(work, buflen, char);
	}
}

#endif /* JP */
