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
#include "system/grid-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-inventory.h"
#include "view/display-messages.h"
#include "window/display-sub-windows.h"

/*!
 * @brief 床上アイテムへにタグ付けがされているかの調査処理 (のはず)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fis_ptr 床上アイテムへの参照ポインタ
 * @param prev_tag 前回選択したアイテムのタグ (のはず)
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す
 * @todo 適切な関数名をどうしても付けられなかったので暫定でauxとした
 */
static bool check_floor_item_tag_aux(PlayerType *player_ptr, fis_type *fis_ptr, char *prev_tag, const ItemTester &item_tester)
{
    if (!fis_ptr->floor || (*fis_ptr->cp >= 0)) {
        return false;
    }

    if (*prev_tag && command_cmd) {
        fis_ptr->floor_num = scan_floor_items(player_ptr, fis_ptr->floor_list, player_ptr->y, player_ptr->x, SCAN_FLOOR_ITEM_TESTER | SCAN_FLOOR_ONLY_MARKED, item_tester);
        if (get_tag_floor(player_ptr->current_floor_ptr, &fis_ptr->k, *prev_tag, fis_ptr->floor_list, fis_ptr->floor_num)) {
            *fis_ptr->cp = 0 - fis_ptr->floor_list[fis_ptr->k];
            command_cmd = 0;
            return true;
        }

        *prev_tag = '\0';
        return false;
    }

    if (!item_tester.okay(&player_ptr->current_floor_ptr->o_list[0 - (*fis_ptr->cp)]) && ((fis_ptr->mode & USE_FULL) == 0)) {
        return false;
    }

    command_cmd = 0;
    return true;
}

/*!
 * @brief インベントリのアイテムにタグ付けを試みる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fis_ptr 床上アイテムへの参照ポインタ
 * @param prev_tag 前回選択したアイテムのタグ (のはず)
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す
 */
static bool get_floor_item_tag_inventory(PlayerType *player_ptr, fis_type *fis_ptr, char *prev_tag, const ItemTester &item_tester)
{
    bool flag = false;
    item_use_flag use_flag = (*fis_ptr->cp >= INVEN_MAIN_HAND) ? USE_EQUIP : USE_INVEN;

    flag |= !get_tag(player_ptr, &fis_ptr->k, *prev_tag, use_flag, item_tester);
    flag |= !get_item_okay(player_ptr, fis_ptr->k, item_tester);

    if (fis_ptr->k < INVEN_MAIN_HAND) {
        flag |= !fis_ptr->inven;
    } else {
        flag |= !fis_ptr->equip;
    }

    if (flag) {
        *prev_tag = '\0';
        return false;
    }

    *fis_ptr->cp = fis_ptr->k;
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
static bool check_floor_item_tag_inventory(PlayerType *player_ptr, fis_type *fis_ptr, char *prev_tag, const ItemTester &item_tester)
{
    if ((!fis_ptr->inven || (*fis_ptr->cp < 0) || (*fis_ptr->cp >= INVEN_PACK)) && (!fis_ptr->equip || (*fis_ptr->cp < INVEN_MAIN_HAND) || (*fis_ptr->cp >= INVEN_TOTAL))) {
        return false;
    }

    if ((*prev_tag != '\0') && command_cmd) {
        return get_floor_item_tag_inventory(player_ptr, fis_ptr, prev_tag, item_tester);
    }

    if (get_item_okay(player_ptr, *fis_ptr->cp, item_tester)) {
        command_cmd = 0;
        return true;
    }

    return false;
}

/*!
 * @brief 床上アイテムにタグ付けがされているかの調査処理 (のはず)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fis_ptr 床上アイテムへの参照ポインタ
 * @param prev_tag 前回選択したアイテムのタグ (のはず)
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す
 */
static bool check_floor_item_tag(PlayerType *player_ptr, fis_type *fis_ptr, char *prev_tag, const ItemTester &item_tester)
{
    if (!repeat_pull(fis_ptr->cp)) {
        return false;
    }

    if (fis_ptr->force && (*fis_ptr->cp == INVEN_FORCE)) {
        command_cmd = 0;
        return true;
    }

    if (check_floor_item_tag_aux(player_ptr, fis_ptr, prev_tag, item_tester)) {
        return true;
    }

    return check_floor_item_tag_inventory(player_ptr, fis_ptr, prev_tag, item_tester);
}

/*!
 * @brief インベントリ内のアイテムが妥当かを判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fis_ptr 床上アイテムへの参照ポインタ
 */
static void test_inventory_floor(PlayerType *player_ptr, fis_type *fis_ptr, const ItemTester &item_tester)
{
    if (!fis_ptr->inven) {
        fis_ptr->i2 = -1;
        return;
    }

    if (!use_menu) {
        return;
    }

    for (int i = 0; i < INVEN_PACK; i++) {
        if (item_tester.okay(&player_ptr->inventory_list[i]) || (fis_ptr->mode & USE_FULL)) {
            fis_ptr->max_inven++;
        }
    }
}

/*!
 * @brief 装備品がが妥当かを判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fis_ptr 床上アイテムへの参照ポインタ
 */
static void test_equipment_floor(PlayerType *player_ptr, fis_type *fis_ptr, const ItemTester &item_tester)
{
    if (!fis_ptr->equip) {
        fis_ptr->e2 = -1;
        return;
    }

    if (!use_menu) {
        return;
    }

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        if (player_ptr->select_ring_slot ? is_ring_slot(i)
                                         : item_tester.okay(&player_ptr->inventory_list[i]) || (fis_ptr->mode & USE_FULL)) {
            fis_ptr->max_equip++;
        }
    }
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
bool get_item_floor(PlayerType *player_ptr, COMMAND_CODE *cp, concptr pmt, concptr str, BIT_FLAGS mode, const ItemTester &item_tester)
{
    fis_type tmp_fis;
    fis_type *fis_ptr = initialize_fis_type(&tmp_fis, cp, mode);
    static char prev_tag = '\0';
    if (check_floor_item_tag(player_ptr, fis_ptr, &prev_tag, item_tester)) {
        return true;
    }

    msg_print(nullptr);
    handle_stuff(player_ptr);
    test_inventory_floor(player_ptr, fis_ptr, item_tester);
    fis_ptr->done = false;
    fis_ptr->item = false;
    fis_ptr->i1 = 0;
    fis_ptr->i2 = INVEN_PACK - 1;
    while ((fis_ptr->i1 <= fis_ptr->i2) && (!get_item_okay(player_ptr, fis_ptr->i1, item_tester))) {
        fis_ptr->i1++;
    }

    while ((fis_ptr->i1 <= fis_ptr->i2) && (!get_item_okay(player_ptr, fis_ptr->i2, item_tester))) {
        fis_ptr->i2--;
    }

    fis_ptr->e1 = INVEN_MAIN_HAND;
    fis_ptr->e2 = INVEN_TOTAL - 1;
    test_equipment_floor(player_ptr, fis_ptr, item_tester);
    if (has_two_handed_weapons(player_ptr) && !(fis_ptr->mode & IGNORE_BOTHHAND_SLOT)) {
        fis_ptr->max_equip++;
    }

    while ((fis_ptr->e1 <= fis_ptr->e2) && (!get_item_okay(player_ptr, fis_ptr->e1, item_tester))) {
        fis_ptr->e1++;
    }

    while ((fis_ptr->e1 <= fis_ptr->e2) && (!get_item_okay(player_ptr, fis_ptr->e2, item_tester))) {
        fis_ptr->e2--;
    }

    if (fis_ptr->equip && has_two_handed_weapons(player_ptr) && !(fis_ptr->mode & IGNORE_BOTHHAND_SLOT)) {
        if (can_attack_with_main_hand(player_ptr)) {
            if (fis_ptr->e2 < INVEN_SUB_HAND) {
                fis_ptr->e2 = INVEN_SUB_HAND;
            }
        } else if (can_attack_with_sub_hand(player_ptr)) {
            fis_ptr->e1 = INVEN_MAIN_HAND;
        }
    }

    fis_ptr->floor_num = 0;
    if (fis_ptr->floor) {
        fis_ptr->floor_num = scan_floor_items(player_ptr, fis_ptr->floor_list, player_ptr->y, player_ptr->x, SCAN_FLOOR_ITEM_TESTER | SCAN_FLOOR_ONLY_MARKED, item_tester);
    }

    if ((mode & USE_INVEN) && (fis_ptr->i1 <= fis_ptr->i2)) {
        fis_ptr->allow_inven = true;
    }

    if ((mode & USE_EQUIP) && (fis_ptr->e1 <= fis_ptr->e2)) {
        fis_ptr->allow_equip = true;
    }

    if ((mode & USE_FLOOR) && (fis_ptr->floor_num)) {
        fis_ptr->allow_floor = true;
    }

    if (!fis_ptr->allow_inven && !fis_ptr->allow_equip && !fis_ptr->allow_floor) {
        command_see = false;
        fis_ptr->oops = true;
        fis_ptr->done = true;

        if (fis_ptr->force) {
            *cp = INVEN_FORCE;
            fis_ptr->item = true;
        }
    } else {
        if (command_see && (command_wrk == USE_EQUIP) && fis_ptr->allow_equip) {
            command_wrk = USE_EQUIP;
        } else if (fis_ptr->allow_inven) {
            command_wrk = USE_INVEN;
        } else if (fis_ptr->allow_equip) {
            command_wrk = USE_EQUIP;
        } else if (fis_ptr->allow_floor) {
            command_wrk = USE_FLOOR;
        }
    }

    /* 追加オプション(always_show_list)が設定されている場合は常に一覧を表示する */
    if (always_show_list || use_menu) {
        command_see = true;
    }

    if (command_see) {
        screen_save();
    }

    while (!fis_ptr->done) {
        int ni = 0;
        int ne = 0;
        for (int i = 0; i < 8; i++) {
            if (!angband_term[i]) {
                continue;
            }

            if (window_flag[i] & PW_INVEN) {
                ni++;
            }

            if (window_flag[i] & PW_EQUIP) {
                ne++;
            }
        }

        if ((command_wrk == (USE_EQUIP) && ni && !ne) || (command_wrk == (USE_INVEN) && !ni && ne)) {
            toggle_inventory_equipment(player_ptr);
            fis_ptr->toggle = !fis_ptr->toggle;
        }

        player_ptr->window_flags |= (PW_INVEN | PW_EQUIP);
        handle_stuff(player_ptr);
        COMMAND_CODE get_item_label = 0;
        if (command_wrk == USE_INVEN) {
            fis_ptr->n1 = I2A(fis_ptr->i1);
            fis_ptr->n2 = I2A(fis_ptr->i2);
            if (command_see) {
                get_item_label = show_inventory(player_ptr, fis_ptr->menu_line, fis_ptr->mode, item_tester);
            }
        } else if (command_wrk == USE_EQUIP) {
            fis_ptr->n1 = I2A(fis_ptr->e1 - INVEN_MAIN_HAND);
            fis_ptr->n2 = I2A(fis_ptr->e2 - INVEN_MAIN_HAND);
            if (command_see) {
                get_item_label = show_equipment(player_ptr, fis_ptr->menu_line, mode, item_tester);
            }
        } else if (command_wrk == USE_FLOOR) {
            int j = fis_ptr->floor_top;
            fis_ptr->k = std::min(fis_ptr->floor_top + 23, fis_ptr->floor_num) - 1;
            fis_ptr->n1 = I2A(j - fis_ptr->floor_top); // TODO: 常に'0'になる。どんな意図でこのようなコードになっているのか不明.
            fis_ptr->n2 = I2A(fis_ptr->k - fis_ptr->floor_top);
            if (command_see) {
                get_item_label = show_floor_items(player_ptr, fis_ptr->menu_line, player_ptr->y, player_ptr->x, &fis_ptr->min_width, item_tester);
            }
        }

        if (command_wrk == USE_INVEN) {
            sprintf(fis_ptr->out_val, _("持ち物:", "Inven:"));
            if (!use_menu) {
                char tmp_val[80];
                sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"), index_to_label(fis_ptr->i1), index_to_label(fis_ptr->i2));
                strcat(fis_ptr->out_val, tmp_val);
            }

            if (!command_see && !use_menu) {
                strcat(fis_ptr->out_val, _(" '*'一覧,", " * to see,"));
            }

            if (fis_ptr->allow_equip) {
                if (!use_menu) {
                    strcat(fis_ptr->out_val, _(" '/' 装備品,", " / for Equip,"));
                } else if (fis_ptr->allow_floor) {
                    strcat(fis_ptr->out_val, _(" '6' 装備品,", " 6 for Equip,"));
                } else {
                    strcat(fis_ptr->out_val, _(" '4'or'6' 装備品,", " 4 or 6 for Equip,"));
                }
            }

            if (fis_ptr->allow_floor) {
                if (!use_menu) {
                    strcat(fis_ptr->out_val, _(" '-'床上,", " - for floor,"));
                } else if (fis_ptr->allow_equip) {
                    strcat(fis_ptr->out_val, _(" '4' 床上,", " 4 for floor,"));
                } else {
                    strcat(fis_ptr->out_val, _(" '4'or'6' 床上,", " 4 or 6 for floor,"));
                }
            }
        } else if (command_wrk == (USE_EQUIP)) {
            sprintf(fis_ptr->out_val, _("装備品:", "Equip:"));
            if (!use_menu) {
                char tmp_val[80];
                sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"), index_to_label(fis_ptr->e1), index_to_label(fis_ptr->e2));
                strcat(fis_ptr->out_val, tmp_val);
            }

            if (!command_see && !use_menu) {
                strcat(fis_ptr->out_val, _(" '*'一覧,", " * to see,"));
            }

            if (fis_ptr->allow_inven) {
                if (!use_menu) {
                    strcat(fis_ptr->out_val, _(" '/' 持ち物,", " / for Inven,"));
                } else if (fis_ptr->allow_floor) {
                    strcat(fis_ptr->out_val, _(" '4' 持ち物,", " 4 for Inven,"));
                } else {
                    strcat(fis_ptr->out_val, _(" '4'or'6' 持ち物,", " 4 or 6 for Inven,"));
                }
            }

            if (fis_ptr->allow_floor) {
                if (!use_menu) {
                    strcat(fis_ptr->out_val, _(" '-'床上,", " - for floor,"));
                } else if (fis_ptr->allow_inven) {
                    strcat(fis_ptr->out_val, _(" '6' 床上,", " 6 for floor,"));
                } else {
                    strcat(fis_ptr->out_val, _(" '4'or'6' 床上,", " 4 or 6 for floor,"));
                }
            }
        } else if (command_wrk == USE_FLOOR) {
            sprintf(fis_ptr->out_val, _("床上:", "Floor:"));
            if (!use_menu) {
                char tmp_val[80];
                sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"), fis_ptr->n1, fis_ptr->n2);
                strcat(fis_ptr->out_val, tmp_val);
            }

            if (!command_see && !use_menu) {
                strcat(fis_ptr->out_val, _(" '*'一覧,", " * to see,"));
            }

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

        if (fis_ptr->force) {
            strcat(fis_ptr->out_val, _(" 'w'練気術,", " w for the Force,"));
        }

        strcat(fis_ptr->out_val, " ESC");
        sprintf(fis_ptr->tmp_val, "(%s) %s", fis_ptr->out_val, pmt);
        prt(fis_ptr->tmp_val, 0, 0);
        fis_ptr->which = inkey();
        if (use_menu) {
            int max_line = 1;
            if (command_wrk == USE_INVEN) {
                max_line = fis_ptr->max_inven;
            } else if (command_wrk == USE_EQUIP) {
                max_line = fis_ptr->max_equip;
            } else if (command_wrk == USE_FLOOR) {
                max_line = std::min(23, fis_ptr->floor_num);
            }
            switch (fis_ptr->which) {
            case ESCAPE:
            case 'z':
            case 'Z':
            case '0': {
                fis_ptr->done = true;
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
                    if (fis_ptr->allow_floor) {
                        command_wrk = USE_FLOOR;
                    } else if (fis_ptr->allow_equip) {
                        command_wrk = USE_EQUIP;
                    } else {
                        bell();
                        break;
                    }
                } else if (command_wrk == (USE_EQUIP)) {
                    if (fis_ptr->allow_inven) {
                        command_wrk = USE_INVEN;
                    } else if (fis_ptr->allow_floor) {
                        command_wrk = USE_FLOOR;
                    } else {
                        bell();
                        break;
                    }
                } else if (command_wrk == (USE_FLOOR)) {
                    if (fis_ptr->allow_equip) {
                        command_wrk = USE_EQUIP;
                    } else if (fis_ptr->allow_inven) {
                        command_wrk = USE_INVEN;
                    } else {
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

                if (command_wrk == USE_INVEN) {
                    max_line = fis_ptr->max_inven;
                } else if (command_wrk == USE_EQUIP) {
                    max_line = fis_ptr->max_equip;
                } else if (command_wrk == USE_FLOOR) {
                    max_line = std::min(23, fis_ptr->floor_num);
                }

                if (fis_ptr->menu_line > max_line) {
                    fis_ptr->menu_line = max_line;
                }

                break;
            }
            case '6':
            case 'l':
            case 'L': {
                if (command_wrk == (USE_INVEN)) {
                    if (fis_ptr->allow_equip) {
                        command_wrk = USE_EQUIP;
                    } else if (fis_ptr->allow_floor) {
                        command_wrk = USE_FLOOR;
                    } else {
                        bell();
                        break;
                    }
                } else if (command_wrk == (USE_EQUIP)) {
                    if (fis_ptr->allow_floor) {
                        command_wrk = USE_FLOOR;
                    } else if (fis_ptr->allow_inven) {
                        command_wrk = USE_INVEN;
                    } else {
                        bell();
                        break;
                    }
                } else if (command_wrk == (USE_FLOOR)) {
                    if (fis_ptr->allow_inven) {
                        command_wrk = USE_INVEN;
                    } else if (fis_ptr->allow_equip) {
                        command_wrk = USE_EQUIP;
                    } else {
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

                if (command_wrk == USE_INVEN) {
                    max_line = fis_ptr->max_inven;
                } else if (command_wrk == USE_EQUIP) {
                    max_line = fis_ptr->max_equip;
                } else if (command_wrk == USE_FLOOR) {
                    max_line = std::min(23, fis_ptr->floor_num);
                }

                if (fis_ptr->menu_line > max_line) {
                    fis_ptr->menu_line = max_line;
                }

                break;
            }
            case 'x':
            case 'X':
            case '\r':
            case '\n': {
                if (command_wrk == USE_FLOOR) {
                    *cp = -get_item_label;
                } else {
                    if (!get_item_okay(player_ptr, get_item_label, item_tester)) {
                        bell();
                        break;
                    }

                    if (!get_item_allow(player_ptr, get_item_label)) {
                        fis_ptr->done = true;
                        break;
                    }

                    *cp = get_item_label;
                }

                fis_ptr->item = true;
                fis_ptr->done = true;
                break;
            }
            case 'w': {
                if (fis_ptr->force) {
                    *cp = INVEN_FORCE;
                    fis_ptr->item = true;
                    fis_ptr->done = true;
                    break;
                }
            }
            }

            if (fis_ptr->menu_line > max_line) {
                fis_ptr->menu_line -= max_line;
            }

            continue;
        }

        switch (fis_ptr->which) {
        case ESCAPE: {
            fis_ptr->done = true;
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
        case '\n':
        case '\r':
        case '+': {
            auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x];
            if (command_wrk != (USE_FLOOR)) {
                break;
            }

            if (g_ptr->o_idx_list.size() < 2) {
                break;
            }

            const auto next_o_idx = fis_ptr->floor_list[1];
            while (g_ptr->o_idx_list.front() != next_o_idx) {
                g_ptr->o_idx_list.rotate(player_ptr->current_floor_ptr);
            }

            player_ptr->window_flags |= PW_FLOOR_ITEM_LIST;
            window_stuff(player_ptr);

            fis_ptr->floor_num = scan_floor_items(player_ptr, fis_ptr->floor_list, player_ptr->y, player_ptr->x, SCAN_FLOOR_ITEM_TESTER | SCAN_FLOOR_ONLY_MARKED, item_tester);
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
                    if (!get_item_allow(player_ptr, fis_ptr->k)) {
                        fis_ptr->done = true;
                        break;
                    }

                    *cp = fis_ptr->k;
                    fis_ptr->item = true;
                    fis_ptr->done = true;
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
                if (!get_tag(player_ptr, &fis_ptr->k, fis_ptr->which, command_wrk, item_tester)) {
                    bell();
                    break;
                }

                if ((fis_ptr->k < INVEN_MAIN_HAND) ? !fis_ptr->inven : !fis_ptr->equip) {
                    bell();
                    break;
                }

                if (!get_item_okay(player_ptr, fis_ptr->k, item_tester)) {
                    bell();
                    break;
                }
            } else {
                if (get_tag_floor(player_ptr->current_floor_ptr, &fis_ptr->k, fis_ptr->which, fis_ptr->floor_list, fis_ptr->floor_num)) {
                    fis_ptr->k = 0 - fis_ptr->floor_list[fis_ptr->k];
                } else {
                    bell();
                    break;
                }
            }

            if (!get_item_allow(player_ptr, fis_ptr->k)) {
                fis_ptr->done = true;
                break;
            }

            *cp = fis_ptr->k;
            fis_ptr->item = true;
            fis_ptr->done = true;
            fis_ptr->cur_tag = fis_ptr->which;
            break;
        }
        case 'w': {
            if (fis_ptr->force) {
                *cp = INVEN_FORCE;
                fis_ptr->item = true;
                fis_ptr->done = true;
                break;
            }
        }
            [[fallthrough]];
        default: {
            bool tag_not_found = false;

            if (command_wrk != USE_FLOOR) {
                if (!get_tag(player_ptr, &fis_ptr->k, fis_ptr->which, command_wrk, item_tester)) {
                    tag_not_found = true;
                } else if ((fis_ptr->k < INVEN_MAIN_HAND) ? !fis_ptr->inven : !fis_ptr->equip) {
                    tag_not_found = true;
                }

                if (!tag_not_found) {
                    fis_ptr->cur_tag = fis_ptr->which;
                }
            } else {
                if (get_tag_floor(player_ptr->current_floor_ptr, &fis_ptr->k, fis_ptr->which, fis_ptr->floor_list, fis_ptr->floor_num)) {
                    fis_ptr->k = 0 - fis_ptr->floor_list[fis_ptr->k];
                    fis_ptr->cur_tag = fis_ptr->which;
                } else {
                    tag_not_found = true;
                }
            }

            if (tag_not_found) {
                auto which = (char)tolower(fis_ptr->which);

                if (command_wrk == (USE_INVEN)) {
                    if (which == '(') {
                        fis_ptr->k = fis_ptr->i1;
                    } else if (which == ')') {
                        fis_ptr->k = fis_ptr->i2;
                    } else {
                        fis_ptr->k = label_to_inventory(player_ptr, which);
                    }
                } else if (command_wrk == (USE_EQUIP)) {
                    if (which == '(') {
                        fis_ptr->k = fis_ptr->e1;
                    } else if (which == ')') {
                        fis_ptr->k = fis_ptr->e2;
                    } else {
                        fis_ptr->k = label_to_equipment(player_ptr, which);
                    }
                } else if (command_wrk == USE_FLOOR) {
                    if (which == '(') {
                        fis_ptr->k = 0;
                    } else if (which == ')') {
                        fis_ptr->k = fis_ptr->floor_num - 1;
                    } else {
                        fis_ptr->k = islower(which) ? A2I(which) : -1;
                    }
                    if (fis_ptr->k < 0 || fis_ptr->k >= fis_ptr->floor_num || fis_ptr->k >= 23) {
                        bell();
                        break;
                    }

                    fis_ptr->k = 0 - fis_ptr->floor_list[fis_ptr->k];
                }
            }

            if ((command_wrk != USE_FLOOR) && !get_item_okay(player_ptr, fis_ptr->k, item_tester)) {
                bell();
                break;
            }

            auto ver = tag_not_found && isupper(fis_ptr->which);
            if (ver && !verify(player_ptr, _("本当に", "Try"), fis_ptr->k)) {
                fis_ptr->done = true;
                break;
            }

            if (!get_item_allow(player_ptr, fis_ptr->k)) {
                fis_ptr->done = true;
                break;
            }

            *cp = fis_ptr->k;
            fis_ptr->item = true;
            fis_ptr->done = true;
            break;
        }
        }
    }

    if (command_see) {
        screen_load();
        command_see = false;
    }

    if (fis_ptr->toggle) {
        toggle_inventory_equipment(player_ptr);
    }

    player_ptr->window_flags |= (PW_INVEN | PW_EQUIP);
    handle_stuff(player_ptr);
    prt("", 0, 0);
    if (fis_ptr->oops && str) {
        msg_print(str);
    }

    if (fis_ptr->item) {
        repeat_push(*cp);
        if (command_cmd) {
            prev_tag = fis_ptr->cur_tag;
        }
        command_cmd = 0;
    }

    return fis_ptr->item;
}
