/*!
 * @file multibyte-char.cpp
 * @brief 内部文字コードの文字単位の処理
 * @details
 * 文字列の位置を扱う関数は、後続のバイトが欠けた多バイト文字を、先頭バイトだけの 1 バイトの文字とみなす
 * (mb_char_length_at() の扱いにそろえる)。
 * ただし mb_valid_char_length_at() は後続のバイトの値まで確かめる別の判定で、ほかの関数と文字の区切りが一致しないことがある。
 */

#include "locale/multibyte-char.h"
#include "locale/character-encoding.h"
#include <algorithm>
#include <cctype>

/*!
 * @brief 1 バイトの文字が表示できる文字かどうかを返す
 * @param c 文字
 * @return ASCII の表示できる文字なら true
 */
bool is_printable_single_byte_char(char c)
{
    return isprint(static_cast<unsigned char>(c)) != 0;
}

/*!
 * @brief 指定した位置から始まる文字のバイト数を返す
 * @param str 文字列
 * @param pos 文字の先頭の位置 (バイト)
 * @return 文字のバイト数。後続のバイトが欠けた多バイト文字は先頭バイトだけを 1 文字とみなして 1、
 *         pos が文字列の範囲外なら 1
 * @details 多バイト文字の後続のバイトは先頭バイトと同じ範囲の値もとるため、pos は文字の先頭を指していること。
 */
size_t mb_char_length_at(std::string_view str, size_t pos)
{
    if (pos >= str.size()) {
        return 1;
    }

    // pos + char_length は pos が大きいと桁あふれするので、残りの長さで確かめる
    const auto char_length = mb_char_byte_length(str[pos]);
    return (str.size() - pos >= char_length) ? char_length : 1;
}

/*!
 * @brief 指定した位置から始まる文字のバイト数を、後続のバイトの値まで確かめて返す
 * @param str 文字列
 * @param pos 文字の先頭の位置 (バイト)
 * @return 後続のバイトが内部文字コードの範囲の値でそろった多バイト文字ならそのバイト数。
 *         1 バイトの文字、不正な並び、後続のバイトが欠けた文字、pos が範囲外のときは 1 (英語版では常に 1)
 * @details mb_char_length_at() は先頭バイトだけで長さを決めるが、こちらは後続のバイトの値も確かめる。
 * 設定ファイルの区切り (':' や '/') を探すときのように、不正な並びのために後ろの文字を読み飛ばしてはいけない処理で使う。
 * utf8_next_char_byte_length() が UTF-8 として正しいとする多バイト文字 (冗長な表現やサロゲートを除く) をその長さの 1 文字とみなす。
 */
size_t mb_valid_char_length_at([[maybe_unused]] std::string_view str, [[maybe_unused]] size_t pos)
{
#ifdef JP
    if (pos >= str.size()) {
        return 1;
    }

    const auto length = utf8_next_char_byte_length(str.substr(pos));
    return (length > 1) ? static_cast<size_t>(length) : 1;
#else
    return 1;
#endif
}

/*!
 * @brief 文字列の指定した位置から、後続のバイトまでそろった多バイト文字が始まるかを返す
 * @param sv 文字列
 * @param pos 位置(バイト)
 * @return pos が多バイト文字の先頭バイトを指し、その後ろに後続のバイトがあれば true (英語版では常に false)
 * @details
 * 多バイト文字の後続のバイトは先頭バイトと同じ範囲の値もとるため、pos は文字の先頭を指していること。
 */
bool is_multibyte_char_at(std::string_view sv, size_t pos)
{
    return mb_char_length_at(sv, pos) > 1;
}

/*!
 * @brief NUL 終端の文字列の先頭が、後続のバイトまでそろった多バイト文字かを返す
 * @param s NUL 終端の文字列
 * @return s が多バイト文字の先頭バイトを指し、その後ろに後続のバイトがあれば true (英語版では常に false)
 */
bool is_multibyte_char_at(const char *s)
{
    const auto char_length = mb_char_byte_length(s[0]);
    if (char_length == 1) {
        return false;
    }

    // 後続のバイトが揃っているかは、NUL 終端を越えて読まないように 1 バイトずつ確かめる
    return std::find(s + 1, s + char_length, '\0') == s + char_length;
}

/*!
 * @brief 文字列の先頭の 1 文字を返す
 * @param str 文字列
 * @return 先頭の 1 文字。str が空なら空文字列
 * @details 後続のバイトが欠けた多バイト文字は、先頭バイトだけを返す。
 */
std::string_view mb_first_char(std::string_view str)
{
    return str.substr(0, mb_char_length_at(str, 0));
}

/*!
 * @brief 指定した位置を含む文字の先頭位置を返す
 * @param str 文字列
 * @param pos 位置 (バイト)
 * @return pos を含む文字の先頭位置。pos が多バイト文字の途中を指していればその文字の先頭、
 *         文字列の長さ以上なら文字列の長さ
 * @details 多バイト文字の後続のバイトは先頭バイトと区別できない場合があるため、文字列の先頭から数える。
 */
size_t mb_char_start_pos(std::string_view str, size_t pos)
{
    if (pos >= str.size()) {
        return str.size();
    }

    size_t start = 0;
    while (true) {
        const auto next = start + mb_char_length_at(str, start);
        if (next > pos) {
            return start;
        }

        start = next;
    }
}

/*!
 * @brief 指定した位置が文字の境界 (文字の先頭か文字列の末尾) かどうかを返す
 * @param str 文字列
 * @param pos 位置 (バイト)
 * @return pos が文字の先頭か文字列の長さに等しければ true。多バイト文字の途中や、文字列の長さを超える位置なら false
 */
bool is_mb_char_boundary(std::string_view str, size_t pos)
{
    // 範囲外の位置では mb_char_start_pos() が長さを返すので、pos が長さに等しいときだけ true になる
    return mb_char_start_pos(str, pos) == pos;
}

/*!
 * @brief 指定した位置以降で最初の文字の境界を返す
 * @param str 文字列
 * @param pos 位置 (バイト)
 * @return pos が多バイト文字の途中を指していればその文字の次の文字の先頭、文字の境界なら pos。
 *         文字列の長さ以上なら文字列の長さ
 * @details 多バイト文字の後続のバイトは先頭バイトと区別できない場合があるため、文字列の先頭から数える。
 */
size_t mb_next_char_boundary(std::string_view str, size_t pos)
{
    if (pos >= str.size()) {
        return str.size();
    }

    const auto start = mb_char_start_pos(str, pos);
    return (start == pos) ? pos : start + mb_char_length_at(str, start);
}

/*!
 * @brief 指定した位置の直前にある文字の先頭位置を返す
 * @param str 文字列
 * @param pos 位置 (バイト)
 * @return pos より前で始まる最後の文字の先頭位置。pos が 0 なら 0、文字列の長さを超えていれば長さとみなす
 * @details 多バイト文字の後続のバイトは先頭バイトと区別できない場合があるため、文字列の先頭から数える。
 */
size_t mb_prev_char_pos(std::string_view str, size_t pos)
{
    pos = std::min(pos, str.size());
    size_t prev = 0;
    for (size_t i = 0; i < pos; i += mb_char_length_at(str, i)) {
        prev = i;
    }

    return prev;
}

namespace {
/*!
 * @brief 1 文字の表示幅を返す
 * @param char_length 文字のバイト数
 * @return 多バイト文字なら 2、1 バイトの文字なら 1
 */
size_t char_display_width(size_t char_length)
{
    return (char_length > 1) ? 2 : 1;
}
}

/*!
 * @brief 文字列の表示幅を返す
 * @param str 文字列
 * @return 表示幅 (桁数)
 * @details 多バイト文字は 2 桁、1 バイトの文字 (後続のバイトが欠けた先頭バイトを含む) は 1 桁と数える。
 * 同梱の fmt も、書式の幅指定 ({:<20} など) をこの関数で数える (fmt/format.h の compute_width())。
 */
size_t str_display_width(std::string_view str)
{
    size_t width = 0;
    for (size_t pos = 0; pos < str.size();) {
        const auto char_length = mb_char_length_at(str, pos);
        width += char_display_width(char_length);
        pos += char_length;
    }

    return width;
}

/*!
 * @brief 文字列の先頭から、指定した表示幅に収まる部分を返す
 * @param str 文字列
 * @param width 表示幅 (桁数) の上限
 * @return 多バイト文字を分断せずに width 桁に収まる、最も長い先頭の部分
 * @details 表示幅は str_display_width() と同じく数える。収まらない多バイト文字と、
 * 後続のバイトが欠けた多バイト文字の先頭バイトは含めない (str_substr() と同じ)。
 */
std::string_view str_prefix_by_width(std::string_view str, size_t width)
{
    size_t pos = 0;
    size_t used = 0;
    while (pos < str.size()) {
        const auto char_length = mb_char_byte_length(str[pos]);
        const auto char_width = char_display_width(char_length);
        if ((pos + char_length > str.size()) || (used + char_width > width)) {
            break;
        }

        pos += char_length;
        used += char_width;
    }

    return str.substr(0, pos);
}

/*!
 * @brief 文字列の先頭から、指定した表示幅に収まる部分を返す (ムーブ版)
 * @param str 文字列。中身はムーブされる
 * @param width 表示幅 (桁数) の上限
 * @return 収まる部分。切り出す範囲は std::string_view を取る版と同じ
 * @details 引数の文字列を縮めて返すため、新たな領域を確保しない。一時オブジェクトを渡したときに、
 * 解放された領域を指す std::string_view を返さないためにも使う。
 */
std::string str_prefix_by_width(std::string &&str, size_t width)
{
    str.resize(str_prefix_by_width(std::string_view(str), width).length());
    return std::move(str);
}

/*!
 * @brief 文字列の先頭から、指定した表示幅に収まる部分を返す (C 文字列版)
 * @param str NUL で終わる文字列
 * @param width 表示幅 (桁数) の上限
 * @return 収まる部分。切り出す範囲は std::string_view を取る版と同じ
 */
std::string_view str_prefix_by_width(const char *str, size_t width)
{
    return str_prefix_by_width(std::string_view(str), width);
}
