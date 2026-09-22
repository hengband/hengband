#include "view/display-store.h"
#include "flavor/flavor-describer.h"
#include "game-option/birth-options.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "locale/japanese.h"
#include "player/race-info-table.h"
#include "store/pricing.h"
#include "store/store-owners.h"
#include "store/store-screen.h"
#include "store/store-util.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "view/display-symbol.h"
#include <algorithm>

/*!
 * @brief プレイヤーの所持金を表示する
 * @param screen 表示する店舗の画面
 * @param num_golds 所持金
 */
void store_prt_gold(const StoreScreen &screen, int num_golds)
{
    const auto row = screen.get_status_row();
    prt(_("手持ちのお金: ", "Gold Remaining: "), row, 53);
    prt(format("%9d", num_golds), row, 68);
}

/*!
 * @brief 店の商品リストを再表示する /
 * Re-displays a single store entry
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param screen 表示する店舗の画面
 * @param pos 表示する在庫の番号
 */
void display_entry(PlayerType *player_ptr, const StoreScreen &screen, int pos)
{
    const auto &store = screen.get_store();
    const auto &item = *store.stock[pos];
    const auto i = screen.get_page_position(pos);

    /* Label it, clear the line --(-- */
    prt(format("%c) ", ((i > 25) ? toupper(I2A(i - 26)) : I2A(i))), i + 6, 0);

    int cur_col = 3;
    if (show_item_graph) {
        term_queue_bigchar(cur_col, i + 6, { item.get_symbol(), {} });
        if (use_bigtile) {
            cur_col++;
        }

        cur_col += 2;
    }

    /* Describe an item in the home */
    int maxwid = 75;
    const auto store_num = store.get_sale_type();
    if ((store_num == StoreSaleType::HOME) || (store_num == StoreSaleType::MUSEUM)) {
        if (show_weights) {
            maxwid -= 10;
        }

        const auto item_name = describe_flavor(player_ptr, item, 0, maxwid);
        c_put_str(tval_to_attr[enum2i(item.bi_key.tval())], item_name, i + 6, cur_col);

        if (show_weights) {
            const auto wgt = item.weight;
            put_str(format(_("%3d.%1d kg", "%3d.%d lb"), _(lb_to_kg_integer(wgt), wgt / 10), _(lb_to_kg_fraction(wgt), wgt % 10)), i + 6, _(67, 68));
        }

        return;
    }

    maxwid = 65;
    if (show_weights) {
        maxwid -= 7;
    }

    const auto item_name = describe_flavor(player_ptr, item, 0, maxwid);
    c_put_str(tval_to_attr[enum2i(item.bi_key.tval())], item_name, i + 6, cur_col);

    if (show_weights) {
        const auto wgt = item.weight;
        put_str(format("%3d.%1d", _(lb_to_kg_integer(wgt), wgt / 10), _(lb_to_kg_fraction(wgt), wgt % 10)), i + 6, _(60, 61));
    }

    const auto price = price_item(player_ptr, item.calc_price(), store, false);
    put_str(format("%9d  ", price), i + 6, 68);
}

/*!
 * @brief 店の商品リストを表示する /
 * Displays a store's inventory -RAK-
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param screen 表示する店舗の画面
 * @details
 * All prices are listed as "per individual object".  -BEN-
 */
void display_store_inventory(PlayerType *player_ptr, const StoreScreen &screen)
{
    const auto &store = screen.get_store();
    const auto store_num = store.get_sale_type();
    const auto page_top = screen.get_page_top();
    const auto page_size = screen.get_page_size();
    const auto page_item_count = std::max(screen.get_page_item_count(), 0);
    for (auto k = 0; k < page_item_count; k++) {
        display_entry(player_ptr, screen, page_top + k);
    }

    for (auto i = page_item_count; i <= page_size; i++) {
        prt("", i + 6, 0);
    }

    put_str(_("          ", "        "), 5, _(20, 22));
    if (screen.has_multiple_pages()) {
        prt(_("-続く-", "-more-"), page_item_count + 6, 3);
        put_str(format(_("(%dページ)  ", "(Page %d)  "), page_top / page_size + 1), 5, _(20, 22));
    }

    if (store_num == StoreSaleType::HOME || store_num == StoreSaleType::MUSEUM) {
        int stock_limit = store.stock_size;
        if (store_num == StoreSaleType::HOME && !powerup_home) {
            stock_limit /= 10;
        }

        put_str(format(_("アイテム数:  %4d/%4d", "Objects:  %4d/%4d"), store.stock_num, stock_limit), screen.get_status_row(), _(27, 30));
    }
}

/*!
 * @brief 店舗情報全体を表示するメインルーチン /
 * Displays store (after clearing screen)		-RAK-
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param screen 表示する店舗の画面
 * @details
 */
void display_store(PlayerType *player_ptr, const StoreScreen &screen)
{
    const auto &store = screen.get_store();
    const auto store_num = store.get_sale_type();
    term_clear();
    if (store_num == StoreSaleType::HOME) {
        put_str(_("我が家", "Your Home"), 3, 31);
        put_str(_("アイテムの一覧", "Item Description"), 5, 4);
        if (show_weights) {
            put_str(_("  重さ", "Weight"), 5, 70);
        }

        store_prt_gold(screen, player_ptr->au);
        display_store_inventory(player_ptr, screen);
        return;
    }

    if (store_num == StoreSaleType::MUSEUM) {
        put_str(_("博物館", "Museum"), 3, 31);
        put_str(_("アイテムの一覧", "Item Description"), 5, 4);
        if (show_weights) {
            put_str(_("  重さ", "Weight"), 5, 70);
        }

        store_prt_gold(screen, player_ptr->au);
        display_store_inventory(player_ptr, screen);
        return;
    }

    const auto &owner = store.get_owner();
    const auto race_name = race_info[enum2i(owner.owner_race)].title.data();
    put_str(format("%s (%s)", owner.owner_name, race_name), 3, 10);

    prt(format("%s (%d)", screen.get_name().data(), owner.max_cost), 3, 50);

    put_str(_("商品の一覧", "Item Description"), 5, 5);
    if (show_weights) {
        put_str(_("  重さ", "Weight"), 5, 60);
    }

    put_str(_(" 価格", "Price"), 5, 72);
    store_prt_gold(screen, player_ptr->au);
    display_store_inventory(player_ptr, screen);
}
