#pragma once

#include "system/h-type.h"

class PlayerType;
void change_floor(PlayerType *player_ptr);

GAME_TURN limit_loaded_floor_monster_trials(GAME_TURN requested_trials);

// 復帰時の生成方針。登録枠がある間だけ実行し、一時的な配置失敗でも残りの試行を続ける。
template <typename HasCapacity, typename Allocate>
void attempt_loaded_floor_monsters(GAME_TURN requested_trials, HasCapacity has_capacity, Allocate allocate)
{
    const auto trials = limit_loaded_floor_monster_trials(requested_trials);
    for (GAME_TURN i = 0; i < trials && has_capacity(); ++i) {
        static_cast<void>(allocate());
    }
}
