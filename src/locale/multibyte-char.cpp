/*!
 * @file multibyte-char.cpp
 * @brief 内部文字コードの文字単位の処理
 * @details
 * 文字列の位置を扱う関数は、後続のバイトが欠けた多バイト文字を、先頭バイトだけの 1 バイトの文字とみなす
 * (mb_char_length_at() の扱いにそろえる)。
 */

#include "locale/multibyte-char.h"
#include <algorithm>
#include <cctype>

/*!
 * @brief 1 バイトの文字が表示できる文字かどうかを返す
 * @param c 文字
 * @return ASCII の表示できる文字か、Shift_JIS の半角カナ (1 バイトの文字) なら true
 */
bool is_printable_single_byte_char(char c)
{
#ifdef JP
    if (iskana(c)) {
        return true;
    }
#endif

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
