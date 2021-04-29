/*
 * @file player-energy.cpp
 * @brief ゲームターン当たりの行動エネルギー増減処理
 * @author Hourier
 * @date 2021/04/29
 */

#include "player-status/player-energy.h"
#include "system/player-type-definition.h"

// todo 第3引数 (演算：代入、加算、減算、他)を導入する。enumを使う
void take_turn(player_type *creature_ptr, PERCENTAGE need_cost)
{
    creature_ptr->energy_use = (ENERGY)need_cost;
}

void free_turn(player_type *creature_ptr)
{
    creature_ptr->energy_use = 0;
}
