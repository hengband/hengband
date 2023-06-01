/*!
 *  @file japanese.cpp
 *  @brief 日本語処理関数
 *  @date 2014/07/07
 */

#include "locale/japanese.h"
#include "locale/utf-8.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <set>
#include <sstream>

#ifdef JP

struct convert_key;

struct convert_key {
    concptr key1;
    concptr key2;
};

static const convert_key s2j_table[] = { { "mb", "nb" }, { "mp", "np" }, { "mv", "nv" }, { "mm", "nm" }, { "x", "ks" },
    /* sindar:シンダール  parantir:パランティア  feanor:フェアノール */
    { "ar$", "a-ru$" }, { "ir$", "ia$" }, { "or$", "o-ru$" }, { "ra", "ラ" }, { "ri", "リ" }, { "ru", "ル" }, { "re", "レ" }, { "ro", "ロ" }, { "ir", "ia" },
    { "ur", "ua" }, { "er", "ea" }, { "ar", "aル" }, { "sha", "シャ" }, { "shi", "シ" }, { "shu", "シュ" }, { "she", "シェ" }, { "sho", "ショ" },
    { "tha", "サ" }, { "thi", "シ" }, { "thu", "ス" }, { "the", "セ" }, { "tho", "ソ" }, { "cha", "ハ" }, { "chi", "ヒ" }, { "chu", "フ" }, { "che", "ヘ" },
    { "cho", "ホ" }, { "dha", "ザ" }, { "dhi", "ジ" }, { "dhu", "ズ" }, { "dhe", "ゼ" }, { "dho", "ゾ" }, { "ba", "バ" }, { "bi", "ビ" }, { "bu", "ブ" },
    { "be", "ベ" }, { "bo", "ボ" }, { "ca", "カ" }, { "ci", "キ" }, { "cu", "ク" }, { "ce", "ケ" }, { "co", "コ" }, { "da", "ダ" }, { "di", "ディ" },
    { "du", "ドゥ" }, { "de", "デ" }, { "do", "ド" }, { "fa", "ファ" }, { "fi", "フィ" }, { "fu", "フ" }, { "fe", "フェ" }, { "fo", "フォ" }, { "ga", "ガ" },
    { "gi", "ギ" }, { "gu", "グ" }, { "ge", "ゲ" }, { "go", "ゴ" }, { "ha", "ハ" }, { "hi", "ヒ" }, { "hu", "フ" }, { "he", "ヘ" }, { "ho", "ホ" },
    { "ja", "ジャ" }, { "ji", "ジ" }, { "ju", "ジュ" }, { "je", "ジェ" }, { "jo", "ジョ" }, { "ka", "カ" }, { "ki", "キ" }, { "ku", "ク" }, { "ke", "ケ" },
    { "ko", "コ" }, { "la", "ラ" }, { "li", "リ" }, { "lu", "ル" }, { "le", "レ" }, { "lo", "ロ" }, { "ma", "マ" }, { "mi", "ミ" }, { "mu", "ム" },
    { "me", "メ" }, { "mo", "モ" }, { "na", "ナ" }, { "ni", "ニ" }, { "nu", "ヌ" }, { "ne", "ネ" }, { "no", "ノ" }, { "pa", "パ" }, { "pi", "ピ" },
    { "pu", "プ" }, { "pe", "ペ" }, { "po", "ポ" }, { "qu", "ク" }, { "sa", "サ" }, { "si", "シ" }, { "su", "ス" }, { "se", "セ" }, { "so", "ソ" },
    { "ta", "タ" }, { "ti", "ティ" }, { "tu", "トゥ" }, { "te", "テ" }, { "to", "ト" }, { "va", "ヴァ" }, { "vi", "ヴィ" }, { "vu", "ヴ" }, { "ve", "ヴェ" },
    { "vo", "ヴォ" }, { "wa", "ワ" }, { "wi", "ウィ" }, { "wu", "ウ" }, { "we", "ウェ" }, { "wo", "ウォ" }, { "ya", "ヤ" }, { "yu", "ユ" }, { "yo", "ヨ" },
    { "za", "ザ" }, { "zi", "ジ" }, { "zu", "ズ" }, { "ze", "ゼ" }, { "zo", "ゾ" }, { "dh", "ズ" }, { "ch", "フ" }, { "th", "ス" }, { "b", "ブ" },
    { "c", "ク" }, { "d", "ド" }, { "f", "フ" }, { "g", "グ" }, { "h", "フ" }, { "j", "ジュ" }, { "k", "ク" }, { "l", "ル" }, { "m", "ム" }, { "n", "ン" },
    { "p", "プ" }, { "q", "ク" }, { "r", "ル" }, { "s", "ス" }, { "t", "ト" }, { "v", "ヴ" }, { "w", "ウ" }, { "y", "イ" }, { "a", "ア" }, { "i", "イ" },
    { "u", "ウ" }, { "e", "エ" }, { "o", "オ" }, { "-", "ー" }, { nullptr, nullptr } };

/*!
 * @brief シンダリンを日本語の読みに変換する
 * @param sindarin 変換前のシンダリン文字列
 * @return std::string 変換後のシンダリン文字列
 * @details
 */
std::string sindarin_to_kana(std::string_view sindarin)
{
    std::string kana;

    for (const auto &ch : sindarin) {
        kana.push_back(isupper(ch) ? static_cast<char>(tolower(ch)) : ch);
    }
    kana.append("$");

    for (auto idx = 0; s2j_table[idx].key1 != nullptr; idx++) {
        concptr pat1 = s2j_table[idx].key1;
        size_t len = strlen(pat1);
        std::string::size_type i = 0;

        while (i < kana.length()) {
            if (strncmp(kana.data() + i, pat1, len) == 0) {
                concptr pat2 = s2j_table[idx].key2;

                kana.replace(i, len, pat2);
                i += strlen(pat2);
            } else {
                if (iskanji(kana[i])) {
                    ++i;
                }
                ++i;
            }
        }
    }

    kana.erase(kana.find('$'));
    return kana;
}

/*!
 * 日本語動詞活用 (打つ＞打って,打ち etc)
 * AND : 殴る,蹴る > 殴り,蹴る
 * TO  : 殴る,蹴る > 殴って蹴る
 * OR  : 殴る,蹴る > 殴ったり蹴ったり
 */
static constexpr struct jverb_table_t {
    std::string_view from;
    std::string_view to_list[3];
} jverb_table[] = {
    { "する", { "し", "して", "した" } },
    { "いる", { "いて", "いて", "いた" } },

    { "える", { "え", "えて", "えた" } },
    { "ける", { "け", "けて", "けた" } },
    { "げる", { "げ", "えて", "げた" } },
    { "せる", { "せ", "せて", "せた" } },
    { "ぜる", { "ぜ", "ぜて", "ぜた" } },
    { "てる", { "て", "てって", "てった" } },
    { "でる", { "で", "でて", "でた" } },
    { "ねる", { "ね", "ねて", "ねた" } },
    { "へる", { "へ", "へて", "へた" } },
    { "べる", { "べ", "べて", "べた" } },
    { "める", { "め", "めて", "めた" } },
    { "れる", { "れ", "れて", "れた" } },

    { "う", { "い", "って", "った" } },
    { "く", { "き", "いて", "いた" } },
    { "ぐ", { "ぎ", "いで", "いだ" } },
    { "す", { "し", "して", "した" } },
    { "ず", { "じ", "じて", "じた" } },
    { "つ", { "ち", "って", "った" } },
    { "づ", { "ぢ", "って", "った" } },
    { "ぬ", { "に", "ねて", "ねた" } },
    { "ふ", { "ひ", "へて", "へた" } },
    { "ぶ", { "び", "んで", "んだ" } },
    { "む", { "み", "んで", "んだ" } },
    { "る", { "り", "って", "った" } },
};

/*!
 * @brief jverb_table_tに従って動詞を活用する
 * @param in 変換元となる原形動詞
 * @param type 変換種類を指定(AND/TO/OR)
 * @return 活用形の動詞
 */
std::string conjugate_jverb(std::string_view in, JVerbConjugationType type)
{
    std::stringstream ss;

    for (const auto &[from, to_list] : jverb_table) {
        const auto stem_length = in.length() - from.length();
        if (in.substr(stem_length) == from) {
            ss << in.substr(0, stem_length) << to_list[enum2i(type)];
            return ss.str();
        }
    }

    constexpr std::string_view conjuctions[3] = {
        "そして",
        "ことにより",
        "ことや",
    };

    ss << in << conjuctions[enum2i(type)];
    return ss.str();
}

static const std::set<std::string_view> kinsoku_list{
    // clang-format off
    "、", "。", "，", "．", "？", "！",
    "ァ", "ィ", "ゥ", "ェ", "ォ", "ャ", "ュ", "ョ", "ッ",
    "ぁ", "ぃ", "ぅ", "ぇ", "ぉ", "ゃ", "ゅ", "ょ", "っ",
    "ー", "～",
    "」", "』", "）", "｝", "］", "》", "】",
    // clang-format on
};

/*!
 * @brief 引数で与えられた文字が行頭禁則文字であるかどうか調べる
 *
 * @param ch 調べる文字
 * @return 行頭禁則文字であるなら true、そうでないなら false
 */
bool is_kinsoku(std::string_view ch)
{
    return (ch.length() >= 2) && kinsoku_list.contains(ch);
}

/*!
 * @brief 文字コードをSJISからEUCに変換する / Convert SJIS string to EUC string
 * @param str 変換する文字列のポインタ
 * @details
 */
void sjis2euc(char *str)
{
    int i;
    unsigned char c1, c2;

    int len = strlen(str);

    std::vector<char> tmp(len + 1);

    for (i = 0; i < len; i++) {
        c1 = str[i];
        if (c1 & 0x80) {
            i++;
            c2 = str[i];
            if (c2 >= 0x9f) {
                c1 = c1 * 2 - (c1 >= 0xe0 ? 0xe0 : 0x60);
                c2 += 2;
            } else {
                c1 = c1 * 2 - (c1 >= 0xe0 ? 0xe1 : 0x61);
                c2 += 0x60 + (c2 < 0x7f);
            }
            tmp[i - 1] = c1;
            tmp[i] = c2;
        } else {
            tmp[i] = c1;
        }
    }
    tmp[len] = 0;
    strcpy(str, tmp.data());
}

/*!
 * @brief 文字コードをEUCからSJISに変換する / Convert EUC string to SJIS string
 * @param str 変換する文字列のポインタ
 * @details
 */
void euc2sjis(char *str)
{
    int i;
    unsigned char c1, c2;

    int len = strlen(str);

    std::vector<char> tmp(len + 1);

    for (i = 0; i < len; i++) {
        c1 = str[i];
        if (c1 & 0x80) {
            i++;
            c2 = str[i];
            if (c1 % 2) {
                c1 = (c1 >> 1) + (c1 < 0xdf ? 0x31 : 0x71);
                c2 -= 0x60 + (c2 < 0xe0);
            } else {
                c1 = (c1 >> 1) + (c1 < 0xdf ? 0x30 : 0x70);
                c2 -= 2;
            }

            tmp[i - 1] = c1;
            tmp[i] = c2;
        } else {
            tmp[i] = c1;
        }
    }
    tmp[len] = 0;
    strcpy(str, tmp.data());
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
    for (auto i = 0; str[i]; i++) {
        unsigned char c1;
        unsigned char c2;

        /* First byte */
        c1 = str[i];

        /* ASCII? */
        if (!(c1 & 0x80)) {
            continue;
        }

        /* Second byte */
        i++;
        c2 = str[i];

        if (((0xa1 <= c1 && c1 <= 0xdf) || (0xfd <= c1 && c1 <= 0xfe)) && (0xa1 <= c2 && c2 <= 0xfe)) {
            /* Only EUC is allowed */
            if (!code) {
                /* EUC */
                code = 2;
            }

            /* Broken string? */
            else if (code != 2) {
                /* No conversion */
                return 0;
            }
        } else {
            auto is_cp932 = (0x81 <= c1 && c1 <= 0x9f) && ((0x40 <= c2 && c2 <= 0x7e) || (0x80 <= c2 && c2 <= 0xfc));
            is_cp932 |= (0xe0 <= c1 && c1 <= 0xfc) && (0x40 <= c2 && c2 <= 0x7e);
            if (!is_cp932) {
                continue;
            }

            /* Only SJIS is allowed */
            if (!code) {
                /* SJIS */
                code = 3;
            }

            /* Broken string? */
            else if (code != 3) {
                /* No conversion */
                return 0;
            }
        }
    }

    switch (code) {
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

    for (i = 0; i < x; i++) {
        if (iskanji(s[i])) {
            i++;
        }
    }
    if ((x == i) && iskanji(s[x])) {
        return true;
    }

    return false;
}

/*!
 * @brief 文字列の文字コードがASCIIかどうかを判定する
 * @param str 判定する文字列へのポインタ
 * @return 文字列の文字コードがASCIIならTRUE、そうでなければFALSE
 */
static bool is_ascii_str(concptr str)
{
    for (; *str; str++) {
        int ch = *str;
        if (!(0x00 < ch && ch <= 0x7f)) {
            return false;
        }
    }
    return true;
}

#if defined(EUC)
#include <algorithm>
#include <iconv.h>
#include <initializer_list>
#include <vector>

// UTF-8 の文字列長は必ずしも3バイトとは限らないが、変愚蛮怒の仕様範囲では3固定.
constexpr auto ENCODING_LENGTH = 3;
class EncodingConverter {
public:
    EncodingConverter(const std::initializer_list<unsigned char> from, const std::initializer_list<unsigned char> to)
        : from(from)
        , to(to)
    {
    }

    bool equals(const unsigned char *p) const
    {
        return std::equal(from.begin(), from.end(), p);
    }

    void replace(unsigned char *p) const
    {
        std::copy_n(to.begin(), ENCODING_LENGTH, p);
    }

private:
    std::vector<unsigned char> from;
    std::vector<unsigned char> to;
};

const std::vector<EncodingConverter> encoding_characters = {
    { { 0xef, 0xbd, 0x9e }, { 0xe3, 0x80, 0x9c } }, /* FULLWIDTH TILDE -> WAVE DASH (全角チルダ → 波ダッシュ) */
    { { 0xef, 0xbc, 0x8d }, { 0xe2, 0x88, 0x92 } }, /* FULLWIDTH HYPHEN-MINUS -> MINUS SIGN (全角ハイフン → マイナス記号) */
};

/*!
 * @brief 受け取ったUTF-8文字列を調べ、特定のコードポイントの文字の置き換えを行う
 *
 * 受け取ったUTF-8の文字列に含まれる文字を1つ1つ調べて、encoding_characters で
 * 定義されている特定のコードポイントの文字の変換を行う。
 *
 * '～'と'－'は、Windows環境(CP932)とLinux/UNIX環境(EUC-JP)でUTF-8に対応する
 * 文字としてそれぞれ別のコードポイントが割り当てられており、別の環境の
 * UTF-8からシステムの文字コードに変換した時に、これらの文字は変換できず
 * 文字化けが起きてしまう。
 *
 * これを避けるため、Linux/UNIX環境(EUC-JP)ではUTF-8→EUC-JPの変換を行う前に
 * 該当するコードポイントの文字をLinux/UNIX環境のものに置き換えてから
 * 変換を行うようにするためのルーチン。
 *
 * @param str コードポイントの置き換えを行う文字列へのポインタ
 */
static void ms_to_jis_unicode(char *str)
{
    for (auto *p = (unsigned char *)str; *p; p++) {
        auto subseq_num = 0;
        if (0x00 < *p && *p <= 0x7f) {
            continue;
        }

        if ((*p & 0xe0) == 0xc0) {
            subseq_num = 1;
        }

        if ((*p & 0xf0) == 0xe0) {
            for (const auto &converter : encoding_characters) {
                if (converter.equals(p)) {
                    converter.replace(p);
                }
            }

            subseq_num = 2;
        }

        if ((*p & 0xf8) == 0xf0) {
            subseq_num = 3;
        }

        p += subseq_num;
    }
}

#elif defined(SJIS) && defined(WINDOWS)
#include <Windows.h>
#endif

#ifdef EUC
/*!
 * @brief 文字列の文字コードをUTF-8からEUC-JPに変換する
 * @param utf8_str 変換元の文字列へのポインタ
 * @param utf8_str_len 変換元の文字列の長さ(文字数ではなくバイト数)
 * @param euc_buf 変換した文字列を格納するバッファへのポインタ
 * @param euc_buf_len 変換した文字列を格納するバッファのサイズ
 * @return 変換に成功した場合変換後の文字列の長さを返す
 *         変換に失敗した場合-1を返す
 */
int utf8_to_euc(char *utf8_str, size_t utf8_str_len, char *euc_buf, size_t euc_buf_len)
{
    static iconv_t cd = nullptr;
    if (!cd) {
        cd = iconv_open("EUC-JP", "UTF-8");
    }

    ms_to_jis_unicode(utf8_str);

    size_t inlen_left = utf8_str_len;
    size_t outlen_left = euc_buf_len;
    char *in = utf8_str;
    char *out = euc_buf;

    if (iconv(cd, &in, &inlen_left, &out, &outlen_left) == (size_t)-1) {
        return -1;
    }

    return euc_buf_len - outlen_left;
}

/*!
 * @brief 文字列の文字コードをEUC-JPからUTF-8に変換する
 * @param euc_str 変換元の文字列へのポインタ
 * @param euc_str_len 変換元の文字列の長さ(文字数ではなくバイト数)
 * @param utf8_buf 変換した文字列を格納するバッファへのポインタ
 * @param utf8_buf_len 変換した文字列を格納するバッファのサイズ
 * @return 変換に成功した場合変換後の文字列の長さを返す
 *         変換に失敗した場合-1を返す
 */
int euc_to_utf8(const char *euc_str, size_t euc_str_len, char *utf8_buf, size_t utf8_buf_len)
{
    static iconv_t cd = nullptr;
    if (!cd) {
        cd = iconv_open("UTF-8", "EUC-JP");
    }

    size_t inlen_left = euc_str_len;
    size_t outlen_left = utf8_buf_len;
    const char *in = euc_str;
    char *out = utf8_buf;

    // iconv は入力バッファを書き換えないのでキャストで const を外してよい
    if (iconv(cd, (char **)&in, &inlen_left, &out, &outlen_left) == (size_t)-1) {
        return -1;
    }

    return utf8_buf_len - outlen_left;
}
#endif

/*!
 * @brief 文字コードがUTF-8の文字列をシステムの文字コードに変換する
 * @param utf8_str 変換するUTF-8の文字列へのポインタ
 * @param sys_str_buffer 変換したシステムの文字コードの文字列を格納するバッファへのポインタ
 * @param sys_str_buflen 変換したシステムの文字コードの文字列を格納するバッファの長さ
 * @return 変換に成功した場合TRUE、失敗した場合FALSEを返す
 */
static bool utf8_to_sys(char *utf8_str, char *sys_str_buffer, size_t sys_str_buflen)
{
#if defined(EUC)

    /* strlen + 1 を渡して文字列終端('\0')を含めて変換する */
    return utf8_to_euc(utf8_str, strlen(utf8_str) + 1, sys_str_buffer, sys_str_buflen) >= 0;

#elif defined(SJIS) && defined(WINDOWS)

    int input_len = strlen(utf8_str) + 1; /* include termination character */

    std::vector<WCHAR> utf16buf(input_len);

    /* UTF-8 -> UTF-16 */
    if (MultiByteToWideChar(CP_UTF8, 0, utf8_str, input_len, utf16buf.data(), input_len) == 0) {
        return false;
    }

    /* UTF-8 -> SJIS(CP932) */
    if (WideCharToMultiByte(932, 0, utf16buf.data(), -1, sys_str_buffer, sys_str_buflen, nullptr, nullptr) == 0) {
        return false;
    }

    return true;

#endif
}

/*!
 * @brief 受け取った文字列の文字コードを推定し、システムの文字コードへ変換する
 * @param strbuf 変換する文字列を格納したバッファへのポインタ。
 *               バッファは変換した文字列で上書きされる。
 *               UTF-8からSJISもしくはEUCへの変換を想定しているのでバッファの長さが足りなくなることはない。
 * @param buflen バッファの長さ。
 */
void guess_convert_to_system_encoding(char *strbuf, int buflen)
{
    if (is_ascii_str(strbuf)) {
        return;
    }

    if (is_utf8_str(strbuf)) {
        std::vector<char> work(buflen);
        angband_strcpy(work.data(), strbuf, buflen);
        if (!utf8_to_sys(work.data(), strbuf, buflen)) {
            msg_print("警告:文字コードの変換に失敗しました");
            msg_print(nullptr);
        }
    }
}

/*!
 * @brief 変愚蛮怒基準のポンド→キログラム変換定義(全体)
 * @param x ポンド値
 * @return キログラム値
 * @details 帝国ポンドとは完全にずれているが、気にするな！
 */
static int lb_to_kg_all(int x)
{
    return x * 5;
}

/*!
 * @brief 変愚蛮怒基準のポンド→キログラム変換定義(整数部)
 * @param x ポンド値
 * @return キログラム値の整数部
 */
int lb_to_kg_integer(int x)
{
    return lb_to_kg_all(x) / 100;
}

/*!
 * 変愚蛮怒基準のポンド→キログラム変換定義(小数部)
 * @param x ポンド値
 * @return キログラム値の小数部
 */
int lb_to_kg_fraction(int x)
{
    return (lb_to_kg_all(x) % 100) / 10;
}

#endif /* JP */
