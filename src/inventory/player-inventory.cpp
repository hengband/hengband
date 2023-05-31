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
#include "system/floor-type-definition.h"
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

/*!
 * @brief 規定の処理にできるアイテムがプレイヤーの利用可能範囲内にあるかどうかを返す /
 * Determine whether get_item() can get some item or not
 * @return アイテムを拾えるならばTRUEを返す。
 * @details assuming mode = (USE_EQUIP | USE_INVEN | USE_FLOOR).
 */
bool can_get_item(PlayerType *player_ptr, const ItemTester &item_tester)
{
    for (int j = 0; j < INVEN_TOTAL; j++) {
        if (item_tester.okay(&player_ptr->inventory_list[j])) {
            return true;
        }
    }

    constexpr auto mode = SCAN_FLOOR_ITEM_TESTER | SCAN_FLOOR_ONLY_MARKED;
    OBJECT_IDX floor_list[23];
    ITEM_NUMBER floor_num = scan_floor_items(player_ptr, floor_list, player_ptr->y, player_ptr->x, mode, item_tester);
    return floor_num != 0;
}

/*!
 * @brief 床上のアイテムを拾う選択用サブルーチン
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す。
 */
static bool py_pickup_floor_aux(PlayerType *player_ptr)
{
    OBJECT_IDX this_o_idx;
    OBJECT_IDX item;
    concptr q = _("どれを拾いますか？", "Get which item? ");
    concptr s = _("もうザックには床にあるどのアイテムも入らない。", "You no longer have any room for the objects on the floor.");
    if (choose_object(player_ptr, &item, q, s, (USE_FLOOR), FuncItemTester(check_store_item_to_inventory, player_ptr))) {
        this_o_idx = 0 - item;
    } else {
        return false;
    }

    describe_pickup_item(player_ptr, this_o_idx);
    return true;
}

/*!
 * @brief 床上のアイテムを拾うメイン処理
 * @param pickup FALSEなら金銭の自動拾いのみを行う/ FALSE then only gold will be picked up
 * @details
 * This is called by py_pickup() when easy_floor is TRUE.
 */
void py_pickup_floor(PlayerType *player_ptr, bool pickup)
{
    int floor_num = 0;
    OBJECT_IDX floor_o_idx = 0;
    int can_pickup = 0;
    auto &o_idx_list = player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].o_idx_list;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    for (auto it = o_idx_list.begin(); it != o_idx_list.end();) {
        const OBJECT_IDX this_o_idx = *it++;
        auto *o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];
        const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
        disturb(player_ptr, false, false);
        if (o_ptr->bi_key.tval() == ItemKindType::GOLD) {
            constexpr auto mes = _(" $%ld の価値がある%sを見つけた。", "You have found %ld gold pieces worth of %s.");
            msg_format(mes, (long)o_ptr->pval, item_name.data());
            sound(SOUND_SELL);
            player_ptr->au += o_ptr->pval;
            rfu.set_flag(MainWindowRedrawingFlag::GOLD);
            rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
            delete_object_idx(player_ptr, this_o_idx);
            continue;
        } else if (o_ptr->marked.has(OmType::SUPRESS_MESSAGE)) {
            o_ptr->marked.reset(OmType::SUPRESS_MESSAGE);
            continue;
        }

        if (check_store_item_to_inventory(player_ptr, o_ptr)) {
            can_pickup++;
        }

        floor_num++;
        floor_o_idx = this_o_idx;
    }

    if (!floor_num) {
        return;
    }

    if (!pickup) {
        if (floor_num == 1) {
            auto *o_ptr = &player_ptr->current_floor_ptr->o_list[floor_o_idx];
            const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
            msg_format(_("%sがある。", "You see %s."), item_name.data());
        } else {
            msg_format(_("%d 個のアイテムの山がある。", "You see a pile of %d items."), floor_num);
        }

        return;
    }

    if (!can_pickup) {
        if (floor_num == 1) {
            auto *o_ptr = &player_ptr->current_floor_ptr->o_list[floor_o_idx];
            const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
            msg_format(_("ザックには%sを入れる隙間がない。", "You have no room for %s."), item_name.data());
        } else {
            msg_print(_("ザックには床にあるどのアイテムも入らない。", "You have no room for any of the objects on the floor."));
        }

        return;
    }

    if (floor_num != 1) {
        while (can_pickup--) {
            if (!py_pickup_floor_aux(player_ptr)) {
                break;
            }
        }

        return;
    }

    if (!carry_query_flag) {
        describe_pickup_item(player_ptr, floor_o_idx);
        return;
    }

    char out_val[MAX_NLEN + 20];
    auto *o_ptr = &player_ptr->current_floor_ptr->o_list[floor_o_idx];
    const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
    strnfmt(out_val, sizeof(out_val), _("%sを拾いますか? ", "Pick up %s? "), item_name.data());
    if (!get_check(out_val)) {
        return;
    }

    describe_pickup_item(player_ptr, floor_o_idx);
}

/*!
 * @brief プレイヤーがオブジェクトを拾った際のメッセージ表示処理 /
 * Helper routine for py_pickup() and py_pickup_floor().
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_idx 取得したオブジェクトの参照ID
 * @details
 * アイテムを拾った際に「２つのケーキを持っている」
 * "You have two cakes." とアイテムを拾った後の合計のみの表示がオリジナルだが、
 * 違和感があるという指摘をうけたので、「～を拾った、～を持っている」という表示にかえてある。
 * そのための配列。
 * Add the given dungeon object to the character's inventory.\n
 * Delete the object afterwards.\n
 */
void describe_pickup_item(PlayerType *player_ptr, OBJECT_IDX o_idx)
{
    auto *o_ptr = &player_ptr->current_floor_ptr->o_list[o_idx];
#ifdef JP
    const auto old_item_name = describe_flavor(player_ptr, o_ptr, OD_NAME_ONLY);
    const auto picked_count_str = describe_count_with_counter_suffix(*o_ptr);
    const auto picked_count = o_ptr->number;
#else
    (void)o_ptr;
#endif

    auto slot = store_item_to_inventory(player_ptr, o_ptr);
    o_ptr = &player_ptr->inventory_list[slot];
    delete_object_idx(player_ptr, o_idx);
    if (player_ptr->ppersonality == PERSONALITY_MUNCHKIN) {
        bool old_known = identify_item(player_ptr, o_ptr);
        autopick_alter_item(player_ptr, slot, (bool)(destroy_identify && !old_known));
        if (o_ptr->marked.has(OmType::AUTODESTROY)) {
            return;
        }
    }

    const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
#ifdef JP
    if (o_ptr->is_specific_artifact(FixedArtifactId::CRIMSON) && (player_ptr->ppersonality == PERSONALITY_COMBAT)) {
        msg_format("こうして、%sは『クリムゾン』を手に入れた。", player_ptr->name);
        msg_print("しかし今、『混沌のサーペント』の放ったモンスターが、");
        msg_format("%sに襲いかかる．．．", player_ptr->name);
    } else {
        if (plain_pickup) {
            msg_format("%s(%c)を持っている。", item_name.data(), index_to_label(slot));
        } else {
            if (o_ptr->number > picked_count) {
                msg_format("%s拾って、%s(%c)を持っている。", picked_count_str.data(), item_name.data(), index_to_label(slot));
            } else {
                msg_format("%s(%c)を拾った。", item_name.data(), index_to_label(slot));
            }
        }
    }

    angband_strcpy(record_o_name, old_item_name.data(), old_item_name.length());
#else
    msg_format("You have %s (%c).", item_name.data(), index_to_label(slot));
    angband_strcpy(record_o_name, item_name.data(), item_name.length());
#endif
    record_turn = w_ptr->game_turn;
    check_find_art_quest_completion(player_ptr, o_ptr);
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
    rfu.set_flag(StatusRedrawingFlag::MONSTER_STATUSES);
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    rfu.set_flag(SubWindowRedrawingFlag::OVERHEAD);
    handle_stuff(player_ptr);
    auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x];
    autopick_pickup_items(player_ptr, g_ptr);
    if (easy_floor) {
        py_pickup_floor(player_ptr, pickup);
        return;
    }

    for (auto it = g_ptr->o_idx_list.begin(); it != g_ptr->o_idx_list.end();) {
        const OBJECT_IDX this_o_idx = *it++;
        auto *o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];
        const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
        disturb(player_ptr, false, false);
        if (o_ptr->bi_key.tval() == ItemKindType::GOLD) {
            int value = (long)o_ptr->pval;
            delete_object_idx(player_ptr, this_o_idx);
            msg_format(_(" $%ld の価値がある%sを見つけた。", "You collect %ld gold pieces worth of %s."), (long)value, item_name.data());
            sound(SOUND_SELL);
            player_ptr->au += value;
            rfu.set_flag(MainWindowRedrawingFlag::GOLD);
            rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
            continue;
        }

        if (o_ptr->marked.has(OmType::SUPRESS_MESSAGE)) {
            o_ptr->marked.reset(OmType::SUPRESS_MESSAGE);
            continue;
        }

        if (!pickup) {
            msg_format(_("%sがある。", "You see %s."), item_name.data());
            continue;
        }

        if (!check_store_item_to_inventory(player_ptr, o_ptr)) {
            msg_format(_("ザックには%sを入れる隙間がない。", "You have no room for %s."), item_name.data());
            continue;
        }

        int is_pickup_successful = true;
        if (carry_query_flag) {
            char out_val[MAX_NLEN + 20];
            strnfmt(out_val, sizeof(out_val), _("%sを拾いますか? ", "Pick up %s? "), item_name.data());
            is_pickup_successful = get_check(out_val);
        }

        if (is_pickup_successful) {
            describe_pickup_item(player_ptr, this_o_idx);
        }
    }
}
