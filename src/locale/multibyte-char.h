/*!
 * @file multibyte-char.h
 * @brief 内部文字コードの文字単位の処理
 * @details
 * 内部文字コード (日本語版の Unix 系では EUC-JP、Windows では Shift_JIS。内部の文字コードを UTF-8 にした構成では UTF-8) の文字列を、
 * バイト単位ではなく文字単位で扱うための関数をまとめる。
 * 文字のバイト数と文字の境界を問い合わせる処理をここに集約し、文字コードごとの判定を呼び出し元に書かないようにする。
 */

#pragma once

#include "locale/character-encoding.h"
#include "system/h-basic.h"
#include <cstddef>
#include <string>
#include <string_view>

/*!
 * @brief 文字の先頭バイトから、その文字のバイト数を返す
 * @param lead 文字の先頭バイト
 * @return 文字のバイト数。英語版では常に 1
 * @details 先頭バイトだけで判定するため、続くバイトが欠けていても多バイト文字のバイト数を返す。
 */
constexpr size_t mb_char_byte_length([[maybe_unused]] char lead)
{
#if defined(JP) && defined(UTF8)
    // UTF-8 の後続のバイト (80-BF) と、先頭に現れない不正なバイト (C0・C1・F5-FF) は 1 バイトの文字とみなす
    const auto length = utf8_lead_byte_length(static_cast<unsigned char>(lead));
    return (length > 0) ? static_cast<size_t>(length) : 1;
#elif defined(JP)
    return iskanji(lead) ? 2 : 1;
#else
    return 1;
#endif
}

/*!
 * @brief 文字の先頭バイトが多バイト文字の先頭かどうかを返す
 * @param lead 文字の先頭バイト
 * @return 多バイト文字の先頭バイトなら true
 */
constexpr bool is_mb_char_lead(char lead)
{
    return mb_char_byte_length(lead) > 1;
}

bool is_printable_single_byte_char(char c);
size_t mb_char_length_at(std::string_view str, size_t pos);
size_t mb_valid_char_length_at(std::string_view str, size_t pos);
bool is_multibyte_char_at(std::string_view sv, size_t pos = 0);
bool is_multibyte_char_at(const char *s);
std::string_view mb_first_char(std::string_view str);
size_t mb_char_start_pos(std::string_view str, size_t pos);
bool is_mb_char_boundary(std::string_view str, size_t pos);
size_t mb_next_char_boundary(std::string_view str, size_t pos);
size_t mb_prev_char_pos(std::string_view str, size_t pos);
size_t str_display_width(std::string_view str);
std::string_view str_prefix_by_width(std::string_view str, size_t width);
std::string str_prefix_by_width(std::string &&str, size_t width);
std::string_view str_prefix_by_width(const char *str, size_t width);
