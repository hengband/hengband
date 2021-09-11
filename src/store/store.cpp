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
#include "core/asking-player.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-town.h"
#include "game-option/birth-options.h"
#include "game-option/game-play-options.h"
#include "io/command-repeater.h"
#include "main/sound-of-music.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-stack.h"
#include "perception/identification.h"
#include "perception/object-perception.h"
#include "store/black-market.h"
#include "store/service-checker.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "util/quarks.h"
#include "view/display-messages.h"
#ifdef JP
#include "locale/japanese.h"
#endif

int store_top = 0;
int store_bottom = 0;
int xtra_stock = 0;
const owner_type *ot_ptr = nullptr;
int16_t old_town_num = 0;
int16_t inner_town_num = 0;

/* We store the current "store feat" here so everyone can access it */
int cur_store_feat;

/* Enable "increments" */
bool allow_inc = false;

/*!
 * @brief 店舗の最大スロット数を返す
 * @param store_idx 店舗ID
 * @return 店舗の最大スロット数
 */
int16_t store_get_stock_max(STORE_TYPE_IDX store_idx, bool powerup)
{
    switch (store_idx) {
    case STORE_HOME:
        return powerup ? STORE_INVEN_MAX * 10 : STORE_INVEN_MAX;
    case STORE_MUSEUM:
        return STORE_INVEN_MAX * 50;
    default:
        return STORE_INVEN_MAX * 3 / 2;
    }
}

/*!
 * @brief アイテムが格納可能な数より多いかをチェックする
 * @param なし
 * @return 
 * 0 : No space
 * 1 : Cannot be combined but there are empty spaces.
 * @details オプション powerup_home が設定されていると我が家が 20 ページまで使える /
 * Free space is always usable
 */
static int check_free_space(void)
{
    if ((cur_store_num == STORE_HOME) && !powerup_home) {
        if (st_ptr->stock_num < ((st_ptr->stock_size) / 10))
            return 1;
    } else if (st_ptr->stock_num < st_ptr->stock_size)
        return 1;

    return 0;
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
            stack_force_notes = false;
            stack_force_costs = false;
        }

        for (int i = 0; i < st_ptr->stock_num; i++) {
            j_ptr = &st_ptr->stock[i];
            if (!object_similar(j_ptr, o_ptr))
                continue;

            if (cur_store_num != STORE_HOME) {
                stack_force_notes = old_stack_force_notes;
                stack_force_costs = old_stack_force_costs;
            }

            return -1;
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

    return check_free_space();
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
    if (repeat_pull(com_val) && (*com_val >= i) && (*com_val <= j))
        return true;

    msg_print(nullptr);
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
    while (true) {
        if (!get_com(out_val, &command, false))
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
        return false;

    repeat_push(*com_val);
    return true;
}

/*!
 * @brief 店のアイテムを調べるコマンドのメインルーチン /
 * Examine an item in a store			   -JDL-
 */
void store_examine(player_type *player_ptr)
{
    if (st_ptr->stock_num <= 0) {
        if (cur_store_num == STORE_HOME)
            msg_print(_("我が家には何も置いてありません。", "Your home is empty."));
        else if (cur_store_num == STORE_MUSEUM)
            msg_print(_("博物館には何も置いてありません。", "The Museum is empty."));
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
    if (!o_ptr->is_fully_known()) {
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
 * @brief 現在の町の店主を交代させる /
 * Shuffle one of the stores.
 * @param which 店舗種類のID
 */
void store_shuffle(player_type *player_ptr, int which)
{
    if ((which == STORE_HOME) || (which == STORE_MUSEUM))
        return;

    cur_store_num = which;
    st_ptr = &town_info[player_ptr->town_num].store[cur_store_num];
    int j = st_ptr->owner;
    while (true) {
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
        if (o_ptr->is_artifact())
            continue;

        o_ptr->discount = 50;
        o_ptr->inscription = quark_add(_("売出中", "on sale"));
    }
}

/*!
 * @brief 店の品揃えを変化させる /
 * Maintain the inventory at the stores.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param town_num 町のID
 * @param store_num 店舗種類のID
 * @param chance 更新商品数
 */
void store_maintenance(player_type *player_ptr, int town_num, int store_num, int chance)
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
    int remain = STORE_TURNOVER + MAX(0, j - STORE_MAX_KEEP);
    int turn_over = 1;
    for (int i = 0; i < chance; i++) {
        auto n = randint0(remain);
        turn_over += n;
        remain -= n;
    }

    j = j - turn_over;
    if (j > STORE_MAX_KEEP)
        j = STORE_MAX_KEEP;
    if (j < STORE_MIN_KEEP)
        j = STORE_MIN_KEEP;

    while (st_ptr->stock_num > j)
        store_delete();

    remain = STORE_MAX_KEEP - st_ptr->stock_num;
    turn_over = 1;
    for (int i = 0; i < chance; i++) {
        auto n = randint0(remain);
        turn_over += n;
        remain -= n;
    }

    j = st_ptr->stock_num + turn_over;
    if (j > STORE_MAX_KEEP)
        j = STORE_MAX_KEEP;
    if (j < STORE_MIN_KEEP)
        j = STORE_MIN_KEEP;
    if (j >= st_ptr->stock_size)
        j = st_ptr->stock_size - 1;

    for (size_t k = 0; k < st_ptr->regular.size(); k++) {
        store_create(player_ptr, st_ptr->regular[k], black_market_crap, store_will_buy, mass_produce);
        if (st_ptr->stock_num >= STORE_MAX_KEEP)
            break;
    }

    while (st_ptr->stock_num < j)
        store_create(player_ptr, 0, black_market_crap, store_will_buy, mass_produce);
}

/*!
 * @brief 店舗情報を初期化する /
 * Initialize the stores
 * @param town_num 町のID
 * @param store_num 店舗種類のID
 */
void store_init(int town_num, int store_num)
{
    cur_store_num = store_num;
    st_ptr = &town_info[town_num].store[store_num];
    while (true) {
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
    for (int k = 0; k < st_ptr->stock_size; k++)
        (&st_ptr->stock[k])->wipe();
}
