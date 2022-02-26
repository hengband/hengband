#include "store/museum.h"
#include "core/asking-player.h"
#include "flavor/flavor-describer.h"
#include "store/home.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/object-type-definition.h"
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

    char out_val[160];
    sprintf(out_val, _("どのアイテムの展示をやめさせますか？", "Which item do you want to order to remove? "));

    COMMAND_CODE item;
    if (!get_stock(&item, out_val, 0, i - 1)) {
        return;
    }

    item = item + store_top;
    ObjectType *o_ptr;
    o_ptr = &st_ptr->stock[item];

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, 0);
    msg_print(_("展示をやめさせたアイテムは二度と見ることはできません！", "Once removed from the Museum, an item will be gone forever!"));
    if (!get_check(format(_("本当に%sの展示をやめさせますか？", "Really order to remove %s from the Museum? "), o_name))) {
        return;
    }

    msg_format(_("%sの展示をやめさせた。", "You ordered to remove %s."), o_name);
    store_item_increase(item, -o_ptr->number);
    store_item_optimize(item);
    (void)combine_and_reorder_home(player_ptr, StoreSaleType::MUSEUM);
    if (st_ptr->stock_num == 0) {
        store_top = 0;
    } else if (store_top >= st_ptr->stock_num) {
        store_top -= store_bottom;
    }

    display_store_inventory(player_ptr);
}
