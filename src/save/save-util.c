#include "save/save-util.h"

FILE *saving_savefile; /* Current save "file" */
byte save_xor_byte; /* Simple encryption */
u32b v_stamp = 0L; /* A simple "checksum" on the actual values */
u32b x_stamp = 0L; /* A simple "checksum" on the encoded bytes */

/*!
 * @brief 1バイトをファイルに書き込む / These functions place information into a savefile a byte at a time
 * @param v 書き込むバイト値
 * @return なし
 */
static void sf_put(byte v)
{
    /* Encode the value, write a character */
    save_xor_byte ^= v;
    (void)putc((int)save_xor_byte, saving_savefile);

    /* Maintain the checksum info */
    v_stamp += v;
    x_stamp += save_xor_byte;
}

/*!
 * @brief 1バイトをファイルに書き込む(sf_put()の糖衣)
 * @param v 書き込むバイト
 * @return なし
 */
void wr_byte(byte v) { sf_put(v); }

/*!
 * @brief 符号なし16ビットをファイルに書き込む
 * @param v 書き込む符号なし16bit値
 * @return なし
 */
void wr_u16b(u16b v)
{
    wr_byte((byte)(v & 0xFF));
    wr_byte((byte)((v >> 8) & 0xFF));
}

/*!
 * @brief 符号あり16ビットをファイルに書き込む
 * @param v 書き込む符号あり16bit値
 * @return なし
 */
void wr_s16b(s16b v) { wr_u16b((u16b)v); }

/*!
 * @brief 符号なし32ビットをファイルに書き込む
 * @param v 書き込む符号なし32bit値
 * @return なし
 */
void wr_u32b(u32b v)
{
    wr_byte((byte)(v & 0xFF));
    wr_byte((byte)((v >> 8) & 0xFF));
    wr_byte((byte)((v >> 16) & 0xFF));
    wr_byte((byte)((v >> 24) & 0xFF));
}

/*!
 * @brief 符号あり32ビットをファイルに書き込む
 * @param v 書き込む符号あり32bit値
 * @return なし
 */
void wr_s32b(s32b v) { wr_u32b((u32b)v); }

/*!
 * @brief 文字列をファイルに書き込む
 * @param str 書き込む文字列
 * @return なし
 */
void wr_string(concptr str)
{
    while (*str) {
        wr_byte(*str);
        str++;
    }
    wr_byte(*str);
}
