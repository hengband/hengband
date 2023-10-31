#include "market/bounty.h"
#include "autopick/autopick.h"
#include "avatar/avatar.h"
#include "cmd-building/cmd-building.h"
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
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "object/object-info.h"
#include "object/object-kind-hook.h"
#include "perception/object-perception.h"
#include "sv-definition/sv-other-types.h"
#include "system/baseitem-info.h"
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
    for (INVENTORY_IDX i = 0; i <= INVEN_SUB_HAND; i++) {
        const auto item_ptr = &player_ptr->inventory_list[i];
        const auto r_idx_of_item = static_cast<MonsterRaceId>(item_ptr->pval);
        if ((item_ptr->bi_key.tval() != ItemKindType::CAPTURE) || (r_idx_of_item != MonsterRaceId::TSUCHINOKO)) {
            continue;
        }

        change = true;
        const auto item_name = describe_flavor(player_ptr, item_ptr, 0);
        if (!input_check(format(_("%s を換金しますか？", "Convert %s into money? "), item_name.data()))) {
            continue;
        }

        msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)(1000000L * item_ptr->number));
        player_ptr->au += 1000000L * item_ptr->number;
        rfu.set_flag(MainWindowRedrawingFlag::GOLD);
        vary_item(player_ptr, i, -item_ptr->number);
    }

    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        const auto item_ptr = &player_ptr->inventory_list[i];
        const auto r_idx_of_item = static_cast<MonsterRaceId>(item_ptr->pval);
        if ((item_ptr->bi_key != BaseitemKey(ItemKindType::CORPSE, SV_CORPSE)) || (r_idx_of_item != MonsterRaceId::TSUCHINOKO)) {
            continue;
        }

        change = true;
        const auto item_name = describe_flavor(player_ptr, item_ptr, 0);
        if (!input_check(format(_("%s を換金しますか？", "Convert %s into money? "), item_name.data()))) {
            continue;
        }

        msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)(200000L * item_ptr->number));
        player_ptr->au += 200000L * item_ptr->number;
        rfu.set_flag(MainWindowRedrawingFlag::GOLD);
        vary_item(player_ptr, i, -item_ptr->number);
    }

    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        const auto item_ptr = &player_ptr->inventory_list[i];
        const auto r_idx_of_item = static_cast<MonsterRaceId>(item_ptr->pval);
        if ((item_ptr->bi_key != BaseitemKey(ItemKindType::CORPSE, SV_SKELETON)) || (r_idx_of_item != MonsterRaceId::TSUCHINOKO)) {
            continue;
        }

        change = true;
        const auto item_name = describe_flavor(player_ptr, item_ptr, 0);
        if (!input_check(format(_("%s を換金しますか？", "Convert %s into money? "), item_name.data()))) {
            continue;
        }

        msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)(100000L * item_ptr->number));
        player_ptr->au += 100000L * item_ptr->number;
        rfu.set_flag(MainWindowRedrawingFlag::GOLD);
        vary_item(player_ptr, i, -item_ptr->number);
    }

    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        const auto item_ptr = &player_ptr->inventory_list[i];
        const auto r_idx_of_item = static_cast<MonsterRaceId>(item_ptr->pval);
        if ((item_ptr->bi_key != BaseitemKey(ItemKindType::CORPSE, SV_CORPSE)) || (monraces_info[r_idx_of_item].name != monraces_info[w_ptr->today_mon].name)) {
            continue;
        }

        change = true;
        const auto item_name = describe_flavor(player_ptr, item_ptr, 0);
        if (!input_check(format(_("%s を換金しますか？", "Convert %s into money? "), item_name.data()))) {
            continue;
        }

        constexpr auto mes = _("賞金 %ld＄を手に入れた。", "You get %ldgp.");
        msg_format(mes, (long int)((monraces_info[w_ptr->today_mon].level * 50 + 100) * item_ptr->number));
        player_ptr->au += (monraces_info[w_ptr->today_mon].level * 50 + 100) * item_ptr->number;
        rfu.set_flag(MainWindowRedrawingFlag::GOLD);
        vary_item(player_ptr, i, -item_ptr->number);
    }

    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        const auto item_ptr = &player_ptr->inventory_list[i];
        const auto r_idx_of_item = static_cast<MonsterRaceId>(item_ptr->pval);
        if ((item_ptr->bi_key != BaseitemKey(ItemKindType::CORPSE, SV_SKELETON)) || (monraces_info[r_idx_of_item].name != monraces_info[w_ptr->today_mon].name)) {
            continue;
        }

        change = true;
        const auto item_name = describe_flavor(player_ptr, item_ptr, 0);
        if (!input_check(format(_("%s を換金しますか？", "Convert %s into money? "), item_name.data()))) {
            continue;
        }

        constexpr auto mes = _("賞金 %ld＄を手に入れた。", "You get %ldgp.");
        msg_format(mes, (long int)((monraces_info[w_ptr->today_mon].level * 30 + 60) * item_ptr->number));
        player_ptr->au += (monraces_info[w_ptr->today_mon].level * 30 + 60) * item_ptr->number;
        rfu.set_flag(MainWindowRedrawingFlag::GOLD);
        vary_item(player_ptr, i, -item_ptr->number);
    }

    for (auto &[r_idx, is_achieved] : w_ptr->bounties) {
        if (is_achieved) {
            continue;
        }

        for (INVENTORY_IDX i = INVEN_PACK - 1; i >= 0; i--) {
            const auto item_ptr = &player_ptr->inventory_list[i];
            const auto r_idx_of_item = static_cast<MonsterRaceId>(item_ptr->pval);
            if ((item_ptr->bi_key.tval() != ItemKindType::CORPSE) || (r_idx_of_item != r_idx)) {
                continue;
            }

            INVENTORY_IDX inventory_new;
            ItemEntity forge;

            const auto item_name = describe_flavor(player_ptr, item_ptr, 0);
            if (!input_check(format(_("%sを渡しますか？", "Hand %s over? "), item_name.data()))) {
                continue;
            }

            vary_item(player_ptr, i, -item_ptr->number);
            chg_virtue(player_ptr, Virtue::JUSTICE, 5);
            is_achieved = true;

            auto num = static_cast<int>(std::count_if(std::begin(w_ptr->bounties), std::end(w_ptr->bounties),
                [](const auto &b_ref) { return b_ref.is_achieved; }));

            msg_print(_(format("これで合計 %d ポイント獲得しました。", num), format("You earned %d point%s total.", num, (num > 1 ? "s" : ""))));

            (&forge)->prep(lookup_baseitem_id(prize_list[num - 1]));
            ItemMagicApplier(player_ptr, &forge, player_ptr->current_floor_ptr->object_level, AM_NO_FIXED_ART).execute();

            object_aware(player_ptr, &forge);
            forge.mark_as_known();

            /*
             * Hand it --- Assume there is an empty slot.
             * Since a corpse is handed at first,
             * there is at least one empty slot.
             */
            inventory_new = store_item_to_inventory(player_ptr, &forge);
            const auto got_item_name = describe_flavor(player_ptr, &forge, 0);
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
void today_target(PlayerType *player_ptr)
{
    auto *r_ptr = &monraces_info[w_ptr->today_mon];

    clear_bldg(4, 18);
    c_put_str(TERM_YELLOW, _("本日の賞金首", "Wanted monster that changes from day to day"), 5, 10);
    c_put_str(TERM_YELLOW, format(_("ターゲット： %s", "target: %s"), r_ptr->name.data()), 6, 10);
    prt(format(_("死体 ---- $%d", "corpse   ---- $%d"), (int)r_ptr->level * 50 + 100), 8, 10);
    prt(format(_("骨   ---- $%d", "skeleton ---- $%d"), (int)r_ptr->level * 30 + 60), 9, 10);
    player_ptr->knows_daily_bounty = true;
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
    TERM_LEN y = 0;

    clear_bldg(4, 18);
    prt(_("死体を持ち帰れば報酬を差し上げます。", "Offer a prize when you bring a wanted monster's corpse"), 4, 10);
    c_put_str(TERM_YELLOW, _("現在の賞金首", "Wanted monsters"), 6, 10);

    for (auto i = 0U; i < std::size(w_ptr->bounties); i++) {
        const auto &[r_idx, is_achieved] = w_ptr->bounties[i];
        MonsterRaceInfo *r_ptr = &monraces_info[r_idx];

        auto color = is_achieved ? TERM_RED : TERM_WHITE;
        auto done_mark = is_achieved ? _("(済)", "(done)") : "";

        c_prt(color, format("%s %s", r_ptr->name.data(), done_mark), y + 7, 10);

        y = (y + 1) % 10;
        if (!y && (i < std::size(w_ptr->bounties) - 1)) {
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
    int max_dl = 3;
    if (!conv_old) {
        for (const auto &d_ref : dungeons_info) {
            if (max_dlv[d_ref.idx] < d_ref.mindepth) {
                continue;
            }
            if (max_dl < max_dlv[d_ref.idx]) {
                max_dl = max_dlv[d_ref.idx];
            }
        }
    } else {
        max_dl = std::max(max_dlv[DUNGEON_ANGBAND], 3);
    }

    get_mon_num_prep_bounty(player_ptr);

    while (true) {
        w_ptr->today_mon = get_mon_num(player_ptr, std::min(max_dl / 2, 40), max_dl, PM_ARENA);
        MonsterRaceInfo *r_ptr;
        r_ptr = &monraces_info[w_ptr->today_mon];

        if (cheat_hear) {
            msg_format(_("日替わり候補: %s ", "Today's candidate: %s "), r_ptr->name.data());
        }

        if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
            continue;
        }
        if (r_ptr->population_flags.has(MonsterPopulationType::NAZGUL) || any_bits(r_ptr->flags7, RF7_UNIQUE2)) {
            continue;
        }
        if (r_ptr->flags2 & RF2_MULTIPLY) {
            continue;
        }
        if (!r_ptr->drop_flags.has_all_of({ MonsterDropType::DROP_CORPSE, MonsterDropType::DROP_SKELETON })) {
            continue;
        }
        if (r_ptr->rarity > 10) {
            continue;
        }
        break;
    }

    player_ptr->knows_daily_bounty = false;
}

/*!
 * @brief 賞金首となるユニークを確定する / Determine bounty uniques
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void determine_bounty_uniques(PlayerType *player_ptr)
{
    get_mon_num_prep_bounty(player_ptr);
    const auto &monraces = MonraceList::get_instance();
    auto is_suitable_for_bounty = [&monraces](MonsterRaceId r_idx) {
        const auto &monrace = monraces[r_idx];
        auto is_suitable = monrace.kind_flags.has(MonsterKindType::UNIQUE);
        is_suitable &= monrace.drop_flags.has_any_of({ MonsterDropType::DROP_CORPSE, MonsterDropType::DROP_SKELETON });
        is_suitable &= monrace.rarity <= 100;
        is_suitable &= !monraces.can_unify_separate(r_idx);
        return is_suitable;
    };

    // 賞金首とするモンスターの種族IDのリストを生成
    std::vector<MonsterRaceId> bounty_r_idx_list;
    while (bounty_r_idx_list.size() < std::size(w_ptr->bounties)) {
        auto r_idx = get_mon_num(player_ptr, 0, MAX_DEPTH - 1, PM_ARENA);
        if (!is_suitable_for_bounty(r_idx)) {
            continue;
        }

        auto is_already_selected = std::any_of(bounty_r_idx_list.begin(), bounty_r_idx_list.end(),
            [r_idx](MonsterRaceId bounty_r_idx) { return r_idx == bounty_r_idx; });
        if (!is_already_selected) {
            bounty_r_idx_list.push_back(r_idx);
        }
    }

    // モンスターのLVで昇順に並び替える
    std::sort(bounty_r_idx_list.begin(), bounty_r_idx_list.end(),
        [](MonsterRaceId r_idx1, MonsterRaceId r_idx2) {
            return monraces_info[r_idx1].level < monraces_info[r_idx2].level;
        });

    // 賞金首情報を設定
    std::transform(bounty_r_idx_list.begin(), bounty_r_idx_list.end(), std::begin(w_ptr->bounties),
        [](MonsterRaceId r_idx) -> bounty_type {
            return { r_idx, false };
        });
}
