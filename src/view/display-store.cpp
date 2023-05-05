#include "view/display-store.h"
#include "flavor/flavor-describer.h"
#include "game-option/birth-options.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "locale/japanese.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-info.h"
#include "player/race-info-table.h"
#include "store/pricing.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "store/store.h" //!< @todo 相互依存している、こっちは残す？.
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"

/*!
 * @brief プレイヤーの所持金を表示する /
 * Displays players gold					-RAK-
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 */
void store_prt_gold(PlayerType *player_ptr)
{
    prt(_("手持ちのお金: ", "Gold Remaining: "), 19 + xtra_stock, 53);
    prt(format("%9ld", (long)player_ptr->au), 19 + xtra_stock, 68);
}

/*!
 * @brief 店の商品リストを再表示する /
 * Re-displays a single store entry
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 表示行
 */
void display_entry(PlayerType *player_ptr, int pos, StoreSaleType store_num)
{
    ItemEntity *o_ptr;
    o_ptr = &st_ptr->stock[pos];
    int i = (pos % store_bottom);

    /* Label it, clear the line --(-- */
    prt(format("%c) ", ((i > 25) ? toupper(I2A(i - 26)) : I2A(i))), i + 6, 0);

    int cur_col = 3;
    if (show_item_graph) {
        const auto a = o_ptr->get_color();
        const auto c = o_ptr->get_symbol();

        term_queue_bigchar(cur_col, i + 6, a, c, 0, 0);
        if (use_bigtile) {
            cur_col++;
        }

        cur_col += 2;
    }

    /* Describe an item in the home */
    int maxwid = 75;
    if ((store_num == StoreSaleType::HOME) || (store_num == StoreSaleType::MUSEUM)) {
        if (show_weights) {
            maxwid -= 10;
        }

        const auto item_name = describe_flavor(player_ptr, o_ptr, 0, maxwid);
        c_put_str(tval_to_attr[enum2i(o_ptr->bi_key.tval())], item_name.data(), i + 6, cur_col);

        if (show_weights) {
            WEIGHT wgt = o_ptr->weight;
            put_str(format(_("%3d.%1d kg", "%3d.%d lb"), _(lb_to_kg_integer(wgt), wgt / 10), _(lb_to_kg_fraction(wgt), wgt % 10)), i + 6, _(67, 68));
        }

        return;
    }

    maxwid = 65;
    if (show_weights) {
        maxwid -= 7;
    }

    const auto item_name = describe_flavor(player_ptr, o_ptr, 0, maxwid);
    c_put_str(tval_to_attr[enum2i(o_ptr->bi_key.tval())], item_name.data(), i + 6, cur_col);

    if (show_weights) {
        int wgt = o_ptr->weight;
        put_str(format("%3d.%1d", _(lb_to_kg_integer(wgt), wgt / 10), _(lb_to_kg_fraction(wgt), wgt % 10)), i + 6, _(60, 61));
    }

    const auto price = price_item(player_ptr, o_ptr, ot_ptr->inflate, false, store_num);
    put_str(format("%9ld  ", (long)price), i + 6, 68);
}

/*!
 * @brief 店の商品リストを表示する /
 * Displays a store's inventory -RAK-
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * All prices are listed as "per individual object".  -BEN-
 */
void display_store_inventory(PlayerType *player_ptr, StoreSaleType store_num)
{
    int k;
    for (k = 0; k < store_bottom; k++) {
        if (store_top + k >= st_ptr->stock_num) {
            break;
        }

        display_entry(player_ptr, store_top + k, store_num);
    }

    for (int i = k; i < store_bottom + 1; i++) {
        prt("", i + 6, 0);
    }

    put_str(_("          ", "        "), 5, _(20, 22));
    if (st_ptr->stock_num > store_bottom) {
        prt(_("-続く-", "-more-"), k + 6, 3);
        put_str(format(_("(%dページ)  ", "(Page %d)  "), store_top / store_bottom + 1), 5, _(20, 22));
    }

    if (store_num == StoreSaleType::HOME || store_num == StoreSaleType::MUSEUM) {
        k = st_ptr->stock_size;
        if (store_num == StoreSaleType::HOME && !powerup_home) {
            k /= 10;
        }

        put_str(format(_("アイテム数:  %4d/%4d", "Objects:  %4d/%4d"), st_ptr->stock_num, k), 19 + xtra_stock, _(27, 30));
    }
}

/*!
 * @brief 店舗情報全体を表示するメインルーチン /
 * Displays store (after clearing screen)		-RAK-
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 */
void display_store(PlayerType *player_ptr, StoreSaleType store_num)
{
    term_clear();
    if (store_num == StoreSaleType::HOME) {
        put_str(_("我が家", "Your Home"), 3, 31);
        put_str(_("アイテムの一覧", "Item Description"), 5, 4);
        if (show_weights) {
            put_str(_("  重さ", "Weight"), 5, 70);
        }

        store_prt_gold(player_ptr);
        display_store_inventory(player_ptr, store_num);
        return;
    }

    if (store_num == StoreSaleType::MUSEUM) {
        put_str(_("博物館", "Museum"), 3, 31);
        put_str(_("アイテムの一覧", "Item Description"), 5, 4);
        if (show_weights) {
            put_str(_("  重さ", "Weight"), 5, 70);
        }

        store_prt_gold(player_ptr);
        display_store_inventory(player_ptr, store_num);
        return;
    }

    concptr store_name = terrains_info[cur_store_feat].name.data();
    concptr owner_name = (ot_ptr->owner_name);
    concptr race_name = race_info[enum2i(ot_ptr->owner_race)].title;
    put_str(format("%s (%s)", owner_name, race_name), 3, 10);

    prt(format("%s (%ld)", store_name, (long)(ot_ptr->max_cost)), 3, 50);

    put_str(_("商品の一覧", "Item Description"), 5, 5);
    if (show_weights) {
        put_str(_("  重さ", "Weight"), 5, 60);
    }

    put_str(_(" 価格", "Price"), 5, 72);
    store_prt_gold(player_ptr);
    display_store_inventory(player_ptr, store_num);
}
