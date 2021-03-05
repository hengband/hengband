﻿#include "inventory/player-inventory.h"
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
#include "grid/grid.h"
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
bool can_get_item(player_type *owner_ptr, tval_type tval)
{
    for (int j = 0; j < INVEN_TOTAL; j++)
        if (item_tester_okay(owner_ptr, &owner_ptr->inventory_list[j], tval))
            return TRUE;

    OBJECT_IDX floor_list[23];
    ITEM_NUMBER floor_num = scan_floor_items(owner_ptr, floor_list, owner_ptr->y, owner_ptr->x, SCAN_FLOOR_ITEM_TESTER | SCAN_FLOOR_ONLY_MARKED, tval);
    return floor_num != 0;
}

/*!
 * @brief 床上のアイテムを拾う選択用サブルーチン
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す。
 */
static bool py_pickup_floor_aux(player_type *owner_ptr)
{
    OBJECT_IDX this_o_idx;
    OBJECT_IDX item;
    item_tester_hook = check_store_item_to_inventory;
    concptr q = _("どれを拾いますか？", "Get which item? ");
    concptr s = _("もうザックには床にあるどのアイテムも入らない。", "You no longer have any room for the objects on the floor.");
    if (choose_object(owner_ptr, &item, q, s, (USE_FLOOR), TV_NONE))
        this_o_idx = 0 - item;
    else
        return FALSE;

    describe_pickup_item(owner_ptr, this_o_idx);
    return TRUE;
}

/*!
 * @brief 床上のアイテムを拾うメイン処理
 * @param pickup FALSEなら金銭の自動拾いのみを行う/ FALSE then only gold will be picked up
 * @return なし
 * @details
 * This is called by py_pickup() when easy_floor is TRUE.
 */
void py_pickup_floor(player_type *owner_ptr, bool pickup)
{
    OBJECT_IDX this_o_idx, next_o_idx = 0;
    GAME_TEXT o_name[MAX_NLEN];
    object_type *o_ptr;
    int floor_num = 0;
    OBJECT_IDX floor_o_idx = 0;
    int can_pickup = 0;
    for (this_o_idx = owner_ptr->current_floor_ptr->grid_array[owner_ptr->y][owner_ptr->x].o_idx; this_o_idx; this_o_idx = next_o_idx) {
        o_ptr = &owner_ptr->current_floor_ptr->o_list[this_o_idx];
        describe_flavor(owner_ptr, o_name, o_ptr, 0);
        next_o_idx = o_ptr->next_o_idx;
        disturb(owner_ptr, FALSE, FALSE);
        if (o_ptr->tval == TV_GOLD) {
            msg_format(_(" $%ld の価値がある%sを見つけた。", "You have found %ld gold pieces worth of %s."), (long)o_ptr->pval, o_name);
            owner_ptr->au += o_ptr->pval;
            owner_ptr->redraw |= (PR_GOLD);
            owner_ptr->window_flags |= (PW_PLAYER);
            delete_object_idx(owner_ptr, this_o_idx);
            continue;
        } else if (o_ptr->marked & OM_NOMSG) {
            o_ptr->marked &= ~(OM_NOMSG);
            continue;
        }

        if (check_store_item_to_inventory(owner_ptr, o_ptr))
            can_pickup++;

        floor_num++;
        floor_o_idx = this_o_idx;
    }

    if (!floor_num)
        return;

    if (!pickup) {
        if (floor_num == 1) {
            o_ptr = &owner_ptr->current_floor_ptr->o_list[floor_o_idx];
            describe_flavor(owner_ptr, o_name, o_ptr, 0);
            msg_format(_("%sがある。", "You see %s."), o_name);
        } else
            msg_format(_("%d 個のアイテムの山がある。", "You see a pile of %d items."), floor_num);

        return;
    }

    if (!can_pickup) {
        if (floor_num == 1) {
            o_ptr = &owner_ptr->current_floor_ptr->o_list[floor_o_idx];
            describe_flavor(owner_ptr, o_name, o_ptr, 0);
            msg_format(_("ザックには%sを入れる隙間がない。", "You have no room for %s."), o_name);
        } else
            msg_print(_("ザックには床にあるどのアイテムも入らない。", "You have no room for any of the objects on the floor."));

        return;
    }

    if (floor_num != 1) {
        while (can_pickup--)
            if (!py_pickup_floor_aux(owner_ptr))
                break;

        return;
    }

    if (carry_query_flag) {
        char out_val[MAX_NLEN + 20];
        o_ptr = &owner_ptr->current_floor_ptr->o_list[floor_o_idx];
        describe_flavor(owner_ptr, o_name, o_ptr, 0);
        (void)sprintf(out_val, _("%sを拾いますか? ", "Pick up %s? "), o_name);
        if (!get_check(out_val))
            return;
    }

    o_ptr = &owner_ptr->current_floor_ptr->o_list[floor_o_idx];
    describe_pickup_item(owner_ptr, floor_o_idx);
}

/*!
 * @brief プレイヤーがオブジェクトを拾った際のメッセージ表示処理 /
 * Helper routine for py_pickup() and py_pickup_floor().
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param o_idx 取得したオブジェクトの参照ID
 * @return なし
 * @details
 * アイテムを拾った際に「２つのケーキを持っている」
 * "You have two cakes." とアイテムを拾った後の合計のみの表示がオリジナルだが、
 * 違和感があるという指摘をうけたので、「～を拾った、～を持っている」という表示にかえてある。
 * そのための配列。
 * Add the given dungeon object to the character's inventory.\n
 * Delete the object afterwards.\n
 */
void describe_pickup_item(player_type *owner_ptr, OBJECT_IDX o_idx)
{
#ifdef JP
    GAME_TEXT o_name[MAX_NLEN];
    GAME_TEXT old_name[MAX_NLEN];
    char kazu_str[80];
    int hirottakazu;
#else
    GAME_TEXT o_name[MAX_NLEN];
#endif

    object_type *o_ptr;
    o_ptr = &owner_ptr->current_floor_ptr->o_list[o_idx];

#ifdef JP
    describe_flavor(owner_ptr, old_name, o_ptr, OD_NAME_ONLY);
    object_desc_count_japanese(kazu_str, o_ptr);
    hirottakazu = o_ptr->number;
#endif

    INVENTORY_IDX slot = store_item_to_inventory(owner_ptr, o_ptr);
    o_ptr = &owner_ptr->inventory_list[slot];
    delete_object_idx(owner_ptr, o_idx);
    if (owner_ptr->pseikaku == PERSONALITY_MUNCHKIN) {
        bool old_known = identify_item(owner_ptr, o_ptr);
        autopick_alter_item(owner_ptr, slot, (bool)(destroy_identify && !old_known));
        if (o_ptr->marked & OM_AUTODESTROY)
            return;
    }

    describe_flavor(owner_ptr, o_name, o_ptr, 0);

#ifdef JP
    if ((o_ptr->name1 == ART_CRIMSON) && (owner_ptr->pseikaku == PERSONALITY_COMBAT)) {
        msg_format("こうして、%sは『クリムゾン』を手に入れた。", owner_ptr->name);
        msg_print("しかし今、『混沌のサーペント』の放ったモンスターが、");
        msg_format("%sに襲いかかる．．．", owner_ptr->name);
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
    record_turn = current_world_ptr->game_turn;
    check_find_art_quest_completion(owner_ptr, o_ptr);
}

/*!
 * @brief プレイヤーがオブジェクト上に乗った際の表示処理 / Player "wants" to pick up an object or gold.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param pickup 自動拾い処理を行うならばTRUEとする
 * @return なし
 */
void carry(player_type *creature_ptr, bool pickup)
{
    verify_panel(creature_ptr);
    creature_ptr->update |= PU_MONSTERS;
    creature_ptr->redraw |= PR_MAP;
    creature_ptr->window_flags |= PW_OVERHEAD;
    handle_stuff(creature_ptr);
    grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x];
    autopick_pickup_items(creature_ptr, g_ptr);
    if (easy_floor) {
        py_pickup_floor(creature_ptr, pickup);
        return;
    }

    OBJECT_IDX next_o_idx = 0;
    for (OBJECT_IDX this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
        object_type *o_ptr;
        o_ptr = &creature_ptr->current_floor_ptr->o_list[this_o_idx];
        GAME_TEXT o_name[MAX_NLEN];
        describe_flavor(creature_ptr, o_name, o_ptr, 0);
        next_o_idx = o_ptr->next_o_idx;
        disturb(creature_ptr, FALSE, FALSE);
        if (o_ptr->tval == TV_GOLD) {
            int value = (long)o_ptr->pval;
            delete_object_idx(creature_ptr, this_o_idx);
            msg_format(_(" $%ld の価値がある%sを見つけた。", "You collect %ld gold pieces worth of %s."), (long)value, o_name);
            sound(SOUND_SELL);
            creature_ptr->au += value;
            creature_ptr->redraw |= (PR_GOLD);
            creature_ptr->window_flags |= (PW_PLAYER);
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

        if (!check_store_item_to_inventory(creature_ptr, o_ptr)) {
            msg_format(_("ザックには%sを入れる隙間がない。", "You have no room for %s."), o_name);
            continue;
        }

        int is_pickup_successful = TRUE;
        if (carry_query_flag) {
            char out_val[MAX_NLEN + 20];
            sprintf(out_val, _("%sを拾いますか? ", "Pick up %s? "), o_name);
            is_pickup_successful = get_check(out_val);
        }

        if (is_pickup_successful) {
            describe_pickup_item(creature_ptr, this_o_idx);
        }
    }
}
