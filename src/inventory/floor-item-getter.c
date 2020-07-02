/*!
 * @brief オブジェクト選択処理
 * @date 2020/07/02
 * @author Hourier
 */

#include "inventory/floor-item-getter.h"
#include "core/stuff-handler.h"
#include "floor/floor-object.h"
#include "floor/floor.h"
#include "floor/object-scanner.h"
#include "game-option/input-options.h"
#include "game-option/option-flags.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "inventory/inventory-util.h"
#include "inventory/player-inventory.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "view/display-sub-windows.h"

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
    char n1 = ' ', n2 = ' ', which = ' ';
    int j;
    COMMAND_CODE i1, i2;
    COMMAND_CODE e1, e2;
    COMMAND_CODE k;
    bool done;
    bool item;
    bool oops = FALSE;
    bool equip = (mode & USE_EQUIP) ? TRUE : FALSE;
    bool inven = (mode & USE_INVEN) ? TRUE : FALSE;
    bool floor = (mode & USE_FLOOR) ? TRUE : FALSE;
    bool force = (mode & USE_FORCE) ? TRUE : FALSE;
    bool allow_equip = FALSE;
    bool allow_inven = FALSE;
    bool allow_floor = FALSE;
    bool toggle = FALSE;
    char tmp_val[160];
    char out_val[160];
    ITEM_NUMBER floor_num;
    OBJECT_IDX floor_list[23];
    int floor_top = 0;
    TERM_LEN min_width = 0;
    int menu_line = (use_menu ? 1 : 0);
    int max_inven = 0;
    int max_equip = 0;
    static char prev_tag = '\0';
    char cur_tag = '\0';
    if (repeat_pull(cp)) {
        if (force && (*cp == INVEN_FORCE)) {
            tval = 0;
            item_tester_hook = NULL;
            command_cmd = 0;
            return TRUE;
        } else if (floor && (*cp < 0)) {
            if (prev_tag && command_cmd) {
                floor_num = scan_floor_items(owner_ptr, floor_list, owner_ptr->y, owner_ptr->x, 0x03, tval);
                if (get_tag_floor(owner_ptr->current_floor_ptr, &k, prev_tag, floor_list, floor_num)) {
                    (*cp) = 0 - floor_list[k];
                    tval = 0;
                    item_tester_hook = NULL;
                    command_cmd = 0;
                    return TRUE;
                }

                prev_tag = '\0';
            } else if (item_tester_okay(owner_ptr, &owner_ptr->current_floor_ptr->o_list[0 - (*cp)], tval) || (mode & USE_FULL)) {
                tval = 0;
                item_tester_hook = NULL;
                command_cmd = 0;
                return TRUE;
            }
        } else if ((inven && (*cp >= 0) && (*cp < INVEN_PACK)) || (equip && (*cp >= INVEN_RARM) && (*cp < INVEN_TOTAL))) {
            if (prev_tag && command_cmd) {
                if (!get_tag(owner_ptr, &k, prev_tag, (*cp >= INVEN_RARM) ? USE_EQUIP : USE_INVEN, tval)) /* Reject */
                    ;
                else if ((k < INVEN_RARM) ? !inven : !equip) /* Reject */
                    ;
                else if (!get_item_okay(owner_ptr, k, tval)) /* Reject */
                    ;
                else {
                    (*cp) = k;
                    tval = 0;
                    item_tester_hook = NULL;
                    command_cmd = 0;
                    return TRUE;
                }

                prev_tag = '\0';
            } else if (get_item_okay(owner_ptr, *cp, tval)) {
                tval = 0;
                item_tester_hook = NULL;
                command_cmd = 0;
                return TRUE;
            }
        }
    }

    msg_print(NULL);
    done = FALSE;
    item = FALSE;
    i1 = 0;
    i2 = INVEN_PACK - 1;
    if (!inven)
        i2 = -1;
    else if (use_menu)
        for (j = 0; j < INVEN_PACK; j++)
            if (item_tester_okay(owner_ptr, &owner_ptr->inventory_list[j], tval) || (mode & USE_FULL))
                max_inven++;

    while ((i1 <= i2) && (!get_item_okay(owner_ptr, i1, tval)))
        i1++;

    while ((i1 <= i2) && (!get_item_okay(owner_ptr, i2, tval)))
        i2--;

    e1 = INVEN_RARM;
    e2 = INVEN_TOTAL - 1;
    if (!equip)
        e2 = -1;
    else if (use_menu)
        for (j = INVEN_RARM; j < INVEN_TOTAL; j++)
            if (select_ring_slot ? is_ring_slot(j) : item_tester_okay(owner_ptr, &owner_ptr->inventory_list[j], tval) || (mode & USE_FULL))
                max_equip++;
    if (owner_ptr->ryoute && !(mode & IGNORE_BOTHHAND_SLOT))
        max_equip++;

    while ((e1 <= e2) && (!get_item_okay(owner_ptr, e1, tval)))
        e1++;

    while ((e1 <= e2) && (!get_item_okay(owner_ptr, e2, tval)))
        e2--;

    if (equip && owner_ptr->ryoute && !(mode & IGNORE_BOTHHAND_SLOT)) {
        if (owner_ptr->migite) {
            if (e2 < INVEN_LARM)
                e2 = INVEN_LARM;
        } else if (owner_ptr->hidarite)
            e1 = INVEN_RARM;
    }

    floor_num = 0;
    if (floor)
        floor_num = scan_floor_items(owner_ptr, floor_list, owner_ptr->y, owner_ptr->x, 0x03, tval);

    if (i1 <= i2)
        allow_inven = TRUE;

    if (e1 <= e2)
        allow_equip = TRUE;

    if (floor_num)
        allow_floor = TRUE;

    if (!allow_inven && !allow_equip && !allow_floor) {
        command_see = FALSE;
        oops = TRUE;
        done = TRUE;

        if (force) {
            *cp = INVEN_FORCE;
            item = TRUE;
        }
    } else {
        if (command_see && (command_wrk == (USE_EQUIP)) && allow_equip)
            command_wrk = (USE_EQUIP);
        else if (allow_inven)
            command_wrk = (USE_INVEN);
        else if (allow_equip)
            command_wrk = (USE_EQUIP);
        else if (allow_floor)
            command_wrk = (USE_FLOOR);
    }

    /*
     * 追加オプション(always_show_list)が設定されている場合は常に一覧を表示する
     */
    if ((always_show_list == TRUE) || use_menu)
        command_see = TRUE;

    if (command_see)
        screen_save();

    while (!done) {
        COMMAND_CODE get_item_label = 0;
        int ni = 0;
        int ne = 0;
        for (j = 0; j < 8; j++) {
            if (!angband_term[j])
                continue;

            if (window_flag[j] & (PW_INVEN))
                ni++;

            if (window_flag[j] & (PW_EQUIP))
                ne++;
        }

        if ((command_wrk == (USE_EQUIP) && ni && !ne) || (command_wrk == (USE_INVEN) && !ni && ne)) {
            toggle_inventory_equipment(owner_ptr);
            toggle = !toggle;
        }

        owner_ptr->window |= (PW_INVEN | PW_EQUIP);
        handle_stuff(owner_ptr);
        if (command_wrk == (USE_INVEN)) {
            n1 = I2A(i1);
            n2 = I2A(i2);
            if (command_see)
                get_item_label = show_inventory(owner_ptr, menu_line, mode, tval);
        } else if (command_wrk == (USE_EQUIP)) {
            n1 = I2A(e1 - INVEN_RARM);
            n2 = I2A(e2 - INVEN_RARM);
            if (command_see)
                get_item_label = show_equipment(owner_ptr, menu_line, mode, tval);
        } else if (command_wrk == (USE_FLOOR)) {
            j = floor_top;
            k = MIN(floor_top + 23, floor_num) - 1;
            n1 = I2A(j - floor_top);
            n2 = I2A(k - floor_top);
            if (command_see)
                get_item_label = show_floor_items(owner_ptr, menu_line, owner_ptr->y, owner_ptr->x, &min_width, tval);
        }

        if (command_wrk == (USE_INVEN)) {
            sprintf(out_val, _("持ち物:", "Inven:"));
            if (!use_menu) {
                sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"), index_to_label(i1), index_to_label(i2));
                strcat(out_val, tmp_val);
            }

            if (!command_see && !use_menu)
                strcat(out_val, _(" '*'一覧,", " * to see,"));

            if (allow_equip) {
                if (!use_menu)
                    strcat(out_val, _(" '/' 装備品,", " / for Equip,"));
                else if (allow_floor)
                    strcat(out_val, _(" '6' 装備品,", " 6 for Equip,"));
                else
                    strcat(out_val, _(" '4'or'6' 装備品,", " 4 or 6 for Equip,"));
            }

            if (allow_floor) {
                if (!use_menu)
                    strcat(out_val, _(" '-'床上,", " - for floor,"));
                else if (allow_equip)
                    strcat(out_val, _(" '4' 床上,", " 4 for floor,"));
                else
                    strcat(out_val, _(" '4'or'6' 床上,", " 4 or 6 for floor,"));
            }
        } else if (command_wrk == (USE_EQUIP)) {
            sprintf(out_val, _("装備品:", "Equip:"));
            if (!use_menu) {
                sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"), index_to_label(e1), index_to_label(e2));
                strcat(out_val, tmp_val);
            }

            if (!command_see && !use_menu)
                strcat(out_val, _(" '*'一覧,", " * to see,"));

            if (allow_inven) {
                if (!use_menu)
                    strcat(out_val, _(" '/' 持ち物,", " / for Inven,"));
                else if (allow_floor)
                    strcat(out_val, _(" '4' 持ち物,", " 4 for Inven,"));
                else
                    strcat(out_val, _(" '4'or'6' 持ち物,", " 4 or 6 for Inven,"));
            }

            if (allow_floor) {
                if (!use_menu)
                    strcat(out_val, _(" '-'床上,", " - for floor,"));
                else if (allow_inven)
                    strcat(out_val, _(" '6' 床上,", " 6 for floor,"));
                else
                    strcat(out_val, _(" '4'or'6' 床上,", " 4 or 6 for floor,"));
            }
        } else if (command_wrk == (USE_FLOOR)) {
            sprintf(out_val, _("床上:", "Floor:"));
            if (!use_menu) {
                sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"), n1, n2);
                strcat(out_val, tmp_val);
            }

            if (!command_see && !use_menu)
                strcat(out_val, _(" '*'一覧,", " * to see,"));

            if (use_menu) {
                if (allow_inven && allow_equip) {
                    strcat(out_val, _(" '4' 装備品, '6' 持ち物,", " 4 for Equip, 6 for Inven,"));
                } else if (allow_inven) {
                    strcat(out_val, _(" '4'or'6' 持ち物,", " 4 or 6 for Inven,"));
                } else if (allow_equip) {
                    strcat(out_val, _(" '4'or'6' 装備品,", " 4 or 6 for Equip,"));
                }
            } else if (allow_inven) {
                strcat(out_val, _(" '/' 持ち物,", " / for Inven,"));
            } else if (allow_equip) {
                strcat(out_val, _(" '/'装備品,", " / for Equip,"));
            }

            if (command_see && !use_menu) {
                strcat(out_val, _(" Enter 次,", " Enter for scroll down,"));
            }
        }

        if (force)
            strcat(out_val, _(" 'w'練気術,", " w for the Force,"));

        strcat(out_val, " ESC");
        sprintf(tmp_val, "(%s) %s", out_val, pmt);
        prt(tmp_val, 0, 0);
        which = inkey();
        if (use_menu) {
            int max_line = 1;
            if (command_wrk == USE_INVEN)
                max_line = max_inven;
            else if (command_wrk == USE_EQUIP)
                max_line = max_equip;
            else if (command_wrk == USE_FLOOR)
                max_line = MIN(23, floor_num);
            switch (which) {
            case ESCAPE:
            case 'z':
            case 'Z':
            case '0': {
                done = TRUE;
                break;
            }
            case '8':
            case 'k':
            case 'K': {
                menu_line += (max_line - 1);
                break;
            }
            case '2':
            case 'j':
            case 'J': {
                menu_line++;
                break;
            }
            case '4':
            case 'h':
            case 'H': {
                if (command_wrk == (USE_INVEN)) {
                    if (allow_floor)
                        command_wrk = USE_FLOOR;
                    else if (allow_equip)
                        command_wrk = USE_EQUIP;
                    else {
                        bell();
                        break;
                    }
                } else if (command_wrk == (USE_EQUIP)) {
                    if (allow_inven)
                        command_wrk = USE_INVEN;
                    else if (allow_floor)
                        command_wrk = USE_FLOOR;
                    else {
                        bell();
                        break;
                    }
                } else if (command_wrk == (USE_FLOOR)) {
                    if (allow_equip)
                        command_wrk = USE_EQUIP;
                    else if (allow_inven)
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
                    max_line = max_inven;
                else if (command_wrk == USE_EQUIP)
                    max_line = max_equip;
                else if (command_wrk == USE_FLOOR)
                    max_line = MIN(23, floor_num);

                if (menu_line > max_line)
                    menu_line = max_line;

                break;
            }
            case '6':
            case 'l':
            case 'L': {
                if (command_wrk == (USE_INVEN)) {
                    if (allow_equip)
                        command_wrk = USE_EQUIP;
                    else if (allow_floor)
                        command_wrk = USE_FLOOR;
                    else {
                        bell();
                        break;
                    }
                } else if (command_wrk == (USE_EQUIP)) {
                    if (allow_floor)
                        command_wrk = USE_FLOOR;
                    else if (allow_inven)
                        command_wrk = USE_INVEN;
                    else {
                        bell();
                        break;
                    }
                } else if (command_wrk == (USE_FLOOR)) {
                    if (allow_inven)
                        command_wrk = USE_INVEN;
                    else if (allow_equip)
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
                    max_line = max_inven;
                else if (command_wrk == USE_EQUIP)
                    max_line = max_equip;
                else if (command_wrk == USE_FLOOR)
                    max_line = MIN(23, floor_num);

                if (menu_line > max_line)
                    menu_line = max_line;

                break;
            }
            case 'x':
            case 'X':
            case '\r':
            case '\n': {
                if (command_wrk == USE_FLOOR)
                    (*cp) = -get_item_label;
                else {
                    if (!get_item_okay(owner_ptr, get_item_label, tval)) {
                        bell();
                        break;
                    }

                    if (!get_item_allow(owner_ptr, get_item_label)) {
                        done = TRUE;
                        break;
                    }

                    (*cp) = get_item_label;
                }

                item = TRUE;
                done = TRUE;
                break;
            }
            case 'w': {
                if (force) {
                    *cp = INVEN_FORCE;
                    item = TRUE;
                    done = TRUE;
                    break;
                }
            }
            }

            if (menu_line > max_line)
                menu_line -= max_line;

            continue;
        }

        switch (which) {
        case ESCAPE: {
            done = TRUE;
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
            floor_num = scan_floor_items(owner_ptr, floor_list, owner_ptr->y, owner_ptr->x, 0x03, tval);
            if (command_see) {
                screen_load();
                screen_save();
            }

            break;
        }
        case '/': {
            if (command_wrk == (USE_INVEN)) {
                if (!allow_equip) {
                    bell();
                    break;
                }
                command_wrk = (USE_EQUIP);
            } else if (command_wrk == (USE_EQUIP)) {
                if (!allow_inven) {
                    bell();
                    break;
                }
                command_wrk = (USE_INVEN);
            } else if (command_wrk == (USE_FLOOR)) {
                if (allow_inven) {
                    command_wrk = (USE_INVEN);
                } else if (allow_equip) {
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
            if (!allow_floor) {
                bell();
                break;
            }

            if (floor_num == 1) {
                if ((command_wrk == (USE_FLOOR)) || (!carry_query_flag)) {
                    k = 0 - floor_list[0];
                    if (!get_item_allow(owner_ptr, k)) {
                        done = TRUE;
                        break;
                    }

                    (*cp) = k;
                    item = TRUE;
                    done = TRUE;
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
                if (!get_tag(owner_ptr, &k, which, command_wrk, tval)) {
                    bell();
                    break;
                }

                if ((k < INVEN_RARM) ? !inven : !equip) {
                    bell();
                    break;
                }

                if (!get_item_okay(owner_ptr, k, tval)) {
                    bell();
                    break;
                }
            } else {
                if (get_tag_floor(owner_ptr->current_floor_ptr, &k, which, floor_list, floor_num)) {
                    k = 0 - floor_list[k];
                } else {
                    bell();
                    break;
                }
            }

            if (!get_item_allow(owner_ptr, k)) {
                done = TRUE;
                break;
            }

            (*cp) = k;
            item = TRUE;
            done = TRUE;
            cur_tag = which;
            break;
        }
        case 'w': {
            if (force) {
                *cp = INVEN_FORCE;
                item = TRUE;
                done = TRUE;
                break;
            }
        }
            /* Fall through */
        default: {
            int ver;
            if (command_wrk != USE_FLOOR) {
                bool not_found = FALSE;
                if (!get_tag(owner_ptr, &k, which, command_wrk, tval))
                    not_found = TRUE;
                else if ((k < INVEN_RARM) ? !inven : !equip)
                    not_found = TRUE;
                else if (!get_item_okay(owner_ptr, k, tval))
                    not_found = TRUE;

                if (!not_found) {
                    (*cp) = k;
                    item = TRUE;
                    done = TRUE;
                    cur_tag = which;
                    break;
                }
            } else {
                if (get_tag_floor(owner_ptr->current_floor_ptr, &k, which, floor_list, floor_num)) {
                    k = 0 - floor_list[k];
                    (*cp) = k;
                    item = TRUE;
                    done = TRUE;
                    cur_tag = which;
                    break;
                }
            }

            ver = isupper(which);
            which = (char)tolower(which);
            if (command_wrk == (USE_INVEN)) {
                if (which == '(')
                    k = i1;
                else if (which == ')')
                    k = i2;
                else
                    k = label_to_inventory(owner_ptr, which);
            } else if (command_wrk == (USE_EQUIP)) {
                if (which == '(')
                    k = e1;
                else if (which == ')')
                    k = e2;
                else
                    k = label_to_equipment(owner_ptr, which);
            } else if (command_wrk == USE_FLOOR) {
                if (which == '(')
                    k = 0;
                else if (which == ')')
                    k = floor_num - 1;
                else
                    k = islower(which) ? A2I(which) : -1;
                if (k < 0 || k >= floor_num || k >= 23) {
                    bell();
                    break;
                }

                k = 0 - floor_list[k];
            }

            if ((command_wrk != USE_FLOOR) && !get_item_okay(owner_ptr, k, tval)) {
                bell();
                break;
            }

            if (ver && !verify(owner_ptr, _("本当に", "Try"), k)) {
                done = TRUE;
                break;
            }

            if (!get_item_allow(owner_ptr, k)) {
                done = TRUE;
                break;
            }

            (*cp) = k;
            item = TRUE;
            done = TRUE;
            break;
        }
        }
    }

    if (command_see) {
        screen_load();
        command_see = FALSE;
    }

    tval = 0;
    item_tester_hook = NULL;
    if (toggle)
        toggle_inventory_equipment(owner_ptr);

    owner_ptr->window |= (PW_INVEN | PW_EQUIP);
    handle_stuff(owner_ptr);
    prt("", 0, 0);
    if (oops && str)
        msg_print(str);

    if (item) {
        repeat_push(*cp);
        if (command_cmd)
            prev_tag = cur_tag;
        command_cmd = 0;
    }

    return item;
}
