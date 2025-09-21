#include "inventory/player-inventory.h"
#include "autopick/autopick.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/quest.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "floor/object-scanner.h"
#include "game-option/auto-destruction-options.h"
#include "game-option/birth-options.h"
#include "game-option/input-options.h"
#include "game-option/play-record-options.h"
#include "game-option/text-display-options.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "player/player-move.h"
#include "spell-kind/spells-perception.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-checker.h"
#include "term/z-form.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"
#ifdef JP
#include "artifact/fixed-art-types.h"
#include "flavor/flavor-util.h"
#endif
#include <fmt/format.h>
#include <range/v3/all.hpp>

/*!
 * @brief 規定の処理にできるアイテムがプレイヤーの利用可能範囲内にあるかどうかを返す /
 * Determine whether get_item() can get some item or not
 * @return アイテムを拾えるならばTRUEを返す。
 * @details assuming mode = (USE_EQUIP | USE_INVEN | USE_FLOOR).
 */
bool can_get_item(PlayerType *player_ptr, const ItemTester &item_tester)
{
    for (int j = 0; j < INVEN_TOTAL; j++) {
        if (item_tester.okay(player_ptr->inventory[j].get())) {
            return true;
        }
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto floor_item_index = scan_floor_items(floor, player_ptr->get_position(), { ScanFloorMode::ITEM_TESTER, ScanFloorMode::ONLY_MARKED }, item_tester);
    return !floor_item_index.empty();
}

/*!
 * @brief アイテムを拾うかどうかをプレイヤーに尋ねる
 *
 * @param item_name アイテム名
 * @return プレイヤーの応答が「拾う」ならtrue、「拾わない」ならfalseを返す
 * ただし、オプション設定のcarry_query_flagが「いいえ」の場合は問い合わせをせず拾うので、常にtrueを返す
 */
static bool query_pickup(std::string_view item_name)
{
    if (!carry_query_flag) {
        return true;
    }

    const auto prompt = fmt::format(_("{}を拾いますか? ", "Pick up {}? "), item_name);
    return input_check(prompt);
}

static void py_pickup_all_golds_on_floor(PlayerType *player_ptr, const Grid &grid)
{
    for (auto it = grid.o_idx_list.begin(); it != grid.o_idx_list.end();) {
        const auto i_idx = *it++;
        auto &item = *player_ptr->current_floor_ptr->o_list[i_idx];
        if (item.bi_key.tval() != ItemKindType::GOLD) {
            continue;
        }

        const auto value = item.pval;
        const auto item_name = describe_flavor(player_ptr, item, 0);
        player_ptr->au += value;

        msg_print(_(" ${} の価値がある{}を見つけた。", "You have found {} gold pieces worth of {}."), value, item_name);
        sound(SoundKind::SELL);

        delete_object_idx(player_ptr, i_idx);

        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(MainWindowRedrawingFlag::GOLD);
        rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
    }
}

static void py_pickup_single_item(PlayerType *player_ptr, short i_idx, bool pickup)
{
    const auto &item = *player_ptr->current_floor_ptr->o_list[i_idx];
    const auto item_name = describe_flavor(player_ptr, item, 0);

    if (!pickup) {
        msg_print(_("{}がある。", "You see {}."), item_name);
        return;
    }

    if (!check_store_item_to_inventory(player_ptr, &item)) {
        msg_print(_("ザックには{}を入れる隙間がない。", "You have no room for {}."), item_name);
        return;
    }

    if (query_pickup(item_name)) {
        process_player_pickup_item(player_ptr, i_idx);
    }
}

static void py_pickup_multiple_items(PlayerType *player_ptr, bool pickup)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(player_ptr->get_position());

    if (!pickup) {
        const auto count_of_items = grid.o_idx_list.size();
        msg_print(_("{} 個のアイテムの山がある。", "You see a pile of {} items."), count_of_items);
        return;
    }

    const auto tester = FuncItemTester(check_store_item_to_inventory, player_ptr);
    const auto can_pickup = [&](auto i_idx) { return tester.okay(floor.o_list.at(i_idx).get()); };
    const auto count_of_pickable_items = ranges::count_if(grid.o_idx_list, can_pickup);
    if (count_of_pickable_items == 0) {
        msg_print(_("ザックには床にあるどのアイテムも入らない。", "You have no room for any of the objects on the floor."));
        return;
    }

    for (auto pick_count = 0; pick_count < count_of_pickable_items; ++pick_count) {
        constexpr auto q = _("どれを拾いますか？", "Get which item? ");
        constexpr auto s = _("もうザックには床にあるどのアイテムも入らない。", "You no longer have any room for the objects on the floor.");
        short i_idx;
        if (!choose_object(player_ptr, &i_idx, q, s, (USE_FLOOR), tester)) {
            break;
        }
        process_player_pickup_item(player_ptr, -i_idx);
    }
}

/*!
 * @brief 床上のアイテムを拾うメイン処理
 * @param pickup FALSEなら金銭の自動拾いのみを行う/ FALSE then only gold will be picked up
 * @details
 * This is called by py_pickup() when easy_floor is TRUE.
 */
static void py_pickup_floor(PlayerType *player_ptr, bool pickup)
{
    const auto &o_list = player_ptr->current_floor_ptr->o_list;
    const auto exclude_marked_as_skip = ranges::views::remove_if([&](auto i_idx) { return o_list.at(i_idx)->marked.has(OmType::SUPRESS_MESSAGE); });
    const auto &grid = player_ptr->current_floor_ptr->get_grid(player_ptr->get_position());

    const auto i_idx_list = grid.o_idx_list | exclude_marked_as_skip | ranges::to_vector;
    const auto count_of_items = i_idx_list.size();

    for (const auto i_idx : grid.o_idx_list) {
        o_list.at(i_idx)->marked.reset(OmType::SUPRESS_MESSAGE);
    }

    switch (count_of_items) {
    case 0:
        return;
    case 1:
        py_pickup_single_item(player_ptr, i_idx_list.front(), pickup);
        return;
    default:
        py_pickup_multiple_items(player_ptr, pickup);
        return;
    }
}

/*!
 * @brief プレイヤーがアイテムを拾った際のメッセージを表示する
 *
 * アイテムを拾った際に「２つのケーキを持っている」(英語版では "You have two cakes.") と
 * アイテムを拾った後の合計のみの表示がオリジナルだが、違和感があるという指摘をうけたので、
 * 日本語版ではゲームオプションの plain_pickup が「いいえ」の場合、「2つのケーキを拾った」もしくは
 * 「2つ拾って、3つのケーキを持っている」(もともと同じアイテムを持っていてスタックした場合)
 * というふうに表示する。
 *
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param picked_item 拾ったアイテムの参照
 * @param picked_slot_item 拾ったアイテムを格納したスロットの参照
 * @param slot 拾ったアイテムを格納したスロットのID
 */
static void print_pickup_message(PlayerType *player_ptr, [[maybe_unused]] const ItemEntity &picked_item, const ItemEntity &picked_slot_item, short slot)
{
    const auto item_name = describe_flavor(player_ptr, picked_slot_item, 0);
    const auto item_name_with_label = fmt::format(_("{}({})", "{} ({})"), item_name, index_to_label(slot));

#ifdef JP
    if (picked_slot_item.is_specific_artifact(FixedArtifactId::CRIMSON) && (player_ptr->ppersonality == PERSONALITY_COMBAT)) {
        msg_print("こうして、{}は『クリムゾン』を手に入れた。", player_ptr->name);
        msg_print("しかし今、『混沌のサーペント』の放ったモンスターが、");
        msg_print("{}に襲いかかる．．．", player_ptr->name);
        return;
    }

    if (plain_pickup) {
        msg_print("{}を持っている。", item_name_with_label);
        return;
    }

    if (picked_slot_item.number > picked_item.number) {
        const auto picked_count_str = describe_count_with_counter_suffix(picked_item);
        msg_print("{}拾って、{}を持っている。", picked_count_str, item_name_with_label);
        return;
    }

    msg_print("{}を拾った。", item_name_with_label);
#else
    msg_print("You have {}.", item_name_with_label);
#endif
}

/*!
 * @brief プレイヤーがアイテムを拾う処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_idx 取得したオブジェクトの参照ID
 */
void process_player_pickup_item(PlayerType *player_ptr, OBJECT_IDX o_idx)
{
    // delete_object_idx()で配列から削除した後にも使用するためshared_ptrをコピーする
    const std::shared_ptr<ItemEntity> picked_item_ptr = player_ptr->current_floor_ptr->o_list[o_idx];

    const auto slot = store_item_to_inventory(player_ptr, picked_item_ptr.get());
    delete_object_idx(player_ptr, o_idx);

    auto &picked_slot_item = *player_ptr->inventory[slot];
    if (player_ptr->ppersonality == PERSONALITY_MUNCHKIN) {
        const auto old_known = identify_item(player_ptr, &picked_slot_item);
        autopick_alter_item(player_ptr, slot, destroy_identify && !old_known);
        if (picked_slot_item.marked.has(OmType::AUTODESTROY)) {
            return;
        }
    }

    print_pickup_message(player_ptr, *picked_item_ptr, picked_slot_item, slot);

    record_item_name = describe_flavor(player_ptr, *picked_item_ptr, OD_NAME_ONLY);
    record_turn = AngbandWorld::get_instance().game_turn;
    check_find_art_quest_completion(player_ptr, &picked_slot_item);
}

/*!
 * @brief プレイヤーがオブジェクト上に乗った際の表示処理 / Player "wants" to pick up an object or gold.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pickup 自動拾い処理を行うならばTRUEとする
 */
void carry(PlayerType *player_ptr, bool pickup)
{
    verify_panel(player_ptr);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    rfu.set_flag(SubWindowRedrawingFlag::OVERHEAD);
    handle_stuff(player_ptr);
    const auto &grid = player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x];
    autopick_pickup_items(player_ptr, grid);

    if (!grid.o_idx_list.empty()) {
        disturb(player_ptr, false, false);
    }

    py_pickup_all_golds_on_floor(player_ptr, grid);

    if (easy_floor) {
        py_pickup_floor(player_ptr, pickup);
        return;
    }

    for (auto it = grid.o_idx_list.begin(); it != grid.o_idx_list.end();) {
        const auto this_o_idx = *it++;
        auto &item = *player_ptr->current_floor_ptr->o_list[this_o_idx];
        const auto item_name = describe_flavor(player_ptr, item, 0);

        if (item.marked.has(OmType::SUPRESS_MESSAGE)) {
            item.marked.reset(OmType::SUPRESS_MESSAGE);
            continue;
        }

        py_pickup_single_item(player_ptr, this_o_idx, pickup);
    }
}
