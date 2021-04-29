/*
 * @file player-energy.cpp
 * @brief ゲームターン当たりの行動エネルギー増減処理
 * @author Hourier
 * @date 2021/04/29
 */

#include "player-status/player-energy.h"
#include "system/player-type-definition.h"

/*
 * @brief プレーヤーの行動エネルギーを更新する
 * @param creature_ptr プレーヤーの参照ポインタ
 * @param need_cost 行動エネルギー
 * @param ut_type 現在値に対する演算方法
 * @return なし
 */
void update_player_turn_energy(player_type *creature_ptr, ENERGY need_cost, update_turn_type ut_type)
{
    switch (ut_type) {
    case update_turn_type::ENERGY_SUBSTITUTION:
        creature_ptr->energy_use = need_cost;
        return;
    case update_turn_type::ENERGY_ADDITION:
        creature_ptr->energy_use += need_cost;
        return;
    case update_turn_type::ENERGY_SUBTRACTION:
        creature_ptr->energy_use -= need_cost;
        return;
    case update_turn_type::ENERGY_MULTIPLICATION:
        creature_ptr->energy_use *= need_cost;
        return;
    case update_turn_type::ENERGY_DIVISION:
        creature_ptr->energy_use /= need_cost;
        return;
    default:
        return;
    }
}

/*
 * @brief ターン消費をなくす (主にコマンド実行に失敗した場合)
 * @param player_type プレーヤーへの参照ポインタ
 * @return なし
 */
void reset_player_turn(player_type *creature_ptr)
{
    update_player_turn_energy(creature_ptr, 0, update_turn_type::ENERGY_SUBSTITUTION);
}
