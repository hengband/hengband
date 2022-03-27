#include "save/save-util.h"

FILE *saving_savefile; /* Current save "file" */
byte save_xor_byte; /* Simple encryption */
uint32_t v_stamp = 0L; /* A simple "checksum" on the actual values */
uint32_t x_stamp = 0L; /* A simple "checksum" on the encoded bytes */

/*!
 * @brief 1バイトをファイルに書き込む / These functions place information into a savefile a byte at a time
 * @param v 書き込むバイト値
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
 * @brief bool値をファイルに書き込む(wr_byte()の糖衣)
 * @param v 書き込むbool値
 */
void wr_bool(bool v)
{
    wr_byte(v ? 1 : 0);
}

/*!
 * @brief 1バイトをファイルに書き込む(sf_put()の糖衣)
 * @param v 書き込むバイト
 */
void wr_byte(byte v)
{
    sf_put(v);
}

/*!
 * @brief 符号なし16ビットをファイルに書き込む
 * @param v 書き込む符号なし16bit値
 */
void wr_u16b(uint16_t v)
{
    wr_byte((byte)(v & 0xFF));
    wr_byte((byte)((v >> 8) & 0xFF));
}

/*!
 * @brief 符号あり16ビットをファイルに書き込む
 * @param v 書き込む符号あり16bit値
 */
void wr_s16b(int16_t v)
{
    wr_u16b((uint16_t)v);
}

/*!
 * @brief 符号なし32ビットをファイルに書き込む
 * @param v 書き込む符号なし32bit値
 */
void wr_u32b(uint32_t v)
{
    wr_byte((byte)(v & 0xFF));
    wr_byte((byte)((v >> 8) & 0xFF));
    wr_byte((byte)((v >> 16) & 0xFF));
    wr_byte((byte)((v >> 24) & 0xFF));
}

/*!
 * @brief 符号あり32ビットをファイルに書き込む
 * @param v 書き込む符号あり32bit値
 */
void wr_s32b(int32_t v)
{
    wr_u32b((uint32_t)v);
}

/*!
 * @brief 文字列をファイルに書き込む
 * @param str 書き込む文字列
 */
void wr_string(std::string_view sv)
{
    for (auto c : sv) {
        wr_byte(c);
    }
    wr_byte('\0');
}
