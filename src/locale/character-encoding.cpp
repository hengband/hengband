/*!
 *  @file character-encoding.cpp
 *  @brief 文字コード処理関数
 */

#include "locale/character-encoding.h"
#include <range/v3/algorithm.hpp>

#ifdef JP
#include "system/angband.h"
#include "view/display-messages.h"
#include <algorithm>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <array>
#include <iconv.h>
#include <utility>
#endif
#endif

/*!
 * @brief 文字列の最初の文字のUTF-8エンコーディングにおけるバイト長を返す
 *
 * UTF-8エンコーディングの文字列が渡されるのを想定し、
 * その文字列の最初の文字のバイト長を返す。
 * UTF-8エンコーディングとして適合しなければ0を返す。
 * また文字列が空の場合も0を返す。
 *
 * @note UTF-8エンコーディングの厳密なバリデーションにはなっていない。
 *       2バイト目以降は0x80-0xBF固定ではなく、バイト長・何バイト目かなど
 *       によって若干変化するが、ここでは簡便のため0x80-0xBFの範囲のみ
 *       チェックする
 *
 * @param str 判定する文字列
 *
 * @return 最初の文字のバイト長を返す。
 *         空文字列もしくはUTF-8エンコーディングに適合しない場合は0を返す。
 */
int utf8_next_char_byte_length(std::string_view str)
{
    if (str.empty()) {
        return 0;
    }

    const auto start_byte = static_cast<unsigned char>(str.front());
    size_t length = 0;

    // バイト長の判定
    if (start_byte <= 0x7f) {
        return 1;
    } else if ((start_byte & 0xe0) == 0xc0) {
        length = 2;
    } else if ((start_byte & 0xf0) == 0xe0) {
        length = 3;
    } else if ((start_byte & 0xf8) == 0xf0) {
        length = 4;
    } else {
        return 0;
    }

    if (str.size() < length) {
        return 0;
    }

    const auto trailing_bytes = str.substr(1, length - 1);
    const auto is_valid_trailing_byte = [](unsigned char c) { return (c & 0xc0) == 0x80; };

    return ranges::all_of(trailing_bytes, is_valid_trailing_byte) ? length : 0;
}

/*!
 * @brief 文字列がUTF-8の文字列として適合かどうかを判定する
 *
 * @param str 判定する文字列
 *
 * @return 文字列がUTF-8として適合ならtrue、そうでなければfalse
 */
bool is_utf8_str(std::string_view str)
{
    while (!str.empty()) {
        const int byte_length = utf8_next_char_byte_length(str);

        if (byte_length == 0) {
            return false;
        }

        str.remove_prefix(byte_length);
    }

    return true;
}

#ifdef JP

/*!
 * @brief 文字コードをSJISからEUCに変換する / Convert SJIS string to EUC string
 * @details 上位ビットの立ったバイトはすべて2バイト文字の1バイト目として扱い、JIS X 0208 の文字だけを変換できる。
 *          半角カナ (1バイトの A1-DF) と、1バイト目が F0 以上の文字 (ユーザー定義文字・IBM 拡張文字) は正しく変換できない。
 * @param str 変換する文字列のポインタ
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
 * @details 上位ビットの立ったバイトはすべて2バイト文字の1バイト目として扱い、JIS X 0208 の文字だけを変換できる。
 *          半角カナ (8E xx) と JIS X 0212 の文字 (8F xx xx) は正しく変換できない。
 * @param str 変換する文字列のポインタ
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
 * @return 変換前の文字コード
 *         ASCII と EUC-JP/Shift_JIS のどちらとも解釈できる文字のみの文字列や、壊れた文字列など、
 *         文字コードを判定できなかった場合は UNKNOWN
 *         (ASCII は判定に使わないので US_ASCII を返すことはない)
 */
CharacterEncoding codeconv(char *str)
{
    auto encoding = CharacterEncoding::UNKNOWN;
    for (auto i = 0; str[i]; i++) {
        /* First byte */
        const auto c1 = static_cast<unsigned char>(str[i]);

        /* ASCII は判定に影響しないので読み飛ばす */
        if (!(c1 & 0x80)) {
            continue;
        }

        /* Second byte */
        i++;
        const auto c2 = static_cast<unsigned char>(str[i]);

        /*
         * Shift_JIS の1バイト目 F0-FC (ユーザー定義文字・IBM 拡張文字) は sjis2euc() で
         * EUC-JP に変換できない (1バイト目が 0x00 や制御文字になる) ため、Shift_JIS とはみなさない。
         * EUC-JP の1バイト目 F5-FE には JIS X 0208 の文字が無いため、EUC-JP ともみなさない。
         * これで、髙 (FB FC) などの IBM 拡張文字を EUC-JP と誤判定しなくなる。
         */
        const auto in_range = [](unsigned char c, unsigned char lo, unsigned char hi) { return (lo <= c) && (c <= hi); };
        const auto is_euc_jp = in_range(c1, 0xa1, 0xf4) && in_range(c2, 0xa1, 0xfe);
        const auto is_cp932 = (in_range(c1, 0x81, 0x9f) || in_range(c1, 0xe0, 0xef)) && (in_range(c2, 0x40, 0x7e) || in_range(c2, 0x80, 0xfc));

        /* どちらとも解釈できる文字 (1バイト目 E0-EF、2バイト目 A1-FC の第2水準漢字など) は判定に使わない */
        if (is_euc_jp && is_cp932) {
            continue;
        }

        if (is_euc_jp) {
            /* Only EUC is allowed */
            if (encoding == CharacterEncoding::UNKNOWN || encoding == CharacterEncoding::EUC_JP) {
                encoding = CharacterEncoding::EUC_JP;
                continue;
            }
        } else if (is_cp932) {
            /* Only SJIS is allowed */
            if (encoding == CharacterEncoding::UNKNOWN || encoding == CharacterEncoding::SHIFT_JIS) {
                encoding = CharacterEncoding::SHIFT_JIS;
                continue;
            }
        }

        /* Broken string, no conversion */
        return CharacterEncoding::UNKNOWN;
    }

    switch (encoding) {
#ifdef EUC
    case CharacterEncoding::SHIFT_JIS:
        sjis2euc(str);
        break;
#endif

#ifdef SJIS
    case CharacterEncoding::EUC_JP:
        euc2sjis(str);
        break;
#endif
    default:
        break;
    }

    return encoding;
}

/*!
 * @brief 文字列sのxバイト目が漢字の1バイト目かどうか判定する
 * @param s 判定する文字列のポインタ
 * @param x 判定する位置(バイト)
 * @return 漢字の1バイト目ならばTRUE
 */
bool iskanji2(const char *s, int x)
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
static bool is_ascii_str(const char *str)
{
    for (; *str; str++) {
        int ch = *str;
        if (!(0x00 < ch && ch <= 0x7f)) {
            return false;
        }
    }
    return true;
}

#ifdef EUC

namespace {
/*!
 * @brief UTF-8 から EUC-JP に変換する前に置き換える文字 (置き換え前, 置き換え後) の UTF-8 のバイト列
 * @details 置き換え前の文字はどちらも継続バイトにならない 0xEF で始まり、置き換え後のバイト列に 0xEF は無い。
 * そのため、文字の途中から誤って一致したり、置き換えた結果がまた一致したりすることはない。
 */
constexpr std::array<std::pair<std::string_view, std::string_view>, 2> MS_TO_JIS_CHARACTERS = { {
    { "\xef\xbd\x9e", "\xe3\x80\x9c" }, /* FULLWIDTH TILDE -> WAVE DASH (全角チルダ → 波ダッシュ) */
    { "\xef\xbc\x8d", "\xe2\x88\x92" }, /* FULLWIDTH HYPHEN-MINUS -> MINUS SIGN (全角ハイフン → マイナス記号) */
} };
}

/*!
 * @brief 受け取ったUTF-8文字列を調べ、特定のコードポイントの文字の置き換えを行う
 *
 * '～'と'－'は、Windows環境(CP932)とLinux/UNIX環境(EUC-JP)でUTF-8に対応する
 * 文字としてそれぞれ別のコードポイントが割り当てられており、別の環境の
 * UTF-8からシステムの文字コードに変換した時に、これらの文字は変換できず
 * 文字化けが起きてしまう。
 *
 * Linux/UNIX環境(EUC-JP)ではUTF-8→EUC-JPの変換を行う前に該当するコードポイントの
 * 文字をLinux/UNIX環境のものに置き換えてから変換を行う。
 *
 * @param str コードポイントの置き換えを行う文字列。途中に '\0' があっても、文字列の長さまで置き換える
 */
static void ms_to_jis_unicode(std::string &str)
{
    for (const auto &[from, to] : MS_TO_JIS_CHARACTERS) {
        for (auto pos = str.find(from); pos != std::string::npos; pos = str.find(from, pos + from.length())) {
            str.replace(pos, from.length(), to);
        }
    }
}

/*!
 * @brief 文字列の文字コードをUTF-8からEUC-JPに変換する
 * @details 変愚蛮怒は全角文字を2バイト固定として扱うため、EUC-JP で3バイトになる JIS X 0212 の文字
 *          (8F xx xx、é など) を正しく表示できない。そうした文字は '?' に置き換える
 *          (Windows 版でも CP932 に無い文字は '?' などに置き換わる)。
 * @param utf8_str 変換元の文字列
 * @return EUC-JPに変換した文字列。変換に失敗した場合はtl::nullopt
 */
tl::optional<std::string> utf8_to_euc(std::string_view utf8_str)
{
    // 空文字列では一時バッファの data() が nullptr になり、iconv() に渡せないので、そのまま返す
    if (utf8_str.empty()) {
        return std::string();
    }

    static const auto cd = iconv_open("EUC-JP", "UTF-8");
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        return tl::nullopt;
    }

    std::string utf8(utf8_str);
    ms_to_jis_unicode(utf8);

    // JIS X 0212 の文字は UTF-8 の2バイトが EUC-JP の3バイトになるので、'?' に置き換える前の変換結果が収まるよう2倍確保する
    std::string euc(utf8.length() * 2, '\0');
    size_t inlen_left = utf8.length();
    size_t outlen_left = euc.length();
    char *in = utf8.data();
    char *out = euc.data();

    if (iconv(cd, &in, &inlen_left, &out, &outlen_left) == (size_t)-1) {
        return tl::nullopt;
    }

    // JIS X 0212 の文字 (3バイト) を '?' に置き換えながら、その場で前に詰める (書き込み位置は読み込み位置を追い越さない)。
    // EUC-JP の2バイト目以降は A1 以上なので、0x8F は JIS X 0212 の文字の先頭にしか現れない
    const auto converted_len = euc.length() - outlen_left;
    size_t euc_len = 0;
    for (size_t i = 0; i < converted_len; euc_len++) {
        if (static_cast<unsigned char>(euc[i]) == 0x8f) {
            euc[euc_len] = '?';
            i += 3;
        } else {
            euc[euc_len] = euc[i++];
        }
    }

    euc.resize(euc_len);
    return euc;
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
    static const auto cd = iconv_open("UTF-8", "EUC-JP");
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        return -1;
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

#if defined(SJIS) && defined(_WIN32)
/*!
 * @brief 文字コードがUTF-8の文字列をシステムの文字コードに変換する (Windows 版の内部バッファ版)
 * @param str 変換するUTF-8の文字列
 * @param sys_str_buffer 変換したシステムの文字コードの文字列を格納するバッファへのポインタ
 * @param sys_str_buflen 変換したシステムの文字コードの文字列を格納するバッファの長さ
 * @return 変換に成功した場合TRUE、失敗した場合FALSEを返す
 */
static bool utf8_to_sys(std::string_view str, char *sys_str_buffer, size_t sys_str_buflen)
{
    std::string utf8_str(str);
    int input_len = utf8_str.length() + 1; /* include termination character */

    std::vector<WCHAR> utf16buf(input_len);

    /* UTF-8 -> UTF-16 */
    if (MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), input_len, utf16buf.data(), input_len) == 0) {
        return false;
    }

    /* UTF-8 -> SJIS(CP932) */
    if (WideCharToMultiByte(932, 0, utf16buf.data(), -1, sys_str_buffer, sys_str_buflen, nullptr, nullptr) == 0) {
        return false;
    }

    return true;
}
#endif

/*!
 * @brief システムの文字コードからUTF-8に変換する
 * @param str システムの文字コードの文字列
 * @return UTF-8に変換した文字列
 *         変換に失敗した場合はtl::nullopt
 */
tl::optional<std::string> sys_to_utf8(std::string_view str)
{
#if defined(EUC)
    std::string utf8str(str.length() * 2 + 1, '\0');
    const auto len = euc_to_utf8(str.data(), str.length(), utf8str.data(), utf8str.size());

    return (len >= 0) ? tl::make_optional(std::move(utf8str.erase(len))) : tl::nullopt;
#elif defined(SJIS) && defined(_WIN32)
    // 長さ 0 を渡すと MultiByteToWideChar() が失敗するので、空文字列はそのまま返す
    if (str.empty()) {
        return tl::make_optional<std::string>();
    }

    // CP932 の半角カナは UTF-8 で3バイトになるなど、変換後の長さは一定の倍率に収まらないので、必要な長さを問い合わせてから変換する
    // SJIS(CP932) -> UTF-16
    const auto sjis_len = static_cast<int>(str.length());
    const auto utf16_len = MultiByteToWideChar(932, 0, str.data(), sjis_len, nullptr, 0);
    if (utf16_len == 0) {
        return tl::nullopt;
    }

    std::wstring utf16_str(utf16_len, L'\0');
    MultiByteToWideChar(932, 0, str.data(), sjis_len, utf16_str.data(), utf16_len);

    // UTF-16 -> UTF-8
    const auto utf8_len = WideCharToMultiByte(CP_UTF8, 0, utf16_str.data(), utf16_len, nullptr, 0, nullptr, nullptr);
    if (utf8_len == 0) {
        return tl::nullopt;
    }

    std::string utf8_str(utf8_len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, utf16_str.data(), utf16_len, utf8_str.data(), utf8_len, nullptr, nullptr);
    return tl::make_optional(std::move(utf8_str));
#else
    return tl::nullopt;
#endif
}

/*!
 * @brief UTF-8からシステムの文字コードに変換する
 *
 * @param str UTF-8の文字列
 * @return システムの文字コードに変換した文字列
 *         変換に失敗した場合や、途中に '\0' を含む場合はtl::nullopt
 */
tl::optional<std::string> utf8_to_sys(std::string_view str)
{
    // Windows 版は変換結果を '\0' 終端の文字列として取り出すので、途中に '\0' があると後ろが黙って欠けてしまう。
    // EUC-JP 版と挙動を揃えるため、どちらの版でも不正な入力として変換しない
    if (str.find('\0') != std::string_view::npos) {
        return tl::nullopt;
    }

#if defined(EUC)
    return utf8_to_euc(str);
#elif defined(SJIS) && defined(_WIN32)
    // UTF-8 -> SJIS でバイト長が増えることはないので、終端文字分を含めて元の文字列と同じ長さを確保しておけばよい
    std::vector<char> sys_str_buf(str.length() + 1);
    if (!utf8_to_sys(str, sys_str_buf.data(), sys_str_buf.size())) {
        return tl::nullopt;
    }

    return tl::make_optional<std::string>(sys_str_buf.data());
#else
    return tl::nullopt;
#endif
}

/*!
 * @brief 受け取った文字列の文字コードを推定し、システムの文字コードへ変換する
 * @param strbuf 変換する文字列を格納したバッファへのポインタ。
 *               バッファは変換した文字列で上書きされる。
 *               変換した文字列がバッファに収まらない場合は変換せず、警告を表示する。
 * @param buflen バッファの長さ。
 * @return 変換後の文字列の長さ（終端文字は含まない）
 */
size_t guess_convert_to_system_encoding(char *strbuf, int buflen)
{
    if (is_ascii_str(strbuf)) {
        return std::string_view(strbuf).length();
    }

    if (is_utf8_str(strbuf)) {
        const auto sys_str = utf8_to_sys(strbuf);
        if (!sys_str || std::ssize(*sys_str) >= buflen) {
            msg_print("警告:文字コードの変換に失敗しました");
            msg_erase();
            return std::string_view(strbuf).length();
        }

        std::copy(sys_str->begin(), sys_str->end(), strbuf);
        strbuf[sys_str->length()] = '\0';
        return sys_str->length();
    }

    return std::string_view(strbuf).length();
}

#endif /* JP */
