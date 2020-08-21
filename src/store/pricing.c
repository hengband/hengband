#include "store/pricing.h"
#include "market/gold-magnification-table.h"
#include "object/object-value.h"
#include "player/player-status-table.h"
#include "store/store-util.h"
#include "store/store.h"

/*!
 * @brief 店舗価格を決定する. 無料にはならない /
 * Determine the price of an item (qty one) in a store.
 * @param o_ptr 店舗に並べるオブジェクト構造体の参照ポインタ
 * @param greed 店主の強欲度
 * @param flip TRUEならば店主にとっての買取価格、FALSEなら売出価格を計算
 * @return アイテムの店舗価格
 * @details
 * <pre>
 * This function takes into account the player's charisma, and the
 * shop-keepers friendliness, and the shop-keeper's base greed, but
 * never lets a shop-keeper lose money in a transaction.
 * The "greed" value should exceed 100 when the player is "buying" the
 * item, and should be less than 100 when the player is "selling" it.
 * Hack -- the black market always charges twice as much as it should.
 * Charisma adjustment runs from 80 to 130
 * Racial adjustment runs from 95 to 130
 * Since greed/charisma/racial adjustments are centered at 100, we need
 * to adjust (by 200) to extract a usable multiplier.  Note that the
 * "greed" value is always something (?).
 * </pre>
 */
PRICE price_item(player_type *player_ptr, object_type *o_ptr, int greed, bool flip)
{
    PRICE price = object_value(player_ptr, o_ptr);
    if (price <= 0)
        return 0L;

    int factor = rgold_adj[ot_ptr->owner_race][player_ptr->prace];
    factor += adj_chr_gold[player_ptr->stat_ind[A_CHR]];
    int adjust;
    if (flip) {
        adjust = 100 + (300 - (greed + factor));
        if (adjust > 100)
            adjust = 100;

        if (cur_store_num == STORE_BLACK)
            price = price / 2;

        price = (price * adjust + 50L) / 100L;
    } else {
        adjust = 100 + ((greed + factor) - 300);
        if (adjust < 100)
            adjust = 100;

        if (cur_store_num == STORE_BLACK)
            price = price * 2;

        price = (s32b)(((u32b)price * (u32b)adjust + 50UL) / 100UL);
    }

    if (price <= 0L)
        return 1L;

    return price;
}

/*!
 * @brief 店舗の割引対象外にするかどうかを判定 /
 * Eliminate need to bargain if player has haggled well in the past
 * @param minprice アイテムの最低販売価格
 * @return 割引を禁止するならTRUEを返す。
 */
bool noneedtobargain(PRICE minprice)
{
    PRICE good = st_ptr->good_buy;
    PRICE bad = st_ptr->bad_buy;
    return (minprice < 10L) || (good == MAX_SHORT) || (good > ((3 * bad) + (5 + (minprice / 50))));
}
