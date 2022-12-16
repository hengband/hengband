﻿/*!
 * @brief 店の処理 / Store commands
 * @date 2022/03/26
 * @author Hourier
 */

#include "store/store.h"
#include "core/asking-player.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-town.h"
#include "game-option/birth-options.h"
#include "game-option/game-play-options.h"
#include "io/command-repeater.h"
#include "locale/japanese.h"
#include "main/sound-of-music.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "object/tval-types.h"
#include "perception/identification.h"
#include "perception/object-perception.h"
#include "store/black-market.h"
#include "store/service-checker.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "util/quarks.h"
#include "view/display-messages.h"
#include "world/world-object.h"

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
int16_t store_get_stock_max(StoreSaleType sst, bool powerup)
{
    switch (sst) {
    case StoreSaleType::HOME:
        return powerup ? STORE_INVEN_MAX * 10 : STORE_INVEN_MAX;
    case StoreSaleType::MUSEUM:
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
static int check_free_space(StoreSaleType store_num)
{
    if ((store_num == StoreSaleType::HOME) && !powerup_home) {
        if (st_ptr->stock_num < ((st_ptr->stock_size) / 10)) {
            return 1;
        }
    } else if (st_ptr->stock_num < st_ptr->stock_size) {
        return 1;
    }

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
int store_check_num(ItemEntity *o_ptr, StoreSaleType store_num)
{
    ItemEntity *j_ptr;
    if ((store_num == StoreSaleType::HOME) || (store_num == StoreSaleType::MUSEUM)) {
        bool old_stack_force_notes = stack_force_notes;
        bool old_stack_force_costs = stack_force_costs;
        if (store_num != StoreSaleType::HOME) {
            stack_force_notes = false;
            stack_force_costs = false;
        }

        for (int i = 0; i < st_ptr->stock_num; i++) {
            j_ptr = &st_ptr->stock[i];
            if (!object_similar(j_ptr, o_ptr)) {
                continue;
            }

            if (store_num != StoreSaleType::HOME) {
                stack_force_notes = old_stack_force_notes;
                stack_force_costs = old_stack_force_costs;
            }

            return -1;
        }

        if (store_num != StoreSaleType::HOME) {
            stack_force_notes = old_stack_force_notes;
            stack_force_costs = old_stack_force_costs;
        }
    } else {
        for (int i = 0; i < st_ptr->stock_num; i++) {
            j_ptr = &st_ptr->stock[i];
            if (store_object_similar(j_ptr, o_ptr)) {
                return -1;
            }
        }
    }

    return check_free_space(store_num);
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
int get_stock(COMMAND_CODE *com_val, concptr pmt, int i, int j, [[maybe_unused]] StoreSaleType store_num)
{
    if (repeat_pull(com_val) && (*com_val >= i) && (*com_val <= j)) {
        return true;
    }

    msg_print(nullptr);
    *com_val = (-1);
    char lo = I2A(i);
    char hi = (j > 25) ? toupper(I2A(j - 26)) : I2A(j);
    char out_val[160];
#ifdef JP
    strnfmt(out_val, sizeof(out_val), "(%s:%c-%c, ESCで中断) %s", (((store_num == StoreSaleType::HOME) || (store_num == StoreSaleType::MUSEUM)) ? "アイテム" : "商品"), lo, hi, pmt);
#else
    strnfmt(out_val, sizeof(out_val), "(Items %c-%c, ESC to exit) %s", lo, hi, pmt);
#endif

    char command;
    while (true) {
        if (!get_com(out_val, &command, false)) {
            break;
        }

        COMMAND_CODE k;
        if (islower(command)) {
            k = A2I(command);
        } else if (isupper(command)) {
            k = A2I(tolower(command)) + 26;
        } else {
            k = -1;
        }

        if ((k >= i) && (k <= j)) {
            *com_val = k;
            break;
        }

        bell();
    }

    prt("", 0, 0);
    if (command == ESCAPE) {
        return false;
    }

    repeat_push(*com_val);
    return true;
}

/*!
 * @brief 店のアイテムを調べるコマンドのメインルーチン /
 * Examine an item in a store			   -JDL-
 */
void store_examine(PlayerType *player_ptr, StoreSaleType store_num)
{
    if (st_ptr->stock_num <= 0) {
        if (store_num == StoreSaleType::HOME) {
            msg_print(_("我が家には何も置いてありません。", "Your home is empty."));
        } else if (store_num == StoreSaleType::MUSEUM) {
            msg_print(_("博物館には何も置いてありません。", "The Museum is empty."));
        } else {
            msg_print(_("現在商品の在庫を切らしています。", "I am currently out of stock."));
        }
        return;
    }

    int i = (st_ptr->stock_num - store_top);
    if (i > store_bottom) {
        i = store_bottom;
    }

    COMMAND_CODE item;
    if (!get_stock(&item, _("どれを調べますか？", "Which item do you want to examine? "), 0, i - 1, store_num)) {
        return;
    }
    item = item + store_top;
    ItemEntity *o_ptr;
    o_ptr = &st_ptr->stock[item];
    if (!o_ptr->is_fully_known()) {
        msg_print(_("このアイテムについて特に知っていることはない。", "You have no special knowledge about that item."));
        return;
    }

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, 0);
    msg_format(_("%sを調べている...", "Examining %s..."), o_name);
    if (!screen_object(player_ptr, o_ptr, SCROBJ_FORCE_DETAIL)) {
        msg_print(_("特に変わったところはないようだ。", "You see nothing special."));
    }
}

/*!
 * @brief 現在の町の店主を交代させる /
 * Shuffle one of the stores.
 * @param which 店舗種類のID
 * @todo init_store()と処理を一部統合＆ランダム選択を改善。
 */
void store_shuffle(PlayerType *player_ptr, StoreSaleType store_num)
{
    auto owner_num = owners.at(store_num).size();
    if ((store_num == StoreSaleType::HOME) || (store_num == StoreSaleType::MUSEUM) || (owner_num <= (uint16_t)max_towns)) {
        return;
    }

    st_ptr = &town_info[player_ptr->town_num].store[enum2i(store_num)];
    int j = st_ptr->owner;
    while (true) {
        st_ptr->owner = (byte)randint0(owner_num);

        if (j == st_ptr->owner) {
            continue;
        }

        int i;
        for (i = 1; i < max_towns; i++) {
            if (i == player_ptr->town_num) {
                continue;
            }

            if (st_ptr->owner == town_info[i].store[enum2i(store_num)].owner) {
                break;
            }
        }

        if (i == max_towns) {
            break;
        }
    }

    ot_ptr = &owners.at(store_num)[st_ptr->owner];
    st_ptr->insult_cur = 0;
    st_ptr->store_open = 0;
    st_ptr->good_buy = 0;
    st_ptr->bad_buy = 0;
    for (int i = 0; i < st_ptr->stock_num; i++) {
        ItemEntity *o_ptr;
        o_ptr = &st_ptr->stock[i];
        if (o_ptr->is_artifact()) {
            continue;
        }

        o_ptr->discount = 50;
        o_ptr->inscription = quark_add(_("売出中", "on sale"));
    }
}

/*!
 * @brief 店舗の品揃え変化のためにアイテムを追加する /
 * Creates a random item and gives it to a store
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * <pre>
 * This algorithm needs to be rethought.  A lot.
 * Currently, "normal" stores use a pre-built array.
 * Note -- the "level" given to "obj_get_num()" is a "favored"
 * level, that is, there is a much higher chance of getting
 * items with a level approaching that of the given level...
 * Should we check for "permission" to have the given item?
 * </pre>
 */
static void store_create(PlayerType *player_ptr, short fix_k_idx, StoreSaleType store_num)
{
    if (st_ptr->stock_num >= st_ptr->stock_size) {
        return;
    }

    const owner_type *ow_ptr = &owners.at(store_num)[st_ptr->owner];

    for (int tries = 0; tries < 4; tries++) {
        short bi_id;
        DEPTH level;
        if (store_num == StoreSaleType::BLACK) {
            level = 25 + randint0(25);
            bi_id = get_obj_index(player_ptr, level, 0x00000000);
            if (bi_id == 0) {
                continue;
            }
        } else if (fix_k_idx > 0) {
            bi_id = fix_k_idx;
            level = rand_range(1, ow_ptr->level);
        } else {
            bi_id = st_ptr->table[randint0(st_ptr->table.size())];
            level = rand_range(1, ow_ptr->level);
        }

        ItemEntity forge;
        ItemEntity *q_ptr;
        q_ptr = &forge;
        q_ptr->prep(bi_id);
        ItemMagicApplier(player_ptr, q_ptr, level, AM_NO_FIXED_ART).execute();
        if (!store_will_buy(player_ptr, q_ptr, store_num)) {
            continue;
        }

        auto pvals = store_same_magic_device_pvals(q_ptr);
        if (pvals.size() >= 2) {
            auto pval = pvals.at(randint0(pvals.size()));
            q_ptr->pval = pval;
        }

        const auto tval = q_ptr->bi_key.tval();
        const auto sval = q_ptr->bi_key.sval();
        if (tval == ItemKindType::LITE) {
            if (sval == SV_LITE_TORCH) {
                q_ptr->fuel = FUEL_TORCH / 2;
            }

            if (sval == SV_LITE_LANTERN) {
                q_ptr->fuel = FUEL_LAMP / 2;
            }
        }

        object_known(q_ptr);
        q_ptr->ident |= IDENT_STORE;
        if (tval == ItemKindType::CHEST) {
            continue;
        }

        if (store_num == StoreSaleType::BLACK) {
            if (black_market_crap(player_ptr, q_ptr) || (q_ptr->get_price() < 10)) {
                continue;
            }
        } else {
            if (q_ptr->get_price() <= 0) {
                continue;
            }
        }

        mass_produce(q_ptr, store_num);
        (void)store_carry(q_ptr);
        break;
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
void store_maintenance(PlayerType *player_ptr, int town_num, StoreSaleType store_num, int chance)
{
    if ((store_num == StoreSaleType::HOME) || (store_num == StoreSaleType::MUSEUM)) {
        return;
    }

    st_ptr = &town_info[town_num].store[enum2i(store_num)];
    ot_ptr = &owners.at(store_num)[st_ptr->owner];
    st_ptr->insult_cur = 0;
    if (store_num == StoreSaleType::BLACK) {
        for (INVENTORY_IDX j = st_ptr->stock_num - 1; j >= 0; j--) {
            auto *o_ptr = &st_ptr->stock[j];
            if (black_market_crap(player_ptr, o_ptr)) {
                store_item_increase(j, 0 - o_ptr->number);
                store_item_optimize(j);
            }
        }
    }

    INVENTORY_IDX j = st_ptr->stock_num;
    int remain = STORE_TURNOVER + std::max(0, j - STORE_MAX_KEEP);
    int turn_over = 1;
    for (int i = 0; i < chance; i++) {
        auto n = randint0(remain);
        turn_over += n;
        remain -= n;
    }

    j = j - turn_over;
    if (j > STORE_MAX_KEEP) {
        j = STORE_MAX_KEEP;
    }
    if (j < STORE_MIN_KEEP) {
        j = STORE_MIN_KEEP;
    }

    while (st_ptr->stock_num > j) {
        store_delete();
    }

    remain = STORE_MAX_KEEP - st_ptr->stock_num;
    turn_over = 1;
    for (int i = 0; i < chance; i++) {
        auto n = randint0(remain);
        turn_over += n;
        remain -= n;
    }

    j = st_ptr->stock_num + turn_over;
    if (j > STORE_MAX_KEEP) {
        j = STORE_MAX_KEEP;
    }
    if (j < STORE_MIN_KEEP) {
        j = STORE_MIN_KEEP;
    }
    if (j >= st_ptr->stock_size) {
        j = st_ptr->stock_size - 1;
    }

    for (size_t k = 0; k < st_ptr->regular.size(); k++) {
        store_create(player_ptr, st_ptr->regular[k], store_num);
        if (st_ptr->stock_num >= STORE_MAX_KEEP) {
            break;
        }
    }

    while (st_ptr->stock_num < j) {
        store_create(player_ptr, 0, store_num);
    }
}

/*!
 * @brief 店舗情報を初期化する /
 * Initialize the stores
 * @param town_num 町のID
 * @param store_num 店舗種類のID
 */
void store_init(int town_num, StoreSaleType store_num)
{
    auto owner_num = owners.at(store_num).size();
    st_ptr = &town_info[town_num].store[enum2i(store_num)];
    while (true) {
        st_ptr->owner = (byte)randint0(owner_num);

        if (owner_num <= (uint16_t)max_towns) {
            break;
        }

        int i;

        for (i = 1; i < (uint16_t)max_towns; i++) {
            if (i == town_num) {
                continue;
            }
            if (st_ptr->owner == town_info[i].store[enum2i(store_num)].owner) {
                break;
            }
        }

        if (i == max_towns) {
            break;
        }
    }

    ot_ptr = &owners.at(store_num)[st_ptr->owner];
    st_ptr->store_open = 0;
    st_ptr->insult_cur = 0;
    st_ptr->good_buy = 0;
    st_ptr->bad_buy = 0;
    st_ptr->stock_num = 0;
    st_ptr->last_visit = -10L * TURNS_PER_TICK * STORE_TICKS;
    for (int k = 0; k < st_ptr->stock_size; k++) {
        (&st_ptr->stock[k])->wipe();
    }
}
