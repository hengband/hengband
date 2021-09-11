#include "birth/birth-body-spec.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "player/player-sex.h"
#include "system/player-type-definition.h"

/*!
 * @brief プレイヤーの身長体重を決める / Get character's height and weight
 */
void get_height_weight(player_type *player_ptr)
{
    int deviation;
    switch (player_ptr->psex) {
    case SEX_MALE:
        player_ptr->ht = randnor(rp_ptr->m_b_ht, rp_ptr->m_m_ht);
        deviation = (int)(player_ptr->ht) * 100 / (int)(rp_ptr->m_b_ht);
        player_ptr->wt = randnor((int)(rp_ptr->m_b_wt) * deviation / 100, (int)(rp_ptr->m_m_wt) * deviation / 300);
        return;
    case SEX_FEMALE:
        player_ptr->ht = randnor(rp_ptr->f_b_ht, rp_ptr->f_m_ht);
        deviation = (int)(player_ptr->ht) * 100 / (int)(rp_ptr->f_b_ht);
        player_ptr->wt = randnor((int)(rp_ptr->f_b_wt) * deviation / 100, (int)(rp_ptr->f_m_wt) * deviation / 300);
    default:
        return;
    }
}

/*!
 * @brief プレイヤーの年齢を決める。 / Computes character's age, height, and weight by henkma
 * @details 内部でget_height_weight()も呼び出している。
 */
void get_ahw(player_type *player_ptr)
{
    player_ptr->age = rp_ptr->b_age + randint1(rp_ptr->m_age);
    get_height_weight(player_ptr);
}

/*!
 * @brief プレイヤーの初期所持金を決める。 / Get the player's starting money
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void get_money(player_type *player_ptr)
{
    int gold = (player_ptr->sc * 6) + randint1(100) + 300;
    if (player_ptr->pclass == CLASS_TOURIST)
        gold += 2000;

    for (int i = 0; i < A_MAX; i++) {
        if (player_ptr->stat_max[i] >= 18 + 50)
            gold -= 300;
        else if (player_ptr->stat_max[i] >= 18 + 20)
            gold -= 200;
        else if (player_ptr->stat_max[i] > 18)
            gold -= 150;
        else
            gold -= (player_ptr->stat_max[i] - 8) * 10;
    }

    const int minimum_deposit = 100;
    if (gold < minimum_deposit)
        gold = minimum_deposit;

    if (player_ptr->pseikaku == PERSONALITY_LAZY)
        gold /= 2;
    else if (player_ptr->pseikaku == PERSONALITY_MUNCHKIN)
        gold = 10000000;
    if (player_ptr->prace == player_race_type::ANDROID)
        gold /= 5;

    player_ptr->au = gold;
}
