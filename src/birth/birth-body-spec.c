#include "system/angband.h"
#include "birth/birth-body-spec.h"
#include "player/player-personality.h"

/*!
 * @brief プレイヤーの身長体重を決める / Get character's height and weight
 * @return なし
 */
void get_height_weight(player_type *creature_ptr)
{
    int h_percent; /* 身長が平均にくらべてどのくらい違うか. */

    /* Calculate the height/weight for males */
    if (creature_ptr->psex == SEX_MALE) {
        creature_ptr->ht = randnor(rp_ptr->m_b_ht, rp_ptr->m_m_ht);
        h_percent = (int)(creature_ptr->ht) * 100 / (int)(rp_ptr->m_b_ht);
        creature_ptr->wt = randnor((int)(rp_ptr->m_b_wt) * h_percent / 100, (int)(rp_ptr->m_m_wt) * h_percent / 300);
    }

    /* Calculate the height/weight for females */
    else if (creature_ptr->psex == SEX_FEMALE) {
        creature_ptr->ht = randnor(rp_ptr->f_b_ht, rp_ptr->f_m_ht);
        h_percent = (int)(creature_ptr->ht) * 100 / (int)(rp_ptr->f_b_ht);
        creature_ptr->wt = randnor((int)(rp_ptr->f_b_wt) * h_percent / 100, (int)(rp_ptr->f_m_wt) * h_percent / 300);
    }
}

/*!
 * @brief プレイヤーの年齢を決める。 / Computes character's age, height, and weight by henkma
 * @details 内部でget_height_weight()も呼び出している。
 * @return なし
 */
void get_ahw(player_type *creature_ptr)
{
    /* Get character's age */
    creature_ptr->age = rp_ptr->b_age + randint1(rp_ptr->m_age);

    /* Get character's height and weight */
    get_height_weight(creature_ptr);
}

/*!
 * @brief プレイヤーの初期所持金を決める。 / Get the player's starting money
 * @return なし
 */
void get_money(player_type *creature_ptr)
{
    int i, gold;

    /* Social Class determines starting gold */
    gold = (creature_ptr->sc * 6) + randint1(100) + 300;
    if (creature_ptr->pclass == CLASS_TOURIST)
        gold += 2000;

    /* Process the stats */
    for (i = 0; i < A_MAX; i++) {
        /* Mega-Hack -- reduce gold for high stats */
        if (creature_ptr->stat_max[i] >= 18 + 50)
            gold -= 300;
        else if (creature_ptr->stat_max[i] >= 18 + 20)
            gold -= 200;
        else if (creature_ptr->stat_max[i] > 18)
            gold -= 150;
        else
            gold -= (creature_ptr->stat_max[i] - 8) * 10;
    }

    /* Minimum 100 gold */
    if (gold < 100)
        gold = 100;

    if (creature_ptr->pseikaku == SEIKAKU_NAMAKE)
        gold /= 2;
    else if (creature_ptr->pseikaku == SEIKAKU_MUNCHKIN)
        gold = 10000000;
    if (creature_ptr->prace == RACE_ANDROID)
        gold /= 5;

    /* Save the gold */
    creature_ptr->au = gold;
}
