#include "store/say-comments.h"
#include "avatar/avatar.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "store/rumor.h"
#include "store/store-owner-comments.h"
#include "store/store-util.h"
#include "view/display-messages.h"

#define RUMOR_CHANCE 8

/*!
 * @brief 取引成功時の店主のメッセージ処理 /
 * ブラックマーケットのときは別のメッセージを出す
 * Successful haggle.
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void store_owner_says_comment(PlayerType *player_ptr)
{
    if (cur_store_num == StoreSaleType::BLACK)
        msg_print(comment_1_B[randint0(MAX_COMMENT_1)]);
    else
        msg_print(comment_1[randint0(MAX_COMMENT_1)]);

    if (one_in_(RUMOR_CHANCE)) {
        msg_print(_("店主は耳うちした:", "The shopkeeper whispers something into your ear:"));
        display_rumor(player_ptr, true);
    }
}

/*!
 * @brief 店主が交渉を終えた際の反応を返す処理 /
 * Let a shop-keeper React to a purchase
 * @param price アイテムの取引額
 * @param value アイテムの実際価値
 * @param guess 店主が当初予想していた価値
 * @details
 * We paid "price", it was worth "value", and we thought it was worth "guess"
 */
void purchase_analyze(PlayerType *player_ptr, PRICE price, PRICE value, PRICE guess)
{
    /* Item was worthless, but we bought it */
    if ((value <= 0) && (price > value)) {
        msg_print(comment_7a[randint0(MAX_COMMENT_7A)]);
        chg_virtue(player_ptr, V_HONOUR, -1);
        chg_virtue(player_ptr, V_JUSTICE, -1);
        sound(SOUND_STORE1);
        return;
    }

    /* Item was cheaper than we thought, and we paid more than necessary */
    if ((value < guess) && (price > value)) {
        msg_print(comment_7b[randint0(MAX_COMMENT_7B)]);
        chg_virtue(player_ptr, V_JUSTICE, -1);
        if (one_in_(4))
            chg_virtue(player_ptr, V_HONOUR, -1);
        sound(SOUND_STORE2);
        return;
    }

    /* Item was a good bargain, and we got away with it */
    if ((value > guess) && (value < (4 * guess)) && (price < value)) {
        msg_print(comment_7c[randint0(MAX_COMMENT_7C)]);
        if (one_in_(4))
            chg_virtue(player_ptr, V_HONOUR, -1);
        else if (one_in_(4))
            chg_virtue(player_ptr, V_HONOUR, 1);
        sound(SOUND_STORE3);
        return;
    }

    /* Item was a great bargain, and we got away with it */
    if ((value > guess) && (price < value)) {
        msg_print(comment_7d[randint0(MAX_COMMENT_7D)]);
        if (one_in_(2))
            chg_virtue(player_ptr, V_HONOUR, -1);
        if (one_in_(4))
            chg_virtue(player_ptr, V_HONOUR, 1);
        if (10 * price < value)
            chg_virtue(player_ptr, V_SACRIFICE, 1);
        sound(SOUND_STORE4);
        return;
    }
}
