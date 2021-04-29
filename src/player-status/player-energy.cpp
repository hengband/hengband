/*
 * @file player-energy.cpp
 * @brief ゲームターン当たりの行動エネルギー増減処理
 * @author Hourier
 * @date 2021/04/29
 */

#include "player-status/player-energy.h"
#include "system/player-type-definition.h"

PlayerEnergy::PlayerEnergy(player_type *creature_ptr)
{
    this->creature_ptr = creature_ptr;
}

/*
 * @brief プレーヤーの行動エネルギーを更新する
 * @param creature_ptr プレーヤーの参照ポインタ
 * @param need_cost 行動エネルギー
 * @param ut_type 現在値に対する演算方法
 * @return なし
 */
void PlayerEnergy::update_player_turn_energy(ENERGY need_cost, update_turn_type ut_type)
{
    switch (ut_type) {
    case update_turn_type::ENERGY_SUBSTITUTION:
        this->creature_ptr->energy_use = need_cost;
        return;
    case update_turn_type::ENERGY_ADDITION:
        this->creature_ptr->energy_use += need_cost;
        return;
    case update_turn_type::ENERGY_SUBTRACTION:
        this->creature_ptr->energy_use -= need_cost;
        return;
    case update_turn_type::ENERGY_MULTIPLICATION:
        this->creature_ptr->energy_use *= need_cost;
        return;
    case update_turn_type::ENERGY_DIVISION:
        this->creature_ptr->energy_use /= need_cost;
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
void PlayerEnergy::reset_player_turn()
{
    update_player_turn_energy(0, update_turn_type::ENERGY_SUBSTITUTION);
}
