#include "inventory/item-getter.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/input-options.h"
#include "game-option/option-flags.h"
#include "game-option/text-display-options.h"
#include "inventory/floor-item-getter.h"
#include "inventory/inventory-slot-types.h"
#include "inventory/inventory-util.h"
#include "inventory/item-selection-util.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "player/player-status-flags.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-inventory.h"
#include "view/display-messages.h"
#include "window/display-sub-windows.h"

/*!
 * @brief オブジェクト選択のモード設定
 * @param item_selection_ptr アイテム選択への参照ポインタ
 */
static void check_item_selection_mode(item_selection_type *item_selection_ptr)
{
    if (item_selection_ptr->mode & USE_EQUIP) {
        item_selection_ptr->equip = true;
    }

    if (item_selection_ptr->mode & USE_INVEN) {
        item_selection_ptr->inven = true;
    }

    if (item_selection_ptr->mode & USE_FLOOR) {
        item_selection_ptr->floor = true;
    }
}

/*!
 * @brief アイテムへにタグ付けがされているかの調査処理 (のはず)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param item_selection_ptr アイテムへの参照ポインタ
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す
 * @todo 適切な関数名をどうしても付けられなかったので暫定でauxとした
 */
static bool check_item_tag_aux(PlayerType *player_ptr, item_selection_type *item_selection_ptr, const ItemTester &item_tester)
{
    if (!item_selection_ptr->floor || (*item_selection_ptr->cp >= 0)) {
        return false;
    }

    ItemEntity *o_ptr;
    item_selection_ptr->k = 0 - (*item_selection_ptr->cp);
    o_ptr = &player_ptr->current_floor_ptr->o_list[item_selection_ptr->k];
    if (!item_tester.okay(o_ptr) && ((item_selection_ptr->mode & USE_FULL) == 0)) {
        return false;
    }

    command_cmd = 0;
    return true;
}

/*!
 * @brief インベントリのアイテムにタグ付けがされているかの調査処理 (のはず)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fis_ptr 床上アイテムへの参照ポインタ
 * @param prev_tag 前回選択したアイテムのタグ (のはず)
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す
 */
static bool check_item_tag_inventory(PlayerType *player_ptr, item_selection_type *item_selection_ptr, char *prev_tag, const ItemTester &item_tester)
{
    if ((!item_selection_ptr->inven || (*item_selection_ptr->cp < 0) || (*item_selection_ptr->cp >= INVEN_PACK)) && (!item_selection_ptr->equip || (*item_selection_ptr->cp < INVEN_MAIN_HAND) || (*item_selection_ptr->cp >= INVEN_TOTAL))) {
        return false;
    }

    if (*prev_tag && command_cmd) {

        bool flag = false;
        item_use_flag use_flag = (*item_selection_ptr->cp >= INVEN_MAIN_HAND) ? USE_EQUIP : USE_INVEN;

        flag |= !get_tag(player_ptr, &item_selection_ptr->k, *prev_tag, use_flag, item_tester);
        flag |= !get_item_okay(player_ptr, item_selection_ptr->k, item_tester);

        if (item_selection_ptr->k < INVEN_MAIN_HAND) {
            flag |= !item_selection_ptr->inven;
        } else {
            flag |= !item_selection_ptr->equip;
        }

        if (flag) {
            *prev_tag = '\0';
            return false;
        }

        *item_selection_ptr->cp = item_selection_ptr->k;
        command_cmd = 0;
        return true;
    }

    if (!get_item_okay(player_ptr, *item_selection_ptr->cp, item_tester)) {
        return false;
    }

    command_cmd = 0;
    return true;
}

/*!
 * @brief アイテムにタグ付けがされているかの調査処理 (のはず)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param item_selection_ptr アイテムへの参照ポインタ
 * @param prev_tag 前回選択したアイテムのタグ (のはず)
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す
 */
static bool check_item_tag(PlayerType *player_ptr, item_selection_type *item_selection_ptr, char *prev_tag, const ItemTester &item_tester)
{
    if (!repeat_pull(item_selection_ptr->cp)) {
        return false;
    }

    if (item_selection_ptr->mode & USE_FORCE && (*item_selection_ptr->cp == INVEN_FORCE)) {
        command_cmd = 0;
        return true;
    }

    if (check_item_tag_aux(player_ptr, item_selection_ptr, item_tester)) {
        return true;
    }

    return check_item_tag_inventory(player_ptr, item_selection_ptr, prev_tag, item_tester);
}

/*!
 * @brief インベントリ内のアイテムが妥当かを判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fis_ptr アイテム選択への参照ポインタ
 */
static void test_inventory(PlayerType *player_ptr, item_selection_type *item_selection_ptr, const ItemTester &item_tester)
{
    if (!item_selection_ptr->inven) {
        item_selection_ptr->i2 = -1;
        return;
    }

    if (!use_menu) {
        return;
    }

    for (int j = 0; j < INVEN_PACK; j++) {
        if (item_tester.okay(&player_ptr->inventory_list[j]) || (item_selection_ptr->mode & USE_FULL)) {
            item_selection_ptr->max_inven++;
        }
    }
}

/*!
 * @brief 装備品が妥当かを判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fis_ptr アイテム選択への参照ポインタ
 */
static void test_equipment(PlayerType *player_ptr, item_selection_type *item_selection_ptr, const ItemTester &item_tester)
{
    if (!item_selection_ptr->equip) {
        item_selection_ptr->e2 = -1;
        return;
    }

    if (!use_menu) {
        return;
    }

    for (int j = INVEN_MAIN_HAND; j < INVEN_TOTAL; j++) {
        if (player_ptr->select_ring_slot ? is_ring_slot(j)
                                         : item_tester.okay(&player_ptr->inventory_list[j]) || (item_selection_ptr->mode & USE_FULL)) {
            item_selection_ptr->max_equip++;
        }
    }

    if (has_two_handed_weapons(player_ptr) && !(item_selection_ptr->mode & IGNORE_BOTHHAND_SLOT)) {
        item_selection_ptr->max_equip++;
    }
}

/*!
 * @brief オブジェクト選択の汎用関数 / General function for the selection of item
 * Let the user select an item, save its "index"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param cp 選択したオブジェクトのID
 * @param pmt 選択目的のメッセージ
 * @param str 選択できるオブジェクトがない場合のキャンセルメッセージ
 * @param mode オプションフラグ
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す
 * Return TRUE only if an acceptable item was chosen by the user
 */
bool get_item(PlayerType *player_ptr, OBJECT_IDX *cp, concptr pmt, concptr str, BIT_FLAGS mode, const ItemTester &item_tester)
{
    static char prev_tag = '\0';
    if (easy_floor || use_menu) {
        return get_item_floor(player_ptr, cp, pmt, str, mode, item_tester);
    }

    item_selection_type tmp_selection;
    item_selection_type *item_selection_ptr = initialize_item_selection_type(&tmp_selection, cp, mode);
    check_item_selection_mode(item_selection_ptr);
    if (check_item_tag(player_ptr, item_selection_ptr, &prev_tag, item_tester)) {
        return true;
    }

    msg_print(nullptr);
    item_selection_ptr->done = false;
    item_selection_ptr->item = false;
    item_selection_ptr->i1 = 0;
    item_selection_ptr->i2 = INVEN_PACK - 1;
    test_inventory(player_ptr, item_selection_ptr, item_tester);
    while ((item_selection_ptr->i1 <= item_selection_ptr->i2) && (!get_item_okay(player_ptr, item_selection_ptr->i1, item_tester))) {
        item_selection_ptr->i1++;
    }

    while ((item_selection_ptr->i1 <= item_selection_ptr->i2) && (!get_item_okay(player_ptr, item_selection_ptr->i2, item_tester))) {
        item_selection_ptr->i2--;
    }

    item_selection_ptr->e1 = INVEN_MAIN_HAND;
    item_selection_ptr->e2 = INVEN_TOTAL - 1;
    test_equipment(player_ptr, item_selection_ptr, item_tester);
    while ((item_selection_ptr->e1 <= item_selection_ptr->e2) && (!get_item_okay(player_ptr, item_selection_ptr->e1, item_tester))) {
        item_selection_ptr->e1++;
    }

    while ((item_selection_ptr->e1 <= item_selection_ptr->e2) && (!get_item_okay(player_ptr, item_selection_ptr->e2, item_tester))) {
        item_selection_ptr->e2--;
    }

    if (item_selection_ptr->equip && has_two_handed_weapons(player_ptr) && !(item_selection_ptr->mode & IGNORE_BOTHHAND_SLOT)) {
        if (can_attack_with_main_hand(player_ptr)) {
            if (item_selection_ptr->e2 < INVEN_SUB_HAND) {
                item_selection_ptr->e2 = INVEN_SUB_HAND;
            }
        } else if (can_attack_with_sub_hand(player_ptr)) {
            item_selection_ptr->e1 = INVEN_MAIN_HAND;
        }
    }

    if (item_selection_ptr->floor) {
        for (const auto this_o_idx : player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].o_idx_list) {
            ItemEntity *o_ptr;
            o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];
            if ((item_tester.okay(o_ptr) || (item_selection_ptr->mode & USE_FULL)) && o_ptr->marked.has(OmType::FOUND)) {
                item_selection_ptr->allow_floor = true;
            }
        }
    }

    if (!item_selection_ptr->allow_floor && (item_selection_ptr->i1 > item_selection_ptr->i2) && (item_selection_ptr->e1 > item_selection_ptr->e2)) {
        command_see = false;
        item_selection_ptr->oops = true;
        item_selection_ptr->done = true;

        if (item_selection_ptr->mode & USE_FORCE) {
            *item_selection_ptr->cp = INVEN_FORCE;
            item_selection_ptr->item = true;
        }
    } else {
        if (command_see && command_wrk && item_selection_ptr->equip) {
            command_wrk = true;
        } else if (item_selection_ptr->inven) {
            command_wrk = false;
        } else if (item_selection_ptr->equip) {
            command_wrk = true;
        } else {
            command_wrk = false;
        }
    }

    /* 追加オプション(always_show_list)が設定されている場合は常に一覧を表示する */
    if (always_show_list || use_menu) {
        command_see = true;
    }

    if (command_see) {
        screen_save();
    }

    while (!item_selection_ptr->done) {
        COMMAND_CODE get_item_label = 0;
        int ni = 0;
        int ne = 0;
        for (auto i = 0U; i < angband_terms.size(); ++i) {
            if (!angband_terms[i]) {
                continue;
            }

            if (window_flag[i] & (PW_INVEN)) {
                ni++;
            }

            if (window_flag[i] & (PW_EQUIP)) {
                ne++;
            }
        }

        if ((command_wrk && ni && !ne) || (!command_wrk && !ni && ne)) {
            toggle_inventory_equipment(player_ptr);
            item_selection_ptr->toggle = !item_selection_ptr->toggle;
        }

        player_ptr->window_flags |= (PW_INVEN | PW_EQUIP);
        handle_stuff(player_ptr);

        if (!command_wrk) {
            if (command_see) {
                get_item_label = show_inventory(player_ptr, item_selection_ptr->menu_line, item_selection_ptr->mode, item_tester);
            }
        } else {
            if (command_see) {
                get_item_label = show_equipment(player_ptr, item_selection_ptr->menu_line, item_selection_ptr->mode, item_tester);
            }
        }

        if (!command_wrk) {
            sprintf(item_selection_ptr->out_val, _("持ち物:", "Inven:"));
            if ((item_selection_ptr->i1 <= item_selection_ptr->i2) && !use_menu) {
                char tmp_val[80];
                sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"), index_to_label(item_selection_ptr->i1),
                    index_to_label(item_selection_ptr->i2));
                strcat(item_selection_ptr->out_val, tmp_val);
            }

            if (!command_see && !use_menu) {
                strcat(item_selection_ptr->out_val, _(" '*'一覧,", " * to see,"));
            }

            if (item_selection_ptr->equip) {
                strcat(item_selection_ptr->out_val, format(_(" %s 装備品,", " %s for Equip,"), use_menu ? _("'4'or'6'", "4 or 6") : _("'/'", "/")));
            }
        } else {
            sprintf(item_selection_ptr->out_val, _("装備品:", "Equip:"));
            if ((item_selection_ptr->e1 <= item_selection_ptr->e2) && !use_menu) {
                char tmp_val[80];
                sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"), index_to_label(item_selection_ptr->e1),
                    index_to_label(item_selection_ptr->e2));
                strcat(item_selection_ptr->out_val, tmp_val);
            }

            if (!command_see && !use_menu) {
                strcat(item_selection_ptr->out_val, _(" '*'一覧,", " * to see,"));
            }

            if (item_selection_ptr->inven) {
                strcat(item_selection_ptr->out_val, format(_(" %s 持ち物,", " %s for Inven,"), use_menu ? _("'4'or'6'", "4 or 6") : _("'/'", "'/'")));
            }
        }

        if (item_selection_ptr->allow_floor) {
            strcat(item_selection_ptr->out_val, _(" '-'床上,", " - for floor,"));
        }

        if (item_selection_ptr->mode & USE_FORCE) {
            strcat(item_selection_ptr->out_val, _(" 'w'練気術,", " w for the Force,"));
        }

        strcat(item_selection_ptr->out_val, " ESC");
        sprintf(item_selection_ptr->tmp_val, "(%s) %s", item_selection_ptr->out_val, pmt);
        prt(item_selection_ptr->tmp_val, 0, 0);
        item_selection_ptr->which = inkey();
        if (use_menu) {
            int max_line = (command_wrk ? item_selection_ptr->max_equip : item_selection_ptr->max_inven);
            switch (item_selection_ptr->which) {
            case ESCAPE:
            case 'z':
            case 'Z':
            case '0': {
                item_selection_ptr->done = true;
                break;
            }

            case '8':
            case 'k':
            case 'K': {
                item_selection_ptr->menu_line += (max_line - 1);
                break;
            }

            case '2':
            case 'j':
            case 'J': {
                item_selection_ptr->menu_line++;
                break;
            }

            case '4':
            case '6':
            case 'h':
            case 'H':
            case 'l':
            case 'L': {
                if (!item_selection_ptr->inven || !item_selection_ptr->equip) {
                    bell();
                    break;
                }

                if (command_see) {
                    screen_load();
                    screen_save();
                }

                command_wrk = !command_wrk;
                max_line = (command_wrk ? item_selection_ptr->max_equip : item_selection_ptr->max_inven);
                if (item_selection_ptr->menu_line > max_line) {
                    item_selection_ptr->menu_line = max_line;
                }

                break;
            }

            case 'x':
            case 'X':
            case '\r':
            case '\n': {
                if (command_wrk == USE_FLOOR) {
                    *item_selection_ptr->cp = -get_item_label;
                } else {
                    if (!get_item_okay(player_ptr, get_item_label, item_tester)) {
                        bell();
                        break;
                    }

                    if (!get_item_allow(player_ptr, get_item_label)) {
                        item_selection_ptr->done = true;
                        break;
                    }

                    *item_selection_ptr->cp = get_item_label;
                }

                item_selection_ptr->item = true;
                item_selection_ptr->done = true;
                break;
            }
            case 'w': {
                if (item_selection_ptr->mode & USE_FORCE) {
                    *item_selection_ptr->cp = INVEN_FORCE;
                    item_selection_ptr->item = true;
                    item_selection_ptr->done = true;
                    break;
                }
            }
            }

            if (item_selection_ptr->menu_line > max_line) {
                item_selection_ptr->menu_line -= max_line;
            }

            continue;
        }

        switch (item_selection_ptr->which) {
        case ESCAPE: {
            item_selection_ptr->done = true;
            break;
        }
        case '*':
        case '?':
        case ' ': {
            if (command_see) {
                command_see = false;
                screen_load();
            } else {
                screen_save();
                command_see = true;
            }

            break;
        }
        case '/': {
            if (!item_selection_ptr->inven || !item_selection_ptr->equip) {
                bell();
                break;
            }

            if (command_see) {
                screen_load();
                screen_save();
            }

            command_wrk = !command_wrk;
            break;
        }
        case '-': {
            if (item_selection_ptr->allow_floor) {
                for (const auto this_o_idx : player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].o_idx_list) {
                    ItemEntity *o_ptr;
                    o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];
                    if (!item_tester.okay(o_ptr) && !(item_selection_ptr->mode & USE_FULL)) {
                        continue;
                    }

                    item_selection_ptr->k = 0 - this_o_idx;
                    if ((other_query_flag && !verify(player_ptr, _("本当に", "Try"), item_selection_ptr->k)) || !get_item_allow(player_ptr, item_selection_ptr->k)) {
                        continue;
                    }

                    *item_selection_ptr->cp = item_selection_ptr->k;
                    item_selection_ptr->item = true;
                    item_selection_ptr->done = true;
                    break;
                }

                if (item_selection_ptr->done) {
                    break;
                }
            }

            bell();
            break;
        }
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            if (!get_tag(player_ptr, &item_selection_ptr->k, item_selection_ptr->which, command_wrk ? USE_EQUIP : USE_INVEN, item_tester)) {
                bell();
                break;
            }

            if ((item_selection_ptr->k < INVEN_MAIN_HAND) ? !item_selection_ptr->inven : !item_selection_ptr->equip) {
                bell();
                break;
            }

            if (!get_item_okay(player_ptr, item_selection_ptr->k, item_tester)) {
                bell();
                break;
            }

            if (!get_item_allow(player_ptr, item_selection_ptr->k)) {
                item_selection_ptr->done = true;
                break;
            }

            *item_selection_ptr->cp = item_selection_ptr->k;
            item_selection_ptr->item = true;
            item_selection_ptr->done = true;
            item_selection_ptr->cur_tag = item_selection_ptr->which;
            break;
        }
        case 'w': {
            if (item_selection_ptr->mode & USE_FORCE) {
                *item_selection_ptr->cp = INVEN_FORCE;
                item_selection_ptr->item = true;
                item_selection_ptr->done = true;
                break;
            }
        }
            [[fallthrough]];
        default: {
            bool tag_not_found = false;

            if (!get_tag(player_ptr, &item_selection_ptr->k, item_selection_ptr->which, command_wrk ? USE_EQUIP : USE_INVEN, item_tester)) {
                tag_not_found = true;
            } else if ((item_selection_ptr->k < INVEN_MAIN_HAND) ? !item_selection_ptr->inven : !item_selection_ptr->equip) {
                tag_not_found = true;
            }

            if (!tag_not_found) {
                item_selection_ptr->cur_tag = item_selection_ptr->which;
            } else {
                auto which = (char)tolower(item_selection_ptr->which);

                if (!command_wrk) {
                    if (which == '(') {
                        item_selection_ptr->k = item_selection_ptr->i1;
                    } else if (which == ')') {
                        item_selection_ptr->k = item_selection_ptr->i2;
                    } else {
                        item_selection_ptr->k = label_to_inventory(player_ptr, which);
                    }
                } else {
                    if (which == '(') {
                        item_selection_ptr->k = item_selection_ptr->e1;
                    } else if (which == ')') {
                        item_selection_ptr->k = item_selection_ptr->e2;
                    } else {
                        item_selection_ptr->k = label_to_equipment(player_ptr, which);
                    }
                }
            }

            if (!get_item_okay(player_ptr, item_selection_ptr->k, item_tester)) {
                bell();
                break;
            }

            auto ver = tag_not_found && isupper(item_selection_ptr->which);
            if (ver && !verify(player_ptr, _("本当に", "Try"), item_selection_ptr->k)) {
                item_selection_ptr->done = true;
                break;
            }

            if (!get_item_allow(player_ptr, item_selection_ptr->k)) {
                item_selection_ptr->done = true;
                break;
            }

            *item_selection_ptr->cp = item_selection_ptr->k;
            item_selection_ptr->item = true;
            item_selection_ptr->done = true;
            break;
        }
        }
    }

    if (command_see) {
        screen_load();
        command_see = false;
    }

    if (item_selection_ptr->toggle) {
        toggle_inventory_equipment(player_ptr);
    }

    player_ptr->window_flags |= (PW_INVEN | PW_EQUIP);
    handle_stuff(player_ptr);
    prt("", 0, 0);
    if (item_selection_ptr->oops && str) {
        msg_print(str);
    }

    if (item_selection_ptr->item) {
        repeat_push(*item_selection_ptr->cp);
        if (command_cmd) {
            prev_tag = item_selection_ptr->cur_tag;
        }
        command_cmd = 0;
    }

    return item_selection_ptr->item;
}
