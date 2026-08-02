/*!
 *  @file character-encoding.cpp
 *  @brief 文字コード処理関数
 */

#include "locale/character-encoding.h"
#include <range/v3/algorithm.hpp>

#ifdef JP
#include "system/angband-exceptions.h"
#include "system/angband.h"
#include "view/display-messages.h"
#include <vector>
#ifdef WIN32
#include <windows.h>
#undef min
#undef max
#else
#include "util/finalizer.h"
#include <algorithm>
#include <iconv.h>
#include <initializer_list>
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
 */
CharacterEncoding codeconv(char *str)
{
    auto encoding = CharacterEncoding::UNKNOWN;
    for (auto i = 0; str[i]; i++) {
        /* First byte */
        const auto c1 = static_cast<unsigned char>(str[i]);

        /* ASCII? */
        if (!(c1 & 0x80)) {
            encoding = CharacterEncoding::US_ASCII;
            continue;
        }

        /* Second byte */
        i++;
        const auto c2 = static_cast<unsigned char>(str[i]);

        const auto is_euc_jp = ((0xa1 <= c1 && c1 <= 0xdf) || (0xfd <= c1 && c1 <= 0xfe)) && (0xa1 <= c2 && c2 <= 0xfe);
        auto is_cp932 = (0x81 <= c1 && c1 <= 0x9f) && ((0x40 <= c2 && c2 <= 0x7e) || (0x80 <= c2 && c2 <= 0xfc));
        is_cp932 |= (0xe0 <= c1 && c1 <= 0xfc) && (0x40 <= c2 && c2 <= 0x7e);

        if (is_euc_jp) {
            /* Only EUC is allowed */
            if (encoding == CharacterEncoding::UNKNOWN || encoding == CharacterEncoding::US_ASCII || encoding == CharacterEncoding::EUC_JP) {
                encoding = CharacterEncoding::EUC_JP;
                continue;
            }
        } else if (is_cp932) {
            /* Only SJIS is allowed */
            if (encoding == CharacterEncoding::UNKNOWN || encoding == CharacterEncoding::US_ASCII || encoding == CharacterEncoding::SHIFT_JIS) {
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

#if defined(EUC)

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
 * '～'と'－'は、Windows環境(CP932)とLinux/UNIX環境(EUC-JP)でUTF-8に対応する
 * 文字としてそれぞれ別のコードポイントが割り当てられており、別の環境の
 * UTF-8からシステムの文字コードに変換した時に、これらの文字は変換できず
 * 文字化けが起きてしまう。
 *
 * Linux/UNIX環境(EUC-JP)ではUTF-8→EUC-JPの変換を行う前に該当するコードポイントの
 * 文字をLinux/UNIX環境のものに置き換えてから変換を行う。
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
 * @brief 文字コードがUTF-8の文字列をシステムの文字コードに変換する (内部バッファ版)
 * @param str 変換するUTF-8の文字列
 * @param sys_str_buffer 変換したシステムの文字コードの文字列を格納するバッファへのポインタ
 * @param sys_str_buflen 変換したシステムの文字コードの文字列を格納するバッファの長さ
 * @return 変換に成功した場合TRUE、失敗した場合FALSEを返す
 */
static bool utf8_to_sys(std::string_view str, char *sys_str_buffer, size_t sys_str_buflen)
{
    std::string utf8_str(str);

#if defined(EUC)

    /* length() + 1 を渡して文字列終端('\0')を含めて変換する */
    return utf8_to_euc(utf8_str.data(), utf8_str.length() + 1, sys_str_buffer, sys_str_buflen) >= 0;

#elif defined(SJIS) && defined(WINDOWS)

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

#else
    return false;
#endif
}

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
#elif defined(SJIS) && defined(WINDOWS)
    /* SJIS(CP932) -> UTF-16 */
    std::vector<WCHAR> utf16buf(str.length());
    const auto utf16_len = MultiByteToWideChar(932, 0, str.data(), str.size(), utf16buf.data(), utf16buf.size());
    if (utf16_len == 0) {
        return tl::nullopt;
    }

    /* UTF-16 -> UTF-8 */
    std::vector<char> utf8buf(str.length() * 2 + 1);
    const auto utf8_len = WideCharToMultiByte(CP_UTF8, 0, utf16buf.data(), utf16_len, utf8buf.data(), utf8buf.size(), nullptr, nullptr);
    if (utf8_len == 0) {
        return tl::nullopt;
    }

    return tl::make_optional<std::string>(utf8buf.data(), utf8_len);
#else
    return tl::nullopt;
#endif
}

/*!
 * @brief UTF-8からシステムの文字コードに変換する
 *
 * @param str UTF-8の文字列
 * @return システムの文字コードに変換した文字列
 *         変換に失敗した場合はtl::nullopt
 */
tl::optional<std::string> utf8_to_sys(std::string_view str)
{
    // UTF-8 -> SJIS or EUC でバイト長が増えることはないので、終端文字分を含めて元の文字列と同じ長さを確保しておけばよい
    std::vector<char> sys_str_buf(str.length() + 1);

    if (!utf8_to_sys(str, sys_str_buf.data(), sys_str_buf.size())) {
        return tl::nullopt;
    }

    return tl::make_optional<std::string>(sys_str_buf.data());
}

/*!
 * @brief 受け取った文字列の文字コードを推定し、システムの文字コードへ変換する
 * @param strbuf 変換する文字列を格納したバッファへのポインタ。
 *               バッファは変換した文字列で上書きされる。
 *               UTF-8からSJISもしくはEUCへの変換を想定しているのでバッファの長さが足りなくなることはない。
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

/*!
 * @brief UTF-8文字列をローカルエンコーディング（Shift-JIS/EUC-JP）に変換する
 * @param str_utf8 変換元のUTF-8文字列
 * @return ローカルエンコーディングに変換した文字列
 */
std::string utf8_to_local(std::string_view str_utf8)
{
    if (str_utf8.empty()) {
        return "";
    }

#ifdef JP
#ifdef WIN32
    // UTF-8 -> UTF-16 (wide string)
    auto utf8_length = static_cast<int>(str_utf8.length());
    const auto wide_length = MultiByteToWideChar(CP_UTF8, 0, str_utf8.data(), utf8_length, NULL, 0);
    if (wide_length == 0) {
        THROW_EXCEPTION(std::runtime_error, "Failed to convert UTF-8 to wide string");
    }

    std::wstring wide_str(wide_length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str_utf8.data(), utf8_length, wide_str.data(), wide_length);

    // UTF-16 -> Shift-JIS (CP932)
    const auto sjis_length = WideCharToMultiByte(932, 0, wide_str.data(), wide_length, NULL, 0, NULL, NULL);
    if (sjis_length == 0) {
        THROW_EXCEPTION(std::runtime_error, "Failed to convert UTF-16 to Shift-JIS");
    }

    std::string str_sjis(sjis_length, '\0');
    WideCharToMultiByte(932, 0, wide_str.data(), wide_length, str_sjis.data(), sjis_length, NULL, NULL);
    return str_sjis;
#else
    // Unix-like systems implementation for UTF-8 to local encoding conversion
    const auto cd = iconv_open("EUC-JP", "UTF-8");
    const auto finalizer = util::make_finalizer([&cd] {
        if (cd != reinterpret_cast<iconv_t>(-1)) {
            iconv_close(cd);
        }
    });
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        THROW_EXCEPTION(std::runtime_error, "iconv_open failed: cannot open UTF-8 to EUC-JP converter");
    }

    auto *in_buf = const_cast<char *>(str_utf8.data());
    const auto in_length = str_utf8.length();
    auto in_bytes = in_length;

    std::string str_eucjp;
    str_eucjp.resize(str_utf8.length() * 2 + 4); //!< 変換途中の一時領域も含め、予め十分なサイズを確保しておく.
    auto *out_buf = str_eucjp.data();
    const auto out_length = str_eucjp.size();
    auto out_bytes = out_length;

    const auto iconv_result = iconv(cd, &in_buf, &in_bytes, &out_buf, &out_bytes);
    if (iconv_result == static_cast<size_t>(-1)) {
        if (errno == EILSEQ) {
            THROW_EXCEPTION(std::runtime_error, "iconv: invalid sequence (EUC-JPに変換できない文字)");
        } else if (errno == E2BIG) {
            THROW_EXCEPTION(std::runtime_error, "iconv: output buffer too small");
        } else {
            THROW_EXCEPTION(std::runtime_error, "iconv conversion failed");
        }
    }

    // 実際に使った長さに調整する.
    str_eucjp.resize(str_eucjp.size() - out_bytes);
    str_eucjp.shrink_to_fit();
    return str_eucjp;
#endif
#else
    return std::string(str_utf8);
#endif
}
