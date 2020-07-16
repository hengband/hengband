#include "store/owner-insults.h"
#include "game-option/birth-options.h"
#include "store/say-comments.h"
#include "store/store.h" // todo 相互参照している.
#include "store/store-util.h"
#include "world/world.h"

/*!
 * @brief 店主の不満度を増やし、プレイヤーを締め出す判定と処理を行う /
 * Increase the insult counter and get angry if too many -RAK-
 * @return プレイヤーを締め出す場合TRUEを返す
 */
int increase_insults(void)
{
    st_ptr->insult_cur++;
    if (st_ptr->insult_cur <= ot_ptr->insult_max)
        return FALSE;

    say_comment_4();
    st_ptr->insult_cur = 0;
    st_ptr->good_buy = 0;
    st_ptr->bad_buy = 0;
    st_ptr->store_open = current_world_ptr->game_turn + TURNS_PER_TICK * TOWN_DAWN / 8 + randint1(TURNS_PER_TICK * TOWN_DAWN / 8);
    return TRUE;
}

/*!
 * @brief 店主の不満度を減らす /
 * Decrease insults 				-RAK-
 * @return プレイヤーを締め出す場合TRUEを返す
 */
void decrease_insults(void)
{
    if (st_ptr->insult_cur)
        st_ptr->insult_cur--;
}

/*!
 * @brief 店主の不満度が増えた場合のみのメッセージを表示する /
 * Have insulted while haggling 			-RAK-
 * @return プレイヤーを締め出す場合TRUEを返す
 */
int haggle_insults(void)
{
    if (increase_insults())
        return TRUE;

    say_comment_5();
    return FALSE;
}

/*!
 * @brief 店主の持つプレイヤーに対する売買の良し悪し経験を記憶する /
 * Update the bargain info
 * @param price 実際の取引価格
 * @param minprice 店主の提示した価格
 * @param num 売買数
 * @return なし
 */
void updatebargain(PRICE price, PRICE minprice, int num)
{
    if (!manual_haggle)
        return;
    if ((minprice / num) < 10L)
        return;
    if (price == minprice) {
        if (st_ptr->good_buy < MAX_SHORT) {
            st_ptr->good_buy++;
        }
    } else {
        if (st_ptr->bad_buy < MAX_SHORT) {
            st_ptr->bad_buy++;
        }
    }
}
