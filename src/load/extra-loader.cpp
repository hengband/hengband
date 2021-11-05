﻿/*!
 * @brief その他の情報を読み込む処理
 * @date 2020/07/05
 * @author Hourier
 * @todo 「その他」が雑多すぎて肥大化している。今後の課題として分割を検討する
 */

#include "load/extra-loader.h"
#include "load/angband-version-comparer.h"
#include "load/dummy-loader.h"
#include "load/load-util.h"
#include "load/world-loader.h"
#include "system/player-type-definition.h"
#include "world/world.h"

/*!
 * @brief その他の情報を読み込む / Read the "extra" information
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void rd_extra(PlayerType *player_ptr)
{
    if (h_older_than(0, 0, 7))
        player_ptr->riding = 0;
    else
        player_ptr->riding = rd_s16b();

    if (h_older_than(1, 5, 0, 0))
        player_ptr->floor_id = 0;
    else
        player_ptr->floor_id = rd_s16b();

    rd_dummy_monsters(player_ptr);
    if (h_older_than(0, 1, 2))
        w_ptr->play_time = 0;
    else
        w_ptr->play_time = rd_u32b();

    rd_visited_towns(player_ptr);
    if (!h_older_than(1, 0, 5))
        player_ptr->count = rd_u32b();
}
