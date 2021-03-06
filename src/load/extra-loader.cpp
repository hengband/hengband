/*!
 * todo 「その他」が雑多すぎて肥大化している。今後の課題として分割を検討する
 * @brief その他の情報を読み込む処理
 * @date 2020/07/05
 * @author Hourier
 */

#include "load/extra-loader.h"
#include "load/angband-version-comparer.h"
#include "load/dummy-loader.h"
#include "load/load-util.h"
#include "load/world-loader.h"
#include "world/world.h"

/*!
 * @brief その他の情報を読み込む / Read the "extra" information
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void rd_extra(player_type *creature_ptr)
{
    if (z_older_than(10, 0, 7))
        creature_ptr->riding = 0;
    else
        rd_s16b(&creature_ptr->riding);

    if (h_older_than(1, 5, 0, 0))
        creature_ptr->floor_id = 0;
    else
        rd_s16b(&creature_ptr->floor_id);

    rd_dummy_monsters(creature_ptr);
    if (z_older_than(10, 1, 2))
        current_world_ptr->play_time = 0;
    else
        rd_u32b(&current_world_ptr->play_time);

    rd_visited_towns(creature_ptr);
    if (!z_older_than(11, 0, 5))
        rd_u32b(&creature_ptr->count);
}
