/*
 * @file player-energy.cpp
 * @brief ゲームターン当たりの行動エネルギー増減処理
 * @author Hourier
 * @date 2021/04/29
 */

#include "player-status/player-energy.h"
#include "system/player-type-definition.h"

PlayerEnergy::PlayerEnergy(PlayerType *player_ptr)
{
    this->player_ptr = player_ptr;
}

/*
 * @brief プレイヤーの行動エネルギーを更新する
 * @param player_ptr プレイヤーの参照ポインタ
 * @param need_cost 行動エネルギー
 * @param ut_type 現在値に対する演算方法
 */
void PlayerEnergy::set_player_turn_energy(ENERGY need_cost)
{
    this->player_ptr->energy_use = need_cost;
}

void PlayerEnergy::add_player_turn_energy(ENERGY need_cost)
{
    this->player_ptr->energy_use += need_cost;
}

void PlayerEnergy::sub_player_turn_energy(ENERGY need_cost)
{
    this->player_ptr->energy_use -= need_cost;
}

void PlayerEnergy::mul_player_turn_energy(ENERGY need_cost)
{
    this->player_ptr->energy_use *= need_cost;
}

void PlayerEnergy::div_player_turn_energy(ENERGY need_cost)
{
    this->player_ptr->energy_use /= need_cost;
}

/*
 * @brief ターン消費をなくす (主にコマンド実行に失敗した場合)
 * @param PlayerType プレイヤーへの参照ポインタ
 */
void PlayerEnergy::reset_player_turn()
{
    set_player_turn_energy(0);
}
