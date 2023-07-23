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
void store_owner_says_comment(PlayerType *player_ptr, StoreSaleType store_num)
{
    if (store_num == StoreSaleType::BLACK) {
        msg_print(rand_choice(comment_1_B));
    } else {
        msg_print(rand_choice(comment_1));
    }

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
        msg_print(rand_choice(comment_7a));
        chg_virtue(player_ptr, Virtue::HONOUR, -1);
        chg_virtue(player_ptr, Virtue::JUSTICE, -1);
        sound(SOUND_STORE1);
        return;
    }

    /* Item was cheaper than we thought, and we paid more than necessary */
    if ((value < guess) && (price > value)) {
        msg_print(rand_choice(comment_7b));
        chg_virtue(player_ptr, Virtue::JUSTICE, -1);
        if (one_in_(4)) {
            chg_virtue(player_ptr, Virtue::HONOUR, -1);
        }
        sound(SOUND_STORE2);
        return;
    }

    /* Item was a good bargain, and we got away with it */
    if ((value > guess) && (value < (4 * guess)) && (price < value)) {
        msg_print(rand_choice(comment_7c));
        if (one_in_(4)) {
            chg_virtue(player_ptr, Virtue::HONOUR, -1);
        } else if (one_in_(4)) {
            chg_virtue(player_ptr, Virtue::HONOUR, 1);
        }
        sound(SOUND_STORE3);
        return;
    }

    /* Item was a great bargain, and we got away with it */
    if ((value > guess) && (price < value)) {
        msg_print(rand_choice(comment_7d));
        if (one_in_(2)) {
            chg_virtue(player_ptr, Virtue::HONOUR, -1);
        }
        if (one_in_(4)) {
            chg_virtue(player_ptr, Virtue::HONOUR, 1);
        }
        if (10 * price < value) {
            chg_virtue(player_ptr, Virtue::SACRIFICE, 1);
        }
        sound(SOUND_STORE4);
        return;
    }
}
