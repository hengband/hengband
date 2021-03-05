﻿#include "birth/birth-body-spec.h"
#include "player/player-personalities-types.h"
#include "player/player-race-types.h"
#include "player/player-race.h"

/*!
 * @brief プレイヤーの身長体重を決める / Get character's height and weight
 * @return なし
 */
void get_height_weight(player_type *creature_ptr)
{
    int deviation;
    switch (creature_ptr->psex) {
    case SEX_MALE:
        creature_ptr->ht = randnor(rp_ptr->m_b_ht, rp_ptr->m_m_ht);
        deviation = (int)(creature_ptr->ht) * 100 / (int)(rp_ptr->m_b_ht);
        creature_ptr->wt = randnor((int)(rp_ptr->m_b_wt) * deviation / 100, (int)(rp_ptr->m_m_wt) * deviation / 300);
        return;
    case SEX_FEMALE:
        creature_ptr->ht = randnor(rp_ptr->f_b_ht, rp_ptr->f_m_ht);
        deviation = (int)(creature_ptr->ht) * 100 / (int)(rp_ptr->f_b_ht);
        creature_ptr->wt = randnor((int)(rp_ptr->f_b_wt) * deviation / 100, (int)(rp_ptr->f_m_wt) * deviation / 300);
    default:
        return;
    }
}

/*!
 * @brief プレイヤーの年齢を決める。 / Computes character's age, height, and weight by henkma
 * @details 内部でget_height_weight()も呼び出している。
 * @return なし
 */
void get_ahw(player_type *creature_ptr)
{
    creature_ptr->age = rp_ptr->b_age + randint1(rp_ptr->m_age);
    get_height_weight(creature_ptr);
}

/*!
 * @brief プレイヤーの初期所持金を決める。 / Get the player's starting money
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void get_money(player_type *creature_ptr)
{
    int gold = (creature_ptr->sc * 6) + randint1(100) + 300;
    if (creature_ptr->pclass == CLASS_TOURIST)
        gold += 2000;

    for (int i = 0; i < A_MAX; i++) {
        if (creature_ptr->stat_max[i] >= 18 + 50)
            gold -= 300;
        else if (creature_ptr->stat_max[i] >= 18 + 20)
            gold -= 200;
        else if (creature_ptr->stat_max[i] > 18)
            gold -= 150;
        else
            gold -= (creature_ptr->stat_max[i] - 8) * 10;
    }

    const int minimum_deposit = 100;
    if (gold < minimum_deposit)
        gold = minimum_deposit;

    if (creature_ptr->pseikaku == PERSONALITY_LAZY)
        gold /= 2;
    else if (creature_ptr->pseikaku == PERSONALITY_MUNCHKIN)
        gold = 10000000;
    if (creature_ptr->prace == RACE_ANDROID)
        gold /= 5;

    creature_ptr->au = gold;
}
