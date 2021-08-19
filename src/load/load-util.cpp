#include "load/load-util.h"
#include "term/screen-processor.h"
#ifdef JP
#include "locale/japanese.h"
#endif

FILE *loading_savefile;
uint32_t loading_savefile_version;
byte load_xor_byte; // Old "encryption" byte.
uint32_t v_check = 0L; // Simple "checksum" on the actual values.
uint32_t x_check = 0L; // Simple "checksum" on the encoded bytes.

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
 */
void rd_byte(byte *ip) { *ip = sf_get(); }

/*!
 * @brief ロードファイルポインタから符号なし16bit値を読み込んでポインタに渡す
 * @param ip 読み込みポインタ
 */
void rd_u16b(uint16_t *ip)
{
    (*ip) = sf_get();
    (*ip) |= ((uint16_t)(sf_get()) << 8);
}

/*!
 * @brief ロードファイルポインタから符号つき16bit値を読み込んでポインタに渡す
 * @param ip 読み込みポインタ
 */
void rd_s16b(int16_t *ip) { rd_u16b((uint16_t *)ip); }

/*!
 * @brief ロードファイルポインタから符号なし32bit値を読み込んでポインタに渡す
 * @param ip 読み込みポインタ
 */
void rd_u32b(uint32_t *ip)
{
    (*ip) = sf_get();
    (*ip) |= ((uint32_t)(sf_get()) << 8);
    (*ip) |= ((uint32_t)(sf_get()) << 16);
    (*ip) |= ((uint32_t)(sf_get()) << 24);
}

/*!
 * @brief ロードファイルポインタから符号つき32bit値を読み込んでポインタに渡す
 * @param ip 読み込みポインタ
 */
void rd_s32b(int32_t *ip) { rd_u32b((uint32_t *)ip); }

/*!
 * @brief ロードファイルポインタから文字列を読み込んでポインタに渡す / Hack -- read a string
 * @param str 読み込みポインタ
 * @param max 最大読み取りバイト数
 */
void rd_string(char *str, int max)
{
    for (int i = 0; true; i++) {
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
 */
void strip_bytes(int n)
{
    byte tmp8u;
    while (n--)
        rd_byte(&tmp8u);
}

/**
 * @brief ロード中のセーブファイルのバージョンが引数で指定したバージョンと比較して古いかどうか調べる
 *
 * @param version 比較するセーブファイルのバージョン
 * @return bool ロード中のセーブファイルのバージョンが version より古いなら true
 *              version と等しいかより新しいなら false
 */
bool loading_savefile_version_is_older_than(uint32_t version)
{
    return loading_savefile_version < version;
}
