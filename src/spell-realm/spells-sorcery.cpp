#include "spell-realm/spells-sorcery.h"
#include "core/asking-player.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-object.h"
#include "game-option/input-options.h"
#include "inventory/inventory-object.h"
#include "io/input-key-requester.h"
#include "object-hook/hook-expendable.h"
#include "object/item-use-flags.h"
#include "object/object-value.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/z-form.h"
#include "view/display-messages.h"

/*!
 * @brief アイテムの価値に応じた錬金術処理 /
 * Turns an object into gold, gain some of its value in a shop
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 処理が実際に行われたらTRUEを返す
 */
bool alchemy(PlayerType *player_ptr)
{
    bool force = false;
    if (command_arg > 0) {
        force = true;
    }

    constexpr auto q = _("どのアイテムを金に変えますか？", "Turn which item to gold? ");
    constexpr auto s = _("金に変えられる物がありません。", "You have nothing to turn to gold.");
    short i_idx;
    auto *o_ptr = choose_object(player_ptr, &i_idx, q, s, (USE_INVEN | USE_FLOOR));
    if (!o_ptr) {
        return false;
    }

    auto amt = 1;
    if (o_ptr->number > 1) {
        amt = input_quantity(o_ptr->number);
        if (amt <= 0) {
            return false;
        }
    }

    const auto old_number = o_ptr->number;
    o_ptr->number = amt;
    const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
    o_ptr->number = old_number;

    if (!force) {
        if (confirm_destroy || (o_ptr->get_price() > 0)) {
            char out_val[MAX_NLEN + 40];
            strnfmt(out_val, sizeof(out_val), _("本当に%sを金に変えますか？", "Really turn %s to gold? "), item_name.data());
            if (!input_check(out_val)) {
                return false;
            }
        }
    }

    if (!can_player_destroy_object(o_ptr)) {
        msg_format(_("%sを金に変えることに失敗した。", "You fail to turn %s to gold!"), item_name.data());
        return false;
    }

    auto price = object_value_real(o_ptr);
    if (price <= 0) {
        msg_format(_("%sをニセの金に変えた。", "You turn %s to fool's gold."), item_name.data());
        vary_item(player_ptr, i_idx, -amt);
        return true;
    }

    price /= 3;

    if (amt > 1) {
        price *= amt;
    }

    if (price > 30000) {
        price = 30000;
    }

    msg_format(_("%sを＄%d の金に変えた。", "You turn %s to %d coins worth of gold."), item_name.data(), price);
    player_ptr->au += price;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::GOLD);
    rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
    vary_item(player_ptr, i_idx, -amt);
    return true;
}
