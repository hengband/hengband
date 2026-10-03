/*!
 * @brief 自動拾いエディタのコマンドを受け付ける
 * @date 2020/04/26
 * @author Hourier
 * @todo 1関数100行以上ある、後で関数を分割すること
 */

#include "autopick/autopick-command-menu.h"
#include "autopick/autopick-menu-data-table.h"
#include "io/input-key-acceptor.h"
#include "system/angband.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include <algorithm>
#include <fmt/format.h>

/*!
 * @brief 自動拾いエディタの画面を再描画する
 * @param redraw 再描画が必要ならTRUE
 * @param menu_depth エディタのメニュー階層
 * @param start
 * @param linestr
 * @param menu_key 自動拾いエディタのメニューで入力したキー
 * @param max_length メニューボックスの横幅 (メニューの最大文字列長に合わせる)
 */
static uint8_t redraw_edit_command_menu(bool redraw, size_t menu_depth, size_t start, std::string_view line, uint8_t initial_menu_key, int max_length)
{
    if (!redraw) {
        return initial_menu_key;
    }

    const auto col0 = 5 + menu_depth * 7;
    const auto row0 = 1 + menu_depth * 3;
    auto row1 = row0 + 1;
    term_putstr(col0, row0, -1, TERM_WHITE, line);

    const auto &menu_data = CommandMenuData::get_instance();
    uint8_t menu_key = 0;
    for (size_t i = start; i < menu_data.size(); i++) {
        const auto &menu_datum = menu_data.get_datum(i);
        if (menu_datum.depth < menu_depth) {
            break;
        }

        if (menu_datum.depth > menu_depth) {
            continue;
        }

        std::stringstream com_key;
        if (!menu_datum.com_id) {
            com_key << _("▼", ">");
        } else if (menu_datum.key) {
            com_key << '^' << static_cast<char>(*menu_datum.key + '@');
        }

        const auto str = fmt::format("| {}) {:<{}} {:2} | ", static_cast<char>(menu_key + 'a'), fmt::bytes(menu_datum.name), max_length, com_key.str());
        term_putstr(col0, row1++, -1, TERM_WHITE, str);
        menu_key++;
    }

    term_putstr(col0, row1, -1, TERM_WHITE, line);
    return menu_key;
}

/*!
 * @brief Display the menu, and get a command
 * @param menu_depth エディタのメニュー階層
 */
EditorCommandId do_command_menu(size_t menu_depth, size_t start)
{
    const auto &menu_data = CommandMenuData::get_instance();
    size_t max_length = 0;
    std::vector<int> menu_ids;
    for (size_t i = start; i < menu_data.size(); i++) {
        const auto &menu_datum = menu_data.get_datum(i);
        if (menu_datum.depth < menu_depth) {
            break;
        }

        if (menu_datum.depth > menu_depth) {
            continue;
        }

        const auto length = menu_datum.name.length();
        if (length > max_length) {
            max_length = length;
        }

        menu_ids.push_back(static_cast<int>(i));
    }

    constexpr char num_alphabets = 26;
    while (std::ssize(menu_ids) < num_alphabets) {
        menu_ids.push_back(-1);
    }

    const auto max_menu_wid = max_length + 3 + 3;
    std::stringstream ss;
    ss << "+";
    for (size_t i = 0; i < max_menu_wid + 2; i++) {
        ss << "-";
    }

    ss << "+";
    auto redraw = true;
    const auto line = ss.str();
    auto menu_key = num_alphabets;
    while (true) {
        menu_key = redraw_edit_command_menu(redraw, menu_depth, start, line, menu_key, max_length);
        redraw = false;
        prt(format(_("(a-%c) コマンド:", "(a-%c) Command:"), menu_key + 'a' - 1), 0, 0);
        const auto key = inkey();
        if (key == ESCAPE) {
            return EditorCommandId::NONE;
        }

        if ((key < 'a') || (key > 'z')) {
            const auto com_id = CommandMenuData::get_instance().get_com_id(key);
            if (com_id > EditorCommandId::NONE) {
                return com_id;
            }

            continue;
        }

        const auto menu_index = key - 'a';
        if (static_cast<size_t>(menu_index) >= menu_ids.size()) {
            continue;
        }

        const auto menu_id = menu_ids[menu_index];
        if (menu_id < 0) {
            continue;
        }

        const auto &com_id_opt = menu_data.get_datum(static_cast<size_t>(menu_id)).com_id;
        if (com_id_opt) {
            return *com_id_opt;
        }

        const auto nested_com_id = do_command_menu(menu_depth + 1, static_cast<size_t>(menu_id) + 1);
        if (nested_com_id != EditorCommandId::NONE) {
            return nested_com_id;
        }

        redraw = true;
    }
}
