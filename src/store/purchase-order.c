#include "store/purchase-order.h"
#include "autopick/autopick.h"
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
#include "object/object-generator.h"
#include "object/object-info.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "perception/object-perception.h"
#include "player-info/avatar.h"
#include "player/race-info-table.h"
#include "store/home.h"
#include "store/owner-insults.h"
#include "store/pricing.h"
#include "store/say-comments.h"
#include "store/store-util.h"
#include "store/store.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "view/display-store.h"
#include "world/world.h"

typedef struct haggle_type {
    object_type *o_ptr;
    s32b *price;
    s32b cur_ask;
    s32b final_ask;
    int noneed;
    bool final;
    concptr pmt;
    s32b min_per;
    s32b max_per;
    s32b last_offer;
    s32b offer;
    bool flag;
    int annoyed;
    bool cancel;
} haggle_type;

static haggle_type *initialize_haggle_type(player_type *player_ptr, haggle_type *haggle_ptr, object_type *o_ptr, s32b *price)
{
    haggle_ptr->o_ptr = o_ptr;
    haggle_ptr->price = price;
    haggle_ptr->cur_ask = price_item(player_ptr, o_ptr, ot_ptr->max_inflate, FALSE);
    haggle_ptr->final_ask = price_item(player_ptr, o_ptr, ot_ptr->min_inflate, FALSE);
    haggle_ptr->noneed = noneedtobargain(haggle_ptr->final_ask);
    haggle_ptr->final = FALSE;
    haggle_ptr->pmt = _("提示価格", "Asking");
    return haggle_ptr;
}

static void settle_haggle_result(haggle_type *haggle_ptr)
{
    if (!haggle_ptr->noneed && manual_haggle)
        return;

    if (haggle_ptr->noneed) {
        msg_print(_("結局この金額にまとまった。", "You eventually agree upon the price."));
        msg_print(NULL);
    } else {
        msg_print(_("すんなりとこの金額にまとまった。", "You quickly agree upon the price."));
        msg_print(NULL);
        haggle_ptr->final_ask += haggle_ptr->final_ask / 10;
    }

    haggle_ptr->cur_ask = haggle_ptr->final_ask;
    haggle_ptr->pmt = _("最終提示価格", "Final Offer");
    haggle_ptr->final = TRUE;
}

static void decide_haggle_offer(player_type *player_ptr, haggle_type *haggle_ptr)
{
    haggle_ptr->cur_ask *= haggle_ptr->o_ptr->number;
    haggle_ptr->final_ask *= haggle_ptr->o_ptr->number;
    haggle_ptr->min_per = ot_ptr->haggle_per;
    haggle_ptr->max_per = haggle_ptr->min_per * 3;
    haggle_ptr->last_offer = object_value(player_ptr, haggle_ptr->o_ptr) * haggle_ptr->o_ptr->number;
    haggle_ptr->last_offer = haggle_ptr->last_offer * (200 - (int)(ot_ptr->max_inflate)) / 100L;
    if (haggle_ptr->last_offer <= 0)
        haggle_ptr->last_offer = 1;
}

static void initialize_haggle_offer(haggle_type *haggle_ptr)
{
    haggle_ptr->offer = 0;
    allow_inc = FALSE;
    haggle_ptr->flag = FALSE;
    haggle_ptr->annoyed = 0;
    haggle_ptr->cancel = FALSE;
    *haggle_ptr->price = 0;
}

static void suggest_haggle_offer(haggle_type *haggle_ptr)
{
    bool loop_flag = TRUE;
    while (!haggle_ptr->flag && loop_flag) {
        char out_val[160];
        (void)sprintf(out_val, "%s :  %ld", haggle_ptr->pmt, (long)haggle_ptr->cur_ask);
        put_str(out_val, 1, 0);
        haggle_ptr->cancel
            = receive_offer(_("提示する金額? ", "What do you offer? "), &haggle_ptr->offer, haggle_ptr->last_offer, 1, haggle_ptr->cur_ask, haggle_ptr->final);
        if (haggle_ptr->cancel) {
            haggle_ptr->flag = TRUE;
        } else if (haggle_ptr->offer > haggle_ptr->cur_ask) {
            say_comment_6();
            haggle_ptr->offer = haggle_ptr->last_offer;
        } else if (haggle_ptr->offer == haggle_ptr->cur_ask) {
            haggle_ptr->flag = TRUE;
            *haggle_ptr->price = haggle_ptr->offer;
        } else {
            loop_flag = FALSE;
        }
    }
}

static void final_haggle_offer(haggle_type *haggle_ptr)
{
    haggle_ptr->final = TRUE;
    haggle_ptr->cur_ask = haggle_ptr->final_ask;
    haggle_ptr->pmt = _("最終提示価格", "What do you offer? ");
    haggle_ptr->annoyed++;
    if (haggle_ptr->annoyed <= 3)
        return;

    (void)increase_insults();
    haggle_ptr->cancel = TRUE;
    haggle_ptr->flag = TRUE;
}

static void show_last_haggle_offer(haggle_type *haggle_ptr)
{
    haggle_ptr->last_offer = haggle_ptr->offer;
    allow_inc = TRUE;
    prt("", 1, 0);
    char out_val[160];
    (void)sprintf(out_val, _("前回の提示金額: $%ld", "Your last offer: %ld"), (long)haggle_ptr->last_offer);
    put_str(out_val, 1, 39);
    say_comment_2(haggle_ptr->cur_ask, haggle_ptr->annoyed);
}

/*!
 * @brief プレイヤーが購入する時の値切り処理メインルーチン /
 * Haggling routine 				-RAK-
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr オブジェクトの構造体参照ポインタ
 * @param price 最終価格を返す参照ポインタ
 * @return プレイヤーの価格に対して店主が不服ならばTRUEを返す /
 * Return TRUE if purchase is NOT successful
 */
static bool purchase_haggle(player_type *player_ptr, object_type *o_ptr, s32b *price)
{
    haggle_type tmp_haggle;
    haggle_type *haggle_ptr = initialize_haggle_type(player_ptr, &tmp_haggle, o_ptr, price);
    settle_haggle_result(haggle_ptr);
    decide_haggle_offer(player_ptr, haggle_ptr);
    initialize_haggle_offer(haggle_ptr);
    while (!haggle_ptr->flag) {
        suggest_haggle_offer(haggle_ptr);
        if (haggle_ptr->flag)
            continue;

        s32b x1 = 100 * (haggle_ptr->offer - haggle_ptr->last_offer) / (haggle_ptr->cur_ask - haggle_ptr->last_offer);
        if (x1 < haggle_ptr->min_per) {
            if (haggle_insults()) {
                haggle_ptr->flag = TRUE;
                haggle_ptr->cancel = TRUE;
            }
        } else if (x1 > haggle_ptr->max_per) {
            x1 = x1 * 3 / 4;
            if (x1 < haggle_ptr->max_per)
                x1 = haggle_ptr->max_per;
        }

        s32b x2 = rand_range(x1 - 2, x1 + 2);
        s32b x3 = ((haggle_ptr->cur_ask - haggle_ptr->offer) * x2 / 100L) + 1;
        if (x3 < 0)
            x3 = 0;
        haggle_ptr->cur_ask -= x3;

        if (haggle_ptr->cur_ask < haggle_ptr->final_ask)
            final_haggle_offer(haggle_ptr);
        else if (haggle_ptr->offer >= haggle_ptr->cur_ask) {
            haggle_ptr->flag = TRUE;
            *haggle_ptr->price = haggle_ptr->offer;
        }

        if (haggle_ptr->flag)
            continue;

        show_last_haggle_offer(haggle_ptr);
    }

    if (haggle_ptr->cancel)
        return TRUE;

    updatebargain(*price, haggle_ptr->final_ask, o_ptr->number);
    return FALSE;
}

static bool show_store_select_item(COMMAND_CODE *item, const int i)
{
    char out_val[160];
#ifdef JP
    /* ブラックマーケットの時は別のメッセージ */
    switch (cur_store_num) {
    case 7:
        sprintf(out_val, "どのアイテムを取りますか? ");
        break;
    case 6:
        sprintf(out_val, "どれ? ");
        break;
    default:
        sprintf(out_val, "どの品物が欲しいんだい? ");
        break;
    }
#else
    if (cur_store_num == STORE_HOME) {
        sprintf(out_val, "Which item do you want to take? ");
    } else {
        sprintf(out_val, "Which item are you interested in? ");
    }
#endif

    return get_stock(item, out_val, 0, i - 1) != 0;
}

static bool process_purchase_result(player_type *player_ptr, object_type *o_ptr, object_type *j_ptr, COMMAND_CODE *item_new,const int amt, int *i, const COMMAND_CODE item)
{
    if (cur_store_num != STORE_HOME)
        return FALSE;

    bool combined_or_reordered;
    distribute_charges(o_ptr, j_ptr, amt);
    *item_new = store_item_to_inventory(player_ptr, j_ptr);
    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, &player_ptr->inventory_list[*item_new], 0);
    msg_format(_("%s(%c)を取った。", "You have %s (%c)."), o_name, index_to_label(*item_new));
    handle_stuff(player_ptr);
    *i = st_ptr->stock_num;
    store_item_increase(item, -amt);
    store_item_optimize(item);
    combined_or_reordered = combine_and_reorder_home(player_ptr, STORE_HOME);
    if (*i == st_ptr->stock_num) {
        if (combined_or_reordered)
            display_store_inventory(player_ptr);
        else
            display_entry(player_ptr, item);

        return TRUE;
    }

    if (st_ptr->stock_num == 0)
        store_top = 0;
    else if (store_top >= st_ptr->stock_num)
        store_top -= store_bottom;

    display_store_inventory(player_ptr);
    chg_virtue(player_ptr, V_SACRIFICE, 1);
    return TRUE;
}

static void shuffle_store(player_type *player_ptr)
{
    if (!one_in_(STORE_SHUFFLE)) {
        msg_print(_("店主は新たな在庫を取り出した。", "The shopkeeper brings out some new stock."));
        return;
    }

    char buf[80];
    msg_print(_("店主は引退した。", "The shopkeeper retires."));
    store_shuffle(player_ptr, cur_store_num);
    prt("", 3, 0);
    sprintf(buf, "%s (%s)", ot_ptr->owner_name, race_info[ot_ptr->owner_race].title);
    put_str(buf, 3, 10);
    sprintf(buf, "%s (%ld)", (f_name + f_info[cur_store_feat].name), (long)(ot_ptr->max_cost));
    prt(buf, 3, 50);
}

static void switch_store_stock(player_type *player_ptr, const int i, const COMMAND_CODE item)
{
    if (st_ptr->stock_num == 0) {
        shuffle_store(player_ptr);
        for (int j = 0; j < 10; j++)
            store_maintenance(player_ptr, player_ptr->town_num, cur_store_num);

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
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void store_purchase(player_type *player_ptr)
{
    if (cur_store_num == STORE_MUSEUM) {
        msg_print(_("博物館から取り出すことはできません。", "Museum."));
        return;
    }

    if (st_ptr->stock_num <= 0) {
        if (cur_store_num == STORE_HOME)
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
    object_type *j_ptr;
    j_ptr = &forge;
    object_copy(j_ptr, o_ptr);

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

    PRICE best = price_item(player_ptr, j_ptr, ot_ptr->min_inflate, FALSE);
    if (o_ptr->number > 1) {
        if ((cur_store_num != STORE_HOME) && (o_ptr->ident & IDENT_FIXED)) {
            msg_format(_("一つにつき $%ldです。", "That costs %ld gold per item."), (long)(best));
        }

        amt = get_quantity(NULL, o_ptr->number);
        if (amt <= 0)
            return;
    }

    j_ptr = &forge;
    object_copy(j_ptr, o_ptr);

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

    COMMAND_CODE item_new;
    if (process_purchase_result(player_ptr, o_ptr, j_ptr, &item_new, amt, &i, item))
        return;

    int choice;
    PRICE price;
    if (o_ptr->ident & (IDENT_FIXED)) {
        choice = 0;
        price = (best * j_ptr->number);
    } else {
        GAME_TEXT o_name[MAX_NLEN];
        describe_flavor(player_ptr, o_name, j_ptr, 0);
        msg_format(_("%s(%c)を購入する。", "Buying %s (%c)."), o_name, I2A(item));
        msg_print(NULL);
        choice = purchase_haggle(player_ptr, j_ptr, &price);
        if (st_ptr->store_open >= current_world_ptr->game_turn)
            return;
    }

    if (choice != 0)
        return;

    if (price == (best * j_ptr->number))
        o_ptr->ident |= (IDENT_FIXED);

    if (player_ptr->au < price) {
        msg_print(_("お金が足りません。", "You do not have enough gold."));
        return;
    }

    say_comment_1(player_ptr);
    if (cur_store_num == STORE_BLACK)
        chg_virtue(player_ptr, V_JUSTICE, -1);
    if ((o_ptr->tval == TV_BOTTLE) && (cur_store_num != STORE_HOME))
        chg_virtue(player_ptr, V_NATURE, -1);

    sound(SOUND_BUY);
    decrease_insults();
    player_ptr->au -= price;
    store_prt_gold(player_ptr);
    object_aware(player_ptr, j_ptr);
    j_ptr->ident &= ~(IDENT_FIXED);
    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, j_ptr, 0);

    msg_format(_("%sを $%ldで購入しました。", "You bought %s for %ld gold."), o_name, (long)price);

    strcpy(record_o_name, o_name);
    record_turn = current_world_ptr->game_turn;

    if (record_buy)
        exe_write_diary(player_ptr, DIARY_BUY, 0, o_name);
    describe_flavor(player_ptr, o_name, o_ptr, OD_NAME_ONLY);
    if (record_rand_art && o_ptr->art_name)
        exe_write_diary(player_ptr, DIARY_ART, 0, o_name);

    j_ptr->inscription = 0;
    j_ptr->feeling = FEEL_NONE;
    j_ptr->ident &= ~(IDENT_STORE);
    item_new = store_item_to_inventory(player_ptr, j_ptr);

    describe_flavor(player_ptr, o_name, &player_ptr->inventory_list[item_new], 0);
    msg_format(_("%s(%c)を手に入れた。", "You have %s (%c)."), o_name, index_to_label(item_new));
    autopick_alter_item(player_ptr, item_new, FALSE);
    if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND))
        o_ptr->pval -= j_ptr->pval;

    handle_stuff(player_ptr);
    i = st_ptr->stock_num;
    store_item_increase(item, -amt);
    store_item_optimize(item);
    switch_store_stock(player_ptr, i, item);
}
