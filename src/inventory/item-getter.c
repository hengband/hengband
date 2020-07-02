#include "inventory/item-getter.h"
#include "core/stuff-handler.h"
#include "floor/floor.h"
#include "game-option/input-options.h"
#include "game-option/option-flags.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "inventory/floor-item-getter.h"
#include "inventory/inventory-util.h"
#include "inventory/player-inventory.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

/*!
 * @brief オブジェクト選択の汎用関数 /
 * Let the user select an item, save its "index"
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param cp 選択したオブジェクトのIDを返す。
 * @param pmt 選択目的のメッセージ
 * @param str 選択できるオブジェクトがない場合のキャンセルメッセージ
 * @param mode オプションフラグ
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す。/
 * Return TRUE only if an acceptable item was chosen by the user.\n
 */
bool get_item(player_type *owner_ptr, OBJECT_IDX *cp, concptr pmt, concptr str, BIT_FLAGS mode, tval_type tval)
{
    OBJECT_IDX this_o_idx, next_o_idx = 0;
    char which = ' ';
    int j;
    OBJECT_IDX k;
    OBJECT_IDX i1, i2;
    OBJECT_IDX e1, e2;
    bool done, item;
    bool oops = FALSE;
    bool equip = FALSE;
    bool inven = FALSE;
    bool floor = FALSE;
    bool allow_floor = FALSE;
    bool toggle = FALSE;
    char tmp_val[160];
    char out_val[160];
    int menu_line = (use_menu ? 1 : 0);
    int max_inven = 0;
    int max_equip = 0;
    static char prev_tag = '\0';
    char cur_tag = '\0';
    if (easy_floor || use_menu)
        return get_item_floor(owner_ptr, cp, pmt, str, mode, tval);

    if (mode & USE_EQUIP)
        equip = TRUE;

    if (mode & USE_INVEN)
        inven = TRUE;

    if (mode & USE_FLOOR)
        floor = TRUE;

    if (repeat_pull(cp)) {
        if (mode & USE_FORCE && (*cp == INVEN_FORCE)) {
            tval = 0;
            item_tester_hook = NULL;
            command_cmd = 0;
            return TRUE;
        } else if (floor && (*cp < 0)) {
            object_type *o_ptr;
            k = 0 - (*cp);
            o_ptr = &owner_ptr->current_floor_ptr->o_list[k];
            if (item_tester_okay(owner_ptr, o_ptr, tval) || (mode & USE_FULL)) {
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
    else if (use_menu) {
        for (j = INVEN_RARM; j < INVEN_TOTAL; j++)
            if (select_ring_slot ? is_ring_slot(j) : item_tester_okay(owner_ptr, &owner_ptr->inventory_list[j], tval) || (mode & USE_FULL))
                max_equip++;

        if (owner_ptr->ryoute && !(mode & IGNORE_BOTHHAND_SLOT))
            max_equip++;
    }

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

    if (floor) {
        for (this_o_idx = owner_ptr->current_floor_ptr->grid_array[owner_ptr->y][owner_ptr->x].o_idx; this_o_idx; this_o_idx = next_o_idx) {
            object_type *o_ptr;
            o_ptr = &owner_ptr->current_floor_ptr->o_list[this_o_idx];
            next_o_idx = o_ptr->next_o_idx;
            if ((item_tester_okay(owner_ptr, o_ptr, tval) || (mode & USE_FULL)) && (o_ptr->marked & OM_FOUND))
                allow_floor = TRUE;
        }
    }

    if (!allow_floor && (i1 > i2) && (e1 > e2)) {
        command_see = FALSE;
        oops = TRUE;
        done = TRUE;

        if (mode & USE_FORCE) {
            *cp = INVEN_FORCE;
            item = TRUE;
        }
    } else {
        if (command_see && command_wrk && equip)
            command_wrk = TRUE;
        else if (inven)
            command_wrk = FALSE;
        else if (equip)
            command_wrk = TRUE;
        else
            command_wrk = FALSE;
    }

    /* 追加オプション(always_show_list)が設定されている場合は常に一覧を表示する */
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

        if ((command_wrk && ni && !ne) || (!command_wrk && !ni && ne)) {
            toggle_inventory_equipment(owner_ptr);
            toggle = !toggle;
        }

        owner_ptr->window |= (PW_INVEN | PW_EQUIP);
        handle_stuff(owner_ptr);

        if (!command_wrk) {
            if (command_see)
                get_item_label = show_inventory(owner_ptr, menu_line, mode, tval);
        } else {
            if (command_see)
                get_item_label = show_equipment(owner_ptr, menu_line, mode, tval);
        }

        if (!command_wrk) {
            sprintf(out_val, _("持ち物:", "Inven:"));
            if ((i1 <= i2) && !use_menu) {
                sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"), index_to_label(i1), index_to_label(i2));
                strcat(out_val, tmp_val);
            }

            if (!command_see && !use_menu)
                strcat(out_val, _(" '*'一覧,", " * to see,"));

            if (equip)
                strcat(out_val, format(_(" %s 装備品,", " %s for Equip,"), use_menu ? _("'4'or'6'", "4 or 6") : _("'/'", "/")));
        } else {
            sprintf(out_val, _("装備品:", "Equip:"));
            if ((e1 <= e2) && !use_menu) {
                sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"), index_to_label(e1), index_to_label(e2));
                strcat(out_val, tmp_val);
            }

            if (!command_see && !use_menu)
                strcat(out_val, _(" '*'一覧,", " * to see,"));

            if (inven)
                strcat(out_val, format(_(" %s 持ち物,", " %s for Inven,"), use_menu ? _("'4'or'6'", "4 or 6") : _("'/'", "'/'")));
        }

        if (allow_floor)
            strcat(out_val, _(" '-'床上,", " - for floor,"));

        if (mode & USE_FORCE)
            strcat(out_val, _(" 'w'練気術,", " w for the Force,"));

        strcat(out_val, " ESC");
        sprintf(tmp_val, "(%s) %s", out_val, pmt);
        prt(tmp_val, 0, 0);
        which = inkey();
        if (use_menu) {
            int max_line = (command_wrk ? max_equip : max_inven);
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
            case '6':
            case 'h':
            case 'H':
            case 'l':
            case 'L': {
                if (!inven || !equip) {
                    bell();
                    break;
                }

                if (command_see) {
                    screen_load();
                    screen_save();
                }

                command_wrk = !command_wrk;
                max_line = (command_wrk ? max_equip : max_inven);
                if (menu_line > max_line)
                    menu_line = max_line;

                break;
            }

            case 'x':
            case 'X':
            case '\r':
            case '\n': {
                if (command_wrk == USE_FLOOR) {
                    (*cp) = -get_item_label;
                } else {
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
                if (mode & USE_FORCE) {
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
        case '/': {
            if (!inven || !equip) {
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
            if (allow_floor) {
                for (this_o_idx = owner_ptr->current_floor_ptr->grid_array[owner_ptr->y][owner_ptr->x].o_idx; this_o_idx; this_o_idx = next_o_idx) {
                    object_type *o_ptr;
                    o_ptr = &owner_ptr->current_floor_ptr->o_list[this_o_idx];
                    next_o_idx = o_ptr->next_o_idx;
                    if (!item_tester_okay(owner_ptr, o_ptr, tval) && !(mode & USE_FULL))
                        continue;

                    k = 0 - this_o_idx;
                    if (other_query_flag && !verify(owner_ptr, _("本当に", "Try"), k) || !get_item_allow(owner_ptr, k))
                        continue;

                    (*cp) = k;
                    item = TRUE;
                    done = TRUE;
                    break;
                }

                if (done)
                    break;
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
            if (!get_tag(owner_ptr, &k, which, command_wrk ? USE_EQUIP : USE_INVEN, tval)) {
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
            if (mode & USE_FORCE) {
                *cp = INVEN_FORCE;
                item = TRUE;
                done = TRUE;
                break;
            }
        }
            /* Fall through */
        default: {
            int ver;
            bool not_found = FALSE;
            if (!get_tag(owner_ptr, &k, which, command_wrk ? USE_EQUIP : USE_INVEN, tval)) {
                not_found = TRUE;
            } else if ((k < INVEN_RARM) ? !inven : !equip) {
                not_found = TRUE;
            } else if (!get_item_okay(owner_ptr, k, tval)) {
                not_found = TRUE;
            }

            if (!not_found) {
                (*cp) = k;
                item = TRUE;
                done = TRUE;
                cur_tag = which;
                break;
            }

            ver = isupper(which);
            which = (char)tolower(which);
            if (!command_wrk) {
                if (which == '(')
                    k = i1;
                else if (which == ')')
                    k = i2;
                else
                    k = label_to_inventory(owner_ptr, which);
            } else {
                if (which == '(')
                    k = e1;
                else if (which == ')')
                    k = e2;
                else
                    k = label_to_equipment(owner_ptr, which);
            }

            if (!get_item_okay(owner_ptr, k, tval)) {
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
