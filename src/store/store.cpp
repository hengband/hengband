/*!
 * @brief 店の処理 / Store commands
 * @date 2022/03/26
 * @author Hourier
 */

#include "store/store.h"
#include "core/asking-player.h"
#include "flavor/flavor-describer.h"
#include "game-option/birth-options.h"
#include "game-option/game-play-options.h"
#include "io/command-repeater.h"
#include "locale/japanese.h"
#include "main/sound-of-music.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "object/tval-types.h"
#include "perception/identification.h"
#include "store/articles-on-sale.h"
#include "store/black-market.h"
#include "store/service-checker.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "system/baseitem/baseitem-list.h"
#include "system/floor/floor-info.h"
#include "system/floor/town-list.h"
#include "system/inner-game-data.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <tl/optional.hpp>

int store_top = 0;
int store_bottom = 0;
int xtra_stock = 0;
size_t old_town_num = 0;
size_t inner_town_num = 0;

/* We store the current "store feat" here so everyone can access it */
short cur_store_feat;

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
    case StoreSaleType::BLACK:
        return STORE_INVEN_MAX * 50;
    case StoreSaleType::MUSEUM:
        return STORE_INVEN_MAX * 50;
    default:
        return STORE_INVEN_MAX * 3 / 2;
    }
}

/*!
 * @brief アイテムが格納可能な数より多いかをチェックする
 * @param store 判定する店舗
 * @return
 * 0 : No space
 * 1 : Cannot be combined but there are empty spaces.
 * @details オプション powerup_home が設定されていると我が家が 20 ページまで使える /
 * Free space is always usable
 */
static int check_free_space(const Store &store)
{
    const auto is_narrow_home = (store.get_sale_type() == StoreSaleType::HOME) && !powerup_home;
    const auto stock_limit = is_narrow_home ? (store.stock_size / 10) : store.stock_size;
    return store.stock_num < stock_limit ? 1 : 0;
}

/*!
 * @brief 店舗に品を置くスペースがあるかどうかの判定を返す /
 * Check to see if the shop will be carrying too many objects	-RAK-
 * @param o_ptr 店舗に置きたいオブジェクト構造体の参照ポインタ
 * @param store 置き先の店舗
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
int store_check_num(const ItemEntity *o_ptr, const Store &store)
{
    const auto store_num = store.get_sale_type();
    if ((store_num == StoreSaleType::HOME) || (store_num == StoreSaleType::MUSEUM)) {
        bool old_stack_force_notes = stack_force_notes;
        bool old_stack_force_costs = stack_force_costs;
        if (store_num != StoreSaleType::HOME) {
            stack_force_notes = false;
            stack_force_costs = false;
        }

        for (auto i = 0; i < store.stock_num; i++) {
            const auto &item = *store.stock[i];
            if (!item.is_similar(*o_ptr)) {
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
        for (auto i = 0; i < store.stock_num; i++) {
            const auto &item = *store.stock[i];
            if (item.is_similar_for_store(*o_ptr)) {
                return -1;
            }
        }
    }

    return check_free_space(store);
}

/*!
 * @brief 店舗のレベルを返す。日数で成長する。(現状BMのみ使用)
 * @param store_num 店舗の種類
 * @return 店舗レベル。BM以外1、BMは日数(MAX1000)
 */
int store_level(StoreSaleType store_num)
{
    if (store_num != StoreSaleType::BLACK) {
        return 1;
    }

    const auto &[day, hour, min] = AngbandWorld::get_instance().extract_date_time(InnerGameData::get_instance().get_start_race());
    return std::min(day, 1000);
}

/*!
 * @brief 店舗からアイテムを選択する
 * @param pmt メッセージキャプション
 * @param min 選択範囲の最小値
 * @param max 選択範囲の最大値
 * @return アイテムを選択したらそのインデックス ('a'等)、キャンセルしたらnullopt
 * 繰り返しコマンドの時は前回の前回のインデックス
 */
tl::optional<short> input_stock(std::string_view fmt, int min, int max, [[maybe_unused]] StoreSaleType store_num)
{
    const auto code = repeat_pull();
    if ((code >= min) && (code <= max)) {
        return code;
    }

    msg_erase();
    const auto lo = I2A(min);
    const auto hi = (max > 25) ? toupper(I2A(max - 26)) : I2A(max);
#ifdef JP
    const auto title = (store_num == StoreSaleType::HOME) || (store_num == StoreSaleType::MUSEUM) ? "アイテム" : "商品";
    const auto prompt = format("(%s:%c-%c, ESCで中断) %s", title, lo, hi, fmt.data());
#else
    const auto prompt = format("(Items %c-%c, ESC to exit) %s", lo, hi, fmt.data());
#endif

    tl::optional<char> command;
    while (true) {
        const auto command_alpha = input_command(prompt);
        if (!command_alpha) {
            break;
        }

        tl::optional<int> command_num;
        if (islower(*command_alpha)) {
            command_num = A2I(*command_alpha);
        } else if (isupper(*command_alpha)) {
            command_num = A2I(tolower(*command_alpha)) + 26;
        }

        if (command_num && (*command_num >= min) && (*command_num <= max)) {
            command = static_cast<short>(*command_num);
            break;
        }

        bell();
    }

    prt("", 0, 0);
    if (!command) {
        return tl::nullopt;
    }

    repeat_push(*command);
    return command;
}

/*!
 * @brief 店のアイテムを調べるコマンドのメインルーチン /
 * Examine an item in a store			   -JDL-
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param store 調べる店舗
 */
void store_examine(PlayerType *player_ptr, const Store &store)
{
    const auto store_num = store.get_sale_type();
    if (store.stock_num <= 0) {
        if (store_num == StoreSaleType::HOME) {
            msg_print(_("我が家には何も置いてありません。", "Your home is empty."));
        } else if (store_num == StoreSaleType::MUSEUM) {
            msg_print(_("博物館には何も置いてありません。", "The Museum is empty."));
        } else {
            msg_print(_("現在商品の在庫を切らしています。", "I am currently out of stock."));
        }
        return;
    }

    int i = (store.stock_num - store_top);
    if (i > store_bottom) {
        i = store_bottom;
    }

    constexpr auto mes = _("どれを調べますか？", "Which item do you want to examine? ");
    auto item_num_opt = input_stock(mes, 0, i - 1, store_num);
    if (!item_num_opt) {
        return;
    }

    const auto item_num = *item_num_opt + store_top;
    const auto &item = *store.stock[item_num];
    if (!item.is_fully_known()) {
        msg_print(_("このアイテムについて特に知っていることはない。", "You have no special knowledge about that item."));
        return;
    }

    const auto item_name = describe_flavor(player_ptr, item, 0);
    msg_format(_("%sを調べている...", "Examining %s..."), item_name.data());
    if (!screen_object(player_ptr, item, SCROBJ_FORCE_DETAIL)) {
        msg_print(_("特に変わったところはないようだ。", "You see nothing special."));
    }
}

/*!
 * @brief 現在の町の店主を交代させる /
 * Shuffle one of the stores.
 * @param which 店舗種類のID
 * @todo init_store()と処理を一部統合＆ランダム選択を改善。
 */
void store_shuffle(StoreSaleType store_num)
{
    auto &towns = TownList::get_instance();
    const auto towns_size = towns.size();
    const auto owner_num = owners.at(store_num).size();
    if ((store_num == StoreSaleType::HOME) || (store_num == StoreSaleType::MUSEUM) || (owner_num <= towns_size)) {
        return;
    }

    auto &world = AngbandWorld::get_instance();
    auto &store = world.get_town().get_store(store_num);
    int j = store.owner;
    while (true) {
        store.owner = randnum0<uint8_t>(owner_num);

        if (j == store.owner) {
            continue;
        }

        size_t i;
        for (i = 1; i < towns_size; i++) {
            if (i == world.get_town_index()) {
                continue;
            }

            if (store.owner == towns.get_town(i).get_store(store_num).owner) {
                break;
            }
        }

        if (i == towns_size) {
            break;
        }
    }

    store.insult_cur = 0;
    store.store_open = 0;
    store.good_buy = 0;
    store.bad_buy = 0;
    for (auto i = 0; i < store.stock_num; i++) {
        auto &item = *store.stock[i];
        if (item.is_fixed_or_random_artifact()) {
            continue;
        }

        item.discount = 50;
        item.inscription.emplace(_("売出中", "on sale"));
    }
}

/*!
 * @brief 店舗の品揃え変化のためにアイテムを追加する /
 * Creates a random item and gives it to a store
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param store アイテムを追加する店舗
 * @param fix_k_idx 追加するベースアイテムのID (0ならばランダムに選ぶ)
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
static void store_create(PlayerType *player_ptr, Store &store, short fix_k_idx)
{
    const auto store_num = store.get_sale_type();
    if (store.stock_num >= store.stock_size) {
        return;
    }

    const auto &world = AngbandWorld::get_instance();
    const int bm_boost = 25 + store_level(store_num) / 4;
    const auto &owner = store.get_owner();
    for (int tries = 0; tries < 4; tries++) {
        short bi_id;
        DEPTH level;
        if (store_num == StoreSaleType::BLACK) {
            level = bm_boost + randint0(25);
            level = std::min(128, level);
            bi_id = player_ptr->current_floor_ptr->select_baseitem_id(level, 0x00000000);
            if (bi_id == 0) {
                continue;
            }
        } else if (fix_k_idx > 0) {
            bi_id = fix_k_idx;
            level = rand_range(1, owner.level);
        } else {
            // svalの無いキーは、仕入れのたびにここで種類を選ぶ
            bi_id = BaseitemList::get_instance().lookup_baseitem_id(rand_choice(store_sale_table.at(store_num)));
            level = rand_range(1, owner.level);
        }

        ItemEntity item(bi_id);
        ItemMagicApplier(player_ptr, &item, level, AM_NO_FIXED_ART).execute();
        if (!store_will_buy(player_ptr, &item, store_num)) {
            continue;
        }

        const auto pvals = store.collect_same_magic_device_pvals(item);
        if (pvals.size() >= 2) {
            auto pval = rand_choice(pvals);
            item.pval = pval;
        }

        const auto tval = item.bi_key.tval();
        const auto sval = item.bi_key.sval();
        if (tval == ItemKindType::LITE) {
            if (sval == SV_LITE_TORCH) {
                item.fuel = FUEL_TORCH / 2;
            }

            if (sval == SV_LITE_LANTERN) {
                item.fuel = FUEL_LAMP / 2;
            }
        }

        item.mark_as_known();
        item.set_identification_flag(IdentificationFlag::STORE);
        if (tval == ItemKindType::CHEST) {
            continue;
        }

        if (store_num == StoreSaleType::BLACK) {
            if (black_market_crap(world.get_town_index(), item) || (item.calc_price() < 10)) {
                continue;
            }
        } else {
            if (item.calc_price() <= 0) {
                continue;
            }
        }

        mass_produce(&item, store_num);
        (void)store.carry(item);
        break;
    }
}

/*!
 * @brief 店の品揃えを変化させる /
 * Maintain the inventory at the stores.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param store 品揃えを変化させる店舗
 * @param chance 更新商品数
 */
void store_maintenance(PlayerType *player_ptr, Store &store, int chance)
{
    const auto store_num = store.get_sale_type();
    if ((store_num == StoreSaleType::HOME) || (store_num == StoreSaleType::MUSEUM)) {
        return;
    }

    store.insult_cur = 0;
    if (store_num == StoreSaleType::BLACK) {
        const auto &world = AngbandWorld::get_instance();
        for (INVENTORY_IDX j = store.stock_num - 1; j >= 0; j--) {
            auto &item = *store.stock[j];
            if (black_market_crap(world.get_town_index(), item)) {
                store.increase_item(j, 0 - item.number);
                store.optimize_item(j);
            }
        }
    }

    const int level = store_level(store_num);
    const short store_max_keep = (store_num == StoreSaleType::BLACK) ? STORE_MAX_KEEP * (level + 60) / 20 : STORE_MAX_KEEP;
    const short store_min_keep = (store_num == StoreSaleType::BLACK) ? STORE_MIN_KEEP * (level + 60) / 20 : STORE_MIN_KEEP;
    const short store_turnover = (store_num == StoreSaleType::BLACK) ? STORE_TURNOVER * (level + 60) / 20 : STORE_TURNOVER;
    chance = (store_num == StoreSaleType::BLACK) ? chance * (level + 60) / 20 : chance;

    auto j = store.stock_num;
    int remain = store_turnover + std::max(0, j - store_max_keep);
    int turn_over = 1;
    for (int i = 0; i < chance; i++) {
        auto n = randint0(remain);
        turn_over += n;
        remain -= n;
    }

    j = j - turn_over;
    if (j > store_max_keep) {
        j = store_max_keep;
    }
    if (j < store_min_keep) {
        j = store_min_keep;
    }

    while (store.stock_num > j) {
        store.delete_item();
    }

    remain = store_max_keep - store.stock_num;
    turn_over = 1;
    for (int i = 0; i < chance; i++) {
        auto n = randint0(remain);
        turn_over += n;
        remain -= n;
    }

    j = store.stock_num + turn_over;
    if (j > store_max_keep) {
        j = store_max_keep;
    }
    if (j < store_min_keep) {
        j = store_min_keep;
    }
    if (j >= store.stock_size) {
        j = store.stock_size - 1;
    }

    for (size_t k = 0; k < store.regular.size(); k++) {
        store_create(player_ptr, store, store.regular[k]);
        if (store.stock_num >= store_max_keep) {
            break;
        }
    }

    while (store.stock_num < j) {
        store_create(player_ptr, store, 0);
    }
}

/*!
 * @brief 店舗情報を初期化する
 * @param town_num 町のID
 * @param store_num 店舗種類のID
 */
void store_init(size_t town_num, StoreSaleType store_num)
{
    const auto owner_num = owners.at(store_num).size();
    auto &towns = TownList::get_instance();
    auto &store = towns.get_town(town_num).get_store(store_num);
    const auto towns_size = towns.size();
    while (true) {
        store.owner = randnum0<uint8_t>(owner_num);

        if (owner_num <= towns_size) {
            break;
        }

        size_t i;
        for (i = 1; i < towns_size; i++) {
            if (i == town_num) {
                continue;
            }
            if (store.owner == towns.get_town(i).get_store(store_num).owner) {
                break;
            }
        }

        if (i == towns_size) {
            break;
        }
    }

    store.store_open = 0;
    store.insult_cur = 0;
    store.good_buy = 0;
    store.bad_buy = 0;
    store.stock_num = 0;
    store.last_visit = -10L * TURNS_PER_TICK * STORE_TICKS;
    for (int k = 0; k < store.stock_size; k++) {
        store.stock[k]->wipe();
    }
}
