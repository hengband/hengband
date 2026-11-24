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
#include <array>
#include <utility>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
#include <iconv.h>
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
 * 先頭バイトは C2-DF・E0-EF・F0-F4 だけを受け付け、冗長な表現 (E0 80-9F・F0 80-8F)、
 * サロゲート (ED A0-BF)、U+10FFFF を超える値 (F4 90-BF) は 2 バイト目の範囲で弾く。
 * EUC-JP の文字 (「澄」の C0 A1 など) を UTF-8 と取り違えないようにするため。
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
    const auto length = utf8_lead_byte_length(start_byte);
    if (length <= 1) {
        return length;
    }

    if (std::ssize(str) < length) {
        return 0;
    }

    // 2 バイト目に許す範囲の判定
    unsigned char second_min = 0x80;
    unsigned char second_max = 0xbf;
    switch (start_byte) {
    case 0xe0:
        second_min = 0xa0;
        break;
    case 0xed:
        second_max = 0x9f;
        break;
    case 0xf0:
        second_min = 0x90;
        break;
    case 0xf4:
        second_max = 0x8f;
        break;
    default:
        break;
    }

    const auto second_byte = static_cast<unsigned char>(str[1]);
    if ((second_byte < second_min) || (second_byte > second_max)) {
        return 0;
    }

    const auto trailing_bytes = str.substr(2, static_cast<size_t>(length - 2));
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

#ifdef UTF8
namespace {
/*!
 * @brief 文字列の中の文字を、対応表に従ってすべて置き換える
 * @param str 置き換える文字列。途中に '\0' があっても、文字列の長さまで置き換える
 * @param pairs (置き換え前, 置き換え後) の UTF-8 のバイト列の表
 */
template <size_t N>
void replace_characters(std::string &str, const std::array<std::pair<std::string_view, std::string_view>, N> &pairs)
{
    for (const auto &[from, to] : pairs) {
        for (auto pos = str.find(from); pos != std::string::npos; pos = str.find(from, pos + to.length())) {
            str.replace(pos, from.length(), to);
        }
    }
}
}
#endif

#ifdef _WIN32
namespace {
/*!
 * @brief 文字列を UTF-16 に変換する (Windows 版)
 * @param str 変換元の文字列
 * @param code_page 変換元のコードページ (CP_UTF8、932 など)
 * @param flags MultiByteToWideChar() に渡すフラグ。MB_ERR_INVALID_CHARS なら、読めないバイトがあれば失敗とする
 * @return UTF-16 に変換した文字列。変換に失敗した場合は tl::nullopt
 */
tl::optional<std::wstring> to_utf16(std::string_view str, UINT code_page, DWORD flags)
{
    // 長さ 0 を渡すと MultiByteToWideChar() が失敗するので、空文字列はそのまま返す
    if (str.empty()) {
        return std::wstring();
    }

    const auto str_len = static_cast<int>(str.length());
    const auto utf16_len = MultiByteToWideChar(code_page, flags, str.data(), str_len, nullptr, 0);
    if (utf16_len == 0) {
        return tl::nullopt;
    }

    std::wstring utf16_str(utf16_len, L'\0');
    MultiByteToWideChar(code_page, flags, str.data(), str_len, utf16_str.data(), utf16_len);
    return utf16_str;
}

/*!
 * @brief UTF-16 の文字列を変換する (Windows 版)
 * @param str 変換元の文字列
 * @param code_page 変換先のコードページ (CP_UTF8、932 など)
 * @param flags WideCharToMultiByte() に渡すフラグ
 * @param used_default_char 変換できない文字を既定の文字 ('?') にしたかを受け取る変数。不要なときと CP_UTF8 のときは nullptr
 * @return 変換した文字列。変換に失敗した場合は tl::nullopt
 */
tl::optional<std::string> from_utf16(std::wstring_view str, UINT code_page, DWORD flags, BOOL *used_default_char)
{
    if (str.empty()) {
        return std::string();
    }

    const auto str_len = static_cast<int>(str.length());
    const auto converted_len = WideCharToMultiByte(code_page, flags, str.data(), str_len, nullptr, 0, nullptr, used_default_char);
    if (converted_len == 0) {
        return tl::nullopt;
    }

    std::string converted(converted_len, '\0');
    WideCharToMultiByte(code_page, flags, str.data(), str_len, converted.data(), converted_len, nullptr, used_default_char);
    return converted;
}

}
#endif

#ifdef UTF8
namespace {
#ifdef _WIN32
/*!
 * @brief Windows で EUC-JP の変換に使うコードページ
 * @details EUC-JP-MS に当たる 51932 は MultiByteToWideChar() で使えないので、20932 を使う。
 * 20932 は EUC-JP-MS と同じく NEC 特殊文字 (AD A1 の ① など) とユーザー定義文字を読めるが、JIS X 0212 の文字 (8F xx xx) は読めない。
 * 全角チルダなど EUC-JP-MS と対応の違う文字は、iconv の EUC-JP と同じく JIS の対応で変換する。
 */
constexpr UINT EUC_JP_CODE_PAGE = 20932;
constexpr UINT SHIFT_JIS_CODE_PAGE = 932; //!< Windows で Shift_JIS の変換に使うコードページ (CP932)
#endif

/*!
 * @brief 旧版の EUC-JP の文字列に現れない、読めないバイトを '?' に置き換える
 * @param str 置き換える文字列
 * @param replace_invalid 読めないバイトを置き換えるか
 * @return 読めないバイトが無いか、置き換えたら true。置き換えずに読めないバイトを見つけたら false
 * @details glibc の iconv と Windows の 20932 は EUC-JP の 80-A0 (8E・8F を除く) と FF を C1 制御文字などとして通すが、
 * 旧版の EUC-JP の文字列には現れないので、読めないバイトとする
 */
bool replace_invalid_euc_bytes(std::string &str, bool replace_invalid)
{
    for (auto &c : str) {
        const auto byte = static_cast<unsigned char>(c);
        if (((byte >= 0x80) && (byte <= 0xa0) && (byte != 0x8e) && (byte != 0x8f)) || (byte == 0xff)) {
            if (!replace_invalid) {
                return false;
            }

            c = '?';
        }
    }

    return true;
}

#ifndef _WIN32
/*!
 * @brief EUC-JP の文字列で、先頭バイトの後ろに文字の続きとして読めないバイトが続く位置を探す
 * @param str 探す文字列
 * @param pos 探し始める位置 (文字の境界)
 * @return 見つけた先頭バイトの位置と true。見つからなければ、探し終えた位置 (文字の境界) と false
 * @details 続きとして読めるのは、8E (半角カナ) では A1-DF、ほかの先頭バイト (A1-FE・8F) では A1-FE である。
 * glibc の iconv はこの並びを読めないとするが、macOS の iconv は 2 バイト目が A1 未満の並び (C6 3A など) や
 * 8F の後ろの ASCII を別の文字として読んでしまうので、iconv に渡す前に見つける。
 * 文字列の末尾で欠けた文字は iconv が読めないとするので、ここでは扱わない。
 * 読めないバイトの後ろから探し直すたびに残り全体を探さないよう、一度に探すのは 256 バイトほどにする。
 */
std::pair<size_t, bool> find_broken_euc_char(std::string_view str, size_t pos)
{
    constexpr size_t SEARCH_LENGTH = 256;
    const auto search_end = std::min(str.length(), pos + SEARCH_LENGTH);
    while (pos < search_end) {
        const auto lead = static_cast<unsigned char>(str[pos]);
        const auto char_len = std::min(legacy_lead_byte_length(str[pos], CharacterEncoding::EUC_JP), str.length() - pos);
        const unsigned char trail_max = (lead == 0x8e) ? 0xdf : 0xfe;
        const auto is_trail_byte = [trail_max](char c) {
            const auto trail = static_cast<unsigned char>(c);
            return (trail >= 0xa1) && (trail <= trail_max);
        };
        if (!ranges::all_of(str.substr(pos + 1, char_len - 1), is_trail_byte)) {
            return { pos, true };
        }

        pos += char_len;
    }

    return { pos, false };
}
#endif

/*!
 * @brief EUC-JP を EUC-JP-MS でなく EUC-JP として読んだ文字列の、EUC-JP-MS と対応が違う文字を、EUC-JP-MS で読んだ文字に戻す
 * @param utf8 EUC-JP から UTF-8 に変換した文字列
 */
void replace_jis_to_ms_characters(std::string &utf8)
{
    // EUC-JP と EUC-JP-MS で対応する文字が違うもの (EUC-JP で読んだ文字, EUC-JP-MS で読んだ文字)
    constexpr std::array<std::pair<std::string_view, std::string_view>, 6> JIS_TO_MS_CHARACTERS = { {
        { "\xe3\x80\x9c", "\xef\xbd\x9e" }, /* A1C1: WAVE DASH -> FULLWIDTH TILDE (波ダッシュ → 全角チルダ) */
        { "\xe2\x80\x96", "\xe2\x88\xa5" }, /* A1C2: DOUBLE VERTICAL LINE -> PARALLEL TO (双柱 → 平行) */
        { "\xe2\x88\x92", "\xef\xbc\x8d" }, /* A1DD: MINUS SIGN -> FULLWIDTH HYPHEN-MINUS (マイナス記号 → 全角ハイフンマイナス) */
        { "\xc2\xa2", "\xef\xbf\xa0" }, /* A1F1: CENT SIGN -> FULLWIDTH CENT SIGN (セント記号 → 全角のセント記号) */
        { "\xc2\xa3", "\xef\xbf\xa1" }, /* A1F2: POUND SIGN -> FULLWIDTH POUND SIGN (ポンド記号 → 全角のポンド記号) */
        { "\xc2\xac", "\xef\xbf\xa2" }, /* A2CC: NOT SIGN -> FULLWIDTH NOT SIGN (否定記号 → 全角の否定記号) */
    } };
    replace_characters(utf8, JIS_TO_MS_CHARACTERS);
}

#ifdef _WIN32
/*!
 * @brief 旧版のデータの文字を、Windows のコードページで変換せずに読めない文字とするかを返す
 * @param chr 文字 (legacy_lead_byte_length() のバイト数で切り出したもの。文字列の末尾で欠けていれば短い)
 * @param encoding chr の文字コード (EUC-JP か Shift_JIS)
 * @return iconv の版と同じく読めない文字とするなら true
 * @details MB_ERR_INVALID_CHARS を付けても、20932 は 8F を U+008F に、2 バイト目が 21-7E の 2 バイトを JIS X 0212 の文字に、
 * CP932 は 80 を U+0080 に変換してしまう。iconv ではどれも読めない。
 * また、20932 は iconv で読める JIS X 0212 の文字 (8F xx xx) を読めない。
 */
bool is_unreadable_legacy_char(std::string_view chr, CharacterEncoding encoding)
{
    const auto lead = static_cast<unsigned char>(chr.front());
    if (encoding == CharacterEncoding::SHIFT_JIS) {
        return lead == 0x80;
    }

    return (lead == 0x8f) || ((chr.length() == 2) && (static_cast<unsigned char>(chr[1]) < 0xa1));
}

/*!
 * @brief 旧版のデータの文字列を UTF-8 に変換する (Windows 版)
 * @param str 変換する文字列
 * @param encoding str の文字コード (EUC-JP か Shift_JIS)
 * @param replace_invalid 読めないバイトを '?' に置き換えて変換を続けるか
 * @return UTF-8 に変換した文字列。変換に失敗した場合は tl::nullopt
 * @details 旧版の EUC-JP は文字列リテラルを euc-jp-ms で変換してビルドしていたので、EUC-JP-MS で読んだ文字にする。
 * 読めない文字は、iconv の版と同じく先頭の 1 バイトを '?' にして、次のバイトから変換を続ける。
 * ただし、20932 で読めない JIS X 0212 の文字は 3 バイトを 1 つの '?' にする (iconv の版では読める)。
 */
tl::optional<std::string> legacy_to_utf8(std::string_view str, CharacterEncoding encoding, bool replace_invalid)
{
    std::string input(str);
    const auto is_euc = encoding == CharacterEncoding::EUC_JP;
    if (is_euc && !replace_invalid_euc_bytes(input, replace_invalid)) {
        return tl::nullopt;
    }

    const auto code_page = is_euc ? EUC_JP_CODE_PAGE : SHIFT_JIS_CODE_PAGE;
    const auto convert = [code_page](std::string_view s) -> tl::optional<std::string> {
        const auto utf16 = to_utf16(s, code_page, MB_ERR_INVALID_CHARS);
        if (!utf16) {
            return tl::nullopt;
        }

        return from_utf16(*utf16, CP_UTF8, 0, nullptr);
    };

    // pos から始まる 1 文字 (文字列の末尾で欠けていれば、残りのバイト)
    const std::string_view input_view(input);
    const auto char_at = [input_view, encoding](size_t pos) { return input_view.substr(pos, legacy_lead_byte_length(input_view[pos], encoding)); };

    // iconv の版と読める文字が違う並びがあれば、まとめて変換せずに、読めない文字として扱う
    auto has_unreadable_char = false;
    for (size_t pos = 0; !has_unreadable_char && (pos < input_view.length());) {
        const auto chr = char_at(pos);
        has_unreadable_char = is_unreadable_legacy_char(chr, encoding);
        pos += chr.length();
    }

    auto utf8 = has_unreadable_char ? tl::nullopt : convert(input);
    if (!utf8) {
        if (!replace_invalid) {
            return tl::nullopt;
        }

        // 読めない文字があれば、1 文字ずつ変換する
        utf8 = std::string();
        for (size_t pos = 0; pos < input_view.length();) {
            const auto chr = char_at(pos);
            const auto converted = is_unreadable_legacy_char(chr, encoding) ? tl::nullopt : convert(chr);
            if (converted) {
                utf8->append(*converted);
                pos += chr.length();
                continue;
            }

            // 読めない JIS X 0212 の文字 (8F と A1-FE の 2 バイト) は、後ろの 2 バイトを別の文字として読まないよう、まとめて 1 つの '?' にする
            utf8->push_back('?');
            // 3 バイトの文字は EUC-JP の 8F xx xx だけ
            const auto is_euc_byte = [](char c) { return static_cast<unsigned char>(c) >= 0xa1; };
            const auto is_jis_x_0212_char = (chr.length() == 3) && is_euc_byte(chr[1]) && is_euc_byte(chr[2]);
            pos += is_jis_x_0212_char ? chr.length() : 1;
        }
    }

    if (is_euc) {
        replace_jis_to_ms_characters(*utf8);
    }

    return utf8;
}
#else
/*!
 * @brief 旧版のデータの文字列を、iconv で UTF-8 に変換する
 * @param str 変換する文字列
 * @param encoding str の文字コード (EUC-JP か Shift_JIS)
 * @param replace_invalid 読めないバイトを '?' に置き換えて変換を続けるか
 * @return UTF-8 に変換した文字列。変換に失敗した場合は tl::nullopt
 * @details 旧版の EUC-JP は文字列リテラルを euc-jp-ms で変換してビルドしていたので、使えれば EUC-JP-MS として読む。
 * 使えずに EUC-JP として読んだときは、全角チルダなど EUC-JP-MS と対応の違う文字を、EUC-JP-MS で読んだ文字に戻す。
 */
tl::optional<std::string> legacy_to_utf8(std::string_view str, CharacterEncoding encoding, bool replace_invalid)
{
    // 空文字列では一時バッファの data() が nullptr になり、iconv() に渡せないので、そのまま返す
    if (str.empty()) {
        return std::string();
    }

    // EUC-JP-MS を開けたか (開けずに EUC-JP を開いたなら false)
    static const auto [euc_cd, is_euc_jp_ms] = [] {
        const auto cd = iconv_open("UTF-8", "EUC-JP-MS");
        if (cd != reinterpret_cast<iconv_t>(-1)) {
            return std::make_pair(cd, true);
        }

        return std::make_pair(iconv_open("UTF-8", "EUC-JP"), false);
    }();
    static const auto sjis_cd = iconv_open("UTF-8", "CP932");
    const auto cd = (encoding == CharacterEncoding::EUC_JP) ? euc_cd : sjis_cd;
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        return tl::nullopt;
    }

    std::string input(str);
    if ((encoding == CharacterEncoding::EUC_JP) && !replace_invalid_euc_bytes(input, replace_invalid)) {
        return tl::nullopt;
    }

    // どちらの文字コードでも、1 バイトあたり UTF-8 で 3 バイト以下になる (Shift_JIS の半角カナは 1 バイトが 3 バイトになる)
    std::string utf8(input.length() * 3, '\0');
    size_t inlen_left = input.length();
    size_t outlen_left = utf8.length();
    char *in = input.data();
    char *out = utf8.data();
    while (inlen_left > 0) {
        // EUC-JP は、続きとして読めないバイトが続く先頭バイトの手前までを変換する。
        // iconv が読めないバイトの次から読み直した位置から探し直すので、iconv と同じ文字の境界で探せる
        const auto pos = static_cast<size_t>(in - input.data());
        const auto [chunk_end, is_broken] = (encoding == CharacterEncoding::EUC_JP) ? find_broken_euc_char(input, pos) : std::make_pair(input.length(), false);
        const auto chunk_len = chunk_end - pos;
        auto chunk_left = chunk_len;

        // 前の呼び出しや前の読めないバイトで途中になった変換の状態を引き継がないよう、初期状態に戻す
        iconv(cd, nullptr, nullptr, nullptr, nullptr);
        const auto is_converted = iconv(cd, &in, &chunk_left, &out, &outlen_left) != static_cast<size_t>(-1);
        inlen_left -= chunk_len - chunk_left;
        if (inlen_left == 0) {
            break;
        }

        // 探した範囲を読めないバイトなしで変換できたら、続きを探す
        if (is_converted && !is_broken) {
            continue;
        }

        // 不正な並び (EILSEQ)、末尾で欠けた文字 (EINVAL)、続きとして読めないバイトが続く先頭バイトは、
        // 置き換えるなら 1 バイトを '?' にして次のバイトから続ける
        if (!replace_invalid || (!is_converted && (errno == E2BIG))) {
            return tl::nullopt;
        }

        *out++ = '?';
        outlen_left--;
        in++;
        inlen_left--;
    }

    utf8.resize(utf8.length() - outlen_left);
    if ((encoding == CharacterEncoding::EUC_JP) && !is_euc_jp_ms) {
        replace_jis_to_ms_characters(utf8);
    }

    return utf8;
}
#endif

/*!
 * @brief UTF-8 として読めないバイトを '?' に置き換える
 * @param str 文字列
 * @return 置き換えた文字列
 */
std::string replace_invalid_utf8(std::string_view str)
{
    std::string replaced;
    replaced.reserve(str.length());
    for (size_t pos = 0; pos < str.length();) {
        const auto length = utf8_next_char_byte_length(str.substr(pos));
        if (length <= 0) {
            replaced.push_back('?');
            pos++;
            continue;
        }

        replaced.append(str.substr(pos, length));
        pos += length;
    }

    return replaced;
}
}
#endif

/*!
 * @brief 旧版の文字コードの文字の先頭バイトから、その文字のバイト数を返す
 * @param lead 文字の先頭バイト
 * @param encoding 文字コード (EUC-JP か Shift_JIS)
 * @return 文字のバイト数。2 バイト目以降が正しいかは確かめない
 */
size_t legacy_lead_byte_length(char lead, CharacterEncoding encoding)
{
    const auto byte = static_cast<unsigned char>(lead);
    if (encoding == CharacterEncoding::SHIFT_JIS) {
        return (((byte >= 0x81) && (byte <= 0x9f)) || ((byte >= 0xe0) && (byte <= 0xfc))) ? 2 : 1;
    }

    if (byte == 0x8f) {
        return 3;
    }

    return iseuckanji(byte) ? 2 : 1;
}

/*!
 * @brief 旧版のデータの文字列の文字コード (EUC-JP か Shift_JIS か) を推定する
 * @param str 推定する文字列
 * @return 推定した文字コード
 *         ASCII と EUC-JP/Shift_JIS のどちらとも解釈できる文字のみの文字列や、壊れた文字列など、
 *         文字コードを判定できなかった場合は UNKNOWN
 *         (ASCII は判定に使わないので US_ASCII を返すことはない)
 */
CharacterEncoding guess_legacy_encoding(std::string_view str)
{
    auto encoding = CharacterEncoding::UNKNOWN;
    for (size_t i = 0; i < str.size(); i++) {
        /* First byte */
        const auto c1 = static_cast<unsigned char>(str[i]);

        /* ASCII は判定に影響しないので読み飛ばす */
        if (!(c1 & 0x80)) {
            continue;
        }

        /* Second byte (文字列の末尾で欠けていれば 0 とみなし、壊れた文字列として扱う) */
        i++;
        const auto c2 = (i < str.size()) ? static_cast<unsigned char>(str[i]) : static_cast<unsigned char>(0);

        /*
         * Shift_JIS の1バイト目 F0-FC (ユーザー定義文字・IBM 拡張文字) は、内部の文字コードが EUC-JP だった旧版が
         * Shift_JIS のデータを読むのに使っていた変換で EUC-JP にできなかった (1バイト目が 0x00 や制御文字になる) ため、Shift_JIS とはみなさない。
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

    return encoding;
}

/*!
 * @brief 旧版のデータの文字列を、内部の文字コードに変換する
 * @param str 変換する文字列
 * @param encoding str の文字コード (guess_legacy_encoding() で推定したものなど)
 * @return 内部の文字コードに変換した文字列。encoding が EUC-JP・Shift_JIS のどちらでもないときは、変換せずに返す
 * @details 変換に失敗すると tl::nullopt を返す。
 */
tl::optional<std::string> legacy_to_sys(std::string_view str, CharacterEncoding encoding)
{
#ifdef UTF8
    switch (encoding) {
    case CharacterEncoding::EUC_JP:
    case CharacterEncoding::SHIFT_JIS:
        return legacy_to_utf8(str, encoding, false);
    default:
        return std::string(str);
    }
#endif
}

/*!
 * @brief 旧版のデータの文字列を内部の文字コードに変換し、変換できない部分は置き換える
 * @param str 変換する文字列
 * @param encoding str の文字コード (guess_legacy_encoding() で推定したものなど)
 * @return 内部の文字コードに変換した文字列
 * @details 読めないバイト (末尾で欠けた文字など) を '?' に置き換える。
 * encoding が UNKNOWN なら、旧版の内部の文字コード (LEGACY_CHARACTER_ENCODING。Windows 版は Shift_JIS、Unix 系は EUC-JP) として読む
 * (旧版の構成が、推定できない文字列を内部の文字コードのまま扱うのと同じ)。
 * US_ASCII・UTF_8 なら、UTF-8 として読めないバイトを '?' に置き換える。
 */
std::string legacy_to_sys_replacing_invalid(std::string_view str, CharacterEncoding encoding)
{
#ifdef UTF8
    // 文字コードが分からなければ旧版の内部の文字コードとして読む
    const auto legacy_encoding = (encoding == CharacterEncoding::UNKNOWN) ? LEGACY_CHARACTER_ENCODING : encoding;
    if ((legacy_encoding == CharacterEncoding::EUC_JP) || (legacy_encoding == CharacterEncoding::SHIFT_JIS)) {
        // 変換できなければ (iconv がその文字コードを扱えないときなど)、UTF-8 として読めないバイトを置き換えるだけにする
        if (auto utf8 = legacy_to_utf8(str, legacy_encoding, true)) {
            return std::move(*utf8);
        }
    }

    return replace_invalid_utf8(str);
#endif
}

// 旧版のファイルの名前を作るためと、ムービーの通信のために使う
#ifdef UTF8

namespace {
/*!
 * @brief UTF-8 から EUC-JP に変換する前に置き換える文字 (置き換え前, 置き換え後) の UTF-8 のバイト列
 * @details EUC-JP-MS (CP932) と EUC-JP で対応の違う文字を、iconv の EUC-JP (Windows では 20932) が受け付ける文字にする
 * (内部の文字コードを UTF-8 にした構成は、旧版の EUC-JP を EUC-JP-MS として読むので、置き換え前の文字になる)。
 * 置き換え前の文字の先頭バイトは継続バイトにならず、置き換え後のバイト列は置き換え前のどの文字も含まない。
 * そのため、文字の途中から誤って一致したり、置き換えた結果がまた一致したりすることはない。
 */
constexpr std::array<std::pair<std::string_view, std::string_view>, 6> MS_TO_JIS_CHARACTERS = { {
    { "\xef\xbd\x9e", "\xe3\x80\x9c" }, /* FULLWIDTH TILDE -> WAVE DASH (全角チルダ → 波ダッシュ) */
    { "\xe2\x88\xa5", "\xe2\x80\x96" }, /* PARALLEL TO -> DOUBLE VERTICAL LINE (平行 → 双柱) */
    { "\xef\xbc\x8d", "\xe2\x88\x92" }, /* FULLWIDTH HYPHEN-MINUS -> MINUS SIGN (全角ハイフン → マイナス記号) */
    { "\xef\xbf\xa0", "\xc2\xa2" }, /* FULLWIDTH CENT SIGN -> CENT SIGN (全角のセント記号 → セント記号) */
    { "\xef\xbf\xa1", "\xc2\xa3" }, /* FULLWIDTH POUND SIGN -> POUND SIGN (全角のポンド記号 → ポンド記号) */
    { "\xef\xbf\xa2", "\xc2\xac" }, /* FULLWIDTH NOT SIGN -> NOT SIGN (全角の否定記号 → 否定記号) */
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
    replace_characters(str, MS_TO_JIS_CHARACTERS);
}

namespace {
/*!
 * @brief UTF-8 の文字列を EUC-JP に変換する
 * @param utf8 変換元の文字列 (空でないこと)
 * @return EUC-JP に変換した文字列。JIS X 0212 の文字は置き換えずに残る。変換に失敗した場合は tl::nullopt
 */
tl::optional<std::string> encode_euc_jp(std::string &utf8)
{
#ifdef _WIN32
    // Windows の EUC-JP のコードページ 20932 は、JIS X 0212 の文字を 2 バイト目が 21-7E の 2 バイトにする。
    // EUC-JP に無い文字は、iconv の版と同じく変換の失敗とする (似た文字に置き換えず、置き換えたら失敗とする)。
    // ただし C1 制御文字 (U+0080-009F) は、glibc の iconv が 1 バイトにするのと違い、20932 には無いので失敗する
    // (表示幅を保って変換する utf8_to_euc_keeping_width() では、どちらも「？」になる)
    const auto utf16 = to_utf16(utf8, CP_UTF8, MB_ERR_INVALID_CHARS);
    if (!utf16) {
        return tl::nullopt;
    }

    BOOL used_default_char = FALSE;
    auto euc = from_utf16(*utf16, EUC_JP_CODE_PAGE, WC_NO_BEST_FIT_CHARS, &used_default_char);
    if (used_default_char) {
        return tl::nullopt;
    }

    return euc;
#else
    static const auto cd = iconv_open("EUC-JP", "UTF-8");
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        return tl::nullopt;
    }

    // JIS X 0212 の文字は UTF-8 の2バイトが EUC-JP の3バイトになるので、'?' に置き換える前の変換結果が収まるよう2倍確保する
    std::string euc(utf8.length() * 2, '\0');
    size_t inlen_left = utf8.length();
    size_t outlen_left = euc.length();
    char *in = utf8.data();
    char *out = euc.data();

    if (iconv(cd, &in, &inlen_left, &out, &outlen_left) == (size_t)-1) {
        return tl::nullopt;
    }

    euc.resize(euc.length() - outlen_left);
    return euc;
#endif
}

/*!
 * @brief EUC-JP の文字列の JIS X 0212 の文字を '?' に置き換える
 * @param euc 置き換える文字列
 * @details iconv は JIS X 0212 の文字を 3 バイト (8F xx xx) に、Windows の 20932 は 2 バイト目が 21-7E の 2 バイトにする。
 * どちらも '?' に置き換えながら、その場で前に詰める (書き込み位置は読み込み位置を追い越さない)。
 */
void replace_jis_x_0212_characters(std::string &euc)
{
    size_t euc_len = 0;
    for (size_t i = 0; i < euc.length();) {
        // glibc の iconv が 1 バイトのまま出す C1 制御文字 (U+0080-008D、U+0090-009F) は、2 バイトの文字の先頭バイトでないので 1 バイトになる
        const auto lead = static_cast<unsigned char>(euc[i]);
        const auto char_len = std::min(legacy_lead_byte_length(euc[i], CharacterEncoding::EUC_JP), euc.length() - i);
        const auto is_jis_x_0212 = (lead == 0x8f) || ((char_len == 2) && (static_cast<unsigned char>(euc[i + 1]) < 0xa1));
        if (is_jis_x_0212) {
            euc[euc_len++] = '?';
            i += char_len;
            continue;
        }

        for (size_t j = 0; j < char_len; j++) {
            euc[euc_len++] = euc[i++];
        }
    }

    euc.resize(euc_len);
}
}

/*!
 * @brief 文字列の文字コードをUTF-8からEUC-JPに変換する
 * @details 変愚蛮怒は全角文字を2バイト固定として扱うため、EUC-JP で3バイトになる JIS X 0212 の文字
 *          (8F xx xx、é など) を正しく表示できない。そうした文字は '?' に置き換える
 *          (Windows 版で使う 20932 は、JIS X 0212 の文字を 2 バイト目が 21-7E の 2 バイトにするので、これも '?' にする)。
 * @param utf8_str 変換元の文字列
 * @return EUC-JPに変換した文字列。変換に失敗した場合はtl::nullopt
 */
tl::optional<std::string> utf8_to_euc(std::string_view utf8_str)
{
    // 空文字列では一時バッファの data() が nullptr になり、iconv() に渡せないので、そのまま返す
    if (utf8_str.empty()) {
        return std::string();
    }

    std::string utf8(utf8_str);
    ms_to_jis_unicode(utf8);
    auto euc = encode_euc_jp(utf8);
    if (!euc) {
        return tl::nullopt;
    }

    replace_jis_x_0212_characters(*euc);
    return euc;
}

#ifdef UTF8
/*!
 * @brief 文字列の文字コードを UTF-8 から EUC-JP に、表示幅とバイト数が等しくなるよう変換する
 * @param utf8_str 変換元の文字列
 * @return EUC-JP に変換した文字列。バイト数は utf8_str の表示幅 (ASCII は 1 桁、ほかは 2 桁) と等しい
 * @details EUC-JP の版と同じく、全角文字を 2 バイトとして扱う通信 (ムービーなど) に使う。
 * EUC-JP の 2 バイトの文字にならない文字 (EUC-JP に無い文字、JIS X 0212 の文字) は「？」に、
 * UTF-8 として読めないバイトは 1 バイトずつ '?' に置き換える。
 */
std::string utf8_to_euc_keeping_width(std::string_view utf8_str)
{
    constexpr std::string_view fullwidth_question = "\xa1\xa9"; //!< ？ (EUC-JP)
    std::string euc;
    while (!utf8_str.empty()) {
        const auto char_length = utf8_next_char_byte_length(utf8_str);
        if (char_length == 0) {
            // ASCII は utf8_next_char_byte_length() が 1 を返すので、ここに来るのは読めないバイトだけ
            euc.push_back('?');
            utf8_str.remove_prefix(1);
            continue;
        }

        const auto ch = utf8_str.substr(0, char_length);
        utf8_str.remove_prefix(char_length);
        if (char_length == 1) {
            euc.append(ch);
            continue;
        }

        const auto converted = utf8_to_euc(ch);
        euc.append((converted && (converted->length() == 2)) ? *converted : fullwidth_question);
    }

    return euc;
}
#endif
#endif

/*!
 * @brief システムの文字コードからUTF-8に変換する
 * @param str システムの文字コードの文字列
 * @return UTF-8に変換した文字列
 *         変換に失敗した場合はtl::nullopt
 */
tl::optional<std::string> sys_to_utf8(std::string_view str)
{
#ifdef UTF8
    // 内部の文字コードは UTF-8 なので変換しない。不正な並びは変換に失敗したものとする
    return is_utf8_str(str) ? tl::make_optional<std::string>(str) : tl::nullopt;
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
    // 変換した文字列の多くは '\0' 終端の文字列として表示などに使われ、途中に '\0' があると後ろが黙って欠けてしまう。
    // 途中に '\0' を含むのは壊れたデータなので、不正な入力として変換しない
    if (str.find('\0') != std::string_view::npos) {
        return tl::nullopt;
    }

#ifdef UTF8
    return is_utf8_str(str) ? tl::make_optional<std::string>(str) : tl::nullopt;
#else
    return tl::nullopt;
#endif
}

/*!
 * @brief 文字列が内部の文字コードのままでよく、変換が要らないかを返す
 * @param str 文字列
 * @return 変換が要らなければ true
 * @details UTF-8 として読める文字列 (ASCII だけのものを含む) は変換しない。
 */
static bool is_system_encoding_text(std::string_view str)
{
#ifdef UTF8
    return is_utf8_str(str);
#endif
}

/*!
 * @brief 内部の文字コードではない文字列の文字コードを推定し、内部の文字コードに変換する
 * @param str 変換する文字列 (is_system_encoding_text() が false のもの)
 * @return 内部の文字コードに変換した文字列。変換に失敗した場合は tl::nullopt
 * @details 旧版の文字コードとみなして推定する。推定できない文字列や読めないバイトを含む文字列も、
 * 不正な UTF-8 を持ち込まないよう置き換えて変換するので、失敗しない。
 * Windows 版では、Shift_JIS として読める文字列は推定せずに Shift_JIS として読む。
 * 旧版の Windows 版は UTF-8 として読めない文字列をすべて Shift_JIS として扱っていたので、それと同じにし、
 * 半角カナの並び (EUC-JP の 2 バイトの文字とも読める) を EUC-JP と推定して取り違えないようにする。
 */
static tl::optional<std::string> convert_to_system_encoding(std::string_view str)
{
#ifdef UTF8
#ifdef _WIN32
    if (auto utf8 = legacy_to_sys(str, LEGACY_CHARACTER_ENCODING)) {
        return utf8;
    }
#endif

    return legacy_to_sys_replacing_invalid(str, guess_legacy_encoding(str));
#endif
}

/*!
 * @brief 文字コードの変換に失敗したことを警告する
 */
static void warn_conversion_failure()
{
    msg_print("警告:文字コードの変換に失敗しました");
    msg_erase();
}

/*!
 * @brief 受け取った文字列の文字コードを推定し、システムの文字コードへ変換する
 * @param str 変換する文字列
 * @return システムの文字コードに変換した文字列。変換が要らない場合と、変換に失敗した場合 (警告を表示する) は元の文字列を返す
 */
std::string guess_convert_to_system_encoding(std::string str)
{
    if (is_system_encoding_text(str)) {
        return str;
    }

    if (auto sys_str = convert_to_system_encoding(str)) {
        return std::move(*sys_str);
    }

    warn_conversion_failure();
    return str;
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
    const std::string_view str(strbuf);
    if (is_system_encoding_text(str)) {
        return str.length();
    }

    const auto sys_str = convert_to_system_encoding(str);
    if (!sys_str || std::ssize(*sys_str) >= buflen) {
        warn_conversion_failure();
        return str.length();
    }

    std::copy(sys_str->begin(), sys_str->end(), strbuf);
    strbuf[sys_str->length()] = '\0';
    return sys_str->length();
}

#endif /* JP */
