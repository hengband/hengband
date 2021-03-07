#include "load/load-util.h"
#include "term/screen-processor.h"
#ifdef JP
#include "locale/japanese.h"
#endif

FILE *loading_savefile;
byte load_xor_byte; // Old "encryption" byte.
u32b v_check = 0L; // Simple "checksum" on the actual values.
u32b x_check = 0L; // Simple "checksum" on the encoded bytes.

/*
 * Japanese Kanji code
 * 0: Unknown
 * 1: ASCII
 * 2: EUC
 * 3: SJIS
 */
byte kanji_code = 0;

/*!
 * @brief ゲームスクリーンにメッセージを表示する / Hack -- Show information on the screen, one line at a time.
 * @param msg 表示文字列
 * @return なし
 * @details
 * Avoid the top two lines, to avoid interference with "msg_print()".
 */
void load_note(concptr msg)
{
    static TERM_LEN y = 2;
    prt(msg, y, 0);
    if (++y >= 24)
        y = 2;

    term_fresh();
}

/*!
 * @brief ロードファイルポインタから1バイトを読み込む
 * @return 読み込んだバイト値
 * @details
 * The following functions are used to load the basic building blocks
 * of savefiles.  They also maintain the "checksum" info for 2.7.0+
 */
byte sf_get(void)
{
    byte c = getc(loading_savefile) & 0xFF;
    byte v = c ^ load_xor_byte;
    load_xor_byte = c;

    v_check += v;
    x_check += load_xor_byte;
    return v;
}

/*!
 * @brief ロードファイルポインタから1バイトを読み込んでポインタに渡す
 * @param ip 読み込みポインタ
 * @return なし
 */
void rd_byte(byte *ip) { *ip = sf_get(); }

/*!
 * @brief ロードファイルポインタから符号なし16bit値を読み込んでポインタに渡す
 * @param ip 読み込みポインタ
 * @return なし
 */
void rd_u16b(u16b *ip)
{
    (*ip) = sf_get();
    (*ip) |= ((u16b)(sf_get()) << 8);
}

/*!
 * @brief ロードファイルポインタから符号つき16bit値を読み込んでポインタに渡す
 * @param ip 読み込みポインタ
 * @return なし
 */
void rd_s16b(s16b *ip) { rd_u16b((u16b *)ip); }

/*!
 * @brief ロードファイルポインタから符号なし32bit値を読み込んでポインタに渡す
 * @param ip 読み込みポインタ
 * @return なし
 */
void rd_u32b(u32b *ip)
{
    (*ip) = sf_get();
    (*ip) |= ((u32b)(sf_get()) << 8);
    (*ip) |= ((u32b)(sf_get()) << 16);
    (*ip) |= ((u32b)(sf_get()) << 24);
}

/*!
 * @brief ロードファイルポインタから符号つき32bit値を読み込んでポインタに渡す
 * @param ip 読み込みポインタ
 * @return なし
 */
void rd_s32b(s32b *ip) { rd_u32b((u32b *)ip); }

/*!
 * @brief ロードファイルポインタから文字列を読み込んでポインタに渡す / Hack -- read a string
 * @param str 読み込みポインタ
 * @param max 最大読み取りバイト数
 * @return なし
 */
void rd_string(char *str, int max)
{
    for (int i = 0; TRUE; i++) {
        byte tmp8u;
        rd_byte(&tmp8u);
        if (i < max)
            str[i] = tmp8u;

        if (!tmp8u)
            break;
    }

    str[max - 1] = '\0';
#ifdef JP
    switch (kanji_code) {
#ifdef SJIS
    case 2:
        euc2sjis(str);
        break;
#endif

#ifdef EUC
    case 3:
        sjis2euc(str);
        break;
#endif

    case 0: {
        byte code = codeconv(str);

        /* 漢字コードが判明したら、それを記録 */
        if (code)
            kanji_code = code;

        break;
    }
    default:
        break;
    }
#endif
}

/*!
 * @brief ロードファイルポインタを指定バイト分飛ばして進める / Hack -- strip some bytes
 * @param n スキップバイト数
 * @return なし
 */
void strip_bytes(int n)
{
    byte tmp8u;
    while (n--)
        rd_byte(&tmp8u);
}
