#include "store/black-market.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "system/floor/town-info.h"
#include "system/floor/town-list.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"

/*!
 * @brief ブラックマーケット用の無価値品の排除判定
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param item 判定したいアイテムへの参照
 * @return ブラックマーケットにとって無価値な品ならばTRUEを返す
 */
bool black_market_crap(PlayerType *player_ptr, const ItemEntity &item)
{
    if (item.is_ego()) {
        return false;
    }

    if (item.to_a > 0) {
        return false;
    }

    if (item.to_h > 0) {
        return false;
    }

    if (item.to_d > 0) {
        return false;
    }

    for (auto sst : STORE_SALE_TYPE_LIST) {
        if (sst == StoreSaleType::HOME || sst == StoreSaleType::MUSEUM) {
            continue;
        }

        const auto &store = towns_info[player_ptr->town_num].stores[sst];
        for (auto j = 0; j < store.stock_num; j++) {
            if (item.bi_id == store.stock[j]->bi_id) {
                return true;
            }
        }
    }

    return false;
}
