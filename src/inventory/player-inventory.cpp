#include "inventory/player-inventory.h"
#include "autopick/autopick.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
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
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-checker.h"
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
bool can_get_item(PlayerType *player_ptr, const ItemTester& item_tester)
{
    for (int j = 0; j < INVEN_TOTAL; j++)
        if (item_tester.okay(&player_ptr->inventory_list[j]))
            return true;

    OBJECT_IDX floor_list[23];
    ITEM_NUMBER floor_num = scan_floor_items(player_ptr, floor_list, player_ptr->y, player_ptr->x, SCAN_FLOOR_ITEM_TESTER | SCAN_FLOOR_ONLY_MARKED, item_tester);
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
    if (choose_object(player_ptr, &item, q, s, (USE_FLOOR), FuncItemTester(check_store_item_to_inventory, player_ptr)))
        this_o_idx = 0 - item;
    else
        return false;

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
    GAME_TEXT o_name[MAX_NLEN];
    ObjectType *o_ptr;
    int floor_num = 0;
    OBJECT_IDX floor_o_idx = 0;
    int can_pickup = 0;
    auto &o_idx_list = player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].o_idx_list;
    for (auto it = o_idx_list.begin(); it != o_idx_list.end();) {
        const OBJECT_IDX this_o_idx = *it++;
        o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];
        describe_flavor(player_ptr, o_name, o_ptr, 0);
        disturb(player_ptr, false, false);
        if (o_ptr->tval == ItemKindType::GOLD) {
            msg_format(_(" $%ld の価値がある%sを見つけた。", "You have found %ld gold pieces worth of %s."), (long)o_ptr->pval, o_name);
            sound(SOUND_SELL);
            player_ptr->au += o_ptr->pval;
            player_ptr->redraw |= (PR_GOLD);
            player_ptr->window_flags |= (PW_PLAYER);
            delete_object_idx(player_ptr, this_o_idx);
            continue;
        } else if (o_ptr->marked & OM_NOMSG) {
            o_ptr->marked &= ~(OM_NOMSG);
            continue;
        }

        if (check_store_item_to_inventory(player_ptr, o_ptr))
            can_pickup++;

        floor_num++;
        floor_o_idx = this_o_idx;
    }

    if (!floor_num)
        return;

    if (!pickup) {
        if (floor_num == 1) {
            o_ptr = &player_ptr->current_floor_ptr->o_list[floor_o_idx];
            describe_flavor(player_ptr, o_name, o_ptr, 0);
            msg_format(_("%sがある。", "You see %s."), o_name);
        } else
            msg_format(_("%d 個のアイテムの山がある。", "You see a pile of %d items."), floor_num);

        return;
    }

    if (!can_pickup) {
        if (floor_num == 1) {
            o_ptr = &player_ptr->current_floor_ptr->o_list[floor_o_idx];
            describe_flavor(player_ptr, o_name, o_ptr, 0);
            msg_format(_("ザックには%sを入れる隙間がない。", "You have no room for %s."), o_name);
        } else
            msg_print(_("ザックには床にあるどのアイテムも入らない。", "You have no room for any of the objects on the floor."));

        return;
    }

    if (floor_num != 1) {
        while (can_pickup--)
            if (!py_pickup_floor_aux(player_ptr))
                break;

        return;
    }

    if (carry_query_flag) {
        char out_val[MAX_NLEN + 20];
        o_ptr = &player_ptr->current_floor_ptr->o_list[floor_o_idx];
        describe_flavor(player_ptr, o_name, o_ptr, 0);
        (void)sprintf(out_val, _("%sを拾いますか? ", "Pick up %s? "), o_name);
        if (!get_check(out_val))
            return;
    }

    o_ptr = &player_ptr->current_floor_ptr->o_list[floor_o_idx];
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
#ifdef JP
    GAME_TEXT o_name[MAX_NLEN];
    GAME_TEXT old_name[MAX_NLEN];
    char kazu_str[80];
    int hirottakazu;
#else
    GAME_TEXT o_name[MAX_NLEN];
#endif

    ObjectType *o_ptr;
    o_ptr = &player_ptr->current_floor_ptr->o_list[o_idx];

#ifdef JP
    describe_flavor(player_ptr, old_name, o_ptr, OD_NAME_ONLY);
    object_desc_count_japanese(kazu_str, o_ptr);
    hirottakazu = o_ptr->number;
#endif

    INVENTORY_IDX slot = store_item_to_inventory(player_ptr, o_ptr);
    o_ptr = &player_ptr->inventory_list[slot];
    delete_object_idx(player_ptr, o_idx);
    if (player_ptr->ppersonality == PERSONALITY_MUNCHKIN) {
        bool old_known = identify_item(player_ptr, o_ptr);
        autopick_alter_item(player_ptr, slot, (bool)(destroy_identify && !old_known));
        if (o_ptr->marked & OM_AUTODESTROY)
            return;
    }

    describe_flavor(player_ptr, o_name, o_ptr, 0);

#ifdef JP
    if ((o_ptr->name1 == ART_CRIMSON) && (player_ptr->ppersonality == PERSONALITY_COMBAT)) {
        msg_format("こうして、%sは『クリムゾン』を手に入れた。", player_ptr->name);
        msg_print("しかし今、『混沌のサーペント』の放ったモンスターが、");
        msg_format("%sに襲いかかる．．．", player_ptr->name);
    } else {
        if (plain_pickup) {
            msg_format("%s(%c)を持っている。", o_name, index_to_label(slot));
        } else {
            if (o_ptr->number > hirottakazu) {
                msg_format("%s拾って、%s(%c)を持っている。", kazu_str, o_name, index_to_label(slot));
            } else {
                msg_format("%s(%c)を拾った。", o_name, index_to_label(slot));
            }
        }
    }

    strcpy(record_o_name, old_name);
#else
    msg_format("You have %s (%c).", o_name, index_to_label(slot));
    strcpy(record_o_name, o_name);
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
    player_ptr->update |= PU_MONSTERS;
    player_ptr->redraw |= PR_MAP;
    player_ptr->window_flags |= PW_OVERHEAD;
    handle_stuff(player_ptr);
    auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x];
    autopick_pickup_items(player_ptr, g_ptr);
    if (easy_floor) {
        py_pickup_floor(player_ptr, pickup);
        return;
    }

    for (auto it = g_ptr->o_idx_list.begin(); it != g_ptr->o_idx_list.end();) {
        const OBJECT_IDX this_o_idx = *it++;
        ObjectType *o_ptr;
        o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];
        GAME_TEXT o_name[MAX_NLEN];
        describe_flavor(player_ptr, o_name, o_ptr, 0);
        disturb(player_ptr, false, false);
        if (o_ptr->tval == ItemKindType::GOLD) {
            int value = (long)o_ptr->pval;
            delete_object_idx(player_ptr, this_o_idx);
            msg_format(_(" $%ld の価値がある%sを見つけた。", "You collect %ld gold pieces worth of %s."), (long)value, o_name);
            sound(SOUND_SELL);
            player_ptr->au += value;
            player_ptr->redraw |= (PR_GOLD);
            player_ptr->window_flags |= (PW_PLAYER);
            continue;
        }

        if (o_ptr->marked & OM_NOMSG) {
            o_ptr->marked &= ~OM_NOMSG;
            continue;
        }

        if (!pickup) {
            msg_format(_("%sがある。", "You see %s."), o_name);
            continue;
        }

        if (!check_store_item_to_inventory(player_ptr, o_ptr)) {
            msg_format(_("ザックには%sを入れる隙間がない。", "You have no room for %s."), o_name);
            continue;
        }

        int is_pickup_successful = true;
        if (carry_query_flag) {
            char out_val[MAX_NLEN + 20];
            sprintf(out_val, _("%sを拾いますか? ", "Pick up %s? "), o_name);
            is_pickup_successful = get_check(out_val);
        }

        if (is_pickup_successful) {
            describe_pickup_item(player_ptr, this_o_idx);
        }
    }
}
