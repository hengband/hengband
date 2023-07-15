#include "locale/utf-8.h"

/*!
 * @brief 文字列の最初の文字のUTF-8エンコーディングにおけるバイト長を返す
 *
 * UTF-8エンコーディングの文字列が渡されるのを想定し、
 * その文字列の最初の文字のバイト長を返す。
 * UTF-8エンコーディングとして適合しなければ0を返す。
 * また文字列終端文字('\\0')の場合も0を返す。
 *
 * @note UTF-8エンコーディングの厳密なバリデーションにはなっていない。
 *       2バイト目以降は0x80-0xBF固定ではなく、バイト長・何バイト目かなど
 *       によって若干変化するが、ここでは簡便のため0x80-0xBFの範囲のみ
 *       チェックする
 *
 * @param str 判定する文字列へのポインタ
 *
 * @return 最初の文字のバイト長を返す。
 *         終端文字もしくはUTF-8エンコーディングに適合しない場合は0を返す。
 */
int utf8_next_char_byte_length(concptr str)
{
    const unsigned char *p = (const unsigned char *)str;
    int length = 0;

    // バイト長の判定
    if (0x00 < *p && *p <= 0x7f) {
        length = 1;
    } else if ((*p & 0xe0) == 0xc0) {
        length = 2;
    } else if ((*p & 0xf0) == 0xe0) {
        length = 3;
    } else if ((*p & 0xf8) == 0xf0) {
        length = 4;
    } else {
        return 0;
    }

    // trailing bytesが0x80-0xBFである事のチェック
    while ((++p) < (const unsigned char *)str + length) {
        if ((*p & 0xc0) != 0x80) {
            return 0;
        }
    }

    return length;
}

/*!
 * @brief 文字列がUTF-8の文字列として適合かどうかを判定する
 *
 * @param str 判定する文字列へのポインタ
 *
 * @return 文字列がUTF-8として適合ならTRUE、そうでなければFALSE
 */
bool is_utf8_str(concptr str)
{
    while (*str) {
        const int byte_length = utf8_next_char_byte_length(str);

        if (byte_length == 0) {
            return false;
        }

        str += byte_length;
    }

    return true;
}
