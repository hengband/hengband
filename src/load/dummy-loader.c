#include "load/dummy-loader.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/monster-loader.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"

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

/*!
 * @brief 変愚蛮怒 v1.5.0より大きなバージョンにおいて、ダミーでモンスターを読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details もはや何に使われていたのか不明
 */
void rd_dummy_monsters(player_type *creature_ptr)
{
    if (h_older_than(1, 5, 0, 2))
        return;

    s16b tmp16s;
    rd_s16b(&tmp16s);
    for (int i = 0; i < tmp16s; i++) {
        monster_type dummy_mon;
        rd_monster(creature_ptr, &dummy_mon);
    }
}

/*!
 * @brief ダミー情報スキップ / Strip the "ghost" info
 * @return なし
 * @details
 * This is such a nasty hack it hurts.
 */
void rd_ghost(void)
{
    char buf[64];
    rd_string(buf, sizeof(buf));
    strip_bytes(60);
}

void rd_dummy3(void)
{
    u32b tmp32u;
    rd_u32b(&tmp32u);

    u16b tmp16u;
    rd_u16b(&tmp16u);

    byte tmp8u;
    rd_byte(&tmp8u);
}
