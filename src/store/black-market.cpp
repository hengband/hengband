#include "store/black-market.h"
#include "floor/floor-town.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief ブラックマーケット用の無価値品の排除判定 /
 * This function will keep 'crap' out of the black market.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return ブラックマーケットにとって無価値な品ならばTRUEを返す
 * @details
 * <pre>
 * Crap is defined as any item that is "available" elsewhere
 * Based on a suggestion by "Lee Vogt" <lvogt@cig.mcel.mot.com>
 * </pre>
 */
bool black_market_crap(player_type *player_ptr, object_type *o_ptr)
{
    if (o_ptr->is_ego())
        return false;

    if (o_ptr->to_a > 0)
        return false;

    if (o_ptr->to_h > 0)
        return false;

    if (o_ptr->to_d > 0)
        return false;

    for (int i = 0; i < MAX_STORES; i++) {
        if (i == STORE_HOME)
            continue;
        if (i == STORE_MUSEUM)
            continue;

        for (int j = 0; j < town_info[player_ptr->town_num].store[i].stock_num; j++) {
            object_type *j_ptr = &town_info[player_ptr->town_num].store[i].stock[j];
            if (o_ptr->k_idx == j_ptr->k_idx)
                return true;
        }
    }

    return false;
}
