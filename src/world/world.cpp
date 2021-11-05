﻿#include "world/world.h"
#include "player-info/race-types.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

world_type world;
world_type *w_ptr = &world;

/*!
 * @brief ゲーム時間が日中かどうかを返す /
 * Whether daytime or not
 * @return 日中ならばTRUE、夜ならばFALSE
 */
bool is_daytime(void)
{
    int32_t len = TURNS_PER_TICK * TOWN_DAWN;
    if ((w_ptr->game_turn % len) < (len / 2))
        return true;
    else
        return false;
}

/*!
 * @brief 現在の日数、時刻を返す /
 * Extract day, hour, min
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param day 日数を返すための参照ポインタ
 * @param hour 時数を返すための参照ポインタ
 * @param min 分数を返すための参照ポインタ
 */
void extract_day_hour_min(PlayerType *player_ptr, int *day, int *hour, int *min)
{
    const int32_t A_DAY = TURNS_PER_TICK * TOWN_DAWN;
    int32_t turn_in_today = (w_ptr->game_turn + A_DAY / 4) % A_DAY;

    switch (player_ptr->start_race) {
    case PlayerRaceType::VAMPIRE:
    case PlayerRaceType::SKELETON:
    case PlayerRaceType::ZOMBIE:
    case PlayerRaceType::SPECTRE:
        *day = (w_ptr->game_turn - A_DAY * 3 / 4) / A_DAY + 1;
        break;
    default:
        *day = (w_ptr->game_turn + A_DAY / 4) / A_DAY + 1;
        break;
    }
    *hour = (24 * turn_in_today / A_DAY) % 24;
    *min = (1440 * turn_in_today / A_DAY) % 60;
}

/*!
 * @brief 実ゲームプレイ時間を更新する
 */
void update_playtime(void)
{
    if (w_ptr->start_time != 0) {
        uint32_t tmp = (uint32_t)time(nullptr);
        w_ptr->play_time += (tmp - w_ptr->start_time);
        w_ptr->start_time = tmp;
    }
}

/*!
 * @brief 勝利したクラスを追加する
 */
void add_winner_class(PlayerClassType c)
{
    if (!w_ptr->noscore)
        w_ptr->sf_winner.set(c);
}

/*!
 * @brief 引退したクラスを追加する
 */
void add_retired_class(PlayerClassType c)
{
    if (!w_ptr->noscore)
        w_ptr->sf_retired.set(c);
}

/*!
 * @brief 勝利したクラスか判定する
 */
bool is_winner_class(PlayerClassType c)
{
    if (c == PlayerClassType::MAX)
        return false;
    return w_ptr->sf_winner.has(c);
}

/*!
 * @brief 引退したクラスか判定する
 */
bool is_retired_class(PlayerClassType c)
{
    if (c == PlayerClassType::MAX)
        return false;
    return w_ptr->sf_retired.has(c);
}
