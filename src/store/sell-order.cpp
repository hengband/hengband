#include "store/sell-order.h"
#include "action/weapon-shield.h"
#include "autopick/autopick.h"
#include "avatar/avatar.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "racial/racial-android.h"
#include "spell-kind/spells-perception.h"
#include "store/home.h"
#include "store/pricing.h"
#include "store/say-comments.h"
#include "store/service-checker.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "view/display-store.h"
#include "view/object-describer.h"
#include "world/world.h"
#include <optional>

/*!
 * @brief プレイヤーが売却する時の確認プロンプト / Prompt to sell for the price
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr オブジェクトの構造体参照ポインタ
 * @return 売るなら(true,売値)、売らないなら(false,0)のタプル
 */
static std::optional<PRICE> prompt_to_sell(PlayerType *player_ptr, ItemEntity *o_ptr, StoreSaleType store_num)
{
    auto price_ask = price_item(player_ptr, o_ptr, ot_ptr->inflate, true, store_num);

    price_ask = std::min(price_ask, ot_ptr->max_cost);
    price_ask *= o_ptr->number;
    const auto s = format(_("売値 $%ld で売りますか？", "Do you sell for $%ld? "), static_cast<long>(price_ask));
    if (input_check_strict(player_ptr, s, UserCheck::DEFAULT_Y)) {
        return price_ask;
    }

    return std::nullopt;
}

/*!
 * @brief 店からの売却処理のメインルーチン /
 * Sell an item to the store (or home)
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void store_sell(PlayerType *player_ptr, StoreSaleType store_num)
{
    concptr q; //!< @note プロンプトメッセージ
    concptr s_none; //!< @note 売る/置くものがない場合のメッセージ
    concptr s_full; //!< @note もう置けない場合のメッセージ
    switch (store_num) {
    case StoreSaleType::HOME:
        q = _("どのアイテムを置きますか? ", "Drop which item? ");
        s_none = _("置けるアイテムを持っていません。", "You don't have any items to drop.");
        s_full = _("我が家にはもう置く場所がない。", "Your home is full.");
        break;
    case StoreSaleType::MUSEUM:
        q = _("どのアイテムを寄贈しますか? ", "Give which item? ");
        s_none = _("寄贈できるアイテムを持っていません。", "You don't have any items to give.");
        s_full = _("博物館はもう満杯だ。", "The Museum is full.");
        break;
    default:
        q = _("どのアイテムを売りますか? ", "Sell which item? ");
        s_none = _("欲しい物がないですねえ。", "You have nothing that I want.");
        s_full = _("すいませんが、店にはもう置く場所がありません。", "I have not the room in my store to keep it.");
        break;
    }

    short item_index;
    const auto options = USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT;
    auto *o_ptr = choose_object(player_ptr, &item_index, q, s_none, options, FuncItemTester(store_will_buy, player_ptr, store_num));
    if (o_ptr == nullptr) {
        return;
    }

    if ((item_index >= INVEN_MAIN_HAND) && o_ptr->is_cursed()) {
        msg_print(_("ふーむ、どうやらそれは呪われているようだね。", "Hmmm, it seems to be cursed."));
        return;
    }

    auto amt = 1;
    if (o_ptr->number > 1) {
        amt = input_quantity(o_ptr->number);
        if (amt <= 0) {
            return;
        }
    }

    ItemEntity forge;
    auto *q_ptr = &forge;
    q_ptr->copy_from(o_ptr);
    q_ptr->number = amt;

    if (o_ptr->is_wand_rod()) {
        q_ptr->pval = o_ptr->pval * amt / o_ptr->number;
    }

    if ((store_num != StoreSaleType::HOME) && (store_num != StoreSaleType::MUSEUM)) {
        q_ptr->inscription.reset();
        q_ptr->feeling = FEEL_NONE;
    }

    if (!store_check_num(q_ptr, store_num)) {
        msg_print(s_full);
        return;
    }

    bool placed = false;
    if ((store_num != StoreSaleType::HOME) && (store_num != StoreSaleType::MUSEUM)) {
        const auto item_name = describe_flavor(player_ptr, q_ptr, 0);
        msg_format(_("%s(%c)を売却する。", "Selling %s (%c)."), item_name.data(), index_to_label(item_index));
        msg_print(nullptr);

        auto res = prompt_to_sell(player_ptr, q_ptr, store_num);
        placed = res.has_value();
        if (placed) {
            const auto price = res.value();
            store_owner_says_comment(player_ptr, store_num);

            sound(SOUND_SELL);
            if (store_num == StoreSaleType::BLACK) {
                chg_virtue(player_ptr, Virtue::JUSTICE, -1);
            }

            const auto tval = o_ptr->bi_key.tval();
            if ((tval == ItemKindType::BOTTLE) && (store_num != StoreSaleType::HOME)) {
                chg_virtue(player_ptr, Virtue::NATURE, 1);
            }

            player_ptr->au += price;
            store_prt_gold(player_ptr);
            const auto dummy = q_ptr->get_price() * q_ptr->number;

            identify_item(player_ptr, o_ptr);
            q_ptr = &forge;
            q_ptr->copy_from(o_ptr);
            q_ptr->number = amt;
            q_ptr->ident |= IDENT_STORE;

            if (o_ptr->is_wand_rod()) {
                q_ptr->pval = o_ptr->pval * amt / o_ptr->number;
            }

            const auto value = q_ptr->get_price() * q_ptr->number;
            const auto sold_item_name = describe_flavor(player_ptr, q_ptr, 0);
            msg_format(_("%sを $%dで売却しました。", "You sold %s for %d gold."), sold_item_name.data(), price);

            if (record_sell) {
                exe_write_diary(player_ptr, DiaryKind::SELL, 0, sold_item_name);
            }

            if (!((tval == ItemKindType::FIGURINE) && (value > 0))) {
                purchase_analyze(player_ptr, price, value, dummy);
            }

            distribute_charges(o_ptr, q_ptr, amt);
            q_ptr->timeout = 0;
            inven_item_increase(player_ptr, item_index, -amt);
            inven_item_describe(player_ptr, item_index);
            if (o_ptr->number > 0) {
                autopick_alter_item(player_ptr, item_index, false);
            }

            inven_item_optimize(player_ptr, item_index);
            int item_pos = store_carry(q_ptr);
            if (item_pos >= 0) {
                store_top = (item_pos / store_bottom) * store_bottom;
                display_store_inventory(player_ptr, store_num);
            }
        }
    } else if (store_num == StoreSaleType::MUSEUM) {
        const auto museum_item_name = describe_flavor(player_ptr, q_ptr, OD_NAME_ONLY);
        if (-1 == store_check_num(q_ptr, store_num)) {
            msg_print(_("それと同じ品物は既に博物館にあるようです。", "The Museum already has one of those items."));
        } else {
            msg_print(_("博物館に寄贈したものは取り出すことができません！！", "You cannot take back items which have been donated to the Museum!!"));
        }

        if (!input_check(format(_("本当に%sを寄贈しますか？", "Really give %s to the Museum? "), museum_item_name.data()))) {
            return;
        }

        identify_item(player_ptr, q_ptr);
        q_ptr->ident |= IDENT_FULL_KNOWN;

        distribute_charges(o_ptr, q_ptr, amt);
        msg_format(_("%sを置いた。(%c)", "You drop %s (%c)."), museum_item_name.data(), index_to_label(item_index));
        placed = true;

        vary_item(player_ptr, item_index, -amt);

        int item_pos = home_carry(player_ptr, q_ptr, store_num);
        if (item_pos >= 0) {
            store_top = (item_pos / store_bottom) * store_bottom;
            display_store_inventory(player_ptr, store_num);
        }
    } else {
        distribute_charges(o_ptr, q_ptr, amt);
        const auto item_name = describe_flavor(player_ptr, q_ptr, 0);
        msg_format(_("%sを置いた。(%c)", "You drop %s (%c)."), item_name.data(), index_to_label(item_index));
        placed = true;
        vary_item(player_ptr, item_index, -amt);
        int item_pos = home_carry(player_ptr, q_ptr, store_num);
        if (item_pos >= 0) {
            store_top = (item_pos / store_bottom) * store_bottom;
            display_store_inventory(player_ptr, store_num);
        }
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
    handle_stuff(player_ptr);

    if (placed && (item_index >= INVEN_MAIN_HAND)) {
        calc_android_exp(player_ptr);
        verify_equip_slot(player_ptr, item_index);
    }
}
