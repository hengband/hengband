#include "store/sell-order.h"
#include "action/weapon-shield.h"
#include "autopick/autopick.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
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
#include "object-hook/hook-checker.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-generator.h"
#include "object/object-info.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "player-info/avatar.h"
#include "racial/racial-android.h"
#include "spell-kind/spells-perception.h"
#include "store/home.h"
#include "store/owner-insults.h"
#include "store/pricing.h"
#include "store/say-comments.h"
#include "store/service-checker.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/object-type-definition.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"
#include "view/display-store.h"
#include "view/object-describer.h"
#include "world/world.h"

/*!
 * @brief プレイヤーが売却する時の値切り処理メインルーチン /
 * Haggling routine 				-RAK-
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr オブジェクトの構造体参照ポインタ
 * @param price 最終価格を返す参照ポインタ
 * @return プレイヤーの価格に対して店主が不服ならばTRUEを返す /
 * Return TRUE if purchase is NOT successful
 */
static bool sell_haggle(player_type *player_ptr, object_type *o_ptr, s32b *price)
{
    s32b cur_ask = price_item(player_ptr, o_ptr, ot_ptr->max_inflate, TRUE);
    s32b final_ask = price_item(player_ptr, o_ptr, ot_ptr->min_inflate, TRUE);
    int noneed = noneedtobargain(final_ask);
    s32b purse = (s32b)(ot_ptr->max_cost);
    bool final = FALSE;
    concptr pmt = _("提示金額", "Offer");
    if (noneed || !manual_haggle || (final_ask >= purse)) {
        if (!manual_haggle && !noneed) {
            final_ask -= final_ask / 10;
        }

        if (final_ask >= purse) {
            msg_print(_("即座にこの金額にまとまった。", "You instantly agree upon the price."));
            msg_print(NULL);
            final_ask = purse;
        } else if (noneed) {
            msg_print(_("結局この金額にまとまった。", "You eventually agree upon the price."));
            msg_print(NULL);
        } else {
            msg_print(_("すんなりとこの金額にまとまった。", "You quickly agree upon the price."));
            msg_print(NULL);
        }

        cur_ask = final_ask;
        final = TRUE;
        pmt = _("最終提示金額", "Final Offer");
    }

    cur_ask *= o_ptr->number;
    final_ask *= o_ptr->number;

    s32b min_per = ot_ptr->haggle_per;
    s32b max_per = min_per * 3;
    s32b last_offer = object_value(player_ptr, o_ptr) * o_ptr->number;
    last_offer = last_offer * ot_ptr->max_inflate / 100L;
    s32b offer = 0;
    allow_inc = FALSE;
    bool flag = FALSE;
    bool loop_flag;
    int annoyed = 0;
    bool cancel = FALSE;
    *price = 0;
    while (!flag) {
        while (TRUE) {
            loop_flag = TRUE;

            char out_val[160];
            (void)sprintf(out_val, "%s :  %ld", pmt, (long)cur_ask);
            put_str(out_val, 1, 0);
            cancel = receive_offer(_("提示する価格? ", "What price do you ask? "), &offer, last_offer, -1, cur_ask, final);

            if (cancel) {
                flag = TRUE;
            } else if (offer < cur_ask) {
                say_comment_6();
                offer = last_offer;
            } else if (offer == cur_ask) {
                flag = TRUE;
                *price = offer;
            } else {
                loop_flag = FALSE;
            }

            if (flag || !loop_flag)
                break;
        }

        if (flag)
            continue;

        s32b x1 = 100 * (last_offer - offer) / (last_offer - cur_ask);
        if (x1 < min_per) {
            if (haggle_insults()) {
                flag = TRUE;
                cancel = TRUE;
            }
        } else if (x1 > max_per) {
            x1 = x1 * 3 / 4;
            if (x1 < max_per)
                x1 = max_per;
        }

        s32b x2 = rand_range(x1 - 2, x1 + 2);
        s32b x3 = ((offer - cur_ask) * x2 / 100L) + 1;
        if (x3 < 0)
            x3 = 0;
        cur_ask += x3;

        if (cur_ask > final_ask) {
            cur_ask = final_ask;
            final = TRUE;
            pmt = _("最終提示金額", "Final Offer");

            annoyed++;
            if (annoyed > 3) {
                flag = TRUE;
#ifdef JP
                /* 追加 $0 で買い取られてしまうのを防止 By FIRST*/
                cancel = TRUE;
#endif
                (void)(increase_insults());
            }
        } else if (offer <= cur_ask) {
            flag = TRUE;
            *price = offer;
        }

        last_offer = offer;
        allow_inc = TRUE;
        prt("", 1, 0);
        char out_val[160];
        (void)sprintf(out_val, _("前回の提示価格 $%ld", "Your last bid %ld"), (long)last_offer);
        put_str(out_val, 1, 39);
        say_comment_3(cur_ask, annoyed);
    }

    if (cancel)
        return TRUE;

    updatebargain(*price, final_ask, o_ptr->number);
    return FALSE;
}

/*!
 * @brief 店からの売却処理のメインルーチン /
 * Sell an item to the store (or home)
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void store_sell(player_type *owner_ptr)
{
    concptr q;
    if (cur_store_num == STORE_HOME)
        q = _("どのアイテムを置きますか? ", "Drop which item? ");
    else if (cur_store_num == STORE_MUSEUM)
        q = _("どのアイテムを寄贈しますか? ", "Give which item? ");
    else
        q = _("どのアイテムを売りますか? ", "Sell which item? ");

    item_tester_hook = store_will_buy;

    /* 我が家でおかしなメッセージが出るオリジナルのバグを修正 */
    concptr s;
    if (cur_store_num == STORE_HOME) {
        s = _("置けるアイテムを持っていません。", "You don't have any item to drop.");
    } else if (cur_store_num == STORE_MUSEUM) {
        s = _("寄贈できるアイテムを持っていません。", "You don't have any item to give.");
    } else {
        s = _("欲しい物がないですねえ。", "You have nothing that I want.");
    }

    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(owner_ptr, &item, q, s, USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT, 0);
    if (!o_ptr)
        return;

    if ((item >= INVEN_RARM) && object_is_cursed(o_ptr)) {
        msg_print(_("ふーむ、どうやらそれは呪われているようだね。", "Hmmm, it seems to be cursed."));
        return;
    }

    int amt = 1;
    if (o_ptr->number > 1) {
        amt = get_quantity(NULL, o_ptr->number);
        if (amt <= 0)
            return;
    }

    object_type forge;
    object_type *q_ptr;
    q_ptr = &forge;
    object_copy(q_ptr, o_ptr);
    q_ptr->number = amt;
    if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND))
        q_ptr->pval = o_ptr->pval * amt / o_ptr->number;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(owner_ptr, o_name, q_ptr, 0);
    if ((cur_store_num != STORE_HOME) && (cur_store_num != STORE_MUSEUM)) {
        q_ptr->inscription = 0;
        q_ptr->feeling = FEEL_NONE;
    }

    if (!store_check_num(q_ptr)) {
        if (cur_store_num == STORE_HOME)
            msg_print(_("我が家にはもう置く場所がない。", "Your home is full."));

        else if (cur_store_num == STORE_MUSEUM)
            msg_print(_("博物館はもう満杯だ。", "Museum is full."));

        else
            msg_print(_("すいませんが、店にはもう置く場所がありません。", "I have not the room in my store to keep it."));

        return;
    }

    int choice;
    PRICE price, value, dummy;
    if ((cur_store_num != STORE_HOME) && (cur_store_num != STORE_MUSEUM)) {
        msg_format(_("%s(%c)を売却する。", "Selling %s (%c)."), o_name, index_to_label(item));
        msg_print(NULL);

        choice = sell_haggle(owner_ptr, q_ptr, &price);
        if (st_ptr->store_open >= current_world_ptr->game_turn)
            return;

        if (choice == 0) {
            say_comment_1(owner_ptr);
            sound(SOUND_SELL);
            if (cur_store_num == STORE_BLACK)
                chg_virtue(owner_ptr, V_JUSTICE, -1);

            if ((o_ptr->tval == TV_BOTTLE) && (cur_store_num != STORE_HOME))
                chg_virtue(owner_ptr, V_NATURE, 1);
            decrease_insults();

            owner_ptr->au += price;
            store_prt_gold(owner_ptr);
            dummy = object_value(owner_ptr, q_ptr) * q_ptr->number;

            identify_item(owner_ptr, o_ptr);
            q_ptr = &forge;
            object_copy(q_ptr, o_ptr);
            q_ptr->number = amt;
            q_ptr->ident |= IDENT_STORE;
            if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND))
                q_ptr->pval = o_ptr->pval * amt / o_ptr->number;

            value = object_value(owner_ptr, q_ptr) * q_ptr->number;
            describe_flavor(owner_ptr, o_name, q_ptr, 0);
            msg_format(_("%sを $%ldで売却しました。", "You sold %s for %ld gold."), o_name, (long)price);

            if (record_sell)
                exe_write_diary(owner_ptr, DIARY_SELL, 0, o_name);

            if (!((o_ptr->tval == TV_FIGURINE) && (value > 0)))
                purchase_analyze(owner_ptr, price, value, dummy);

            distribute_charges(o_ptr, q_ptr, amt);
            q_ptr->timeout = 0;
            inven_item_increase(owner_ptr, item, -amt);
            inven_item_describe(owner_ptr, item);
            if (o_ptr->number > 0)
                autopick_alter_item(owner_ptr, item, FALSE);

            inven_item_optimize(owner_ptr, item);
            handle_stuff(owner_ptr);
            int item_pos = store_carry(owner_ptr, q_ptr);
            if (item_pos >= 0) {
                store_top = (item_pos / store_bottom) * store_bottom;
                display_store_inventory(owner_ptr);
            }
        }
    } else if (cur_store_num == STORE_MUSEUM) {
        char o2_name[MAX_NLEN];
        describe_flavor(owner_ptr, o2_name, q_ptr, OD_NAME_ONLY);

        if (-1 == store_check_num(q_ptr))
            msg_print(_("それと同じ品物は既に博物館にあるようです。", "The Museum already has one of those items."));
        else
            msg_print(_("博物館に寄贈したものは取り出すことができません！！", "You cannot take back items which have been donated to the Museum!!"));
        
        if (!get_check(format(_("本当に%sを寄贈しますか？", "Really give %s to the Museum? "), o2_name)))
            return;

        identify_item(owner_ptr, q_ptr);
        q_ptr->ident |= IDENT_FULL_KNOWN;

        distribute_charges(o_ptr, q_ptr, amt);
        msg_format(_("%sを置いた。(%c)", "You drop %s (%c)."), o_name, index_to_label(item));
        choice = 0;

        vary_item(owner_ptr, item, -amt);
        handle_stuff(owner_ptr);

        int item_pos = home_carry(owner_ptr, q_ptr);
        if (item_pos >= 0) {
            store_top = (item_pos / store_bottom) * store_bottom;
            display_store_inventory(owner_ptr);
        }
    } else {
        distribute_charges(o_ptr, q_ptr, amt);
        msg_format(_("%sを置いた。(%c)", "You drop %s (%c)."), o_name, index_to_label(item));
        choice = 0;
        vary_item(owner_ptr, item, -amt);
        handle_stuff(owner_ptr);
        int item_pos = home_carry(owner_ptr, q_ptr);
        if (item_pos >= 0) {
            store_top = (item_pos / store_bottom) * store_bottom;
            display_store_inventory(owner_ptr);
        }
    }

    if ((choice == 0) && (item >= INVEN_RARM)) {
        calc_android_exp(owner_ptr);
        verify_equip_slot(owner_ptr, item);
    }
}
