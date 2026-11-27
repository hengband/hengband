/*!
 * @file ac-damage-reduction.h
 * @brief AC による打撃のダメージの軽減
 */

#pragma once

#include <algorithm>

/*!
 * @brief AC による軽減を引いた打撃のダメージを返す
 * @param damage 軽減する前のダメージ
 * @param ac 打撃を受ける側の AC
 * @return 軽減した後のダメージ
 * @details ダメージの AC / 250 を軽減する。AC は 150 で頭打ちになり、最大で 6 割を軽減する。
 * AC が負ならダメージは増える。
 */
constexpr int reduce_damage_by_ac(int damage, int ac)
{
    return damage - damage * std::min(ac, 150) / 250;
}
