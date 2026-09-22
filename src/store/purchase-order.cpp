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
#include "object/object-info.h"
#include "object/object-stack.h"
#include "perception/object-perception.h"
#include "player/race-info-table.h"
#include "store/home.h"
#include "store/pricing.h"
#include "store/say-comments.h"
#include "store/store-owners.h"
#include "store/store-screen.h"
#include "store/store.h"
#include "system/player-type-definition.h"
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
 * @param store 購入元の店舗
 * @param o_ptr オブジェクトの構造体参照ポインタ
 * @return プレイヤーが購入するなら購入価格、購入しないならnullopt
 */
static tl::optional<PRICE> prompt_to_buy(PlayerType *player_ptr, const Store &store, ItemEntity *o_ptr)
{
    auto price_ask = price_item(player_ptr, o_ptr->calc_price(), store, StoreTradeType::PLAYER_BUYS);

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
 * @param screen 我が家の画面
 * @param item_home 取得元オブジェクト
 * @param item_inventory 取得先オブジェクト(指定数量分)
 * @param i_idx 取得先インベントリ番号
 */
static void take_item_from_home(PlayerType *player_ptr, StoreScreen &screen, ItemEntity &item_home, ItemEntity &item_inventory, short i_idx)
{
    auto &store = screen.get_store();
    const auto amt = item_inventory.number;
    distribute_charges(&item_home, &item_inventory, amt);

    const auto item_new = store_item_to_inventory(player_ptr, &item_inventory);
    const auto item_name = describe_flavor(player_ptr, *player_ptr->inventory[item_new], 0);
    handle_stuff(player_ptr);
    msg_format(_("%s(%c)を取った。", "You have %s (%c)."), item_name.data(), index_to_label(item_new));

    const auto stock_num = store.stock_num;
    store.increase_item(i_idx, -amt);
    store.optimize_item(i_idx);

    const auto combined_or_reordered = combine_and_reorder_home(player_ptr, store);
    if (stock_num == store.stock_num) {
        if (combined_or_reordered) {
            display_store_inventory(player_ptr, screen);
            return;
        }

        display_entry(player_ptr, screen, i_idx);
        return;
    }

    screen.adjust_page_after_removal();
    display_store_inventory(player_ptr, screen);
    chg_virtue(player_ptr, Virtue::SACRIFICE, 1);
}

static void shuffle_store(const StoreScreen &screen)
{
    const auto &store = screen.get_store();
    if (!one_in_(STORE_SHUFFLE)) {
        msg_print(_("店主は新たな在庫を取り出した。", "The shopkeeper brings out some new stock."));
        return;
    }

    msg_print(_("店主は引退した。", "The shopkeeper retires."));
    store_shuffle(screen.get_town_index(), store.get_sale_type());
    prt("", 3, 0);
    const auto &owner = store.get_owner();
    put_str(format("%s (%s)", owner.owner_name, race_info[enum2i(owner.owner_race)].title.data()), 3, 10);
    prt(format("%s (%d)", screen.get_name().data(), owner.max_cost), 3, 50);
}

static void switch_store_stock(PlayerType *player_ptr, StoreScreen &screen, const int i, const COMMAND_CODE item)
{
    auto &store = screen.get_store();
    if (store.stock_num == 0) {
        shuffle_store(screen);
        store_maintenance(player_ptr, screen.get_town_index(), store, 10);

        screen.reset_page();
        display_store_inventory(player_ptr, screen);
        return;
    }

    if (store.stock_num != i) {
        screen.adjust_page_after_removal();
        display_store_inventory(player_ptr, screen);
        return;
    }

    display_entry(player_ptr, screen, item);
}

/*!
 * @brief 店からの購入処理のメインルーチン /
 * Buy an item from a store 			-RAK-
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param screen 購入元の店舗の画面
 */
void store_purchase(PlayerType *player_ptr, StoreScreen &screen)
{
    auto &store = screen.get_store();
    const auto store_num = store.get_sale_type();
    if (store_num == StoreSaleType::MUSEUM) {
        msg_print(_("博物館から取り出すことはできません。", "Items cannot be taken out of the Museum."));
        return;
    }

    if (store.stock_num <= 0) {
        if (store_num == StoreSaleType::HOME) {
            msg_print(_("我が家には何も置いてありません。", "Your home is empty."));
        } else {
            msg_print(_("現在商品の在庫を切らしています。", "I am currently out of stock."));
        }
        return;
    }

    auto item_num_opt = show_store_select_item(screen.get_page_item_count(), store_num);
    if (!item_num_opt) {
        return;
    }

    const short item_num = *item_num_opt + screen.get_page_top();
    auto &item_store = *store.stock[item_num];
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

    const auto best = price_item(player_ptr, item.calc_price(), store, StoreTradeType::PLAYER_BUYS);
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
        take_item_from_home(player_ptr, screen, item_store, item, item_num);
        return;
    }

    COMMAND_CODE item_new;
    const auto purchased_item_name = describe_flavor(player_ptr, item, 0);
    const auto item_index = *item_num_opt;
    const auto item_index_char = (item_index > 25) ? toupper(I2A(item_index - 26)) : I2A(item_index);

    msg_format(_("%s(%c)を購入する。", "Buying %s (%c)."), purchased_item_name.data(), item_index_char);
    msg_erase();

    const auto &world = AngbandWorld::get_instance();
    auto res = prompt_to_buy(player_ptr, store, &item);
    if (store.store_open >= world.game_turn) {
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

    store_owner_says_comment(price, store_num);
    if (store_num == StoreSaleType::BLACK) {
        chg_virtue(player_ptr, Virtue::JUSTICE, -1);
    }
    if ((item_store.bi_key.tval() == ItemKindType::BOTTLE) && (store_num != StoreSaleType::HOME)) {
        chg_virtue(player_ptr, Virtue::NATURE, -1);
    }

    sound(SoundKind::BUY);
    player_ptr->au -= price;
    store_prt_gold(screen, player_ptr->au);
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
    item.reset_identification_flag(IdentificationFlag::STORE);

    const auto idx = find_autopick_list(player_ptr, &item);
    auto_inscribe_item(&item, idx);

    item_new = store_item_to_inventory(player_ptr, &item);
    handle_stuff(player_ptr);

    const auto got_item_name = describe_flavor(player_ptr, *player_ptr->inventory[item_new], 0);
    msg_format(_("%s(%c)を手に入れた。", "You have %s (%c)."), got_item_name.data(), index_to_label(item_new));

    if (item_store.is_wand_rod()) {
        item_store.pval -= item.pval;
    }

    const auto stock_num = store.stock_num;
    store.increase_item(item_num, -amt);
    store.optimize_item(item_num);
    switch_store_stock(player_ptr, screen, stock_num, item_num);
}
