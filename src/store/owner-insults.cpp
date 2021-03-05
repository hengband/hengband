#include "store/owner-insults.h"
#include "core/asking-player.h"
#include "game-option/birth-options.h"
#include "store/say-comments.h"
#include "store/store.h"
#include "store/store-util.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"
#include "world/world.h"

/* Last "increment" during haggling */
static s32b last_inc = 0L;

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

/*!
 * @brief 交渉価格を確認と認証の是非を行う /
 * Get a haggle
 * @param pmt メッセージ
 * @param poffer 別途価格提示をした場合の値を返す参照ポインタ
 * @param price 現在の交渉価格
 * @param final 最終確定価格ならばTRUE
 * @return プレイヤーを締め出す場合TRUEを返す
 */
static int get_haggle(concptr pmt, s32b *poffer, PRICE price, int final)
{
    GAME_TEXT buf[128];
    if (!allow_inc)
        last_inc = 0L;

    if (final) {
        sprintf(buf, _("%s [承諾] ", "%s [accept] "), pmt);
    } else if (last_inc < 0) {
        sprintf(buf, _("%s [-$%ld] ", "%s [-%ld] "), pmt, (long)(ABS(last_inc)));
    } else if (last_inc > 0) {
        sprintf(buf, _("%s [+$%ld] ", "%s [+%ld] "), pmt, (long)(ABS(last_inc)));
    } else {
        sprintf(buf, "%s ", pmt);
    }

    msg_print(NULL);
    GAME_TEXT out_val[160];
    while (TRUE) {
        bool res;
        prt(buf, 0, 0);
        strcpy(out_val, "");

        /*
         * Ask the user for a response.
         * Don't allow to use numpad as cursor key.
         */
        res = askfor_aux(out_val, 32, FALSE);
        prt("", 0, 0);
        if (!res)
            return FALSE;

        concptr p;
        for (p = out_val; *p == ' '; p++) /* loop */
            ;

        if (*p == '\0') {
            if (final) {
                *poffer = price;
                last_inc = 0L;
                break;
            }

            if (allow_inc && last_inc) {
                *poffer += last_inc;
                break;
            }

            msg_print(_("値がおかしいです。", "Invalid response."));
            msg_print(NULL);
            continue;
        }

        s32b i = atol(p);
        if ((*p == '+' || *p == '-')) {
            if (allow_inc) {
                *poffer += i;
                last_inc = i;
                break;
            }
        } else {
            *poffer = i;
            last_inc = 0L;
            break;
        }
    }

    return TRUE;
}

/*!
 * @brief 店主がプレイヤーからの交渉価格を判断する /
 * Receive an offer (from the player)
 * @param pmt メッセージ
 * @param poffer 店主からの交渉価格を返す参照ポインタ
 * @param last_offer 現在の交渉価格
 * @param factor 店主の価格基準倍率
 * @param price アイテムの実価値
 * @param final 最終価格確定ならばTRUE
 * @return プレイヤーの価格に対して不服ならばTRUEを返す /
 * Return TRUE if offer is NOT okay
 */
bool receive_offer(concptr pmt, s32b *poffer, s32b last_offer, int factor, PRICE price, int final)
{
    while (TRUE) {
        if (!get_haggle(pmt, poffer, price, final))
            return TRUE;

        if (((*poffer) * factor) >= (last_offer * factor))
            break;

        if (haggle_insults())
            return TRUE;

        *poffer = last_offer;
    }

    return FALSE;
}
