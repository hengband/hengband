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
void PlayerEnergy::set_player_turn_energy(ENERGY need_cost)
{
    this->creature_ptr->energy_use = need_cost;
}

void PlayerEnergy::add_player_turn_energy(ENERGY need_cost)
{
    this->creature_ptr->energy_use += need_cost;
}

void PlayerEnergy::sub_player_turn_energy(ENERGY need_cost)
{
    this->creature_ptr->energy_use -= need_cost;
}

void PlayerEnergy::mul_player_turn_energy(ENERGY need_cost)
{
    this->creature_ptr->energy_use *= need_cost;
}

void PlayerEnergy::div_player_turn_energy(ENERGY need_cost)
{
    this->creature_ptr->energy_use /= need_cost;
}

/*
 * @brief ターン消費をなくす (主にコマンド実行に失敗した場合)
 * @param player_type プレーヤーへの参照ポインタ
 * @return なし
 */
void PlayerEnergy::reset_player_turn()
{
    set_player_turn_energy(0);
}
