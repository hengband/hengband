#include "inventory/player-inventory.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "floor/floor-object.h"
#include "floor/floor.h"
#include "game-option/birth-options.h"
#include "game-option/input-options.h"
#include "game-option/option-flags.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-util.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-flavor.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "object/object-mark-types.h"
#include "player/player-move.h"
#include "sv-definition/sv-other-types.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

void prepare_label_string(player_type *creature_ptr, char *label, BIT_FLAGS mode, tval_type tval);

/*!
 * @brief 所持/装備オブジェクトIDの部位表現を返す /
 * Return a string mentioning how a given item is carried
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param i 部位表現を求めるプレイヤーの所持/装備オブジェクトID
 * @return 部位表現の文字列ポインタ
 */
static concptr mention_use(player_type *owner_ptr, int i)
{
    concptr p;

    /* Examine the location */
    switch (i) {
#ifdef JP
    case INVEN_RARM:
        p = owner_ptr->heavy_wield[0] ? "運搬中" : ((owner_ptr->ryoute && owner_ptr->migite) ? " 両手" : (left_hander ? " 左手" : " 右手"));
        break;
#else
    case INVEN_RARM:
        p = owner_ptr->heavy_wield[0] ? "Just lifting" : (owner_ptr->migite ? "Wielding" : "On arm");
        break;
#endif

#ifdef JP
    case INVEN_LARM:
        p = owner_ptr->heavy_wield[1] ? "運搬中" : ((owner_ptr->ryoute && owner_ptr->hidarite) ? " 両手" : (left_hander ? " 右手" : " 左手"));
        break;
#else
    case INVEN_LARM:
        p = owner_ptr->heavy_wield[1] ? "Just lifting" : (owner_ptr->hidarite ? "Wielding" : "On arm");
        break;
#endif

    case INVEN_BOW:
        p = (adj_str_hold[owner_ptr->stat_ind[A_STR]] < owner_ptr->inventory_list[i].weight / 10) ? _("運搬中", "Just holding") : _("射撃用", "Shooting");
        break;
    case INVEN_RIGHT:
        p = (left_hander ? _("左手指", "On left hand") : _("右手指", "On right hand"));
        break;
    case INVEN_LEFT:
        p = (left_hander ? _("右手指", "On right hand") : _("左手指", "On left hand"));
        break;
    case INVEN_NECK:
        p = _("  首", "Around neck");
        break;
    case INVEN_LITE:
        p = _(" 光源", "Light source");
        break;
    case INVEN_BODY:
        p = _("  体", "On body");
        break;
    case INVEN_OUTER:
        p = _("体の上", "About body");
        break;
    case INVEN_HEAD:
        p = _("  頭", "On head");
        break;
    case INVEN_HANDS:
        p = _("  手", "On hands");
        break;
    case INVEN_FEET:
        p = _("  足", "On feet");
        break;
    default:
        p = _("ザック", "In pack");
        break;
    }

    return p;
}

/*!
 * @brief 装備アイテム一覧を表示する /
 * Choice window "shadow" of the "show_equip()" function
 * @return なし
 */
void display_equipment(player_type *owner_ptr, tval_type tval)
{
    if (!owner_ptr || !owner_ptr->inventory_list)
        return;

    TERM_LEN wid, hgt;
    Term_get_size(&wid, &hgt);

    TERM_COLOR attr = TERM_WHITE;
    char tmp_val[80];
    GAME_TEXT o_name[MAX_NLEN];
    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        o_ptr = &owner_ptr->inventory_list[i];
        tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';
        if (select_ring_slot ? is_ring_slot(i) : item_tester_okay(owner_ptr, o_ptr, tval)) {
            tmp_val[0] = index_to_label(i);
            tmp_val[1] = ')';
        }

        Term_putstr(0, i - INVEN_RARM, 3, TERM_WHITE, tmp_val);
        if ((((i == INVEN_RARM) && owner_ptr->hidarite) || ((i == INVEN_LARM) && owner_ptr->migite)) && owner_ptr->ryoute) {
            strcpy(o_name, _("(武器を両手持ち)", "(wielding with two-hands)"));
            attr = TERM_WHITE;
        } else {
            object_desc(owner_ptr, o_name, o_ptr, 0);
            attr = tval_to_attr[o_ptr->tval % 128];
        }

        int n = strlen(o_name);
        if (o_ptr->timeout) {
            attr = TERM_L_DARK;
        }
        Term_putstr(3, i - INVEN_RARM, n, attr, o_name);

        Term_erase(3 + n, i - INVEN_RARM, 255);

        if (show_weights) {
            int wgt = o_ptr->weight * o_ptr->number;
            sprintf(tmp_val, _("%3d.%1d kg", "%3d.%1d lb"), _(lbtokg1(wgt), wgt / 10), _(lbtokg2(wgt), wgt % 10));
            prt(tmp_val, i - INVEN_RARM, wid - (show_labels ? 28 : 9));
        }

        if (show_labels) {
            Term_putstr(wid - 20, i - INVEN_RARM, -1, TERM_WHITE, " <-- ");
            prt(mention_use(owner_ptr, i), i - INVEN_RARM, wid - 15);
        }
    }

    for (int i = INVEN_TOTAL - INVEN_RARM; i < hgt; i++) {
        Term_erase(0, i, 255);
    }
}

/*!
 * @brief サブウィンドウに所持品、装備品リストの表示を行う /
 * Flip "inven" and "equip" in any sub-windows
 * @return なし
 */
void toggle_inventory_equipment(player_type *owner_ptr)
{
    for (int j = 0; j < 8; j++) {
        if (!angband_term[j])
            continue;

        if (window_flag[j] & (PW_INVEN)) {
            window_flag[j] &= ~(PW_INVEN);
            window_flag[j] |= (PW_EQUIP);
            owner_ptr->window |= (PW_EQUIP);
            continue;
        }

        if (window_flag[j] & PW_EQUIP) {
            window_flag[j] &= ~(PW_EQUIP);
            window_flag[j] |= PW_INVEN;
            owner_ptr->window |= PW_INVEN;
        }
    }
}

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
    ITEM_NUMBER floor_num = scan_floor(owner_ptr, floor_list, owner_ptr->y, owner_ptr->x, 0x03, tval);
    return floor_num != 0;
}

/*!
 * @brief タグIDにあわせてタグアルファベットのリストを返す /
 * Move around label characters with correspond tags
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param label ラベルリストを取得する文字列参照ポインタ
 * @param mode 所持品リストか装備品リストかの切り替え
 * @return なし
 */
void prepare_label_string(player_type *owner_ptr, char *label, BIT_FLAGS mode, tval_type tval)
{
    concptr alphabet_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int offset = (mode == USE_EQUIP) ? INVEN_RARM : 0;
    strcpy(label, alphabet_chars);
    for (int i = 0; i < 52; i++) {
        COMMAND_CODE index;
        SYMBOL_CODE c = alphabet_chars[i];
        if (!get_tag(owner_ptr, &index, c, mode, tval))
            continue;

        if (label[i] == c)
            label[i] = ' ';

        label[index - offset] = c;
    }
}

/*!
 * @brief タグIDにあわせてタグアルファベットのリストを返す(床上アイテム用) /
 * Move around label characters with correspond tags (floor version)
 * @param label ラベルリストを取得する文字列参照ポインタ
 * @param floor_list 床上アイテムの配列
 * @param floor_num  床上アイテムの配列ID
 * @return なし
 */
/*
 */
static void prepare_label_string_floor(floor_type *floor_ptr, char *label, FLOOR_IDX floor_list[], ITEM_NUMBER floor_num)
{
    concptr alphabet_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    strcpy(label, alphabet_chars);
    for (int i = 0; i < 52; i++) {
        COMMAND_CODE index;
        SYMBOL_CODE c = alphabet_chars[i];
        if (!get_tag_floor(floor_ptr, &index, c, floor_list, floor_num))
            continue;

        if (label[i] == c)
            label[i] = ' ';

        label[index] = c;
    }
}

/*!
 * @brief 所持アイテムの表示を行う /
 * Display the inventory.
 * @param target_item アイテムの選択処理を行うか否か。
 * @return 選択したアイテムのタグ
 * @details
 * Hack -- do not display "trailing" empty slots
 */
COMMAND_CODE show_inventory(player_type *owner_ptr, int target_item, BIT_FLAGS mode, tval_type tval)
{
    COMMAND_CODE i;
    int k, l, z = 0;
    object_type *o_ptr;
    GAME_TEXT o_name[MAX_NLEN];
    char tmp_val[80];
    COMMAND_CODE out_index[23];
    TERM_COLOR out_color[23];
    char out_desc[23][MAX_NLEN];
    COMMAND_CODE target_item_label = 0;
    char inven_label[52 + 1];

    int col = command_gap;
    TERM_LEN wid, hgt;
    Term_get_size(&wid, &hgt);
    int len = wid - col - 1;
    for (i = 0; i < INVEN_PACK; i++) {
        o_ptr = &owner_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        z = i + 1;
    }

    prepare_label_string(owner_ptr, inven_label, USE_INVEN, tval);
    for (k = 0, i = 0; i < z; i++) {
        o_ptr = &owner_ptr->inventory_list[i];
        if (!item_tester_okay(owner_ptr, o_ptr, tval) && !(mode & USE_FULL))
            continue;

        object_desc(owner_ptr, o_name, o_ptr, 0);
        out_index[k] = i;
        out_color[k] = tval_to_attr[o_ptr->tval % 128];
        if (o_ptr->timeout)
            out_color[k] = TERM_L_DARK;

        (void)strcpy(out_desc[k], o_name);
        l = strlen(out_desc[k]) + 5;
        if (show_weights)
            l += 9;

        if (show_item_graph) {
            l += 2;
            if (use_bigtile)
                l++;
        }

        if (l > len)
            len = l;

        k++;
    }

    col = (len > wid - 4) ? 0 : (wid - len - 1);
    int cur_col;
    int j;
    for (j = 0; j < k; j++) {
        i = out_index[j];
        o_ptr = &owner_ptr->inventory_list[i];
        prt("", j + 1, col ? col - 2 : col);
        if (use_menu && target_item) {
            if (j == (target_item - 1)) {
                strcpy(tmp_val, _("》", "> "));
                target_item_label = i;
            } else
                strcpy(tmp_val, "  ");
        } else if (i <= INVEN_PACK)
            sprintf(tmp_val, "%c)", inven_label[i]);
        else
            sprintf(tmp_val, "%c)", index_to_label(i));

        put_str(tmp_val, j + 1, col);
        cur_col = col + 3;
        if (show_item_graph) {
            TERM_COLOR a = object_attr(o_ptr);
            SYMBOL_CODE c = object_char(o_ptr);
            Term_queue_bigchar(cur_col, j + 1, a, c, 0, 0);
            if (use_bigtile)
                cur_col++;

            cur_col += 2;
        }

        c_put_str(out_color[j], out_desc[j], j + 1, cur_col);
        if (show_weights) {
            int wgt = o_ptr->weight * o_ptr->number;
            (void)sprintf(tmp_val, _("%3d.%1d kg", "%3d.%1d lb"), _(lbtokg1(wgt), wgt / 10), _(lbtokg2(wgt), wgt % 10));
            prt(tmp_val, j + 1, wid - 9);
        }
    }

    if (j && (j < 23))
        prt("", j + 1, col ? col - 2 : col);

    command_gap = col;
    return target_item_label;
}

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

/*
 * Choose an item and get auto-picker entry from it.
 */
object_type *choose_object(player_type *owner_ptr, OBJECT_IDX *idx, concptr q, concptr s, BIT_FLAGS option, tval_type tval)
{
    OBJECT_IDX item;
    if (!get_item(owner_ptr, &item, q, s, option, tval))
        return NULL;

    if (idx)
        *idx = item;

    if (item == INVEN_FORCE)
        return NULL;

    return ref_item(owner_ptr, item);
}

/*!
 * todo ここの引数をfloor_typeにするとコンパイルが通らない、要確認
 * @brief 床下に落ちているオブジェクトの数を返す / scan_floor
 * @param items オブジェクトのIDリストを返すための配列参照ポインタ
 * @param y 走査するフロアのY座標
 * @param x 走査するフロアのX座標
 * @param mode オプションフラグ
 * @return 対象のマスに落ちているアイテム数
 * @details
 * Return a list of o_list[] indexes of items at the given floor
 * location. Valid flags are:
 *
 *		mode & 0x01 -- Item tester
 *		mode & 0x02 -- Marked items only
 *		mode & 0x04 -- Stop after first
 */
ITEM_NUMBER scan_floor(player_type *owner_ptr, OBJECT_IDX *items, POSITION y, POSITION x, BIT_FLAGS mode, tval_type item_tester_tval)
{
    floor_type *floor_ptr = owner_ptr->current_floor_ptr;
    if (!in_bounds(floor_ptr, y, x))
        return 0;

    OBJECT_IDX this_o_idx, next_o_idx;
    ITEM_NUMBER num = 0;
    for (this_o_idx = floor_ptr->grid_array[y][x].o_idx; this_o_idx; this_o_idx = next_o_idx) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;
        if ((mode & 0x01) && !item_tester_okay(owner_ptr, o_ptr, item_tester_tval))
            continue;

        if ((mode & 0x02) && !(o_ptr->marked & OM_FOUND))
            continue;

        if (num < 23)
            items[num] = this_o_idx;

        num++;
        if (mode & 0x04)
            break;
    }

    return num;
}

/*!
 * @brief 床下に落ちているアイテムの一覧を返す / Display a list of the items on the floor at the given location.
 * @param target_item カーソルの初期値
 * @param y 走査するフロアのY座標
 * @param x 走査するフロアのX座標
 * @param min_width 表示の長さ
 * @return 選択したアイテムの添え字
 * @details
 */
COMMAND_CODE show_floor(player_type *owner_ptr, int target_item, POSITION y, POSITION x, TERM_LEN *min_width, tval_type item_tester_tval)
{
    COMMAND_CODE i, m;
    int j, k, l;
    object_type *o_ptr;
    GAME_TEXT o_name[MAX_NLEN];
    char tmp_val[80];
    COMMAND_CODE out_index[23];
    TERM_COLOR out_color[23];
    char out_desc[23][MAX_NLEN];
    COMMAND_CODE target_item_label = 0;
    OBJECT_IDX floor_list[23];
    ITEM_NUMBER floor_num;
    TERM_LEN wid, hgt;
    char floor_label[52 + 1];
    bool dont_need_to_show_weights = TRUE;
    Term_get_size(&wid, &hgt);
    int len = MAX((*min_width), 20);
    floor_num = scan_floor(owner_ptr, floor_list, y, x, 0x03, item_tester_tval);
    floor_type *floor_ptr = owner_ptr->current_floor_ptr;
    for (k = 0, i = 0; i < floor_num && i < 23; i++) {
        o_ptr = &floor_ptr->o_list[floor_list[i]];
        object_desc(owner_ptr, o_name, o_ptr, 0);
        out_index[k] = i;
        out_color[k] = tval_to_attr[o_ptr->tval & 0x7F];
        strcpy(out_desc[k], o_name);
        l = strlen(out_desc[k]) + 5;
        if (show_weights)
            l += 9;

        if (o_ptr->tval != TV_GOLD)
            dont_need_to_show_weights = FALSE;

        if (l > len)
            len = l;

        k++;
    }

    if (show_weights && dont_need_to_show_weights)
        len -= 9;

    *min_width = len;
    int col = (len > wid - 4) ? 0 : (wid - len - 1);
    prepare_label_string_floor(floor_ptr, floor_label, floor_list, floor_num);
    for (j = 0; j < k; j++) {
        m = floor_list[out_index[j]];
        o_ptr = &floor_ptr->o_list[m];
        prt("", j + 1, col ? col - 2 : col);
        if (use_menu && target_item) {
            if (j == (target_item - 1)) {
                strcpy(tmp_val, _("》", "> "));
                target_item_label = m;
            } else
                strcpy(tmp_val, "   ");
        } else {
            sprintf(tmp_val, "%c)", floor_label[j]);
        }

        put_str(tmp_val, j + 1, col);
        c_put_str(out_color[j], out_desc[j], j + 1, col + 3);
        if (show_weights && (o_ptr->tval != TV_GOLD)) {
            int wgt = o_ptr->weight * o_ptr->number;
            sprintf(tmp_val, _("%3d.%1d kg", "%3d.%1d lb"), _(lbtokg1(wgt), wgt / 10), _(lbtokg2(wgt), wgt % 10));
            prt(tmp_val, j + 1, wid - 9);
        }
    }

    if (j && (j < 23))
        prt("", j + 1, col ? col - 2 : col);

    return target_item_label;
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
                floor_num = scan_floor(owner_ptr, floor_list, owner_ptr->y, owner_ptr->x, 0x03, tval);
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
        floor_num = scan_floor(owner_ptr, floor_list, owner_ptr->y, owner_ptr->x, 0x03, tval);

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
                get_item_label = show_floor(owner_ptr, menu_line, owner_ptr->y, owner_ptr->x, &min_width, tval);
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
            floor_num = scan_floor(owner_ptr, floor_list, owner_ptr->y, owner_ptr->x, 0x03, tval);
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
    if (choose_object(owner_ptr, &item, q, s, (USE_FLOOR), 0))
        this_o_idx = 0 - item;
    else
        return FALSE;

    py_pickup_aux(owner_ptr, this_o_idx);
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
        object_desc(owner_ptr, o_name, o_ptr, 0);
        next_o_idx = o_ptr->next_o_idx;
        disturb(owner_ptr, FALSE, FALSE);
        if (o_ptr->tval == TV_GOLD) {
            msg_format(_(" $%ld の価値がある%sを見つけた。", "You have found %ld gold pieces worth of %s."), (long)o_ptr->pval, o_name);
            owner_ptr->au += o_ptr->pval;
            owner_ptr->redraw |= (PR_GOLD);
            owner_ptr->window |= (PW_PLAYER);
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
            object_desc(owner_ptr, o_name, o_ptr, 0);
            msg_format(_("%sがある。", "You see %s."), o_name);
        } else
            msg_format(_("%d 個のアイテムの山がある。", "You see a pile of %d items."), floor_num);

        return;
    }

    if (!can_pickup) {
        if (floor_num == 1) {
            o_ptr = &owner_ptr->current_floor_ptr->o_list[floor_o_idx];
            object_desc(owner_ptr, o_name, o_ptr, 0);
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
        object_desc(owner_ptr, o_name, o_ptr, 0);
        (void)sprintf(out_val, _("%sを拾いますか? ", "Pick up %s? "), o_name);
        if (!get_check(out_val))
            return;
    }

    o_ptr = &owner_ptr->current_floor_ptr->o_list[floor_o_idx];
    py_pickup_aux(owner_ptr, floor_o_idx);
}

/*!
 * @brief 所持アイテム一覧を表示する /
 * Choice window "shadow" of the "show_inven()" function
 * @return なし
 */
void display_inventory(player_type *owner_ptr, tval_type tval)
{
    register int i, n, z = 0;
    object_type *o_ptr;
    TERM_COLOR attr = TERM_WHITE;
    char tmp_val[80];
    GAME_TEXT o_name[MAX_NLEN];
    TERM_LEN wid, hgt;

    if (!owner_ptr || !owner_ptr->inventory_list)
        return;

    Term_get_size(&wid, &hgt);
    for (i = 0; i < INVEN_PACK; i++) {
        o_ptr = &owner_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        z = i + 1;
    }

    for (i = 0; i < z; i++) {
        o_ptr = &owner_ptr->inventory_list[i];
        tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';
        if (item_tester_okay(owner_ptr, o_ptr, tval)) {
            tmp_val[0] = index_to_label(i);
            tmp_val[1] = ')';
        }

        Term_putstr(0, i, 3, TERM_WHITE, tmp_val);
        object_desc(owner_ptr, o_name, o_ptr, 0);
        n = strlen(o_name);
        attr = tval_to_attr[o_ptr->tval % 128];
        if (o_ptr->timeout) {
            attr = TERM_L_DARK;
        }

        Term_putstr(3, i, n, attr, o_name);
        Term_erase(3 + n, i, 255);

        if (show_weights) {
            int wgt = o_ptr->weight * o_ptr->number;
            sprintf(tmp_val, _("%3d.%1d kg", "%3d.%1d lb"), _(lbtokg1(wgt), wgt / 10), _(lbtokg2(wgt), wgt % 10));
            prt(tmp_val, i, wid - 9);
        }
    }

    for (i = z; i < hgt; i++)
        Term_erase(0, i, 255);
}

/*!
 * @brief 装備アイテムの表示を行う /
 * Display the equipment.
 * @param target_item アイテムの選択処理を行うか否か。
 * @return 選択したアイテムのタグ
 */
COMMAND_CODE show_equipment(player_type *owner_ptr, int target_item, BIT_FLAGS mode, tval_type tval)
{
    COMMAND_CODE i;
    int j, k, l;
    object_type *o_ptr;
    char tmp_val[80];
    GAME_TEXT o_name[MAX_NLEN];
    COMMAND_CODE out_index[23];
    TERM_COLOR out_color[23];
    char out_desc[23][MAX_NLEN];
    COMMAND_CODE target_item_label = 0;
    TERM_LEN wid, hgt;
    char equip_label[52 + 1];
    int col = command_gap;
    Term_get_size(&wid, &hgt);
    int len = wid - col - 1;
    for (k = 0, i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &owner_ptr->inventory_list[i];
        if (!(select_ring_slot ? is_ring_slot(i) : item_tester_okay(owner_ptr, o_ptr, tval) || (mode & USE_FULL))
            && (!((((i == INVEN_RARM) && owner_ptr->hidarite) || ((i == INVEN_LARM) && owner_ptr->migite)) && owner_ptr->ryoute)
                || (mode & IGNORE_BOTHHAND_SLOT)))
            continue;

        object_desc(owner_ptr, o_name, o_ptr, 0);
        if ((((i == INVEN_RARM) && owner_ptr->hidarite) || ((i == INVEN_LARM) && owner_ptr->migite)) && owner_ptr->ryoute) {
            (void)strcpy(out_desc[k], _("(武器を両手持ち)", "(wielding with two-hands)"));
            out_color[k] = TERM_WHITE;
        } else {
            (void)strcpy(out_desc[k], o_name);
            out_color[k] = tval_to_attr[o_ptr->tval % 128];
        }

        out_index[k] = i;
        if (o_ptr->timeout)
            out_color[k] = TERM_L_DARK;
        l = strlen(out_desc[k]) + (2 + _(1, 3));

        if (show_labels)
            l += (_(7, 14) + 2);

        if (show_weights)
            l += 9;

        if (show_item_graph)
            l += 2;

        if (l > len)
            len = l;

        k++;
    }

    col = (len > wid - _(6, 4)) ? 0 : (wid - len - 1);
    prepare_label_string(owner_ptr, equip_label, USE_EQUIP, tval);
    for (j = 0; j < k; j++) {
        i = out_index[j];
        o_ptr = &owner_ptr->inventory_list[i];
        prt("", j + 1, col ? col - 2 : col);
        if (use_menu && target_item) {
            if (j == (target_item - 1)) {
                strcpy(tmp_val, _("》", "> "));
                target_item_label = i;
            } else
                strcpy(tmp_val, "  ");
        } else if (i >= INVEN_RARM)
            sprintf(tmp_val, "%c)", equip_label[i - INVEN_RARM]);
        else
            sprintf(tmp_val, "%c)", index_to_label(i));

        put_str(tmp_val, j + 1, col);
        int cur_col = col + 3;
        if (show_item_graph) {
            TERM_COLOR a = object_attr(o_ptr);
            SYMBOL_CODE c = object_char(o_ptr);
            Term_queue_bigchar(cur_col, j + 1, a, c, 0, 0);
            if (use_bigtile)
                cur_col++;

            cur_col += 2;
        }

        if (show_labels) {
            (void)sprintf(tmp_val, _("%-7s: ", "%-14s: "), mention_use(owner_ptr, i));
            put_str(tmp_val, j + 1, cur_col);
            c_put_str(out_color[j], out_desc[j], j + 1, _(cur_col + 9, cur_col + 16));
        } else
            c_put_str(out_color[j], out_desc[j], j + 1, cur_col);

        if (!show_weights)
            continue;

        int wgt = o_ptr->weight * o_ptr->number;
        (void)sprintf(tmp_val, _("%3d.%1d kg", "%3d.%d lb"), _(lbtokg1(wgt), wgt / 10), _(lbtokg2(wgt), wgt % 10));
        prt(tmp_val, j + 1, wid - 9);
    }

    if (j && (j < 23))
        prt("", j + 1, col ? col - 2 : col);

    command_gap = col;
    return target_item_label;
}

/*!
 * @brief 所持/装備オブジェクトIDの現在の扱い方の状態表現を返す /
 * Return a string describing how a given item is being worn.
 * @param i 状態表現を求めるプレイヤーの所持/装備オブジェクトID
 * @return 状態表現内容の文字列ポインタ
 * @details
 * Currently, only used for items in the equipment, inventory.
 */
concptr describe_use(player_type *owner_ptr, int i)
{
    concptr p;
    switch (i) {
#ifdef JP
    case INVEN_RARM:
        p = owner_ptr->heavy_wield[0]
            ? "運搬中の"
            : ((owner_ptr->ryoute && owner_ptr->migite) ? "両手に装備している" : (left_hander ? "左手に装備している" : "右手に装備している"));
        break;
#else
    case INVEN_RARM:
        p = owner_ptr->heavy_wield[0] ? "just lifting" : (owner_ptr->migite ? "attacking monsters with" : "wearing on your arm");
        break;
#endif

#ifdef JP
    case INVEN_LARM:
        p = owner_ptr->heavy_wield[1]
            ? "運搬中の"
            : ((owner_ptr->ryoute && owner_ptr->hidarite) ? "両手に装備している" : (left_hander ? "右手に装備している" : "左手に装備している"));
        break;
#else
    case INVEN_LARM:
        p = owner_ptr->heavy_wield[1] ? "just lifting" : (owner_ptr->hidarite ? "attacking monsters with" : "wearing on your arm");
        break;
#endif

    case INVEN_BOW:
        p = (adj_str_hold[owner_ptr->stat_ind[A_STR]] < owner_ptr->inventory_list[i].weight / 10) ? _("持つだけで精一杯の", "just holding")
                                                                                                  : _("射撃用に装備している", "shooting missiles with");
        break;
    case INVEN_RIGHT:
        p = (left_hander ? _("左手の指にはめている", "wearing on your left hand") : _("右手の指にはめている", "wearing on your right hand"));
        break;
    case INVEN_LEFT:
        p = (left_hander ? _("右手の指にはめている", "wearing on your right hand") : _("左手の指にはめている", "wearing on your left hand"));
        break;
    case INVEN_NECK:
        p = _("首にかけている", "wearing around your neck");
        break;
    case INVEN_LITE:
        p = _("光源にしている", "using to light the way");
        break;
    case INVEN_BODY:
        p = _("体に着ている", "wearing on your body");
        break;
    case INVEN_OUTER:
        p = _("身にまとっている", "wearing on your back");
        break;
    case INVEN_HEAD:
        p = _("頭にかぶっている", "wearing on your head");
        break;
    case INVEN_HANDS:
        p = _("手につけている", "wearing on your hands");
        break;
    case INVEN_FEET:
        p = _("足にはいている", "wearing on your feet");
        break;
    default:
        p = _("ザックに入っている", "carrying in your pack");
        break;
    }

    return p;
}
