#include "store/purchase-order.h"
#include "autopick/autopick-finder.h"
#include "autopick/autopick-util.h"
#include "avatar/avatar.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "game-option/play-record-options.h"
#include "inventory/inventory-object.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-info.h"
#include "object/object-stack.h"
#include "perception/object-perception.h"
#include "player/race-info-table.h"
#include "store/home.h"
#include "store/pricing.h"
#include "store/say-comments.h"
#include "store/store-owners.h"
#include "store/store.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "view/display-store.h"
#include "world/world.h"
#include <fmt/format.h>
#include <string>
#include <tl/optional.hpp>

/*!
 * @brief プレイヤーが購入する時の値切り処理メインルーチン /
 * Haggling routine 				-RAK-
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr オブジェクトの構造体参照ポインタ
 * @param price 最終価格を返す参照ポインタ
 * @return プレイヤーの価格に対して店主が不服ならばTRUEを返す /
 * Return TRUE if purchase is NOT successful
 */
static tl::optional<PRICE> prompt_to_buy(PlayerType *player_ptr, ItemEntity *o_ptr, StoreSaleType store_num)
{
    auto price_ask = price_item(player_ptr, o_ptr->calc_price(), ot_ptr->inflate, false, store_num);

    price_ask *= o_ptr->number;
    const auto s = fmt::format(_("買値 ${} で買いますか？", "Do you buy for ${}? "), price_ask);
    if (input_check_strict(player_ptr, s, UserCheck::DEFAULT_Y)) {
        return price_ask;
    }

    return tl::nullopt;
}

/*!
 * @brief 店舗から購入する際のアイテム選択プロンプト
 * @param i 店舗インベントリストック数
 * @return 選択したらtrue、しなかったらfalse
 */
static tl::optional<short> show_store_select_item(const int i, StoreSaleType store_num)
{
    std::string prompt;
    switch (store_num) {
    case StoreSaleType::HOME:
        prompt = _("どのアイテムを取りますか? ", "Which item do you want to take? ");
        break;
    case StoreSaleType::BLACK:
        prompt = _("どれ? ", "Which item, huh? ");
        break;
    default:
        prompt = _("どの品物が欲しいんだい? ", "Which item are you interested in? ");
        break;
    }

    return input_stock(prompt, 0, i - 1, store_num);
}

/*!
 * @brief 家のアイテムを取得する
 * @param player_ptr プレイヤー情報の参照ポインタ
 * @param item_home 取得元オブジェクト
 * @param item_inventory 取得先オブジェクト(指定数量分)
 * @param i_idx 取得先インベントリ番号
 */
static void take_item_from_home(PlayerType *player_ptr, ItemEntity &item_home, ItemEntity &item_inventory, short i_idx)
{
    const auto amt = item_inventory.number;
    distribute_charges(&item_home, &item_inventory, amt);

    const auto item_new = store_item_to_inventory(player_ptr, &item_inventory);
    const auto item_name = describe_flavor(player_ptr, *player_ptr->inventory[item_new], 0);
    handle_stuff(player_ptr);
    msg_format(_("%s(%c)を取った。", "You have %s (%c)."), item_name.data(), index_to_label(item_new));

    const auto stock_num = st_ptr->stock_num;
    st_ptr->increase_item(i_idx, -amt);
    st_ptr->optimize_item(i_idx);

    const auto combined_or_reordered = combine_and_reorder_home(player_ptr, StoreSaleType::HOME);
    if (stock_num == st_ptr->stock_num) {
        if (combined_or_reordered) {
            display_store_inventory(player_ptr, StoreSaleType::HOME);
            return;
        }

        display_entry(player_ptr, i_idx, StoreSaleType::HOME);
        return;
    }

    if (st_ptr->stock_num == 0) {
        store_top = 0;
    } else if (store_top >= st_ptr->stock_num) {
        store_top -= store_bottom;
    }

    display_store_inventory(player_ptr, StoreSaleType::HOME);
    chg_virtue(player_ptr, Virtue::SACRIFICE, 1);
}

static void shuffle_store(PlayerType *player_ptr, StoreSaleType store_num)
{
    if (!one_in_(STORE_SHUFFLE)) {
        msg_print(_("店主は新たな在庫を取り出した。", "The shopkeeper brings out some new stock."));
        return;
    }

    msg_print(_("店主は引退した。", "The shopkeeper retires."));
    store_shuffle(player_ptr, store_num);
    prt("", 3, 0);
    put_str(format("%s (%s)", ot_ptr->owner_name, race_info[enum2i(ot_ptr->owner_race)].title.data()), 3, 10);
    const auto &terrains = TerrainList::get_instance();
    prt(format("%s (%d)", terrains.get_terrain(cur_store_feat).name.data(), ot_ptr->max_cost), 3, 50);
}

static void switch_store_stock(PlayerType *player_ptr, const int i, const COMMAND_CODE item, StoreSaleType store_num)
{
    if (st_ptr->stock_num == 0) {
        shuffle_store(player_ptr, store_num);
        store_maintenance(player_ptr, player_ptr->town_num, store_num, 10);

        store_top = 0;
        display_store_inventory(player_ptr, store_num);
        return;
    }

    if (st_ptr->stock_num != i) {
        if (store_top >= st_ptr->stock_num) {
            store_top -= store_bottom;
        }

        display_store_inventory(player_ptr, store_num);
        return;
    }

    display_entry(player_ptr, item, store_num);
}

/*!
 * @brief 店からの購入処理のメインルーチン /
 * Buy an item from a store 			-RAK-
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void store_purchase(PlayerType *player_ptr, StoreSaleType store_num)
{
    if (store_num == StoreSaleType::MUSEUM) {
        msg_print(_("博物館から取り出すことはできません。", "Items cannot be taken out of the Museum."));
        return;
    }

    if (st_ptr->stock_num <= 0) {
        if (store_num == StoreSaleType::HOME) {
            msg_print(_("我が家には何も置いてありません。", "Your home is empty."));
        } else {
            msg_print(_("現在商品の在庫を切らしています。", "I am currently out of stock."));
        }
        return;
    }

    int i = (st_ptr->stock_num - store_top);
    if (i > store_bottom) {
        i = store_bottom;
    }

    auto item_num_opt = show_store_select_item(i, store_num);
    if (!item_num_opt) {
        return;
    }

    const short item_num = *item_num_opt + store_top;
    auto &item_store = *st_ptr->stock[item_num];
    auto amt = 1;
    auto item = item_store.clone();

    /*
     * If a rod or wand, allocate total maximum timeouts or charges
     * between those purchased and left on the shelf.
     */
    reduce_charges(&item, item_store.number - amt);
    item.number = amt;
    if (!check_store_item_to_inventory(player_ptr, &item)) {
        msg_print(_("そんなにアイテムを持てない。", "You cannot carry that many different items."));
        return;
    }

    const auto best = price_item(player_ptr, item.calc_price(), ot_ptr->inflate, false, store_num);
    if (item_store.number > 1) {
        if (store_num != StoreSaleType::HOME) {
            msg_format(_("一つにつき $%dです。", "That costs %d gold per item."), best);
        }

        amt = input_quantity(item_store.number);
        if (amt <= 0) {
            return;
        }
    }

    item = item_store.clone();

    /*
     * If a rod or wand, allocate total maximum timeouts or charges
     * between those purchased and left on the shelf.
     */
    reduce_charges(&item, item_store.number - amt);
    item.number = amt;
    if (!check_store_item_to_inventory(player_ptr, &item)) {
        msg_print(_("ザックにそのアイテムを入れる隙間がない。", "You cannot carry that many items."));
        return;
    }

    if (store_num == StoreSaleType::HOME) {
        take_item_from_home(player_ptr, item_store, item, item_num);
        return;
    }

    COMMAND_CODE item_new;
    const auto purchased_item_name = describe_flavor(player_ptr, item, 0);
    msg_format(_("%s(%c)を購入する。", "Buying %s (%c)."), purchased_item_name.data(), I2A(item_num));
    msg_erase();

    const auto &world = AngbandWorld::get_instance();
    auto res = prompt_to_buy(player_ptr, &item, store_num);
    if (st_ptr->store_open >= world.game_turn) {
        return;
    }
    if (!res) {
        return;
    }

    const auto price = *res;

    if (player_ptr->au < price) {
        msg_print(_("お金が足りません。", "You do not have enough gold."));
        return;
    }

    store_owner_says_comment(player_ptr, store_num);
    if (store_num == StoreSaleType::BLACK) {
        chg_virtue(player_ptr, Virtue::JUSTICE, -1);
    }
    if ((item_store.bi_key.tval() == ItemKindType::BOTTLE) && (store_num != StoreSaleType::HOME)) {
        chg_virtue(player_ptr, Virtue::NATURE, -1);
    }

    sound(SoundKind::BUY);
    player_ptr->au -= price;
    store_prt_gold(player_ptr->au);
    object_aware(player_ptr, item);

    msg_print(_("{}を ${}で購入しました。", "You bought {} for {} gold."), purchased_item_name, price);
    record_item_name = purchased_item_name;
    record_turn = world.game_turn;
    const auto &floor = *player_ptr->current_floor_ptr;
    if (record_buy) {
        exe_write_diary(floor, DiaryKind::BUY, 0, purchased_item_name);
    }

    const auto diary_item_name = describe_flavor(player_ptr, item_store, OD_NAME_ONLY);
    if (record_rand_art && item_store.is_random_artifact()) {
        exe_write_diary(floor, DiaryKind::ART, 0, diary_item_name);
    }

    item.inscription.reset();
    item.feeling = FEEL_NONE;
    item.ident &= ~(IDENT_STORE);

    const auto idx = find_autopick_list(player_ptr, &item);
    auto_inscribe_item(&item, idx);

    item_new = store_item_to_inventory(player_ptr, &item);
    handle_stuff(player_ptr);

    const auto got_item_name = describe_flavor(player_ptr, *player_ptr->inventory[item_new], 0);
    msg_format(_("%s(%c)を手に入れた。", "You have %s (%c)."), got_item_name.data(), index_to_label(item_new));

    if (item_store.is_wand_rod()) {
        item_store.pval -= item.pval;
    }

    i = st_ptr->stock_num;
    st_ptr->increase_item(item_num, -amt);
    st_ptr->optimize_item(item_num);
    switch_store_stock(player_ptr, i, item_num, store_num);
}
