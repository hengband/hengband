#include "load/dummy-loader.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/monster-loader.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief ダミーバイトを読み込む
 * @param なし
 * @details もはや何に使われていたのか不明
 */
void rd_dummy1(void)
{
    auto tmp16s = rd_s16b();
    strip_bytes(2 * tmp16s);
}

/*!
 * @brief ダミーバイトを読み込む
 * @param なし
 * @details もはや何に使われていたのか不明
 */
void rd_dummy2(void)
{
    strip_bytes(48);
    strip_bytes(12);
}

/*!
 * @brief 変愚蛮怒 v1.5.0より大きなバージョンにおいて、ダミーでモンスターを読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details もはや何に使われていたのか不明
 */
void rd_dummy_monsters(player_type *player_ptr)
{
    if (h_older_than(1, 5, 0, 2))
        return;

    auto tmp16s = rd_s16b();
    for (int i = 0; i < tmp16s; i++) {
        monster_type dummy_mon;
        rd_monster(player_ptr, &dummy_mon);
    }
}

/*!
 * @brief ダミー情報スキップ / Strip the "ghost" info
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
    strip_bytes(2);
    strip_bytes(1);
}
