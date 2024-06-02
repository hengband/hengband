/*!
 * @brief 自動拾いエディタのコマンドを受け付ける
 * @date 2020/04/26
 * @author Hourier
 * @todo 1関数100行以上ある、後で関数を分割すること
 */

#include "autopick/autopick-command-menu.h"
#include "autopick/autopick-menu-data-table.h"
#include "autopick/autopick-util.h"
#include "io/input-key-acceptor.h"
#include "system/angband.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"

/*!
 * @brief 自動拾いエディタの画面を再描画する
 * @param redraw 再描画が必要ならTRUE
 * @param level command_menu_type 構造体におけるメニュー (詳細不明)
 * @param start
 * @param linestr
 * @param menu_key 自動拾いエディタのメニューで入力したキー
 * @param max_len
 */
static uint8_t redraw_edit_command_menu(bool redraw, int level, int start, std::string_view line, uint8_t initial_menu_key, int max_len)
{
    if (!redraw) {
        return initial_menu_key;
    }

    const auto col0 = 5 + level * 7;
    const auto row0 = 1 + level * 3;
    auto row1 = row0 + 1;
    term_putstr(col0, row0, -1, TERM_WHITE, line);

    uint8_t menu_key = 0;
    for (int i = start; menu_data[i].level >= level; i++) {
        std::stringstream com_key;
        if (menu_data[i].level > level) {
            continue;
        }

        if (menu_data[i].com_id == -1) {
            com_key << _("▼", ">");
        } else if (menu_data[i].key != -1) {
            com_key << '^' << static_cast<char>(menu_data[i].key + '@');
        }

        const auto str = format("| %c) %-*s %2s | ", menu_key + 'a', max_len, menu_data[i].name, com_key.str().data());
        term_putstr(col0, row1++, -1, TERM_WHITE, str);
        menu_key++;
    }

    term_putstr(col0, row1, -1, TERM_WHITE, line);
    return menu_key;
}

/*!
 * @brief Display the menu, and get a command
 */
int do_command_menu(int level, int start)
{
    auto max_len = 0;
    std::vector<int> menu_ids;
    for (auto i = start; menu_data[i].level >= level; i++) {
        /* Ignore lower level sub menus */
        if (menu_data[i].level > level) {
            continue;
        }

        const int len = strlen(menu_data[i].name);
        if (len > max_len) {
            max_len = len;
        }

        menu_ids.push_back(i);
    }

    constexpr char num_alphabets = 26;
    while (std::ssize(menu_ids) < num_alphabets) {
        menu_ids.push_back(-1);
    }

    const auto max_menu_wid = max_len + 3 + 3;
    std::stringstream ss;
    ss << "+";
    for (int i = 0; i < max_menu_wid + 2; i++) {
        ss << "-";
    }

    ss << "+";
    auto redraw = true;
    const auto line = ss.str();
    auto menu_key = num_alphabets;
    while (true) {
        menu_key = redraw_edit_command_menu(redraw, level, start, line, menu_key, max_len);
        redraw = false;
        prt(format(_("(a-%c) コマンド:", "(a-%c) Command:"), menu_key + 'a' - 1), 0, 0);
        const auto key = inkey();
        if (key == ESCAPE) {
            return 0;
        }

        int com_id;
        const auto is_alphabet = (key >= 'a') && (key <= 'z');
        if (!is_alphabet) {
            com_id = get_com_id(key);
            if (com_id) {
                return com_id;
            }

            continue;
        }

        const auto menu_id = menu_ids[key - 'a'];
        if (menu_id < 0) {
            continue;
        }

        com_id = menu_data[menu_id].com_id;
        if (com_id > 0) {
            return com_id;
        }

        if (com_id != -1) {
            continue;
        }

        com_id = do_command_menu(level + 1, menu_id + 1);
        if (com_id > 0) {
            return com_id;
        }

        redraw = true;
    }
}
