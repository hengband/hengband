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
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-inventory.h"
#include "view/display-messages.h"
#include "window/display-sub-windows.h"
#include <fmt/format.h>
#include <sstream>
#include <tl/optional.hpp>

/*!
 * @brief 床上アイテムへにタグ付けがされているかの調査処理 (のはず)
 * @param floor フロアへの参照
 * @param p_pos プレイヤーの座標
 * @param fis 床上アイテムへの参照
 * @param prev_tag 前回選択したアイテムのタグ (のはず)
 * @return 選択したアイテムインデックス (なければnullopt)とタグの組
 * @todo 適切な関数名をどうしても付けられなかったので暫定でauxとした
 */
static std::pair<tl::optional<short>, char> check_floor_item_tag_aux(const FloorType &floor, const Pos2D &p_pos, FloorItemSelection &fis, short i_idx, char prev_tag, const ItemTester &item_tester)
{
    if (!fis.floor || (i_idx >= 0)) {
        return { tl::nullopt, prev_tag };
    }

    if ((prev_tag != '\0') && command_cmd) {
        fis.floor_list = scan_floor_items(floor, p_pos, { ScanFloorMode::ITEM_TESTER, ScanFloorMode::ONLY_MARKED }, item_tester);
        if (get_tag_floor(floor, &fis.k, prev_tag, fis.floor_list.data(), fis.floor_list.size())) {
            command_cmd = 0;
            return { -fis.floor_list[fis.k], prev_tag };
        }

        return { tl::nullopt, '\0' };
    }

    if (floor.prevent_repeat_floor_item_idx || std::cmp_greater_equal(-i_idx, floor.o_list.size())) {
        return { tl::nullopt, prev_tag };
    }

    if (!item_tester.okay(floor.o_list[-i_idx].get()) && ((fis.mode & USE_FULL) == 0)) {
        return { tl::nullopt, prev_tag };
    }

    command_cmd = 0;
    return { i_idx, prev_tag };
}

/*!
 * @brief インベントリのアイテムにタグ付けを試みる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fis_ptr 床上アイテムへの参照ポインタ
 * @param prev_tag 前回選択したアイテムのタグ (のはず)
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す
 */
static bool get_floor_item_tag_inventory(PlayerType *player_ptr, FloorItemSelection *fis_ptr, char *prev_tag, const ItemTester &item_tester)
{
    bool flag = false;
    item_use_flag use_flag = (fis_ptr->cp >= INVEN_MAIN_HAND) ? USE_EQUIP : USE_INVEN;

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

    fis_ptr->cp = fis_ptr->k;
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
static bool check_floor_item_tag_inventory(PlayerType *player_ptr, FloorItemSelection *fis_ptr, char *prev_tag, const ItemTester &item_tester)
{
    if ((!fis_ptr->inven || (fis_ptr->cp < 0) || (fis_ptr->cp >= INVEN_PACK)) && (!fis_ptr->equip || (fis_ptr->cp < INVEN_MAIN_HAND) || (fis_ptr->cp >= INVEN_TOTAL))) {
        return false;
    }

    if ((*prev_tag != '\0') && command_cmd) {
        return get_floor_item_tag_inventory(player_ptr, fis_ptr, prev_tag, item_tester);
    }

    if (get_item_okay(player_ptr, fis_ptr->cp, item_tester)) {
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
static bool check_floor_item_tag(PlayerType *player_ptr, FloorItemSelection *fis_ptr, char *prev_tag, const ItemTester &item_tester)
{
    const auto code = repeat_pull();
    if (!code) {
        return false;
    }

    fis_ptr->cp = *code;
    if (fis_ptr->force && (fis_ptr->cp == INVEN_FORCE)) {
        command_cmd = 0;
        return true;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    const auto &[floor_item_indice, tag] = check_floor_item_tag_aux(floor, p_pos, *fis_ptr, *code, *prev_tag, item_tester);
    *prev_tag = tag;
    if (floor_item_indice) {
        fis_ptr->cp = *floor_item_indice;
        return true;
    }

    return check_floor_item_tag_inventory(player_ptr, fis_ptr, prev_tag, item_tester);
}

/*!
 * @brief インベントリ内のアイテムが妥当かを判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fis_ptr 床上アイテムへの参照ポインタ
 */
static void test_inventory_floor(PlayerType *player_ptr, FloorItemSelection *fis_ptr, const ItemTester &item_tester)
{
    if (!fis_ptr->inven) {
        fis_ptr->i2 = -1;
        return;
    }

    if (!use_menu) {
        return;
    }

    for (int i = 0; i < INVEN_PACK; i++) {
        if (item_tester.okay(player_ptr->inventory[i].get()) || (fis_ptr->mode & USE_FULL)) {
            fis_ptr->max_inven++;
        }
    }
}

/*!
 * @brief 装備品がが妥当かを判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fis_ptr 床上アイテムへの参照ポインタ
 */
static void test_equipment_floor(PlayerType *player_ptr, FloorItemSelection *fis_ptr, const ItemTester &item_tester)
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
                                         : item_tester.okay(player_ptr->inventory[i].get()) || (fis_ptr->mode & USE_FULL)) {
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
    FloorItemSelection fis(mode);
    static char prev_tag = '\0';
    if (check_floor_item_tag(player_ptr, &fis, &prev_tag, item_tester)) {
        *cp = fis.cp;
        return true;
    }

    msg_erase();
    handle_stuff(player_ptr);
    test_inventory_floor(player_ptr, &fis, item_tester);
    fis.done = false;
    fis.item = false;
    fis.i1 = 0;
    fis.i2 = INVEN_PACK - 1;
    while ((fis.i1 <= fis.i2) && (!get_item_okay(player_ptr, fis.i1, item_tester))) {
        fis.i1++;
    }

    while ((fis.i1 <= fis.i2) && (!get_item_okay(player_ptr, fis.i2, item_tester))) {
        fis.i2--;
    }

    fis.e1 = INVEN_MAIN_HAND;
    fis.e2 = INVEN_TOTAL - 1;
    test_equipment_floor(player_ptr, &fis, item_tester);
    if (has_two_handed_weapons(player_ptr) && !(fis.mode & IGNORE_BOTHHAND_SLOT)) {
        fis.max_equip++;
    }

    while ((fis.e1 <= fis.e2) && (!get_item_okay(player_ptr, fis.e1, item_tester))) {
        fis.e1++;
    }

    while ((fis.e1 <= fis.e2) && (!get_item_okay(player_ptr, fis.e2, item_tester))) {
        fis.e2--;
    }

    if (fis.equip && has_two_handed_weapons(player_ptr) && !(fis.mode & IGNORE_BOTHHAND_SLOT)) {
        if (can_attack_with_main_hand(player_ptr)) {
            if (fis.e2 < INVEN_SUB_HAND) {
                fis.e2 = INVEN_SUB_HAND;
            }
        } else if (can_attack_with_sub_hand(player_ptr)) {
            fis.e1 = INVEN_MAIN_HAND;
        }
    }

    if (fis.floor) {
        const auto &floor = *player_ptr->current_floor_ptr;
        fis.floor_list = scan_floor_items(floor, player_ptr->get_position(), { ScanFloorMode::ITEM_TESTER, ScanFloorMode::ONLY_MARKED }, item_tester);
    }

    if ((mode & USE_INVEN) && (fis.i1 <= fis.i2)) {
        fis.allow_inven = true;
    }

    if ((mode & USE_EQUIP) && (fis.e1 <= fis.e2)) {
        fis.allow_equip = true;
    }

    if ((mode & USE_FLOOR) && !fis.floor_list.empty()) {
        fis.allow_floor = true;
    }

    if (!fis.allow_inven && !fis.allow_equip && !fis.allow_floor) {
        command_see = false;
        fis.oops = true;
        fis.done = true;

        if (fis.force) {
            fis.cp = INVEN_FORCE;
            fis.item = true;
        }
    } else {
        if (command_see && (command_wrk == USE_EQUIP) && fis.allow_equip) {
            command_wrk = USE_EQUIP;
        } else if (fis.allow_inven) {
            command_wrk = USE_INVEN;
        } else if (fis.allow_equip) {
            command_wrk = USE_EQUIP;
        } else if (fis.allow_floor) {
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

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
    };
    while (!fis.done) {
        int ni = 0;
        int ne = 0;
        for (auto i = 0U; i < angband_terms.size(); ++i) {
            if (!angband_terms[i]) {
                continue;
            }

            if (g_window_flags[i].has(SubWindowRedrawingFlag::INVENTORY)) {
                ni++;
            }

            if (g_window_flags[i].has(SubWindowRedrawingFlag::EQUIPMENT)) {
                ne++;
            }
        }

        if ((command_wrk == (USE_EQUIP) && ni && !ne) || (command_wrk == (USE_INVEN) && !ni && ne)) {
            toggle_inventory_equipment();
            fis.toggle = !fis.toggle;
        }

        rfu.set_flags(flags);
        handle_stuff(player_ptr);
        COMMAND_CODE get_item_label = 0;
        if (command_wrk == USE_INVEN) {
            fis.n1 = I2A(fis.i1);
            fis.n2 = I2A(fis.i2);
            if (command_see) {
                get_item_label = show_inventory(player_ptr, fis.menu_line, fis.mode, item_tester);
            }
        } else if (command_wrk == USE_EQUIP) {
            fis.n1 = I2A(fis.e1 - INVEN_MAIN_HAND);
            fis.n2 = I2A(fis.e2 - INVEN_MAIN_HAND);
            if (command_see) {
                get_item_label = show_equipment(player_ptr, fis.menu_line, mode, item_tester);
            }
        } else if (command_wrk == USE_FLOOR) {
            int j = fis.floor_top;
            fis.k = std::min(fis.floor_top + 23, static_cast<int>(fis.floor_list.size())) - 1;
            fis.n1 = I2A(j - fis.floor_top); // TODO: 常に'0'になる。どんな意図でこのようなコードになっているのか不明.
            fis.n2 = I2A(fis.k - fis.floor_top);
            if (command_see) {
                get_item_label = show_floor_items(player_ptr, fis.menu_line, player_ptr->y, player_ptr->x, &fis.min_width, item_tester);
            }
        }

        std::stringstream ss;
        if (command_wrk == USE_INVEN) {
            ss << _("持ち物:", "Inven:");
            if (!use_menu) {
                ss << fmt::format(_("{}-{},'(',')',", " {}-{},'(',')',"), index_to_label(fis.i1), index_to_label(fis.i2));
            }

            if (!command_see && !use_menu) {
                ss << _(" '*'一覧,", " * to see,");
            }

            if (fis.allow_equip) {
                if (!use_menu) {
                    ss << _(" '/' 装備品,", " / for Equip,");
                } else if (fis.allow_floor) {
                    ss << _(" '6' 装備品,", " 6 for Equip,");
                } else {
                    ss << _(" '4'or'6' 装備品,", " 4 or 6 for Equip,");
                }
            }

            if (fis.allow_floor) {
                if (!use_menu) {
                    ss << _(" '-'床上,", " - for floor,");
                } else if (fis.allow_equip) {
                    ss << _(" '4' 床上,", " 4 for floor,");
                } else {
                    ss << _(" '4'or'6' 床上,", " 4 or 6 for floor,");
                }
            }
        } else if (command_wrk == (USE_EQUIP)) {
            ss << _("装備品:", "Equip:");
            if (!use_menu) {
                ss << fmt::format(_("{}-{},'(',')',", " {}-{},'(',')',"), index_to_label(fis.e1), index_to_label(fis.e2));
            }

            if (!command_see && !use_menu) {
                ss << _(" '*'一覧,", " * to see,");
            }

            if (fis.allow_inven) {
                if (!use_menu) {
                    ss << _(" '/' 持ち物,", " / for Inven,");
                } else if (fis.allow_floor) {
                    ss << _(" '4' 持ち物,", " 4 for Inven,");
                } else {
                    ss << _(" '4'or'6' 持ち物,", " 4 or 6 for Inven,");
                }
            }

            if (fis.allow_floor) {
                if (!use_menu) {
                    ss << _(" '-'床上,", " - for floor,");
                } else if (fis.allow_inven) {
                    ss << _(" '6' 床上,", " 6 for floor,");
                } else {
                    ss << _(" '4'or'6' 床上,", " 4 or 6 for floor,");
                }
            }
        } else if (command_wrk == USE_FLOOR) {
            ss << _("床上:", "Floor:");
            if (!use_menu) {
                ss << fmt::format(_("{}-{},'(',')',", " {}-{},'(',')',"), fis.n1, fis.n2);
            }

            if (!command_see && !use_menu) {
                ss << _(" '*'一覧,", " * to see,");
            }

            if (use_menu) {
                if (fis.allow_inven && fis.allow_equip) {
                    ss << _(" '4' 装備品, '6' 持ち物,", " 4 for Equip, 6 for Inven,");
                } else if (fis.allow_inven) {
                    ss << _(" '4'or'6' 持ち物,", " 4 or 6 for Inven,");
                } else if (fis.allow_equip) {
                    ss << _(" '4'or'6' 装備品,", " 4 or 6 for Equip,");
                }
            } else if (fis.allow_inven) {
                ss << _(" '/' 持ち物,", " / for Inven,");
            } else if (fis.allow_equip) {
                ss << _(" '/'装備品,", " / for Equip,");
            }

            if (command_see && !use_menu) {
                ss << _(" Enter 次,", " Enter for scroll down,");
            }
        }

        if (fis.force) {
            ss << _(" 'w'練気術,", " w for the Force,");
        }

        ss << " ESC";
        prt(fmt::format("({}) {}", ss.str(), pmt), 0, 0);
        fis.which = inkey();
        if (use_menu) {
            int max_line = 1;
            if (command_wrk == USE_INVEN) {
                max_line = fis.max_inven;
            } else if (command_wrk == USE_EQUIP) {
                max_line = fis.max_equip;
            } else if (command_wrk == USE_FLOOR) {
                max_line = std::min(23, static_cast<int>(fis.floor_list.size()));
            }
            switch (fis.which) {
            case ESCAPE:
            case 'z':
            case 'Z':
            case '0': {
                fis.done = true;
                break;
            }
            case '8':
            case 'k':
            case 'K': {
                fis.menu_line += (max_line - 1);
                break;
            }
            case '2':
            case 'j':
            case 'J': {
                fis.menu_line++;
                break;
            }
            case '4':
            case 'h':
            case 'H': {
                if (command_wrk == (USE_INVEN)) {
                    if (fis.allow_floor) {
                        command_wrk = USE_FLOOR;
                    } else if (fis.allow_equip) {
                        command_wrk = USE_EQUIP;
                    } else {
                        bell();
                        break;
                    }
                } else if (command_wrk == (USE_EQUIP)) {
                    if (fis.allow_inven) {
                        command_wrk = USE_INVEN;
                    } else if (fis.allow_floor) {
                        command_wrk = USE_FLOOR;
                    } else {
                        bell();
                        break;
                    }
                } else if (command_wrk == (USE_FLOOR)) {
                    if (fis.allow_equip) {
                        command_wrk = USE_EQUIP;
                    } else if (fis.allow_inven) {
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
                    max_line = fis.max_inven;
                } else if (command_wrk == USE_EQUIP) {
                    max_line = fis.max_equip;
                } else if (command_wrk == USE_FLOOR) {
                    max_line = std::min(23, static_cast<int>(fis.floor_list.size()));
                }

                if (fis.menu_line > max_line) {
                    fis.menu_line = max_line;
                }

                break;
            }
            case '6':
            case 'l':
            case 'L': {
                if (command_wrk == (USE_INVEN)) {
                    if (fis.allow_equip) {
                        command_wrk = USE_EQUIP;
                    } else if (fis.allow_floor) {
                        command_wrk = USE_FLOOR;
                    } else {
                        bell();
                        break;
                    }
                } else if (command_wrk == (USE_EQUIP)) {
                    if (fis.allow_floor) {
                        command_wrk = USE_FLOOR;
                    } else if (fis.allow_inven) {
                        command_wrk = USE_INVEN;
                    } else {
                        bell();
                        break;
                    }
                } else if (command_wrk == (USE_FLOOR)) {
                    if (fis.allow_inven) {
                        command_wrk = USE_INVEN;
                    } else if (fis.allow_equip) {
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
                    max_line = fis.max_inven;
                } else if (command_wrk == USE_EQUIP) {
                    max_line = fis.max_equip;
                } else if (command_wrk == USE_FLOOR) {
                    max_line = std::min(23, static_cast<int>(fis.floor_list.size()));
                }

                if (fis.menu_line > max_line) {
                    fis.menu_line = max_line;
                }

                break;
            }
            case 'x':
            case 'X':
            case '\r':
            case '\n': {
                if (command_wrk == USE_FLOOR) {
                    fis.cp = -get_item_label;
                } else {
                    if (!get_item_okay(player_ptr, get_item_label, item_tester)) {
                        bell();
                        break;
                    }

                    if (!get_item_allow(player_ptr, get_item_label)) {
                        fis.done = true;
                        break;
                    }

                    fis.cp = get_item_label;
                }

                fis.item = true;
                fis.done = true;
                break;
            }
            case 'w': {
                if (fis.force) {
                    fis.cp = INVEN_FORCE;
                    fis.item = true;
                    fis.done = true;
                    break;
                }
            }
            }

            if (fis.menu_line > max_line) {
                fis.menu_line -= max_line;
            }

            continue;
        }

        switch (fis.which) {
        case ESCAPE: {
            fis.done = true;
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
            auto &grid = player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x];
            if (command_wrk != (USE_FLOOR)) {
                break;
            }

            if (grid.o_idx_list.size() < 2) {
                break;
            }

            const auto next_o_idx = fis.floor_list[1];
            while (grid.o_idx_list.front() != next_o_idx) {
                grid.o_idx_list.rotate(*player_ptr->current_floor_ptr);
            }

            rfu.set_flag(SubWindowRedrawingFlag::FLOOR_ITEMS);
            window_stuff(player_ptr);
            const auto &floor = *player_ptr->current_floor_ptr;
            fis.floor_list = scan_floor_items(floor, player_ptr->get_position(), { ScanFloorMode::ITEM_TESTER, ScanFloorMode::ONLY_MARKED }, item_tester);
            if (command_see) {
                screen_load();
                screen_save();
            }

            break;
        }
        case '/': {
            if (command_wrk == (USE_INVEN)) {
                if (!fis.allow_equip) {
                    bell();
                    break;
                }
                command_wrk = (USE_EQUIP);
            } else if (command_wrk == (USE_EQUIP)) {
                if (!fis.allow_inven) {
                    bell();
                    break;
                }
                command_wrk = (USE_INVEN);
            } else if (command_wrk == (USE_FLOOR)) {
                if (fis.allow_inven) {
                    command_wrk = (USE_INVEN);
                } else if (fis.allow_equip) {
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
            if (!fis.allow_floor) {
                bell();
                break;
            }

            if (fis.floor_list.size() == 1) {
                if ((command_wrk == (USE_FLOOR)) || (!carry_query_flag)) {
                    fis.k = -fis.floor_list[0];
                    if (!get_item_allow(player_ptr, fis.k)) {
                        fis.done = true;
                        break;
                    }

                    fis.cp = fis.k;
                    fis.item = true;
                    fis.done = true;
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
                if (!get_tag(player_ptr, &fis.k, fis.which, command_wrk, item_tester)) {
                    bell();
                    break;
                }

                if ((fis.k < INVEN_MAIN_HAND) ? !fis.inven : !fis.equip) {
                    bell();
                    break;
                }

                if (!get_item_okay(player_ptr, fis.k, item_tester)) {
                    bell();
                    break;
                }
            } else {
                if (get_tag_floor(*player_ptr->current_floor_ptr, &fis.k, fis.which, fis.floor_list.data(), fis.floor_list.size())) {
                    fis.k = -fis.floor_list[fis.k];
                } else {
                    bell();
                    break;
                }
            }

            if (!get_item_allow(player_ptr, fis.k)) {
                fis.done = true;
                break;
            }

            fis.cp = fis.k;
            fis.item = true;
            fis.done = true;
            fis.cur_tag = fis.which;
            break;
        }
        case 'w': {
            if (fis.force) {
                fis.cp = INVEN_FORCE;
                fis.item = true;
                fis.done = true;
                break;
            }
        }
            [[fallthrough]];
        default: {
            bool tag_not_found = false;

            if (command_wrk != USE_FLOOR) {
                if (!get_tag(player_ptr, &fis.k, fis.which, command_wrk, item_tester)) {
                    tag_not_found = true;
                } else if ((fis.k < INVEN_MAIN_HAND) ? !fis.inven : !fis.equip) {
                    tag_not_found = true;
                }

                if (!tag_not_found) {
                    fis.cur_tag = fis.which;
                }
            } else {
                if (get_tag_floor(*player_ptr->current_floor_ptr, &fis.k, fis.which, fis.floor_list.data(), fis.floor_list.size())) {
                    fis.k = -fis.floor_list[fis.k];
                    fis.cur_tag = fis.which;
                } else {
                    tag_not_found = true;
                }
            }

            if (tag_not_found) {
                auto which = (char)tolower(fis.which);

                if (command_wrk == (USE_INVEN)) {
                    if (which == '(') {
                        fis.k = fis.i1;
                    } else if (which == ')') {
                        fis.k = fis.i2;
                    } else {
                        fis.k = label_to_inventory(player_ptr, which);
                    }
                } else if (command_wrk == (USE_EQUIP)) {
                    if (which == '(') {
                        fis.k = fis.e1;
                    } else if (which == ')') {
                        fis.k = fis.e2;
                    } else {
                        fis.k = label_to_equipment(player_ptr, which);
                    }
                } else if (command_wrk == USE_FLOOR) {
                    if (which == '(') {
                        fis.k = 0;
                    } else if (which == ')') {
                        fis.k = fis.floor_list.size() - 1;
                    } else {
                        fis.k = islower(which) ? A2I(which) : -1;
                    }
                    if (fis.k < 0 || fis.k >= static_cast<short>(fis.floor_list.size()) || fis.k >= 23) {
                        bell();
                        break;
                    }

                    fis.k = 0 - fis.floor_list[fis.k];
                }
            }

            if ((command_wrk != USE_FLOOR) && !get_item_okay(player_ptr, fis.k, item_tester)) {
                bell();
                break;
            }

            auto ver = tag_not_found && isupper(fis.which);
            if (ver && !verify(player_ptr, _("本当に", "Try"), fis.k)) {
                fis.done = true;
                break;
            }

            if (!get_item_allow(player_ptr, fis.k)) {
                fis.done = true;
                break;
            }

            fis.cp = fis.k;
            fis.item = true;
            fis.done = true;
            break;
        }
        }
    }

    if (command_see) {
        screen_load();
        command_see = false;
    }

    if (fis.toggle) {
        toggle_inventory_equipment();
    }

    rfu.set_flags(flags);
    handle_stuff(player_ptr);
    prt("", 0, 0);
    if (fis.oops && str) {
        msg_print(str);
    }

    if (!fis.item) {
        return false;
    }

    repeat_push(fis.cp);
    if (command_cmd) {
        prev_tag = fis.cur_tag;
    }

    command_cmd = 0;
    *cp = fis.cp;
    return true;
}
