#pragma once
/*!
 * @file run-execution.h
 * @brief プレイヤーの走行処理ヘッダ
 */

#include "floor/geometry.h"
#include "system/angband.h"

extern bool ignore_avoid_run;

/*!
 * @brief 走っている途中の進み方 (セーブに書いて、再開したときに続きから走るのに使う)
 */
struct RunningCourse {
    Direction current; //!< 走っている方向
    Direction prevdir; //!< 来た方向
    bool openarea; //!< 開けた場所を走っているか
    bool breakright; //!< 右側の切れ目を探しているか
    bool breakleft; //!< 左側の切れ目を探しているか
};

class PlayerType;
void run_step(PlayerType *player_ptr, const Direction &dir);
RunningCourse get_running_course();
void set_running_course(const RunningCourse &course);
