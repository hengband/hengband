#include "store/black-market.h"
#include "floor/floor-town.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"

/*!
 * @brief ブラックマーケット用の無価値品の排除判定 /
 * This function will keep 'crap' out of the black market.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return ブラックマーケットにとって無価値な品ならばTRUEを返す
 * @details
 * <pre>
 * Crap is defined as any item that is "available" elsewhere
 * Based on a suggestion by "Lee Vogt" <lvogt@cig.mcel.mot.com>
 * </pre>
 */
bool black_market_crap(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    if (o_ptr->is_ego()) {
        return false;
    }

    if (o_ptr->to_a > 0) {
        return false;
    }

    if (o_ptr->to_h > 0) {
        return false;
    }

    if (o_ptr->to_d > 0) {
        return false;
    }

    for (auto sst : STORE_SALE_TYPE_LIST) {
        if (sst == StoreSaleType::HOME || sst == StoreSaleType::MUSEUM) {
            continue;
        }

        const auto &store = towns_info[player_ptr->town_num].stores[sst];
        for (auto j = 0; j < store.stock_num; j++) {
            if (o_ptr->bi_id == store.stock[j].bi_id) {
                return true;
            }
        }
    }

    return false;
}
