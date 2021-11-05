﻿#include "store/purchase-order.h"
#include "autopick/autopick-finder.h"
#include "autopick/autopick-util.h"
#include "autopick/autopick.h"
#include "avatar/avatar.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "grid/feature.h"
#include "inventory/inventory-object.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-info.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "perception/object-perception.h"
#include "player/race-info-table.h"
#include "store/home.h"
#include "store/pricing.h"
#include "store/say-comments.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "view/display-store.h"
#include "world/world.h"
#include <optional>

/*!
 * @brief プレイヤーが購入する時の値切り処理メインルーチン /
 * Haggling routine 				-RAK-
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr オブジェクトの構造体参照ポインタ
 * @param price 最終価格を返す参照ポインタ
 * @return プレイヤーの価格に対して店主が不服ならばTRUEを返す /
 * Return TRUE if purchase is NOT successful
 */
static std::optional<PRICE> prompt_to_buy(PlayerType *player_ptr, object_type *o_ptr)
{
    auto price_ask = price_item(player_ptr, o_ptr, ot_ptr->inflate, false);

    price_ask *= o_ptr->number;
    concptr s = format(_("買値 $%ld で買いますか？", "Do you buy for $%ld? "), static_cast<long>(price_ask));
    if (get_check_strict(player_ptr, s, CHECK_DEFAULT_Y)) {
        return price_ask;
    }

    return std::nullopt;
}

/*!
 * @brief 店舗から購入する際のアイテム選択プロンプト
 * @param item 店舗インベントリ番号(アドレス渡し)
 * @param i 店舗インベントリストック数
 * @return 選択したらtrue、しなかったらfalse
 * @details
 * 選択したインベントリ番号はitemに返る。
 * ブラックマーケットの時は別のメッセージ。
 */
static bool show_store_select_item(COMMAND_CODE *item, const int i)
{
    char out_val[160];

    switch (cur_store_num) {
    case StoreSaleType::HOME:
        sprintf(out_val, _("どのアイテムを取りますか? ", "Which item do you want to take? "));
        break;
    case StoreSaleType::BLACK:
        sprintf(out_val, _("どれ? ", "Which item, huh? "));
        break;
    default:
        sprintf(out_val, _("どの品物が欲しいんだい? ", "Which item are you interested in? "));
        break;
    }

    return get_stock(item, out_val, 0, i - 1) != 0;
}

/*!
 * @brief 家のアイテムを取得する
 * @param player_ptr プレイヤー情報の参照ポインタ
 * @param o_ptr 取得元オブジェクト
 * @param j_ptr 取得先オブジェクト(指定数量分)
 * @param item_new 取得先インベントリ番号(アドレス渡し)
 * @param amt 数量
 * @param i お店のストック数(アドレス渡し)
 * @param 取得元インベントリ番号
 */
static void take_item_from_home(PlayerType *player_ptr, object_type *o_ptr, object_type *j_ptr, const COMMAND_CODE item)
{
    const int amt = j_ptr->number;
    distribute_charges(o_ptr, j_ptr, amt);

    GAME_TEXT o_name[MAX_NLEN];
    auto item_new = store_item_to_inventory(player_ptr, j_ptr);
    describe_flavor(player_ptr, o_name, &player_ptr->inventory_list[item_new], 0);
    handle_stuff(player_ptr);
    msg_format(_("%s(%c)を取った。", "You have %s (%c)."), o_name, index_to_label(item_new));

    auto i = st_ptr->stock_num;
    store_item_increase(item, -amt);
    store_item_optimize(item);

    auto combined_or_reordered = combine_and_reorder_home(player_ptr, StoreSaleType::HOME);

    if (i == st_ptr->stock_num) {
        if (combined_or_reordered)
            display_store_inventory(player_ptr);
        else
            display_entry(player_ptr, item);
        return;
    }

    if (st_ptr->stock_num == 0)
        store_top = 0;
    else if (store_top >= st_ptr->stock_num)
        store_top -= store_bottom;

    display_store_inventory(player_ptr);
    chg_virtue(player_ptr, V_SACRIFICE, 1);
}

static void shuffle_store(PlayerType *player_ptr)
{
    if (!one_in_(STORE_SHUFFLE)) {
        msg_print(_("店主は新たな在庫を取り出した。", "The shopkeeper brings out some new stock."));
        return;
    }

    char buf[80];
    msg_print(_("店主は引退した。", "The shopkeeper retires."));
    store_shuffle(player_ptr, cur_store_num);
    prt("", 3, 0);
    sprintf(buf, "%s (%s)", ot_ptr->owner_name, race_info[enum2i(ot_ptr->owner_race)].title);
    put_str(buf, 3, 10);
    sprintf(buf, "%s (%ld)", f_info[cur_store_feat].name.c_str(), (long)(ot_ptr->max_cost));
    prt(buf, 3, 50);
}

static void switch_store_stock(PlayerType *player_ptr, const int i, const COMMAND_CODE item)
{
    if (st_ptr->stock_num == 0) {
        shuffle_store(player_ptr);
        store_maintenance(player_ptr, player_ptr->town_num, cur_store_num, 10);

        store_top = 0;
        display_store_inventory(player_ptr);
        return;
    }

    if (st_ptr->stock_num != i) {
        if (store_top >= st_ptr->stock_num)
            store_top -= store_bottom;

        display_store_inventory(player_ptr);
        return;
    }

    display_entry(player_ptr, item);
}

/*!
 * @brief 店からの購入処理のメインルーチン /
 * Buy an item from a store 			-RAK-
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void store_purchase(PlayerType *player_ptr)
{
    if (cur_store_num == StoreSaleType::MUSEUM) {
        msg_print(_("博物館から取り出すことはできません。", "Items cannot be taken out of the Museum."));
        return;
    }

    if (st_ptr->stock_num <= 0) {
        if (cur_store_num == StoreSaleType::HOME)
            msg_print(_("我が家には何も置いてありません。", "Your home is empty."));
        else
            msg_print(_("現在商品の在庫を切らしています。", "I am currently out of stock."));
        return;
    }

    int i = (st_ptr->stock_num - store_top);
    if (i > store_bottom)
        i = store_bottom;

    COMMAND_CODE item;
    if (!show_store_select_item(&item, i))
        return;

    item = item + store_top;
    object_type *o_ptr;
    o_ptr = &st_ptr->stock[item];

    ITEM_NUMBER amt = 1;
    object_type forge;
    object_type *j_ptr = &forge;
    j_ptr->copy_from(o_ptr);

    /*
     * If a rod or wand, allocate total maximum timeouts or charges
     * between those purchased and left on the shelf.
     */
    reduce_charges(j_ptr, o_ptr->number - amt);
    j_ptr->number = amt;
    if (!check_store_item_to_inventory(player_ptr, j_ptr)) {
        msg_print(_("そんなにアイテムを持てない。", "You cannot carry that many different items."));
        return;
    }

    PRICE best = price_item(player_ptr, j_ptr, ot_ptr->inflate, false);
    if (o_ptr->number > 1) {
        if (cur_store_num != StoreSaleType::HOME) {
            msg_format(_("一つにつき $%ldです。", "That costs %ld gold per item."), (long)(best));
        }

        amt = get_quantity(nullptr, o_ptr->number);
        if (amt <= 0)
            return;
    }

    j_ptr = &forge;
    j_ptr->copy_from(o_ptr);

    /*
     * If a rod or wand, allocate total maximum timeouts or charges
     * between those purchased and left on the shelf.
     */
    reduce_charges(j_ptr, o_ptr->number - amt);
    j_ptr->number = amt;
    if (!check_store_item_to_inventory(player_ptr, j_ptr)) {
        msg_print(_("ザックにそのアイテムを入れる隙間がない。", "You cannot carry that many items."));
        return;
    }

    if (cur_store_num == StoreSaleType::HOME) {
        take_item_from_home(player_ptr, o_ptr, j_ptr, item);
        return;
    }

    COMMAND_CODE item_new;
    PRICE price;
    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, j_ptr, 0);
    msg_format(_("%s(%c)を購入する。", "Buying %s (%c)."), o_name, I2A(item));
    msg_print(nullptr);

    auto res = prompt_to_buy(player_ptr, j_ptr);
    if (st_ptr->store_open >= w_ptr->game_turn)
        return;
    if (!res)
        return;

    price = res.value();

    if (player_ptr->au < price) {
        msg_print(_("お金が足りません。", "You do not have enough gold."));
        return;
    }

    store_owner_says_comment(player_ptr);
    if (cur_store_num == StoreSaleType::BLACK)
        chg_virtue(player_ptr, V_JUSTICE, -1);
    if ((o_ptr->tval == ItemKindType::BOTTLE) && (cur_store_num != StoreSaleType::HOME))
        chg_virtue(player_ptr, V_NATURE, -1);

    sound(SOUND_BUY);
    player_ptr->au -= price;
    store_prt_gold(player_ptr);
    object_aware(player_ptr, j_ptr);

    describe_flavor(player_ptr, o_name, j_ptr, 0);
    msg_format(_("%sを $%ldで購入しました。", "You bought %s for %ld gold."), o_name, (long)price);

    strcpy(record_o_name, o_name);
    record_turn = w_ptr->game_turn;

    if (record_buy)
        exe_write_diary(player_ptr, DIARY_BUY, 0, o_name);

    describe_flavor(player_ptr, o_name, o_ptr, OD_NAME_ONLY);
    if (record_rand_art && o_ptr->art_name)
        exe_write_diary(player_ptr, DIARY_ART, 0, o_name);

    j_ptr->inscription = 0;
    j_ptr->feeling = FEEL_NONE;
    j_ptr->ident &= ~(IDENT_STORE);

    const auto idx = find_autopick_list(player_ptr, j_ptr);
    auto_inscribe_item(player_ptr, j_ptr, idx);

    item_new = store_item_to_inventory(player_ptr, j_ptr);
    handle_stuff(player_ptr);

    describe_flavor(player_ptr, o_name, &player_ptr->inventory_list[item_new], 0);
    msg_format(_("%s(%c)を手に入れた。", "You have %s (%c)."), o_name, index_to_label(item_new));

    if ((o_ptr->tval == ItemKindType::ROD) || (o_ptr->tval == ItemKindType::WAND))
        o_ptr->pval -= j_ptr->pval;

    i = st_ptr->stock_num;
    store_item_increase(item, -amt);
    store_item_optimize(item);
    switch_store_stock(player_ptr, i, item);
}
