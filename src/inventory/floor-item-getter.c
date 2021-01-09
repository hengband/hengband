/*!
 * @brief オブジェクト選択処理
 * @date 2020/07/02
 * @author Hourier
 */

#include "inventory/floor-item-getter.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/floor-object.h"
#include "floor/object-scanner.h"
#include "game-option/input-options.h"
#include "game-option/option-flags.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
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
#include "player/player-status-flags.h"
#include "system/floor-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-inventory.h"
#include "view/display-messages.h"
#include "window/display-sub-windows.h"

/*!
 * todo 適切な関数名をどうしても付けられなかったので暫定でauxとした
 * @brief 床上アイテムへにタグ付けがされているかの調査処理 (のはず)
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param fis_ptr 床上アイテムへの参照ポインタ
 * @param prev_tag 前回選択したアイテムのタグ (のはず)
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す
 */
static bool check_floor_item_tag_aux(player_type *owner_ptr, fis_type *fis_ptr, char *prev_tag)
{
    if (!fis_ptr->floor || (*fis_ptr->cp >= 0))
        return FALSE;

    if (*prev_tag && command_cmd) {
        fis_ptr->floor_num = scan_floor_items(owner_ptr, fis_ptr->floor_list, owner_ptr->y, owner_ptr->x, 0x03, fis_ptr->tval);
        if (get_tag_floor(owner_ptr->current_floor_ptr, &fis_ptr->k, *prev_tag, fis_ptr->floor_list, fis_ptr->floor_num)) {
            *fis_ptr->cp = 0 - fis_ptr->floor_list[fis_ptr->k];
            fis_ptr->tval = 0;
            item_tester_hook = NULL;
            command_cmd = 0;
            return TRUE;
        }

        *prev_tag = '\0';
        return FALSE;
    }

    if (!item_tester_okay(owner_ptr, &owner_ptr->current_floor_ptr->o_list[0 - (*fis_ptr->cp)], fis_ptr->tval) && ((fis_ptr->mode & USE_FULL) == 0))
        return FALSE;

    fis_ptr->tval = 0;
    item_tester_hook = NULL;
    command_cmd = 0;
    return TRUE;
}

/*!
 * @brief インベントリのアイテムにタグ付けを試みる
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param fis_ptr 床上アイテムへの参照ポインタ
 * @param prev_tag 前回選択したアイテムのタグ (のはず)
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す
 */
static bool get_floor_item_tag_inventory(player_type *owner_ptr, fis_type *fis_ptr, char *prev_tag)
{
    if ((*prev_tag == '\0') || !command_cmd)
        return FALSE;

    if (!get_tag(owner_ptr, &fis_ptr->k, *prev_tag, (*fis_ptr->cp >= INVEN_RARM) ? USE_EQUIP : USE_INVEN, fis_ptr->tval)
        || ((fis_ptr->k < INVEN_RARM) ? !fis_ptr->inven : !fis_ptr->equip) || !get_item_okay(owner_ptr, fis_ptr->k, fis_ptr->tval)) {
        *prev_tag = '\0';
        return FALSE;
    }

    *fis_ptr->cp = fis_ptr->k;
    fis_ptr->tval = TV_NONE;
    item_tester_hook = NULL;
    command_cmd = 0;
    return TRUE;
}

/*!
 * @brief インベントリのアイテムにタグ付けがされているかの調査処理 (のはず)
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param fis_ptr 床上アイテムへの参照ポインタ
 * @param prev_tag 前回選択したアイテムのタグ (のはず)
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す
 */
static bool check_floor_item_tag_inventory(player_type *owner_ptr, fis_type *fis_ptr, char *prev_tag)
{
    if ((!fis_ptr->inven || (*fis_ptr->cp < 0) || (*fis_ptr->cp >= INVEN_PACK))
        && (!fis_ptr->equip || (*fis_ptr->cp < INVEN_RARM) || (*fis_ptr->cp >= INVEN_TOTAL)))
        return FALSE;

    if (get_floor_item_tag_inventory(owner_ptr, fis_ptr, prev_tag))
        return TRUE;

    if (get_item_okay(owner_ptr, *fis_ptr->cp, fis_ptr->tval)) {
        fis_ptr->tval = 0;
        item_tester_hook = NULL;
        command_cmd = 0;
        return TRUE;
    }

    return FALSE;
}

/*!
 * @brief 床上アイテムにタグ付けがされているかの調査処理 (のはず)
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param fis_ptr 床上アイテムへの参照ポインタ
 * @param prev_tag 前回選択したアイテムのタグ (のはず)
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す
 */
static bool check_floor_item_tag(player_type *owner_ptr, fis_type *fis_ptr, char *prev_tag)
{
    if (!repeat_pull(fis_ptr->cp))
        return FALSE;

    if (fis_ptr->force && (*fis_ptr->cp == INVEN_FORCE)) {
        fis_ptr->tval = 0;
        item_tester_hook = NULL;
        command_cmd = 0;
        return TRUE;
    }

    if (check_floor_item_tag_aux(owner_ptr, fis_ptr, prev_tag))
        return TRUE;

    return check_floor_item_tag_inventory(owner_ptr, fis_ptr, prev_tag);
}

/*!
 * @brief インベントリ内のアイテムが妥当かを判定する
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param fis_ptr 床上アイテムへの参照ポインタ
 * @return なし
 */
static void test_inventory_floor(player_type *owner_ptr, fis_type *fis_ptr)
{
    if (!fis_ptr->inven) {
        fis_ptr->i2 = -1;
        return;
    }

    if (!use_menu)
        return;

    for (int i = 0; i < INVEN_PACK; i++)
        if (item_tester_okay(owner_ptr, &owner_ptr->inventory_list[i], fis_ptr->tval) || (fis_ptr->mode & USE_FULL))
            fis_ptr->max_inven++;
}

/*!
 * @brief 装備品がが妥当かを判定する
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param fis_ptr 床上アイテムへの参照ポインタ
 * @return なし
 */
static void test_equipment_floor(player_type *owner_ptr, fis_type *fis_ptr)
{
    if (!fis_ptr->equip) {
        fis_ptr->e2 = -1;
        return;
    }

    if (!use_menu)
        return;

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++)
        if (owner_ptr->select_ring_slot ? is_ring_slot(i) : item_tester_okay(owner_ptr, &owner_ptr->inventory_list[i], fis_ptr->tval) || (fis_ptr->mode & USE_FULL))
            fis_ptr->max_equip++;
}

/*!
 * @brief オブジェクト選択の汎用関数(床上アイテム用) /
 * Let the user select an item, save its "index"
 * @param cp 選択したオブジェクトのIDを返す。
 * @param pmt 選択目的のメッセージ
 * @param str 選択できるオブジェクトがない場合のキャンセルメッセージ
 * @param mode オプションフラグ
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す。/
 */
bool get_item_floor(player_type *owner_ptr, COMMAND_CODE *cp, concptr pmt, concptr str, BIT_FLAGS mode, tval_type tval)
{
    fis_type tmp_fis;
    fis_type *fis_ptr = initialize_fis_type(&tmp_fis, cp, mode, tval);
    static char prev_tag = '\0';
    if (check_floor_item_tag(owner_ptr, fis_ptr, &prev_tag))
        return TRUE;

    msg_print(NULL);
    test_inventory_floor(owner_ptr, fis_ptr);
    fis_ptr->done = FALSE;
    fis_ptr->item = FALSE;
    fis_ptr->i1 = 0;
    fis_ptr->i2 = INVEN_PACK - 1;
    while ((fis_ptr->i1 <= fis_ptr->i2) && (!get_item_okay(owner_ptr, fis_ptr->i1, fis_ptr->tval)))
        fis_ptr->i1++;

    while ((fis_ptr->i1 <= fis_ptr->i2) && (!get_item_okay(owner_ptr, fis_ptr->i2, fis_ptr->tval)))
        fis_ptr->i2--;

    fis_ptr->e1 = INVEN_RARM;
    fis_ptr->e2 = INVEN_TOTAL - 1;
    test_equipment_floor(owner_ptr, fis_ptr);
    if (has_two_handed_weapons(owner_ptr) && !(fis_ptr->mode & IGNORE_BOTHHAND_SLOT))
        fis_ptr->max_equip++;

    while ((fis_ptr->e1 <= fis_ptr->e2) && (!get_item_okay(owner_ptr, fis_ptr->e1, fis_ptr->tval)))
        fis_ptr->e1++;

    while ((fis_ptr->e1 <= fis_ptr->e2) && (!get_item_okay(owner_ptr, fis_ptr->e2, fis_ptr->tval)))
        fis_ptr->e2--;

    if (fis_ptr->equip && has_two_handed_weapons(owner_ptr) && !(fis_ptr->mode & IGNORE_BOTHHAND_SLOT)) {
        if (has_right_hand_weapon(owner_ptr)) {
            if (fis_ptr->e2 < INVEN_LARM)
                fis_ptr->e2 = INVEN_LARM;
        } else if (has_left_hand_weapon(owner_ptr))
            fis_ptr->e1 = INVEN_RARM;
    }

    fis_ptr->floor_num = 0;
    if (fis_ptr->floor)
        fis_ptr->floor_num = scan_floor_items(owner_ptr, fis_ptr->floor_list, owner_ptr->y, owner_ptr->x, 0x03, fis_ptr->tval);

    if ((mode & USE_INVEN) && (fis_ptr->i1 <= fis_ptr->i2))
        fis_ptr->allow_inven = TRUE;

    if ((mode & USE_EQUIP) && (fis_ptr->e1 <= fis_ptr->e2))
        fis_ptr->allow_equip = TRUE;

    if ((mode & USE_FLOOR) && (fis_ptr->floor_num))
        fis_ptr->allow_floor = TRUE;

    if (!fis_ptr->allow_inven && !fis_ptr->allow_equip && !fis_ptr->allow_floor) {
        command_see = FALSE;
        fis_ptr->oops = TRUE;
        fis_ptr->done = TRUE;

        if (fis_ptr->force) {
            *cp = INVEN_FORCE;
            fis_ptr->item = TRUE;
        }
    } else {
        if (command_see && (command_wrk == USE_EQUIP) && fis_ptr->allow_equip)
            command_wrk = USE_EQUIP;
        else if (fis_ptr->allow_inven)
            command_wrk = USE_INVEN;
        else if (fis_ptr->allow_equip)
            command_wrk = USE_EQUIP;
        else if (fis_ptr->allow_floor)
            command_wrk = USE_FLOOR;
    }

    /* 追加オプション(always_show_list)が設定されている場合は常に一覧を表示する */
    if ((always_show_list == TRUE) || use_menu)
        command_see = TRUE;

    if (command_see)
        screen_save();

    while (!fis_ptr->done) {
        int ni = 0;
        int ne = 0;
        for (int i = 0; i < 8; i++) {
            if (!angband_term[i])
                continue;

            if (window_flag[i] & PW_INVEN)
                ni++;

            if (window_flag[i] & PW_EQUIP)
                ne++;
        }

        if ((command_wrk == (USE_EQUIP) && ni && !ne) || (command_wrk == (USE_INVEN) && !ni && ne)) {
            toggle_inventory_equipment(owner_ptr);
            fis_ptr->toggle = !fis_ptr->toggle;
        }

        owner_ptr->window |= (PW_INVEN | PW_EQUIP);
        handle_stuff(owner_ptr);
        COMMAND_CODE get_item_label = 0;
        if (command_wrk == USE_INVEN) {
            fis_ptr->n1 = I2A(fis_ptr->i1);
            fis_ptr->n2 = I2A(fis_ptr->i2);
            if (command_see)
                get_item_label = show_inventory(owner_ptr, fis_ptr->menu_line, fis_ptr->mode, fis_ptr->tval);
        } else if (command_wrk == USE_EQUIP) {
            fis_ptr->n1 = I2A(fis_ptr->e1 - INVEN_RARM);
            fis_ptr->n2 = I2A(fis_ptr->e2 - INVEN_RARM);
            if (command_see)
                get_item_label = show_equipment(owner_ptr, fis_ptr->menu_line, mode, fis_ptr->tval);
        } else if (command_wrk == USE_FLOOR) {
            int j = fis_ptr->floor_top;
            fis_ptr->k = MIN(fis_ptr->floor_top + 23, fis_ptr->floor_num) - 1;
            fis_ptr->n1 = I2A(j - fis_ptr->floor_top); // TODO: 常に'0'になる。どんな意図でこのようなコードになっているのか不明.
            fis_ptr->n2 = I2A(fis_ptr->k - fis_ptr->floor_top);
            if (command_see)
                get_item_label = show_floor_items(owner_ptr, fis_ptr->menu_line, owner_ptr->y, owner_ptr->x, &fis_ptr->min_width, fis_ptr->tval);
        }

        if (command_wrk == USE_INVEN) {
            sprintf(fis_ptr->out_val, _("持ち物:", "Inven:"));
            if (!use_menu) {
                sprintf(fis_ptr->tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"), index_to_label(fis_ptr->i1), index_to_label(fis_ptr->i2));
                strcat(fis_ptr->out_val, fis_ptr->tmp_val);
            }

            if (!command_see && !use_menu)
                strcat(fis_ptr->out_val, _(" '*'一覧,", " * to see,"));

            if (fis_ptr->allow_equip) {
                if (!use_menu)
                    strcat(fis_ptr->out_val, _(" '/' 装備品,", " / for Equip,"));
                else if (fis_ptr->allow_floor)
                    strcat(fis_ptr->out_val, _(" '6' 装備品,", " 6 for Equip,"));
                else
                    strcat(fis_ptr->out_val, _(" '4'or'6' 装備品,", " 4 or 6 for Equip,"));
            }

            if (fis_ptr->allow_floor) {
                if (!use_menu)
                    strcat(fis_ptr->out_val, _(" '-'床上,", " - for floor,"));
                else if (fis_ptr->allow_equip)
                    strcat(fis_ptr->out_val, _(" '4' 床上,", " 4 for floor,"));
                else
                    strcat(fis_ptr->out_val, _(" '4'or'6' 床上,", " 4 or 6 for floor,"));
            }
        } else if (command_wrk == (USE_EQUIP)) {
            sprintf(fis_ptr->out_val, _("装備品:", "Equip:"));
            if (!use_menu) {
                sprintf(fis_ptr->tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"), index_to_label(fis_ptr->e1), index_to_label(fis_ptr->e2));
                strcat(fis_ptr->out_val, fis_ptr->tmp_val);
            }

            if (!command_see && !use_menu)
                strcat(fis_ptr->out_val, _(" '*'一覧,", " * to see,"));

            if (fis_ptr->allow_inven) {
                if (!use_menu)
                    strcat(fis_ptr->out_val, _(" '/' 持ち物,", " / for Inven,"));
                else if (fis_ptr->allow_floor)
                    strcat(fis_ptr->out_val, _(" '4' 持ち物,", " 4 for Inven,"));
                else
                    strcat(fis_ptr->out_val, _(" '4'or'6' 持ち物,", " 4 or 6 for Inven,"));
            }

            if (fis_ptr->allow_floor) {
                if (!use_menu)
                    strcat(fis_ptr->out_val, _(" '-'床上,", " - for floor,"));
                else if (fis_ptr->allow_inven)
                    strcat(fis_ptr->out_val, _(" '6' 床上,", " 6 for floor,"));
                else
                    strcat(fis_ptr->out_val, _(" '4'or'6' 床上,", " 4 or 6 for floor,"));
            }
        } else if (command_wrk == USE_FLOOR) {
            sprintf(fis_ptr->out_val, _("床上:", "Floor:"));
            if (!use_menu) {
                sprintf(fis_ptr->tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"), fis_ptr->n1, fis_ptr->n2);
                strcat(fis_ptr->out_val, fis_ptr->tmp_val);
            }

            if (!command_see && !use_menu)
                strcat(fis_ptr->out_val, _(" '*'一覧,", " * to see,"));

            if (use_menu) {
                if (fis_ptr->allow_inven && fis_ptr->allow_equip) {
                    strcat(fis_ptr->out_val, _(" '4' 装備品, '6' 持ち物,", " 4 for Equip, 6 for Inven,"));
                } else if (fis_ptr->allow_inven) {
                    strcat(fis_ptr->out_val, _(" '4'or'6' 持ち物,", " 4 or 6 for Inven,"));
                } else if (fis_ptr->allow_equip) {
                    strcat(fis_ptr->out_val, _(" '4'or'6' 装備品,", " 4 or 6 for Equip,"));
                }
            } else if (fis_ptr->allow_inven) {
                strcat(fis_ptr->out_val, _(" '/' 持ち物,", " / for Inven,"));
            } else if (fis_ptr->allow_equip) {
                strcat(fis_ptr->out_val, _(" '/'装備品,", " / for Equip,"));
            }

            if (command_see && !use_menu) {
                strcat(fis_ptr->out_val, _(" Enter 次,", " Enter for scroll down,"));
            }
        }

        if (fis_ptr->force)
            strcat(fis_ptr->out_val, _(" 'w'練気術,", " w for the Force,"));

        strcat(fis_ptr->out_val, " ESC");
        sprintf(fis_ptr->tmp_val, "(%s) %s", fis_ptr->out_val, pmt);
        prt(fis_ptr->tmp_val, 0, 0);
        fis_ptr->which = inkey();
        if (use_menu) {
            int max_line = 1;
            if (command_wrk == USE_INVEN)
                max_line = fis_ptr->max_inven;
            else if (command_wrk == USE_EQUIP)
                max_line = fis_ptr->max_equip;
            else if (command_wrk == USE_FLOOR)
                max_line = MIN(23, fis_ptr->floor_num);
            switch (fis_ptr->which) {
            case ESCAPE:
            case 'z':
            case 'Z':
            case '0': {
                fis_ptr->done = TRUE;
                break;
            }
            case '8':
            case 'k':
            case 'K': {
                fis_ptr->menu_line += (max_line - 1);
                break;
            }
            case '2':
            case 'j':
            case 'J': {
                fis_ptr->menu_line++;
                break;
            }
            case '4':
            case 'h':
            case 'H': {
                if (command_wrk == (USE_INVEN)) {
                    if (fis_ptr->allow_floor)
                        command_wrk = USE_FLOOR;
                    else if (fis_ptr->allow_equip)
                        command_wrk = USE_EQUIP;
                    else {
                        bell();
                        break;
                    }
                } else if (command_wrk == (USE_EQUIP)) {
                    if (fis_ptr->allow_inven)
                        command_wrk = USE_INVEN;
                    else if (fis_ptr->allow_floor)
                        command_wrk = USE_FLOOR;
                    else {
                        bell();
                        break;
                    }
                } else if (command_wrk == (USE_FLOOR)) {
                    if (fis_ptr->allow_equip)
                        command_wrk = USE_EQUIP;
                    else if (fis_ptr->allow_inven)
                        command_wrk = USE_INVEN;
                    else {
                        bell();
                        break;
                    }
                } else {
                    bell();
                    break;
                }

                if (command_see) {
                    screen_load();
                    screen_save();
                }

                if (command_wrk == USE_INVEN)
                    max_line = fis_ptr->max_inven;
                else if (command_wrk == USE_EQUIP)
                    max_line = fis_ptr->max_equip;
                else if (command_wrk == USE_FLOOR)
                    max_line = MIN(23, fis_ptr->floor_num);

                if (fis_ptr->menu_line > max_line)
                    fis_ptr->menu_line = max_line;

                break;
            }
            case '6':
            case 'l':
            case 'L': {
                if (command_wrk == (USE_INVEN)) {
                    if (fis_ptr->allow_equip)
                        command_wrk = USE_EQUIP;
                    else if (fis_ptr->allow_floor)
                        command_wrk = USE_FLOOR;
                    else {
                        bell();
                        break;
                    }
                } else if (command_wrk == (USE_EQUIP)) {
                    if (fis_ptr->allow_floor)
                        command_wrk = USE_FLOOR;
                    else if (fis_ptr->allow_inven)
                        command_wrk = USE_INVEN;
                    else {
                        bell();
                        break;
                    }
                } else if (command_wrk == (USE_FLOOR)) {
                    if (fis_ptr->allow_inven)
                        command_wrk = USE_INVEN;
                    else if (fis_ptr->allow_equip)
                        command_wrk = USE_EQUIP;
                    else {
                        bell();
                        break;
                    }
                } else {
                    bell();
                    break;
                }

                if (command_see) {
                    screen_load();
                    screen_save();
                }

                if (command_wrk == USE_INVEN)
                    max_line = fis_ptr->max_inven;
                else if (command_wrk == USE_EQUIP)
                    max_line = fis_ptr->max_equip;
                else if (command_wrk == USE_FLOOR)
                    max_line = MIN(23, fis_ptr->floor_num);

                if (fis_ptr->menu_line > max_line)
                    fis_ptr->menu_line = max_line;

                break;
            }
            case 'x':
            case 'X':
            case '\r':
            case '\n': {
                if (command_wrk == USE_FLOOR)
                    *cp = -get_item_label;
                else {
                    if (!get_item_okay(owner_ptr, get_item_label, fis_ptr->tval)) {
                        bell();
                        break;
                    }

                    if (!get_item_allow(owner_ptr, get_item_label)) {
                        fis_ptr->done = TRUE;
                        break;
                    }

                    *cp = get_item_label;
                }

                fis_ptr->item = TRUE;
                fis_ptr->done = TRUE;
                break;
            }
            case 'w': {
                if (fis_ptr->force) {
                    *cp = INVEN_FORCE;
                    fis_ptr->item = TRUE;
                    fis_ptr->done = TRUE;
                    break;
                }
            }
            }

            if (fis_ptr->menu_line > max_line)
                fis_ptr->menu_line -= max_line;

            continue;
        }

        switch (fis_ptr->which) {
        case ESCAPE: {
            fis_ptr->done = TRUE;
            break;
        }
        case '*':
        case '?':
        case ' ': {
            if (command_see) {
                command_see = FALSE;
                screen_load();
            } else {
                screen_save();
                command_see = TRUE;
            }

            break;
        }
        case '\n':
        case '\r':
        case '+': {
            int i;
            OBJECT_IDX o_idx;
            grid_type *g_ptr = &owner_ptr->current_floor_ptr->grid_array[owner_ptr->y][owner_ptr->x];
            if (command_wrk != (USE_FLOOR))
                break;

            o_idx = g_ptr->o_idx;
            if (!(o_idx && owner_ptr->current_floor_ptr->o_list[o_idx].next_o_idx))
                break;

            excise_object_idx(owner_ptr->current_floor_ptr, o_idx);
            i = g_ptr->o_idx;
            while (owner_ptr->current_floor_ptr->o_list[i].next_o_idx)
                i = owner_ptr->current_floor_ptr->o_list[i].next_o_idx;

            owner_ptr->current_floor_ptr->o_list[i].next_o_idx = o_idx;
            fis_ptr->floor_num = scan_floor_items(owner_ptr, fis_ptr->floor_list, owner_ptr->y, owner_ptr->x, 0x03, fis_ptr->tval);
            if (command_see) {
                screen_load();
                screen_save();
            }

            break;
        }
        case '/': {
            if (command_wrk == (USE_INVEN)) {
                if (!fis_ptr->allow_equip) {
                    bell();
                    break;
                }
                command_wrk = (USE_EQUIP);
            } else if (command_wrk == (USE_EQUIP)) {
                if (!fis_ptr->allow_inven) {
                    bell();
                    break;
                }
                command_wrk = (USE_INVEN);
            } else if (command_wrk == (USE_FLOOR)) {
                if (fis_ptr->allow_inven) {
                    command_wrk = (USE_INVEN);
                } else if (fis_ptr->allow_equip) {
                    command_wrk = (USE_EQUIP);
                } else {
                    bell();
                    break;
                }
            }

            if (command_see) {
                screen_load();
                screen_save();
            }

            break;
        }
        case '-': {
            if (!fis_ptr->allow_floor) {
                bell();
                break;
            }

            if (fis_ptr->floor_num == 1) {
                if ((command_wrk == (USE_FLOOR)) || (!carry_query_flag)) {
                    fis_ptr->k = 0 - fis_ptr->floor_list[0];
                    if (!get_item_allow(owner_ptr, fis_ptr->k)) {
                        fis_ptr->done = TRUE;
                        break;
                    }

                    *cp = fis_ptr->k;
                    fis_ptr->item = TRUE;
                    fis_ptr->done = TRUE;
                    break;
                }
            }

            if (command_see) {
                screen_load();
                screen_save();
            }

            command_wrk = (USE_FLOOR);
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
            if (command_wrk != USE_FLOOR) {
                if (!get_tag(owner_ptr, &fis_ptr->k, fis_ptr->which, command_wrk, fis_ptr->tval)) {
                    bell();
                    break;
                }

                if ((fis_ptr->k < INVEN_RARM) ? !fis_ptr->inven : !fis_ptr->equip) {
                    bell();
                    break;
                }

                if (!get_item_okay(owner_ptr, fis_ptr->k, fis_ptr->tval)) {
                    bell();
                    break;
                }
            } else {
                if (get_tag_floor(owner_ptr->current_floor_ptr, &fis_ptr->k, fis_ptr->which, fis_ptr->floor_list, fis_ptr->floor_num)) {
                    fis_ptr->k = 0 - fis_ptr->floor_list[fis_ptr->k];
                } else {
                    bell();
                    break;
                }
            }

            if (!get_item_allow(owner_ptr, fis_ptr->k)) {
                fis_ptr->done = TRUE;
                break;
            }

            *cp = fis_ptr->k;
            fis_ptr->item = TRUE;
            fis_ptr->done = TRUE;
            fis_ptr->cur_tag = fis_ptr->which;
            break;
        }
        case 'w': {
            if (fis_ptr->force) {
                *cp = INVEN_FORCE;
                fis_ptr->item = TRUE;
                fis_ptr->done = TRUE;
                break;
            }
        }
            /* Fall through */
        default: {
            int ver;
            if (command_wrk != USE_FLOOR) {
                bool not_found = FALSE;
                if (!get_tag(owner_ptr, &fis_ptr->k, fis_ptr->which, command_wrk, fis_ptr->tval))
                    not_found = TRUE;
                else if ((fis_ptr->k < INVEN_RARM) ? !fis_ptr->inven : !fis_ptr->equip)
                    not_found = TRUE;
                else if (!get_item_okay(owner_ptr, fis_ptr->k, fis_ptr->tval))
                    not_found = TRUE;

                if (!not_found) {
                    *cp = fis_ptr->k;
                    fis_ptr->item = TRUE;
                    fis_ptr->done = TRUE;
                    fis_ptr->cur_tag = fis_ptr->which;
                    break;
                }
            } else {
                if (get_tag_floor(owner_ptr->current_floor_ptr, &fis_ptr->k, fis_ptr->which, fis_ptr->floor_list, fis_ptr->floor_num)) {
                    fis_ptr->k = 0 - fis_ptr->floor_list[fis_ptr->k];
                    *cp = fis_ptr->k;
                    fis_ptr->item = TRUE;
                    fis_ptr->done = TRUE;
                    fis_ptr->cur_tag = fis_ptr->which;
                    break;
                }
            }

            ver = isupper(fis_ptr->which);
            fis_ptr->which = (char)tolower(fis_ptr->which);
            if (command_wrk == (USE_INVEN)) {
                if (fis_ptr->which == '(')
                    fis_ptr->k = fis_ptr->i1;
                else if (fis_ptr->which == ')')
                    fis_ptr->k = fis_ptr->i2;
                else
                    fis_ptr->k = label_to_inventory(owner_ptr, fis_ptr->which);
            } else if (command_wrk == (USE_EQUIP)) {
                if (fis_ptr->which == '(')
                    fis_ptr->k = fis_ptr->e1;
                else if (fis_ptr->which == ')')
                    fis_ptr->k = fis_ptr->e2;
                else
                    fis_ptr->k = label_to_equipment(owner_ptr, fis_ptr->which);
            } else if (command_wrk == USE_FLOOR) {
                if (fis_ptr->which == '(')
                    fis_ptr->k = 0;
                else if (fis_ptr->which == ')')
                    fis_ptr->k = fis_ptr->floor_num - 1;
                else
                    fis_ptr->k = islower(fis_ptr->which) ? A2I(fis_ptr->which) : -1;
                if (fis_ptr->k < 0 || fis_ptr->k >= fis_ptr->floor_num || fis_ptr->k >= 23) {
                    bell();
                    break;
                }

                fis_ptr->k = 0 - fis_ptr->floor_list[fis_ptr->k];
            }

            if ((command_wrk != USE_FLOOR) && !get_item_okay(owner_ptr, fis_ptr->k, fis_ptr->tval)) {
                bell();
                break;
            }

            if (ver && !verify(owner_ptr, _("本当に", "Try"), fis_ptr->k)) {
                fis_ptr->done = TRUE;
                break;
            }

            if (!get_item_allow(owner_ptr, fis_ptr->k)) {
                fis_ptr->done = TRUE;
                break;
            }

            *cp = fis_ptr->k;
            fis_ptr->item = TRUE;
            fis_ptr->done = TRUE;
            break;
        }
        }
    }

    if (command_see) {
        screen_load();
        command_see = FALSE;
    }

    fis_ptr->tval = 0;
    item_tester_hook = NULL;
    if (fis_ptr->toggle)
        toggle_inventory_equipment(owner_ptr);

    owner_ptr->window |= (PW_INVEN | PW_EQUIP);
    handle_stuff(owner_ptr);
    prt("", 0, 0);
    if (fis_ptr->oops && str)
        msg_print(str);

    if (fis_ptr->item) {
        repeat_push(*cp);
        if (command_cmd)
            prev_tag = fis_ptr->cur_tag;
        command_cmd = 0;
    }

    return fis_ptr->item;
}
