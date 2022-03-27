#include "market/bounty.h"
#include "autopick/autopick.h"
#include "avatar/avatar.h"
#include "cmd-building/cmd-building.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/stuff-handler.h"
#include "dungeon/dungeon.h"
#include "flavor/flavor-describer.h"
#include "game-option/cheat-options.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "market/bounty-prize-table.h"
#include "market/building-util.h"
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
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
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
    bool change = false;
    GAME_TEXT o_name[MAX_NLEN];
    ObjectType *o_ptr;

    for (INVENTORY_IDX i = 0; i <= INVEN_SUB_HAND; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        const auto r_idx_of_item = static_cast<MonsterRaceId>(o_ptr->pval);

        if ((o_ptr->tval == ItemKindType::CAPTURE) && (r_idx_of_item == MonsterRaceId::TSUCHINOKO)) {
            char buf[MAX_NLEN + 32];
            describe_flavor(player_ptr, o_name, o_ptr, 0);
            sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "), o_name);
            if (get_check(buf)) {
                msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)(1000000L * o_ptr->number));
                player_ptr->au += 1000000L * o_ptr->number;
                player_ptr->redraw |= (PR_GOLD);
                vary_item(player_ptr, i, -o_ptr->number);
            }

            change = true;
        }
    }

    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        const auto r_idx_of_item = static_cast<MonsterRaceId>(o_ptr->pval);

        if ((o_ptr->tval == ItemKindType::CORPSE) && (o_ptr->sval == SV_CORPSE) && (r_idx_of_item == MonsterRaceId::TSUCHINOKO)) {
            char buf[MAX_NLEN + 32];
            describe_flavor(player_ptr, o_name, o_ptr, 0);
            sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "), o_name);
            if (get_check(buf)) {
                msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)(200000L * o_ptr->number));
                player_ptr->au += 200000L * o_ptr->number;
                player_ptr->redraw |= (PR_GOLD);
                vary_item(player_ptr, i, -o_ptr->number);
            }

            change = true;
        }
    }

    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        const auto r_idx_of_item = static_cast<MonsterRaceId>(o_ptr->pval);

        if ((o_ptr->tval == ItemKindType::CORPSE) && (o_ptr->sval == SV_SKELETON) && (r_idx_of_item == MonsterRaceId::TSUCHINOKO)) {
            char buf[MAX_NLEN + 32];
            describe_flavor(player_ptr, o_name, o_ptr, 0);
            sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "), o_name);
            if (get_check(buf)) {
                msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)(100000L * o_ptr->number));
                player_ptr->au += 100000L * o_ptr->number;
                player_ptr->redraw |= (PR_GOLD);
                vary_item(player_ptr, i, -o_ptr->number);
            }

            change = true;
        }
    }

    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        const auto r_idx_of_item = static_cast<MonsterRaceId>(o_ptr->pval);

        if ((o_ptr->tval == ItemKindType::CORPSE) && (o_ptr->sval == SV_CORPSE) && (streq(r_info[r_idx_of_item].name.c_str(), r_info[w_ptr->today_mon].name.c_str()))) {
            char buf[MAX_NLEN + 32];
            describe_flavor(player_ptr, o_name, o_ptr, 0);
            sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "), o_name);
            if (get_check(buf)) {
                msg_format(
                    _("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)((r_info[w_ptr->today_mon].level * 50 + 100) * o_ptr->number));
                player_ptr->au += (r_info[w_ptr->today_mon].level * 50 + 100) * o_ptr->number;
                player_ptr->redraw |= (PR_GOLD);
                vary_item(player_ptr, i, -o_ptr->number);
            }

            change = true;
        }
    }

    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        const auto r_idx_of_item = static_cast<MonsterRaceId>(o_ptr->pval);

        if ((o_ptr->tval == ItemKindType::CORPSE) && (o_ptr->sval == SV_SKELETON) && (streq(r_info[r_idx_of_item].name.c_str(), r_info[w_ptr->today_mon].name.c_str()))) {
            char buf[MAX_NLEN + 32];
            describe_flavor(player_ptr, o_name, o_ptr, 0);
            sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "), o_name);
            if (get_check(buf)) {
                msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)((r_info[w_ptr->today_mon].level * 30 + 60) * o_ptr->number));
                player_ptr->au += (r_info[w_ptr->today_mon].level * 30 + 60) * o_ptr->number;
                player_ptr->redraw |= (PR_GOLD);
                vary_item(player_ptr, i, -o_ptr->number);
            }

            change = true;
        }
    }

    for (auto &[r_idx, is_achieved] : w_ptr->bounties) {
        if (is_achieved) {
            continue;
        }

        for (INVENTORY_IDX i = INVEN_PACK - 1; i >= 0; i--) {
            o_ptr = &player_ptr->inventory_list[i];
            const auto r_idx_of_item = static_cast<MonsterRaceId>(o_ptr->pval);

            if ((o_ptr->tval != ItemKindType::CORPSE) || (r_idx_of_item != r_idx)) {
                continue;
            }

            char buf[MAX_NLEN + 20];
            INVENTORY_IDX item_new;
            ObjectType forge;

            describe_flavor(player_ptr, o_name, o_ptr, 0);
            sprintf(buf, _("%sを渡しますか？", "Hand %s over? "), o_name);
            if (!get_check(buf)) {
                continue;
            }

            vary_item(player_ptr, i, -o_ptr->number);
            chg_virtue(player_ptr, V_JUSTICE, 5);
            is_achieved = true;

            auto num = std::count_if(std::begin(w_ptr->bounties), std::end(w_ptr->bounties),
                [](const auto &b_ref) { return b_ref.is_achieved; });

            msg_format(_("これで合計 %d ポイント獲得しました。", "You earned %d point%s total."), num, (num > 1 ? "s" : ""));

            (&forge)->prep(lookup_kind(prize_list[num - 1].tval, prize_list[num - 1].sval));
            ItemMagicApplier(player_ptr, &forge, player_ptr->current_floor_ptr->object_level, AM_NO_FIXED_ART).execute();

            object_aware(player_ptr, &forge);
            object_known(&forge);

            /*
             * Hand it --- Assume there is an empty slot.
             * Since a corpse is handed at first,
             * there is at least one empty slot.
             */
            item_new = store_item_to_inventory(player_ptr, &forge);
            describe_flavor(player_ptr, o_name, &forge, 0);
            msg_format(_("%s(%c)を貰った。", "You get %s (%c). "), o_name, index_to_label(item_new));

            autopick_alter_item(player_ptr, item_new, false);
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
    char buf[160];
    auto *r_ptr = &r_info[w_ptr->today_mon];

    clear_bldg(4, 18);
    c_put_str(TERM_YELLOW, _("本日の賞金首", "Wanted monster that changes from day to day"), 5, 10);
    sprintf(buf, _("ターゲット： %s", "target: %s"), r_ptr->name.c_str());
    c_put_str(TERM_YELLOW, buf, 6, 10);
    sprintf(buf, _("死体 ---- $%d", "corpse   ---- $%d"), (int)r_ptr->level * 50 + 100);
    prt(buf, 8, 10);
    sprintf(buf, _("骨   ---- $%d", "skeleton ---- $%d"), (int)r_ptr->level * 30 + 60);
    prt(buf, 9, 10);
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
        monster_race *r_ptr = &r_info[r_idx];

        auto color = is_achieved ? TERM_RED : TERM_WHITE;
        auto done_mark = is_achieved ? _("(済)", "(done)") : "";

        c_prt(color, format("%s %s", r_ptr->name.c_str(), done_mark), y + 7, 10);

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
        for (const auto &d_ref : d_info) {
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
        w_ptr->today_mon = get_mon_num(player_ptr, std::min(max_dl / 2, 40), max_dl, GMN_ARENA);
        monster_race *r_ptr;
        r_ptr = &r_info[w_ptr->today_mon];

        if (cheat_hear) {
            msg_format("日替わり候補: %s ", r_ptr->name.c_str());
        }

        if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
            continue;
        }
        if (r_ptr->flags7 & (RF7_NAZGUL | RF7_UNIQUE2)) {
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

    auto is_suitable_for_bounty = [](MonsterRaceId r_idx) {
        const auto &r_ref = r_info[r_idx];
        bool is_suitable = r_ref.kind_flags.has(MonsterKindType::UNIQUE);
        is_suitable &= r_ref.drop_flags.has_any_of({ MonsterDropType::DROP_CORPSE, MonsterDropType::DROP_SKELETON });
        is_suitable &= r_ref.rarity <= 100;
        is_suitable &= !no_questor_or_bounty_uniques(r_idx);
        return is_suitable;
    };

    // 賞金首とするモンスターの種族IDのリストを生成
    std::vector<MonsterRaceId> bounty_r_idx_list;
    while (bounty_r_idx_list.size() < std::size(w_ptr->bounties)) {
        auto r_idx = get_mon_num(player_ptr, 0, MAX_DEPTH - 1, GMN_ARENA);
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
            return r_info[r_idx1].level < r_info[r_idx2].level;
        });

    // 賞金首情報を設定
    std::transform(bounty_r_idx_list.begin(), bounty_r_idx_list.end(), std::begin(w_ptr->bounties),
        [](MonsterRaceId r_idx) -> bounty_type {
            return { r_idx, false };
        });
}
