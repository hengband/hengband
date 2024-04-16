#include "market/bounty.h"
#include "autopick/autopick.h"
#include "avatar/avatar.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "game-option/cheat-options.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "market/bounty-prize-table.h"
#include "market/building-util.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "object/object-info.h"
#include "perception/object-perception.h"
#include "sv-definition/sv-other-types.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>

/*!
 * @brief 賞金首の引き換え処理 / Get prize
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 各種賞金首のいずれかでも換金が行われたか否か。
 */
bool exchange_cash(PlayerType *player_ptr)
{
    auto change = false;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    constexpr auto fmt_convert = _("%s を換金しますか？", "Convert %s into money? ");
    constexpr auto fmt_reward = _("賞金 %d＄を手に入れた。", "You get %dgp.");
    for (INVENTORY_IDX i = 0; i <= INVEN_SUB_HAND; i++) {
        const auto &item = player_ptr->inventory_list[i];
        if (item.bi_key.tval() != ItemKindType::CAPTURE) {
            continue;
        }

        if (item.get_monrace().idx != MonsterRaceId::TSUCHINOKO) {
            continue;
        }

        change = true;
        const auto item_name = describe_flavor(player_ptr, &item, 0);
        if (!input_check(format(fmt_convert, item_name.data()))) {
            continue;
        }

        const auto reward_money = 1000000 * item.number;
        msg_format(fmt_reward, reward_money);
        player_ptr->au += reward_money;
        rfu.set_flag(MainWindowRedrawingFlag::GOLD);
        vary_item(player_ptr, i, -item.number);
    }

    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        const auto &item = player_ptr->inventory_list[i];
        if (!item.is_corpse()) {
            continue;
        }

        if (item.get_monrace().idx != MonsterRaceId::TSUCHINOKO) {
            continue;
        }

        change = true;
        const auto item_name = describe_flavor(player_ptr, &item, 0);
        if (!input_check(format(fmt_convert, item_name.data()))) {
            continue;
        }

        const auto reward_money = 200000 * item.number;
        msg_format(fmt_reward, reward_money);
        player_ptr->au += reward_money;
        rfu.set_flag(MainWindowRedrawingFlag::GOLD);
        vary_item(player_ptr, i, -item.number);
    }

    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        const auto &item = player_ptr->inventory_list[i];
        if (item.bi_key != BaseitemKey(ItemKindType::MONSTER_REMAINS, SV_SKELETON)) {
            continue;
        }

        if (item.get_monrace().idx != MonsterRaceId::TSUCHINOKO) {
            continue;
        }

        change = true;
        const auto item_name = describe_flavor(player_ptr, &item, 0);
        if (!input_check(format(fmt_convert, item_name.data()))) {
            continue;
        }

        const auto reward_money = 100000 * item.number;
        msg_format(fmt_reward, reward_money);
        player_ptr->au += reward_money;
        rfu.set_flag(MainWindowRedrawingFlag::GOLD);
        vary_item(player_ptr, i, -item.number);
    }

    auto &world = AngbandWorld::get_instance();
    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        const auto &item = player_ptr->inventory_list[i];
        const auto &monrace = world.get_today_bounty();
        if (!item.is_corpse() || (item.get_monrace().name != monrace.name)) {
            continue;
        }

        change = true;
        const auto item_name = describe_flavor(player_ptr, &item, 0);
        if (!input_check(format(fmt_convert, item_name.data()))) {
            continue;
        }

        const auto reward_money = (monrace.level * 50 + 100) * item.number;
        msg_format(fmt_reward, reward_money);
        player_ptr->au += reward_money;
        rfu.set_flag(MainWindowRedrawingFlag::GOLD);
        vary_item(player_ptr, i, -item.number);
    }

    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        const auto &item = player_ptr->inventory_list[i];
        const auto &monrace = world.get_today_bounty();
        if ((item.bi_key != BaseitemKey(ItemKindType::MONSTER_REMAINS, SV_SKELETON)) || (item.get_monrace().name != monrace.name)) {
            continue;
        }

        change = true;
        const auto item_name = describe_flavor(player_ptr, &item, 0);
        if (!input_check(format(fmt_convert, item_name.data()))) {
            continue;
        }

        const auto reward_money = (monrace.level * 30 + 60) * item.number;
        msg_format(fmt_reward, reward_money);
        player_ptr->au += reward_money;
        rfu.set_flag(MainWindowRedrawingFlag::GOLD);
        vary_item(player_ptr, i, -item.number);
    }

    for (auto &[monrace_id, is_achieved] : world.bounties) {
        if (is_achieved) {
            continue;
        }

        for (INVENTORY_IDX i = INVEN_PACK - 1; i >= 0; i--) {
            auto &item = player_ptr->inventory_list[i];
            if ((item.bi_key.tval() != ItemKindType::MONSTER_REMAINS) || (item.get_monrace().idx != monrace_id)) {
                continue;
            }

            INVENTORY_IDX inventory_new;
            const auto item_name = describe_flavor(player_ptr, &item, 0);
            if (!input_check(format(_("%sを渡しますか？", "Hand %s over? "), item_name.data()))) {
                continue;
            }

            vary_item(player_ptr, i, -item.number);
            chg_virtue(player_ptr, Virtue::JUSTICE, 5);
            is_achieved = true;

            const auto num = static_cast<int>(std::count_if(std::begin(world.bounties), std::end(world.bounties),
                [](const auto &bounty) { return bounty.is_achieved; }));

            msg_print(_(format("これで合計 %d ポイント獲得しました。", num), format("You earned %d point%s total.", num, (num > 1 ? "s" : ""))));

            ItemEntity prize_item(prize_list[num - 1]);
            ItemMagicApplier(player_ptr, &prize_item, player_ptr->current_floor_ptr->object_level, AM_NO_FIXED_ART).execute();
            object_aware(player_ptr, &prize_item);
            prize_item.mark_as_known();

            /*
             * Hand it --- Assume there is an empty slot.
             * Since a corpse is handed at first,
             * there is at least one empty slot.
             */
            inventory_new = store_item_to_inventory(player_ptr, &prize_item);
            const auto got_item_name = describe_flavor(player_ptr, &prize_item, 0);
            msg_format(_("%s(%c)を貰った。", "You get %s (%c). "), got_item_name.data(), index_to_label(inventory_new));

            autopick_alter_item(player_ptr, inventory_new, false);
            handle_stuff(player_ptr);
            change = true;
        }
    }

    if (change) {
        return true;
    }

    msg_print(_("賞金を得られそうなものは持っていなかった。", "You have nothing."));
    msg_print(nullptr);
    return false;
}

/*!
 * @brief 本日の賞金首情報を表示する。
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void today_target()
{
    auto &world = AngbandWorld::get_instance();
    const auto &monrace = world.get_today_bounty();
    clear_bldg(4, 18);
    c_put_str(TERM_YELLOW, _("本日の賞金首", "Wanted monster that changes from day to day"), 5, 10);
    c_put_str(TERM_YELLOW, format(_("ターゲット： %s", "target: %s"), monrace.name.data()), 6, 10);
    prt(format(_("死体 ---- $%d", "corpse   ---- $%d"), monrace.level * 50 + 100), 8, 10);
    prt(format(_("骨   ---- $%d", "skeleton ---- $%d"), monrace.level * 30 + 60), 9, 10);
    world.knows_daily_bounty = true;
}

/*!
 * @brief ツチノコの賞金首情報を表示する。
 */
void tsuchinoko(void)
{
    clear_bldg(4, 18);
    c_put_str(TERM_YELLOW, _("一獲千金の大チャンス！！！", "Big chance for quick money!!!"), 5, 10);
    c_put_str(TERM_YELLOW, _("ターゲット：幻の珍獣「ツチノコ」", "target: the rarest animal 'Tsuchinoko'"), 6, 10);
    c_put_str(TERM_WHITE, _("生け捕り ---- $1,000,000", "catch alive ---- $1,000,000"), 8, 10);
    c_put_str(TERM_WHITE, _("死体     ----   $200,000", "corpse      ----   $200,000"), 9, 10);
    c_put_str(TERM_WHITE, _("骨       ----   $100,000", "bones       ----   $100,000"), 10, 10);
}

/*!
 * @brief 通常の賞金首情報を表示する。
 */
void show_bounty(void)
{
    clear_bldg(4, 18);
    prt(_("死体を持ち帰れば報酬を差し上げます。", "Offer a prize when you bring a wanted monster's corpse"), 4, 10);
    c_put_str(TERM_YELLOW, _("現在の賞金首", "Wanted monsters"), 6, 10);
    const auto &monraces = MonraceList::get_instance();
    const auto &world = AngbandWorld::get_instance();
    const int size_bounties = std::ssize(world.bounties);
    auto y = 0;
    for (auto i = 0; i < size_bounties; i++) {
        const auto &[monrace_id, is_achieved] = world.bounties[i];
        const auto &monrace = monraces.get_monrace(monrace_id);
        const auto color = is_achieved ? TERM_RED : TERM_WHITE;
        const auto done_mark = is_achieved ? _("(済)", "(done)") : "";
        c_prt(color, format("%s %s", monrace.name.data(), done_mark), y + 7, 10);
        y = (y + 1) % 10;
        if (!y && (i < size_bounties - 1)) {
            prt(_("何かキーを押してください", "Hit any key."), 0, 0);
            (void)inkey();
            prt("", 0, 0);
            clear_bldg(7, 18);
        }
    }
}

/*!
 * @brief 今日の賞金首を確定する / Determine today's bounty monster
 * @param PlayerType プレイヤーへの参照ポインタ
 * @note conv_old is used if loaded 0.0.3 or older save file
 */
void determine_daily_bounty(PlayerType *player_ptr, bool conv_old)
{
    auto max_dl = 3;
    if (!conv_old) {
        for (const auto &dungeon : dungeons_info) {
            if (max_dlv[dungeon.idx] < dungeon.mindepth) {
                continue;
            }

            if (max_dl < max_dlv[dungeon.idx]) {
                max_dl = max_dlv[dungeon.idx];
            }
        }
    } else {
        max_dl = std::max(max_dlv[DUNGEON_ANGBAND], 3);
    }

    get_mon_num_prep_bounty(player_ptr);
    auto &world = AngbandWorld::get_instance();
    while (true) {
        world.today_mon = get_mon_num(player_ptr, std::min(max_dl / 2, 40), max_dl, PM_ARENA);
        const auto &monrace = world.get_today_bounty();
        if (cheat_hear) {
            msg_format(_("日替わり候補: %s ", "Today's candidate: %s "), monrace.name.data());
        }

        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            continue;
        }

        if (monrace.population_flags.has_any_of({ MonsterPopulationType::NAZGUL, MonsterPopulationType::ONLY_ONE, MonsterPopulationType::BUNBUN_STRIKER })) {
            continue;
        }

        if (monrace.misc_flags.has(MonsterMiscType::MULTIPLY)) {
            continue;
        }

        if (!monrace.drop_flags.has_all_of({ MonsterDropType::DROP_CORPSE, MonsterDropType::DROP_SKELETON })) {
            continue;
        }

        if (monrace.rarity > 10) {
            continue;
        }

        break;
    }

    world.knows_daily_bounty = false;
}

/*!
 * @brief 賞金首となるユニークを確定する / Determine bounty uniques
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void determine_bounty_uniques(PlayerType *player_ptr)
{
    get_mon_num_prep_bounty(player_ptr);
    const auto &monraces = MonraceList::get_instance();
    auto is_suitable_for_bounty = [&monraces](auto monrace_id) {
        const auto &monrace = monraces.get_monrace(monrace_id);
        auto is_suitable = monrace.kind_flags.has(MonsterKindType::UNIQUE);
        is_suitable &= monrace.drop_flags.has_any_of({ MonsterDropType::DROP_CORPSE, MonsterDropType::DROP_SKELETON });
        is_suitable &= monrace.rarity <= 100;
        is_suitable &= !monraces.can_unify_separate(monrace_id);
        return is_suitable;
    };

    // 賞金首とするモンスターの種族IDのリストを生成
    std::vector<MonsterRaceId> bounty_monrace_ids;
    auto &world = AngbandWorld::get_instance();
    while (bounty_monrace_ids.size() < std::size(world.bounties)) {
        const auto monrace_id = get_mon_num(player_ptr, 0, MAX_DEPTH - 1, PM_ARENA);
        if (!is_suitable_for_bounty(monrace_id)) {
            continue;
        }

        const auto is_already_selected = std::any_of(bounty_monrace_ids.begin(), bounty_monrace_ids.end(),
            [monrace_id](auto bounty_monrace_id) { return monrace_id == bounty_monrace_id; });
        if (!is_already_selected) {
            bounty_monrace_ids.push_back(monrace_id);
        }
    }

    // モンスターのLVで昇順に並び替える
    std::sort(bounty_monrace_ids.begin(), bounty_monrace_ids.end(),
        [&monraces](auto id1, auto id2) {
            return monraces.get_monrace(id1).level < monraces.get_monrace(id2).level;
        });

    // 賞金首情報を設定
    std::transform(bounty_monrace_ids.begin(), bounty_monrace_ids.end(), std::begin(world.bounties),
        [](auto monrace_id) -> bounty_type {
            return { monrace_id, false };
        });
}
