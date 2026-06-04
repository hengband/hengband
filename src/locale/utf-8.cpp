#include "locale/utf-8.h"
#include <range/v3/algorithm.hpp>

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
