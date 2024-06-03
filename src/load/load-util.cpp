#include "load/load-util.h"
#include "locale/character-encoding.h"
#include "locale/japanese.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"

FILE *loading_savefile;
uint32_t loading_savefile_version;
byte load_xor_byte; // Old "encryption" byte.
uint32_t v_check = 0L; // Simple "checksum" on the actual values.
uint32_t x_check = 0L; // Simple "checksum" on the encoded bytes.

/*
 * Character encoding of loading savefile.
 */
CharacterEncoding loading_character_encoding = CharacterEncoding::UNKNOWN;

/*!
 * @brief ゲームスクリーンにメッセージを表示する / Hack -- Show information on the screen, one line at a time.
 * @param msg 表示文字列
 * @details
 * Avoid the top two lines, to avoid interference with "msg_print()".
 */
void load_note(std::string_view msg)
{
    static TERM_LEN y = 2;
    prt(msg, y, 0);
    if (++y >= MAIN_TERM_MIN_ROWS) {
        y = 2;
    }

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
 * @brief ロードファイルポインタからbool値を読み込む
 */
bool rd_bool()
{
    return rd_byte() != 0;
}

/*!
 * @brief ロードファイルポインタから1バイトを読み込む
 */
byte rd_byte()
{
    return sf_get();
}

/*!
 * @brief ロードファイルポインタから符号なし16bit値を読み込む
 */
uint16_t rd_u16b()
{
    uint16_t val = sf_get();
    val |= (static_cast<uint16_t>(sf_get()) << 8);

    return val;
}

/*!
 * @brief ロードファイルポインタから符号つき16bit値を読み込む
 */
int16_t rd_s16b()
{
    return static_cast<int16_t>(rd_u16b());
}

/*!
 * @brief ロードファイルポインタから符号なし32bit値を読み込む
 */
uint32_t rd_u32b()
{
    uint32_t val = sf_get();
    val |= (static_cast<uint32_t>(sf_get()) << 8);
    val |= (static_cast<uint32_t>(sf_get()) << 16);
    val |= (static_cast<uint32_t>(sf_get()) << 24);

    return val;
}

/*!
 * @brief ロードファイルポインタから符号つき32bit値を読み込む
 */
int32_t rd_s32b()
{
    return static_cast<int32_t>(rd_u32b());
}

/*!
 * @brief ロードファイルポインタから文字列を読み込む
 */
std::string rd_string()
{
    std::string str;
    str.reserve(1024);

    while (true) {
        const auto ch = static_cast<char>(rd_byte());
        if (ch == '\0') {
            break;
        }

        str.push_back(ch);
    }

#ifdef JP
    switch (loading_character_encoding) {
#ifdef SJIS
    case CharacterEncoding::EUC_JP:
        euc2sjis(str.data());
        break;
#endif

#ifdef EUC
    case CharacterEncoding::SHIFT_JIS:
        sjis2euc(str.data());
        break;
#endif

    case CharacterEncoding::UNKNOWN: {
        const auto code = codeconv(str.data());

        /* 漢字コードが判明したら、それを記録 */
        if (code != CharacterEncoding::UNKNOWN) {
            loading_character_encoding = code;
        }

        break;
    }
    default:
        break;
    }
#endif

    str.shrink_to_fit();
    return str;
}

/*!
 * @brief ロードファイルポインタを指定バイト分飛ばして進める / Hack -- strip some bytes
 * @param n スキップバイト数
 */
void strip_bytes(int n)
{
    while (n-- > 0) {
        (void)rd_byte();
    }
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
