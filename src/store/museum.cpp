#include "store/museum.h"
#include "core/asking-player.h"
#include "flavor/flavor-describer.h"
#include "store/home.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/item-entity.h"
#include "view/display-messages.h"
#include "view/display-store.h"

/*!
 * @brief 博物館のアイテムを除去するコマンドのメインルーチン /
 * Remove an item from museum (Originally from TOband)
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void museum_remove_object(PlayerType *player_ptr)
{
    if (st_ptr->stock_num <= 0) {
        msg_print(_("博物館には何も置いてありません。", "The Museum is empty."));
        return;
    }

    int i = st_ptr->stock_num - store_top;
    if (i > store_bottom) {
        i = store_bottom;
    }

    constexpr auto mes = _("どのアイテムの展示をやめさせますか？", "Which item do you want to order to remove? ");
    auto item_num_opt = input_stock(mes, 0, i - 1, StoreSaleType::MUSEUM);
    if (!item_num_opt) {
        return;
    }

    const short item_num = *item_num_opt + store_top;
    auto *o_ptr = &st_ptr->stock[item_num];
    const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
    msg_print(_("展示をやめさせたアイテムは二度と見ることはできません！", "Once removed from the Museum, an item will be gone forever!"));
    if (!input_check(format(_("本当に%sの展示をやめさせますか？", "Really order to remove %s from the Museum? "), item_name.data()))) {
        return;
    }

    msg_format(_("%sの展示をやめさせた。", "You ordered to remove %s."), item_name.data());
    store_item_increase(item_num, -o_ptr->number);
    store_item_optimize(item_num);
    (void)combine_and_reorder_home(player_ptr, StoreSaleType::MUSEUM);
    if (st_ptr->stock_num == 0) {
        store_top = 0;
    } else if (store_top >= st_ptr->stock_num) {
        store_top -= store_bottom;
    }

    display_store_inventory(player_ptr, StoreSaleType::MUSEUM);
}
