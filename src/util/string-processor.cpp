/*!
 * @file string-processor.cpp
 * @brief 文字列処理の汎用ユーティリティ
 * @details
 * 日本語版が扱う文字コードは、Unix系ではEUC-JP、WindowsではShift_JISである。
 * Shift_JISでは2バイト文字の後半バイトがASCIIの範囲(0x40-0x7e)と重なるため、
 * 単純にバイト単位で検索すると、2バイト文字の後半バイトを独立した文字と誤認する。
 * 後半バイトが 0x5c (バックスラッシュ)になる「ソ」「表」などがその代表で、
 * いわゆるダメ文字と呼ばれる。
 *
 * このファイルの関数のうち文字や文字列を検索・分割するものは、文字列を先頭から
 * 1文字ずつ走査して2バイト文字の後半バイトを読み飛ばすことで、この問題を回避する。
 * EUC-JPは後半バイトがASCIIの範囲と重ならないため同じ問題は起きないが、
 * 処理は文字コードによらず共通である。
 */

#include "util/string-processor.h"
#include "system/h-basic.h"
#include <algorithm>
#include <array>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view.hpp>
#include <utility>

namespace {
constexpr std::array<char, 16> hex_symbol_table = { { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' } }; //!< 数値から16進数(8進数にも流用する)の文字への変換テーブル

constexpr auto trim_front = ranges::views::drop_while([](char c) { return c == ' ' || c == '\t'; });
constexpr auto trim_back = ranges::views::reverse | trim_front | ranges::views::reverse;

/*!
 * @brief 2バイト文字を分断しないように部分文字列の範囲を補正する
 * @param sv 対象の文字列
 * @param pos 補正前の開始位置(バイト)
 * @param n 補正前の長さ(バイト)
 * @return 補正後の開始位置と長さの組
 * @details
 * 開始位置が2バイト文字の後半バイトを指す場合は次のバイトまで進め、
 * 終了位置が2バイト文字の前半バイトを指す場合は手前のバイトまで戻す。
 * pos や pos + n が文字列の長さを超える場合は、文字列の末尾に丸める。
 */
std::pair<size_t, size_t> adjust_substr_pos(std::string_view sv, size_t pos, size_t n)
{
    const auto start = std::min(pos, sv.length());
    const auto end = n == std::string_view::npos ? sv.length() : std::min(pos + n, sv.length());

#ifdef JP
    size_t seek_pos = 0;
    while (seek_pos < start) {
        if (iskanji(sv[seek_pos])) {
            ++seek_pos;
        }
        ++seek_pos;
    }

    // 文字列の末尾が2バイト文字の前半バイトだけの場合、seek_pos が文字列長を超えうる
    seek_pos = std::min(seek_pos, sv.length());
    const auto mb_pos = seek_pos;

    while (seek_pos < end) {
        if (iskanji(sv[seek_pos])) {
            if (seek_pos == end - 1) {
                break;
            }
            ++seek_pos;
        }
        ++seek_pos;
    }
    const auto mb_n = seek_pos - mb_pos;

    return { mb_pos, mb_n };
#else
    return { start, end - start };
#endif
}
}

/*!
 * @brief 文字列をバッファへコピーする (BSDのstrlcpy相当)
 * @param buf コピー先のバッファ。src と領域が重なっていてはならない
 * @param src コピー元の文字列
 * @param bufsize コピー先のバッファのサイズ(NUL終端の分を含む)。0の場合 buf には一切書き込まない
 * @return src のバイト数
 * @details
 * 最大 bufsize - 1 バイトまでコピーし、必ずNUL終端する。
 * 戻り値と bufsize を比較することで切り詰めが起きたかどうかを判定できる。
 * 例: if (angband_strcpy(buf, src, sizeof(buf)) >= sizeof(buf)) ...
 *
 * 日本語版では2バイト文字を途中で切り詰めないよう、後半バイトまで収まらない場合は
 * その文字ごとコピーしない。
 */
size_t angband_strcpy(char *buf, std::string_view src, size_t bufsize)
{
    if (bufsize == 0) {
        return src.length();
    }

    // NUL終端の分を除いた、実際にコピーできるバイト数
    const auto capacity = bufsize - 1;
    size_t len = 0;

    while ((len < src.length()) && (len < capacity)) {
#ifdef JP
        if (iskanji(src[len])) {
            // 2バイト文字は後半バイトまで収まる場合だけコピーし、途中で分断しない
            if ((len + 2 > capacity) || (len + 1 >= src.length())) {
                break;
            }
            buf[len] = src[len];
            buf[len + 1] = src[len + 1];
            len += 2;
            continue;
        }
#endif
        buf[len] = src[len];
        ++len;
    }

    buf[len] = '\0';
    return src.length();
}

/*!
 * @brief NUL終端された文字列の末尾へ文字列を追記する (BSDのstrlcat相当)
 * @param buf 追記先のNUL終端された文字列。src と領域が重なっていてはならない
 * @param src 追記する文字列
 * @param bufsize 追記先のバッファのサイズ(NUL終端の分を含む)。
 *                buf に追記する余地が無い場合は buf を書き換えない
 * @return 追記しようとした結果のバイト数 (buf の元のバイト数 + src のバイト数)
 * @details
 * bufsize を超えて書き込むことはない。1バイトでも追記した場合は必ずNUL終端する。
 * 戻り値と bufsize を比較することで切り詰めが起きたかどうかを判定できる。
 * 例: if (angband_strcat(buf, src, sizeof(buf)) >= sizeof(buf)) ...
 */
size_t angband_strcat(char *buf, std::string_view src, size_t bufsize)
{
    const auto dlen = strlen(buf);

    // bufsize - 1 は bufsize が 0 のとき桁溢れするため、加算の形で比較する
    if (dlen + 1 >= bufsize) {
        return dlen + src.length();
    }

    return dlen + angband_strcpy(buf + dlen, src, bufsize - dlen);
}

/*!
 * @brief 2バイト文字を考慮しつつ文字列を検索する (ANSIのstrstr相当)
 * @param haystack 検索対象のNUL終端された文字列
 * @param needle 検索する文字列
 * @return 最初に見つかった位置へのポインタ。見つからなければnullptr
 * @details
 * 2バイト文字の後半バイトから始まる位置にはマッチしない。
 * needle が空文字列の場合は haystack の先頭を返す。
 */
char *angband_strstr(const char *haystack, std::string_view needle)
{
    std::string_view haystack_view(haystack);
    auto l1 = haystack_view.length();
    auto l2 = needle.length();
    if (l1 < l2) {
        return nullptr;
    }

    for (size_t i = 0; i <= l1 - l2; i++) {
        const auto part = haystack_view.substr(i);
        if (part.starts_with(needle)) {
            return const_cast<char *>(haystack) + i;
        }

#ifdef JP
        if (iskanji(*(haystack + i))) {
            i++;
        }
#endif
    }

    return nullptr;
}

/*!
 * @brief 2バイト文字を考慮しつつ文字を検索する (ANSIのstrchr相当)
 * @param ptr 検索対象のNUL終端された文字列
 * @param ch 検索する文字
 * @return 最初に見つかった位置へのポインタ。見つからなければnullptr
 * @details
 * 2バイト文字の後半バイトにはマッチしない。
 * strchrと異なり、終端のNUL文字は検索できない(nullptrを返す)。
 */
char *angband_strchr(const char *ptr, char ch)
{
    for (; *ptr != '\0'; ptr++) {
        if (*ptr == ch) {
            return const_cast<char *>(ptr);
        }

#ifdef JP
        if (iskanji(*ptr)) {
            ptr++;
        }
#endif
    }

    return nullptr;
}

/*!
 * @brief 文字列の左端の半角スペースを読み飛ばす
 * @param p NUL終端された文字列
 * @return 左端の半角スペースを読み飛ばした位置へのポインタ
 * @details
 * 元の文字列は書き換えない。タブや全角スペースは対象外。
 * タブも含めて取り除きたい場合や新しい文字列が欲しい場合は str_ltrim() を使う。
 */
char *ltrim(char *p)
{
    while (p[0] == ' ') {
        p++;
    }
    return p;
}

/*!
 * @brief 文字列の右端の半角スペースをNUL文字で潰す
 * @param p NUL終端された文字列。この文字列自体が書き換えられる
 * @return p と同じポインタ
 * @details
 * 2バイト文字の後半バイトが 0x20 になる文字コードは無いため、2バイト文字を壊すことはない。
 * タブや全角スペースは対象外。
 * 元の文字列を書き換えたくない場合は str_rtrim() を使う。
 */
char *rtrim(char *p)
{
    for (auto i = strlen(p); (i > 0) && (p[i - 1] == ' '); --i) {
        p[i - 1] = '\0';
    }

    return p;
}

/*!
 * @brief 2バイト文字を考慮しつつ文字列が含まれるかどうかを調べる
 * @param src 検索対象の文字列
 * @param find 検索する文字列
 * @return src に find が含まれるならtrue
 * @details
 * 2バイト文字の後半バイトから始まる位置にはマッチしない。
 * find には正しい長さを持つ文字列を渡すこと。NUL終端されていない char の配列や
 * char 単体のアドレスを渡すと、std::string_view への変換で範囲外を読む。
 */
bool str_find(const std::string &src, std::string_view find)
{
    return angband_strstr(src.data(), find) != nullptr;
}

/*!
 * @brief 文字列の両端の空白を削除する
 *
 * 文字列 str の両端にある空白(スペースおよびタブ)を削除し、
 * 削除した文字列を std::string 型のオブジェクトとして返す。
 * 文字列全体が空白の場合は空文字列を返す。
 *
 * @param str 操作の対象とする文字列
 * @return std::string strの両端の空白を削除した文字列
 * @details
 * 2バイト文字の後半バイトが 0x20 や 0x09 になる文字コードは無いため、2バイト文字を壊すことはない。
 */
std::string str_trim(std::string_view str)
{
    return str | trim_front | trim_back | ranges::to<std::string>();
}

/*!
 * @brief 文字列の右端の空白を削除する
 *
 * 文字列 str の右端にある空白(スペースおよびタブ)を削除し、
 * 削除した文字列を std::string 型のオブジェクトとして返す。
 * 文字列全体が空白の場合は空文字列を返す。
 *
 * @param str 操作の対象とする文字列
 * @return std::string strの右端の空白を削除した文字列
 * @details
 * 2バイト文字の後半バイトが 0x20 や 0x09 になる文字コードは無いため、2バイト文字を壊すことはない。
 */
std::string str_rtrim(std::string_view str)
{
    return str | trim_back | ranges::to<std::string>();
}

/*!
 * @brief 文字列の左端の空白を削除する
 *
 * 文字列 str の左端にある空白(スペースおよびタブ)を削除し、
 * 削除した文字列を std::string 型のオブジェクトとして返す。
 * 文字列全体が空白の場合は空文字列を返す。
 *
 * @param str 操作の対象とする文字列
 * @return std::string strの左端の空白を削除した文字列
 * @details
 * 2バイト文字の後半バイトが 0x20 や 0x09 になる文字コードは無いため、2バイト文字を壊すことはない。
 */
std::string str_ltrim(std::string_view str)
{
    return str | trim_front | ranges::to<std::string>();
}

/*!
 * @brief 文字列を指定した文字で分割する
 *
 * 文字列 str を delim で指定した文字で分割し、分割した文字列を要素とする配列を
 * std::vector<std::string> 型のオブジェクトとして返す。
 *
 * @param str 操作の対象とする文字列
 * @param delim 文字列を分割する文字
 * @param trim trueの場合、分割した文字列の両端の空白を削除する
 * @param num 1以上の場合、その要素数だけvectorの領域を事前に確保する(速度向上が目的)
 * @return std::vector<std::string> 分割した文字列を要素とする配列
 * @details
 * 戻り値の要素数は必ず1以上になる。str が空文字列の場合は、空文字列1個を要素とする配列を返す。
 * 区切り文字が先頭・末尾にある場合や連続する場合は、その分だけ空文字列の要素が生まれる。
 * num は領域確保のヒントに過ぎず、戻り値の要素数を num に揃えるものではない。
 *
 * 2バイト文字の後半バイトは読み飛ばすため、delim にダメ文字と重なる文字を指定しても
 * 2バイト文字の途中で分割することはない。
 */
std::vector<std::string> str_split(std::string_view str, char delim, bool trim, int num)
{
    std::vector<std::string> result;
    if (num > 0) {
        result.reserve(num);
    }

    auto make_str = [trim](std::string_view sv) { return trim ? str_trim(sv) : std::string(sv); };

    while (true) {
        bool found = false;
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == delim) {
                result.push_back(make_str(str.substr(0, i)));
                str.remove_prefix(i + 1);
                found = true;
                break;
            }
#ifdef JP
            if (iskanji(str[i])) {
                ++i;
            }
#endif
        }
        if (!found) {
            result.push_back(make_str(str));
            return result;
        }
    }
}

/*!
 * @brief 文字列を指定したバイト数ごとに分割する
 *
 * 文字列 str を len バイトずつに分割し、分割した文字列を要素とする配列を
 * std::vector<std::string> 型のオブジェクトとして返す。
 *
 * @param str 操作の対象とする文字列
 * @param len 分割するバイト数。全角文字を含む文字列には2以上を指定すること
 * @return std::vector<std::string> 分割した文字列を要素とする配列
 * @details
 * 全角文字は2バイトを占める。分割位置に全角文字がかかる場合はその手前で区切るため、
 * 各要素が len バイトちょうどになるとは限らない。
 * str が空文字列の場合は空の配列を返す。
 *
 * len が 0 の場合や、len が 1 で分割位置に全角文字が来る場合は1バイトも取り出せず
 * 分割が進まないため、そこで打ち切る(残りの文字列は戻り値に含まれない)。
 */
std::vector<std::string> str_separate(std::string_view str, size_t len)
{
    std::vector<std::string> result;

    while (!str.empty()) {
        auto separated = str_substr(str, 0, len);
        if (separated.empty()) {
            // len が小さすぎて1文字も取り出せない。これ以上分割できないので打ち切る
            break;
        }

        str.remove_prefix(separated.size());
        result.push_back(std::move(separated));
    }

    return result;
}

/*!
 * @brief 文字列から指定した文字を取り除く
 *
 * 文字列 str から文字列 erase_chars に含まれる文字をすべて削除し、
 * 削除した文字列を std::string 型のオブジェクトとして返す。
 *
 * @param str 操作の対象とする文字列
 * @param erase_chars 削除する文字を指定する文字列
 * @return std::string 指定した文字をすべて削除した文字列
 * @details
 * 2バイト文字の後半バイトは削除の対象にしない。erase_chars にダメ文字と重なる文字を
 * 指定しても2バイト文字を壊すことはない。
 */
std::string str_erase(std::string str, std::string_view erase_chars)
{
    for (auto it = str.begin(); it != str.end();) {
        if (erase_chars.find(*it) != std::string_view::npos) {
            it = str.erase(it);
            continue;
        }
#ifdef JP
        if (iskanji(*it) && (it + 1 != str.end())) {
            ++it;
        }
#endif
        ++it;
    }

    return str;
}

/*!
 * @brief 文字列 str に含まれる文字列 old_str をすべて new_str に置き換える
 *
 * 一致する文字列が重複している場合は、前方を優先する。
 *
 * @param str 操作の対象とする文字列
 * @param old_str 置き換える文字列。空文字列の場合は str をそのまま返す
 * @param new_str 置き換え後の文字列
 * @return std::string old_str をすべて new_str で置き換えた文字列
 * @details
 * 2バイト文字の後半バイトから始まる位置にはマッチしない。
 */
std::string str_replace(std::string_view str, std::string_view old_str, std::string_view new_str)
{
    if (old_str.empty()) {
        return std::string(str);
    }

    const auto mb_char_indexes = str_find_all_multibyte_chars(str);
    std::string result;

    for (size_t start_pos = 0;;) {
        const auto found_pos = str.find(old_str, start_pos);
        if (found_pos == std::string_view::npos) {
            result.append(str.substr(start_pos));
            return result;
        }

        if ((found_pos > 0) && mb_char_indexes.contains(found_pos - 1)) {
            // 2バイト文字の後半バイトから始まる位置に一致したので、置き換えずに読み進める
            result.append(str.substr(start_pos, found_pos + 1 - start_pos));
            start_pos = found_pos + 1;
            continue;
        }

        result.append(str.substr(start_pos, found_pos - start_pos)).append(new_str);
        start_pos = found_pos + old_str.length();
    }
}

/*!
 * @brief 2バイト文字を考慮して部分文字列を取得する
 *
 * 引数で与えられた文字列の pos バイト目から n バイトの部分文字列を取得する。
 * 但し、以下の通り2バイト文字の途中で分断されないようにする。
 * - 開始位置 pos バイト目が2バイト文字の後半バイトの場合は pos+1 バイト目を開始位置とする。
 * - 終了位置 pos+n バイト目が2バイト文字の前半バイトの場合は pos+n-1 バイト目を終了位置とする。
 *
 * @param sv 文字列
 * @param pos 部分文字列の開始位置(バイト)
 * @param n 部分文字列の長さ(バイト)
 * @return 部分文字列
 * @details
 * pos が文字列の長さ以上の場合は空文字列を返す。pos + n が文字列の長さを超える場合は
 * 文字列の末尾までを返す。例外は送出しない。
 */
std::string str_substr(std::string_view sv, size_t pos, size_t n)
{
    const auto &[mb_pos, mb_n] = adjust_substr_pos(sv, pos, n);
    return std::string(sv.substr(mb_pos, mb_n));
}

/*!
 * @brief 2バイト文字を考慮して部分文字列を取得する (ムーブ版)
 *
 * 引数で与えられた文字列を pos バイト目から n バイトの部分文字列にして返す。
 * 範囲の補正の仕方は std::string_view を取る版と同じ。
 *
 * @param str 文字列。中身はムーブされる
 * @param pos 部分文字列の開始位置(バイト)
 * @param n 部分文字列の長さ(バイト)
 * @return 部分文字列
 * @details
 * 引数の文字列を縮めて返すため、新たな領域を確保しない。
 */
std::string str_substr(std::string &&str, size_t pos, size_t n)
{
    const auto &[mb_pos, mb_n] = adjust_substr_pos(str, pos, n);
    str.erase(mb_pos + mb_n);
    str.erase(0, mb_pos);
    return std::move(str);
}

/*!
 * @brief 2バイト文字を考慮して部分文字列を取得する (const char * 版)
 * @param str NUL終端された文字列
 * @param pos 部分文字列の開始位置(バイト)
 * @param n 部分文字列の長さ(バイト)
 * @return 部分文字列
 * @details
 * const char * は std::string_view にも std::string にも同じだけの変換で到達できるため、
 * このオーバーロードが無いと std::string_view 版と std::string && 版の解決が曖昧になる。
 * それを解消するために用意している。
 */
std::string str_substr(const char *str, size_t pos, size_t n)
{
    return str_substr(std::string_view(str), pos, n);
}

/*!
 * @brief 文字列を大文字に変換する
 * @param str 変換元の文字列
 * @return 大文字に変換した文字列
 * @details
 * 変換は1バイトずつ行い、英字以外の文字は変換されない。
 * 日本語版では2バイト文字を変換の対象とせず、そのまま通す。
 */
std::string str_toupper(std::string_view str)
{
    std::string uc_str;
    uc_str.reserve(str.size());
    for (size_t i = 0; i < str.length(); ++i) {
        const auto ch = str[i];
#ifdef JP
        // 2バイト文字は変換せずそのまま通す。後半バイトを欠いている場合も同じ
        if (iskanji(ch)) {
            uc_str.push_back(ch);
            if (i + 1 < str.length()) {
                uc_str.push_back(str[++i]);
            }
            continue;
        }
#endif
        uc_str.push_back(static_cast<char>(toupper(static_cast<unsigned char>(ch))));
    }

    return uc_str;
}

/*!
 * @brief 文字列を小文字に変換する
 * @param str 変換元の文字列
 * @return 小文字に変換した文字列
 * @details
 * 変換は1バイトずつ行い、英字以外の文字は変換されない。
 * 日本語版では2バイト文字を変換の対象とせず、そのまま通す。
 */
std::string str_tolower(std::string_view str)
{
    std::string lc_str;
    lc_str.reserve(str.size());
    for (size_t i = 0; i < str.length(); ++i) {
        const auto ch = str[i];
#ifdef JP
        // 2バイト文字は変換せずそのまま通す。後半バイトを欠いている場合も同じ
        if (iskanji(ch)) {
            lc_str.push_back(ch);
            if (i + 1 < str.length()) {
                lc_str.push_back(str[++i]);
            }
            continue;
        }
#endif
        lc_str.push_back(static_cast<char>(tolower(static_cast<unsigned char>(ch))));
    }

    return lc_str;
}

/*!
 * @brief 文字列の最初の文字を大文字に変換する
 *
 * @param str 変換元の文字列
 * @return 変換後の文字列
 * @details
 * 最初の文字が英字でない場合は何も変換しない。空文字列を渡した場合は空文字列を返す。
 * 日本語版では最初の文字が2バイト文字の場合も変換しない。
 */
std::string str_upcase_first(std::string_view str)
{
    if (str.empty()) {
        return {};
    }

    std::string result_str(str);

    const auto first_char = static_cast<unsigned char>(result_str[0]);
#ifdef JP
    // 2バイト文字の前半バイトをロケール依存の変換にかけない
    if (iskanji(first_char)) {
        return result_str;
    }
#endif

    if (isalpha(first_char)) {
        result_str[0] = static_cast<char>(toupper(first_char));
    }

    return result_str;
}

/*!
 * @brief 文字列に含まれるすべての2バイト文字の位置を取得する
 *
 * @param str 文字列
 * @return std::set<int> 2バイト文字の前半バイトのインデックスの集合
 * @details
 * 英語版では常に空の集合を返す。
 * 文字列の末尾が2バイト文字の前半バイトだけで終わっている場合も、その位置を集合に含める。
 */
std::set<int> str_find_all_multibyte_chars([[maybe_unused]] std::string_view str)
{
#ifdef JP
    std::set<int> mb_chars;
    for (auto i = 0; std::cmp_less(i, str.length()); ++i) {
        if (iskanji(str[i])) {
            mb_chars.insert(mb_chars.end(), i);
            ++i;
        }
    }

    return mb_chars;
#else
    return {};
#endif
}

/*!
 * @brief 文字列から指定した文字以降の部分文字列を抽出する
 * @param str 抽出対象の文字列
 * @param find 検索する文字
 * @return 見つかった位置から末尾までの部分文字列。見つからなければtl::nullopt
 * @details
 * 戻り値は str の一部を指すビューであり、str より長生きさせてはならない。
 * 現時点ではシフトJISのダメ文字は考慮しない (必要に応じて拡張する)。
 */
tl::optional<std::string_view> extract_suffix(std::string_view str, char find)
{
    if (const auto pos = str.find(find); pos != std::string_view::npos) {
        return str.substr(pos);
    }

    return tl::nullopt;
}

/*!
 * @brief 文字列から指定した文字列以降の部分文字列を抽出する
 * @param str 抽出対象の文字列
 * @param find 検索する文字列
 * @return 見つかった位置から末尾までの部分文字列。見つからなければtl::nullopt
 * @details
 * 戻り値は str の一部を指すビューであり、str より長生きさせてはならない。
 * 現時点ではシフトJISのダメ文字は考慮しない (必要に応じて拡張する)。
 */
tl::optional<std::string_view> extract_suffix(std::string_view str, std::string_view find)
{
    if (const auto pos = str.find(find); pos != std::string_view::npos) {
        return str.substr(pos);
    }

    return tl::nullopt;
}

/*!
 * @brief 整数の桁数を数える
 *
 * @param value 数える整数
 * @param base 桁数を数える基数(省略した場合のデフォルト値は10)
 * @return 整数の桁数。基数が不正(2未満)な場合は0を返す。
 * @details
 * 符号は桁数に数えない(-123 は 3 を返す)。0 は 1 桁として数える。
 */
int count_digits(int value, int base)
{
    if (base < 2) {
        return 0;
    }

    int count = 0;

    do {
        value /= base;
        count++;
    } while (value != 0);

    return count;
}

/*!
 * @brief 1バイトの値を16進数2桁で表したときの上位桁の文字を返す
 * @param value 変換する値
 * @return 上位4ビットを表す16進数の大文字('0'-'9', 'A'-'F')
 */
char hexify_upper(uint8_t value)
{
    return hex_symbol_table.at(value / 16);
}

/*!
 * @brief 1バイトの値を16進数2桁で表したときの下位桁の文字を返す
 * @param value 変換する値
 * @return 下位4ビットを表す16進数の大文字('0'-'9', 'A'-'F')
 */
char hexify_lower(uint8_t value)
{
    return hex_symbol_table.at(value % 16);
}

/*!
 * @brief 値を8進数1桁の文字に変換する
 * @param i 変換する値。8以上の場合は8で割った余りを変換する
 * @return 8進数の文字('0'-'7')
 */
char octify(uint8_t i)
{
    return hex_symbol_table.at(i % 8);
}

/*!
 * @brief 文字がASCIIの数字かどうかを判定する
 * @param c 判定する文字
 * @return '0'から'9'のいずれかならtrue
 * @details
 * std::isdigit と異なりロケールの影響を受けず、負の値の char を渡しても未定義動作にならない。
 */
bool is_numeric(char c)
{
    return (c >= '0') && (c <= '9');
}
