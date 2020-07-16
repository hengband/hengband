/*!
 * @brief 店の処理 / Store commands
 * @date 2014/02/02
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "store/store.h"
#include "action/weapon-shield.h"
#include "autopick/autopick-pref-processor.h"
#include "autopick/autopick.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-io/cmd-diary.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-io/cmd-gameoption.h"
#include "cmd-io/cmd-help.h"
#include "cmd-io/cmd-knowledge.h"
#include "cmd-io/cmd-lore.h"
#include "cmd-io/cmd-macro.h"
#include "cmd-io/cmd-process-screen.h"
#include "cmd-io/macro-util.h"
#include "cmd-item/cmd-item.h"
#include "cmd-item/cmd-magiceat.h"
#include "cmd-item/cmd-smith.h"
#include "cmd-item/cmd-zapwand.h"
#include "cmd-visual/cmd-draw.h"
#include "cmd-visual/cmd-visuals.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-events.h"
#include "floor/floor-object.h"
#include "floor/floor-town.h"
#include "floor/floor.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/game-play-options.h"
#include "game-option/input-options.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "grid/feature.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/command-repeater.h"
#include "io/input-key-requester.h"
#include "io/write-diary.h"
#include "main/music-definitions-table.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "market/gold-magnification-table.h"
#include "mind/mind-sniper.h"
#include "mind/mind.h"
#include "mind/racial-android.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-generator.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "perception/identification.h"
#include "perception/object-perception.h"
#include "player/avatar.h"
#include "player/player-class.h"
#include "player/player-status.h"
#include "player/race-info-table.h"
#include "spell-kind/spells-perception.h"
#include "store/black-market.h"
#include "store/home.h"
#include "store/rumor.h"
#include "store/say-comments.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/object-sort.h"
#include "util/quarks.h"
#include "view/display-messages.h"
#include "window/display-sub-windows.h"
#include "view/display-store.h" // todo 暫定、後で消す.
#include "view/object-describer.h"
#include "world/world.h"
#ifdef JP
#include "locale/japanese.h"
#endif

int store_top = 0;
int store_bottom = 0;
int xtra_stock = 0;
const owner_type *ot_ptr = NULL;
s16b old_town_num = 0;
s16b inner_town_num = 0;

/* We store the current "store feat" here so everyone can access it */
int cur_store_feat;

/* Enable "increments" */
bool allow_inc = FALSE;

/*!
 * @brief 店舗価格を決定する. 無料にはならない /
 * Determine the price of an item (qty one) in a store.
 * @param o_ptr 店舗に並べるオブジェクト構造体の参照ポインタ
 * @param greed 店主の強欲度
 * @param flip TRUEならば店主にとっての買取価格、FALSEなら売出価格を計算
 * @return アイテムの店舗価格
 * @details
 * <pre>
 * This function takes into account the player's charisma, and the
 * shop-keepers friendliness, and the shop-keeper's base greed, but
 * never lets a shop-keeper lose money in a transaction.
 * The "greed" value should exceed 100 when the player is "buying" the
 * item, and should be less than 100 when the player is "selling" it.
 * Hack -- the black market always charges twice as much as it should.
 * Charisma adjustment runs from 80 to 130
 * Racial adjustment runs from 95 to 130
 * Since greed/charisma/racial adjustments are centered at 100, we need
 * to adjust (by 200) to extract a usable multiplier.  Note that the
 * "greed" value is always something (?).
 * </pre>
 */
PRICE price_item(player_type *player_ptr, object_type *o_ptr, int greed, bool flip)
{
    PRICE price = object_value(player_ptr, o_ptr);
    if (price <= 0)
        return (0L);

    int factor = rgold_adj[ot_ptr->owner_race][player_ptr->prace];
    factor += adj_chr_gold[player_ptr->stat_ind[A_CHR]];
    int adjust;
    if (flip) {
        adjust = 100 + (300 - (greed + factor));
        if (adjust > 100)
            adjust = 100;
        if (cur_store_num == STORE_BLACK)
            price = price / 2;

        price = (price * adjust + 50L) / 100L;
    } else {
        adjust = 100 + ((greed + factor) - 300);
        if (adjust < 100)
            adjust = 100;
        if (cur_store_num == STORE_BLACK)
            price = price * 2;

        price = (s32b)(((u32b)price * (u32b)adjust + 50UL) / 100UL);
    }

    if (price <= 0L)
        return (1L);
    return price;
}

/*!
 * @brief 店舗に品を置くスペースがあるかどうかの判定を返す /
 * Check to see if the shop will be carrying too many objects	-RAK-
 * @param o_ptr 店舗に置きたいオブジェクト構造体の参照ポインタ
 * @return 置き場がないなら0、重ね合わせできるアイテムがあるなら-1、スペースがあるなら1を返す。
 * @details
 * <pre>
 * Note that the shop, just like a player, will not accept things
 * it cannot hold.	Before, one could "nuke" potions this way.
 * Return value is now int:
 *  0 : No space
 * -1 : Can be combined to existing slot.
 *  1 : Cannot be combined but there are empty spaces.
 * </pre>
 */
int store_check_num(object_type *o_ptr)
{
    object_type *j_ptr;
    if ((cur_store_num == STORE_HOME) || (cur_store_num == STORE_MUSEUM)) {
        bool old_stack_force_notes = stack_force_notes;
        bool old_stack_force_costs = stack_force_costs;
        if (cur_store_num != STORE_HOME) {
            stack_force_notes = FALSE;
            stack_force_costs = FALSE;
        }

        for (int i = 0; i < st_ptr->stock_num; i++) {
            j_ptr = &st_ptr->stock[i];
            if (object_similar(j_ptr, o_ptr)) {
                if (cur_store_num != STORE_HOME) {
                    stack_force_notes = old_stack_force_notes;
                    stack_force_costs = old_stack_force_costs;
                }

                return -1;
            }
        }

        if (cur_store_num != STORE_HOME) {
            stack_force_notes = old_stack_force_notes;
            stack_force_costs = old_stack_force_costs;
        }
    } else {
        for (int i = 0; i < st_ptr->stock_num; i++) {
            j_ptr = &st_ptr->stock[i];
            if (store_object_similar(j_ptr, o_ptr))
                return -1;
        }
    }

    /* Free space is always usable */
    /*
     * オプション powerup_home が設定されていると
     * 我が家が 20 ページまで使える
     */
    if ((cur_store_num == STORE_HOME) && (powerup_home == FALSE)) {
        if (st_ptr->stock_num < ((st_ptr->stock_size) / 10)) {
            return 1;
        }
    } else {
        if (st_ptr->stock_num < st_ptr->stock_size) {
            return 1;
        }
    }

    return 0;
}

/*!
 * @brief 店舗の割引対象外にするかどうかを判定 /
 * Eliminate need to bargain if player has haggled well in the past
 * @param minprice アイテムの最低販売価格
 * @return 割引を禁止するならTRUEを返す。
 */
bool noneedtobargain(PRICE minprice)
{
    PRICE good = st_ptr->good_buy;
    PRICE bad = st_ptr->bad_buy;
    if (minprice < 10L)
        return TRUE;
    if (good == MAX_SHORT)
        return TRUE;
    if (good > ((3 * bad) + (5 + (minprice / 50))))
        return TRUE;

    return FALSE;
}

/*!
 * @brief 店主の持つプレイヤーに対する売買の良し悪し経験を記憶する /
 * Update the bargain info
 * @param price 実際の取引価格
 * @param minprice 店主の提示した価格
 * @param num 売買数
 * @return なし
 */
void updatebargain(PRICE price, PRICE minprice, int num)
{
    if (!manual_haggle)
        return;
    if ((minprice / num) < 10L)
        return;
    if (price == minprice) {
        if (st_ptr->good_buy < MAX_SHORT) {
            st_ptr->good_buy++;
        }
    } else {
        if (st_ptr->bad_buy < MAX_SHORT) {
            st_ptr->bad_buy++;
        }
    }
}

/*!
 * @brief プレイヤーの所持金を表示する /
 * Displays players gold					-RAK-
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 */
void store_prt_gold(player_type *player_ptr)
{
    prt(_("手持ちのお金: ", "Gold Remaining: "), 19 + xtra_stock, 53);
    char out_val[64];
    sprintf(out_val, "%9ld", (long)player_ptr->au);
    prt(out_val, 19 + xtra_stock, 68);
}

/*!
 * @brief 店舗からアイテムを選択する /
 * Get the ID of a store item and return its value	-RAK-
 * @param com_val 選択IDを返す参照ポインタ
 * @param pmt メッセージキャプション
 * @param i 選択範囲の最小値
 * @param j 選択範囲の最大値
 * @return 実際に選択したらTRUE、キャンセルしたらFALSE
 */
int get_stock(COMMAND_CODE *com_val, concptr pmt, int i, int j)
{
    if (repeat_pull(com_val) && (*com_val >= i) && (*com_val <= j)) {
        return TRUE;
    }

    msg_print(NULL);
    *com_val = (-1);
    char lo = I2A(i);
    char hi = (j > 25) ? toupper(I2A(j - 26)) : I2A(j);
    char out_val[160];
#ifdef JP
    (void)sprintf(out_val, "(%s:%c-%c, ESCで中断) %s", (((cur_store_num == STORE_HOME) || (cur_store_num == STORE_MUSEUM)) ? "アイテム" : "商品"), lo, hi, pmt);
#else
    (void)sprintf(out_val, "(Items %c-%c, ESC to exit) %s", lo, hi, pmt);
#endif

    char command;
    while (TRUE) {
        if (!get_com(out_val, &command, FALSE))
            break;

        COMMAND_CODE k;
        if (islower(command))
            k = A2I(command);
        else if (isupper(command))
            k = A2I(tolower(command)) + 26;
        else
            k = -1;

        if ((k >= i) && (k <= j)) {
            *com_val = k;
            break;
        }

        bell();
    }

    prt("", 0, 0);
    if (command == ESCAPE)
        return FALSE;

    repeat_push(*com_val);
    return TRUE;
}

/*!
 * @brief 店主の不満度を増やし、プレイヤーを締め出す判定と処理を行う /
 * Increase the insult counter and get angry if too many -RAK-
 * @return プレイヤーを締め出す場合TRUEを返す
 */
int increase_insults(void)
{
    st_ptr->insult_cur++;
    if (st_ptr->insult_cur <= ot_ptr->insult_max)
        return FALSE;

    say_comment_4();

    st_ptr->insult_cur = 0;
    st_ptr->good_buy = 0;
    st_ptr->bad_buy = 0;
    st_ptr->store_open = current_world_ptr->game_turn + TURNS_PER_TICK * TOWN_DAWN / 8 + randint1(TURNS_PER_TICK * TOWN_DAWN / 8);

    return TRUE;
}

/*!
 * @brief 店主の不満度を減らす /
 * Decrease insults 				-RAK-
 * @return プレイヤーを締め出す場合TRUEを返す
 */
void decrease_insults(void)
{
    if (st_ptr->insult_cur)
        st_ptr->insult_cur--;
}

/*!
 * @brief 店主の不満度が増えた場合のみのメッセージを表示する /
 * Have insulted while haggling 			-RAK-
 * @return プレイヤーを締め出す場合TRUEを返す
 */
int haggle_insults(void)
{
    if (increase_insults())
        return TRUE;

    say_comment_5();
    return FALSE;
}

/*
 * Mega-Hack -- Last "increment" during haggling
 */
static s32b last_inc = 0L;

/*!
 * @brief 交渉価格を確認と認証の是非を行う /
 * Get a haggle
 * @param pmt メッセージ
 * @param poffer 別途価格提示をした場合の値を返す参照ポインタ
 * @param price 現在の交渉価格
 * @param final 最終確定価格ならばTRUE
 * @return プレイヤーを締め出す場合TRUEを返す
 */
static int get_haggle(concptr pmt, s32b *poffer, PRICE price, int final)
{
    GAME_TEXT buf[128];
    if (!allow_inc)
        last_inc = 0L;

    if (final) {
        sprintf(buf, _("%s [承諾] ", "%s [accept] "), pmt);
    } else if (last_inc < 0) {
        sprintf(buf, _("%s [-$%ld] ", "%s [-%ld] "), pmt, (long)(ABS(last_inc)));
    } else if (last_inc > 0) {
        sprintf(buf, _("%s [+$%ld] ", "%s [+%ld] "), pmt, (long)(ABS(last_inc)));
    } else {
        sprintf(buf, "%s ", pmt);
    }

    msg_print(NULL);
    GAME_TEXT out_val[160];
    while (TRUE) {
        bool res;
        prt(buf, 0, 0);
        strcpy(out_val, "");

        /*
         * Ask the user for a response.
         * Don't allow to use numpad as cursor key.
         */
        res = askfor_aux(out_val, 32, FALSE);
        prt("", 0, 0);
        if (!res)
            return FALSE;

        concptr p;
        for (p = out_val; *p == ' '; p++) /* loop */
            ;

        if (*p == '\0') {
            if (final) {
                *poffer = price;
                last_inc = 0L;
                break;
            }

            if (allow_inc && last_inc) {
                *poffer += last_inc;
                break;
            }

            msg_print(_("値がおかしいです。", "Invalid response."));
            msg_print(NULL);
            continue;
        }

        s32b i = atol(p);
        if ((*p == '+' || *p == '-')) {
            if (allow_inc) {
                *poffer += i;
                last_inc = i;
                break;
            }
        } else {
            *poffer = i;
            last_inc = 0L;
            break;
        }
    }

    return TRUE;
}

/*!
 * @brief 店主がプレイヤーからの交渉価格を判断する /
 * Receive an offer (from the player)
 * @param pmt メッセージ
 * @param poffer 店主からの交渉価格を返す参照ポインタ
 * @param last_offer 現在の交渉価格
 * @param factor 店主の価格基準倍率
 * @param price アイテムの実価値
 * @param final 最終価格確定ならばTRUE
 * @return プレイヤーの価格に対して不服ならばTRUEを返す /
 * Return TRUE if offer is NOT okay
 */
bool receive_offer(concptr pmt, s32b *poffer, s32b last_offer, int factor, PRICE price, int final)
{
    while (TRUE) {
        if (!get_haggle(pmt, poffer, price, final))
            return TRUE;
        if (((*poffer) * factor) >= (last_offer * factor))
            break;
        if (haggle_insults())
            return TRUE;

        (*poffer) = last_offer;
    }

    return FALSE;
}

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
    o_ptr = choose_object(owner_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
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

    /*
     * Hack -- If a rod or wand, allocate total maximum
     * timeouts or charges to those being sold. -LM-
     */
    if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND)) {
        q_ptr->pval = o_ptr->pval * amt / o_ptr->number;
    }

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(owner_ptr, o_name, q_ptr, 0);

    /* Remove any inscription, feeling for stores */
    if ((cur_store_num != STORE_HOME) && (cur_store_num != STORE_MUSEUM)) {
        q_ptr->inscription = 0;
        q_ptr->feeling = FEEL_NONE;
    }

    /* Is there room in the store (or the home?) */
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

            /*
             * Hack -- If a rod or wand, let the shopkeeper know just
             * how many charges he really paid for. -LM-
             */
            if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND)) {
                q_ptr->pval = o_ptr->pval * amt / o_ptr->number;
            }

            value = object_value(owner_ptr, q_ptr) * q_ptr->number;
            describe_flavor(owner_ptr, o_name, q_ptr, 0);
            msg_format(_("%sを $%ldで売却しました。", "You sold %s for %ld gold."), o_name, (long)price);

            if (record_sell)
                exe_write_diary(owner_ptr, DIARY_SELL, 0, o_name);

            if (!((o_ptr->tval == TV_FIGURINE) && (value > 0))) {
                purchase_analyze(owner_ptr, price, value, dummy);
            }

            /*
             * Hack -- Allocate charges between those wands or rods sold
             * and retained, unless all are being sold. -LM-
             */
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

        if (-1 == store_check_num(q_ptr)) {
            msg_print(_("それと同じ品物は既に博物館にあるようです。", "The Museum already has one of those items."));
        } else {
            msg_print(_("博物館に寄贈したものは取り出すことができません！！", "You cannot take back items which have been donated to the Museum!!"));
        }

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

/*!
 * @brief 店のアイテムを調べるコマンドのメインルーチン /
 * Examine an item in a store			   -JDL-
 * @return なし
 */
void store_examine(player_type *player_ptr)
{
    if (st_ptr->stock_num <= 0) {
        if (cur_store_num == STORE_HOME)
            msg_print(_("我が家には何も置いてありません。", "Your home is empty."));
        else if (cur_store_num == STORE_MUSEUM)
            msg_print(_("博物館には何も置いてありません。", "Museum is empty."));
        else
            msg_print(_("現在商品の在庫を切らしています。", "I am currently out of stock."));
        return;
    }

    int i = (st_ptr->stock_num - store_top);
    if (i > store_bottom)
        i = store_bottom;

    char out_val[160];
    sprintf(out_val, _("どれを調べますか？", "Which item do you want to examine? "));

    COMMAND_CODE item;
    if (!get_stock(&item, out_val, 0, i - 1))
        return;
    item = item + store_top;
    object_type *o_ptr;
    o_ptr = &st_ptr->stock[item];
    if (!object_is_fully_known(o_ptr)) {
        msg_print(_("このアイテムについて特に知っていることはない。", "You have no special knowledge about that item."));
        return;
    }

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, 0);
    msg_format(_("%sを調べている...", "Examining %s..."), o_name);

    if (!screen_object(player_ptr, o_ptr, SCROBJ_FORCE_DETAIL))
        msg_print(_("特に変わったところはないようだ。", "You see nothing special."));
}

/*!
 * @brief 博物館のアイテムを除去するコマンドのメインルーチン /
 * Remove an item from museum (Originally from TOband)
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void museum_remove_object(player_type *player_ptr)
{
    if (st_ptr->stock_num <= 0) {
        msg_print(_("博物館には何も置いてありません。", "Museum is empty."));
        return;
    }

    int i = st_ptr->stock_num - store_top;
    if (i > store_bottom)
        i = store_bottom;

    char out_val[160];
    sprintf(out_val, _("どのアイテムの展示をやめさせますか？", "Which item do you want to order to remove? "));

    COMMAND_CODE item;
    if (!get_stock(&item, out_val, 0, i - 1))
        return;

    item = item + store_top;
    object_type *o_ptr;
    o_ptr = &st_ptr->stock[item];

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, 0);

    msg_print(_("展示をやめさせたアイテムは二度と見ることはできません！", "Once removed from the Museum, an item will be gone forever!"));
    if (!get_check(format(_("本当に%sの展示をやめさせますか？", "Really order to remove %s from the Museum? "), o_name)))
        return;

    msg_format(_("%sの展示をやめさせた。", "You ordered to remove %s."), o_name);

    store_item_increase(item, -o_ptr->number);
    store_item_optimize(item);

    (void)combine_and_reorder_home(player_ptr, STORE_MUSEUM);
    if (st_ptr->stock_num == 0)
        store_top = 0;

    else if (store_top >= st_ptr->stock_num)
        store_top -= store_bottom;
    display_store_inventory(player_ptr);
}

/*!
 * @brief 現在の町の店主を交代させる /
 * Shuffle one of the stores.
 * @param which 店舗種類のID
 * @return なし
 */
void store_shuffle(player_type *player_ptr, int which)
{
    if (which == STORE_HOME)
        return;
    if (which == STORE_MUSEUM)
        return;

    cur_store_num = which;
    st_ptr = &town_info[player_ptr->town_num].store[cur_store_num];
    int j = st_ptr->owner;
    while (TRUE) {
        st_ptr->owner = (byte)randint0(MAX_OWNERS);
        if (j == st_ptr->owner)
            continue;
        int i;
        for (i = 1; i < max_towns; i++) {
            if (i == player_ptr->town_num)
                continue;
            if (st_ptr->owner == town_info[i].store[cur_store_num].owner)
                break;
        }

        if (i == max_towns)
            break;
    }

    ot_ptr = &owners[cur_store_num][st_ptr->owner];
    st_ptr->insult_cur = 0;
    st_ptr->store_open = 0;
    st_ptr->good_buy = 0;
    st_ptr->bad_buy = 0;
    for (int i = 0; i < st_ptr->stock_num; i++) {
        object_type *o_ptr;
        o_ptr = &st_ptr->stock[i];
        if (object_is_artifact(o_ptr))
            continue;

        o_ptr->discount = 50;
        o_ptr->ident &= ~(IDENT_FIXED);
        o_ptr->inscription = quark_add(_("売出中", "on sale"));
    }
}

/*!
 * @brief 店の品揃えを変化させる /
 * Maintain the inventory at the stores.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param town_num 町のID
 * @param store_num 店舗種類のID
 * @return なし
 */
void store_maintenance(player_type *player_ptr, int town_num, int store_num)
{
    cur_store_num = store_num;
    if ((store_num == STORE_HOME) || (store_num == STORE_MUSEUM))
        return;

    st_ptr = &town_info[town_num].store[store_num];
    ot_ptr = &owners[store_num][st_ptr->owner];
    st_ptr->insult_cur = 0;
    if (store_num == STORE_BLACK) {
        for (INVENTORY_IDX j = st_ptr->stock_num - 1; j >= 0; j--) {
            object_type *o_ptr = &st_ptr->stock[j];
            if (black_market_crap(player_ptr, o_ptr)) {
                store_item_increase(j, 0 - o_ptr->number);
                store_item_optimize(j);
            }
        }
    }

    INVENTORY_IDX j = st_ptr->stock_num;
    j = j - randint1(STORE_TURNOVER);
    if (j > STORE_MAX_KEEP)
        j = STORE_MAX_KEEP;
    if (j < STORE_MIN_KEEP)
        j = STORE_MIN_KEEP;
    if (j < 0)
        j = 0;

    while (st_ptr->stock_num > j)
        store_delete();

    j = st_ptr->stock_num;
    j = j + randint1(STORE_TURNOVER);
    if (j > STORE_MAX_KEEP)
        j = STORE_MAX_KEEP;
    if (j < STORE_MIN_KEEP)
        j = STORE_MIN_KEEP;
    if (j >= st_ptr->stock_size)
        j = st_ptr->stock_size - 1;

    while (st_ptr->stock_num < j)
        store_create(player_ptr, black_market_crap);
}

/*!
 * @brief 店舗情報を初期化する /
 * Initialize the stores
 * @param town_num 町のID
 * @param store_num 店舗種類のID
 * @return なし
 */
void store_init(int town_num, int store_num)
{
    cur_store_num = store_num;
    st_ptr = &town_info[town_num].store[store_num];
    while (TRUE) {
        st_ptr->owner = (byte)randint0(MAX_OWNERS);
        int i;
        for (i = 1; i < max_towns; i++) {
            if (i == town_num)
                continue;
            if (st_ptr->owner == town_info[i].store[store_num].owner)
                break;
        }

        if (i == max_towns)
            break;
    }

    ot_ptr = &owners[store_num][st_ptr->owner];

    st_ptr->store_open = 0;
    st_ptr->insult_cur = 0;
    st_ptr->good_buy = 0;
    st_ptr->bad_buy = 0;
    st_ptr->stock_num = 0;
    st_ptr->last_visit = -10L * TURNS_PER_TICK * STORE_TICKS;
    for (int k = 0; k < st_ptr->stock_size; k++) {
        object_wipe(&st_ptr->stock[k]);
    }
}
