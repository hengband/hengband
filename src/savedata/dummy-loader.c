#include "savedata/dummy-loader.h"
#include "savedata/load-util.h"

/*!
 * @brief ダミーバイトを読み込む
 * @param なし
 * @return なし
 * @details もはや何に使われていたのか不明
 */
void rd_dummy1(void)
{
    s16b tmp16s;
    rd_s16b(&tmp16s);
    for (int i = 0; i < tmp16s; i++) {
        s16b tmp16s2;
        rd_s16b(&tmp16s2);
    }
}

/*!
 * @brief ダミーバイトを読み込む
 * @param なし
 * @return なし
 * @details もはや何に使われていたのか不明
 */
void rd_dummy2(void)
{
    byte tmp8u;
    for (int i = 0; i < 48; i++)
        rd_byte(&tmp8u);

    strip_bytes(12);
}
