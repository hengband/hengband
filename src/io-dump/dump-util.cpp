#include "io-dump/dump-util.h"
#include "floor/geometry.h"
#include "game-option/keymap-directory-getter.h"
#include "game-option/special-options.h"
#include "system/terrain/terrain-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

DisplaySymbolsClipboard DisplaySymbolsClipboard::instance{};

DisplaySymbolsClipboard::DisplaySymbolsClipboard()
    : symbol()
    , symbols(DEFAULT_SYMBOLS)
{
}

DisplaySymbolsClipboard &DisplaySymbolsClipboard::get_instance()
{
    return instance;
}

void DisplaySymbolsClipboard::reset_symbols()
{
    this->symbols = DEFAULT_SYMBOLS;
}

void DisplaySymbolsClipboard::set_symbol(const std::map<int, DisplaySymbol> &symbol_configs)
{
    this->symbols = symbol_configs;
}

/*!
 * @brief シンボル変更処理 / Do visual mode command -- Change symbols
 * @param ch
 * @param visual_list_ptr
 * @param height
 * @param width
 * @param attr_ptr_ptr
 * @param char_left_ptr
 * @param cur_attr_ptr
 * @param cur_char_ptr
 * @param need_redraw
 * @return 何かコマンドを入れたらTRUE
 */
bool visual_mode_command(char ch, bool *visual_list_ptr,
    int height, int width,
    TERM_COLOR *attr_top_ptr, byte *char_left_ptr,
    TERM_COLOR *cur_attr_ptr, char *cur_char_ptr, bool *need_redraw)
{
    static TERM_COLOR attr_old = 0;
    static char char_old = 0;
    auto &symbols_cb = DisplaySymbolsClipboard::get_instance();
    switch (ch) {
    case ESCAPE: {
        if (!*visual_list_ptr) {
            return false;
        }

        *cur_attr_ptr = attr_old;
        *cur_char_ptr = char_old;
        *visual_list_ptr = false;
        return true;
    }
    case '\n':
    case '\r': {
        if (!*visual_list_ptr) {
            return false;
        }

        *visual_list_ptr = false;
        *need_redraw = true;
        return true;
    }
    case 'V':
    case 'v': {
        if (*visual_list_ptr) {
            return false;
        }

        *visual_list_ptr = true;
        *attr_top_ptr = std::max<int8_t>(0, (*cur_attr_ptr & 0x7f) - 5);
        *char_left_ptr = std::max<int8_t>(0, *cur_char_ptr - 10);
        attr_old = *cur_attr_ptr;
        char_old = *cur_char_ptr;
        return true;
    }
    case 'C':
    case 'c':
        symbols_cb.symbol = { *cur_attr_ptr, *cur_char_ptr };
        symbols_cb.reset_symbols();
        return true;
    case 'P':
    case 'p': {
        const auto &symbols = symbols_cb.symbol;
        const auto has_character = symbols.has_character();
        if (symbols.color || (!(symbols.character & 0x80) && has_character)) {
            *cur_attr_ptr = symbols.color;
            *attr_top_ptr = std::max<int8_t>(0, (*cur_attr_ptr & 0x7f) - 5);
            if (!*visual_list_ptr) {
                *need_redraw = true;
            }
        }

        if (has_character) {
            /* Set the char */
            *cur_char_ptr = symbols.character;
            *char_left_ptr = std::max<int8_t>(0, *cur_char_ptr - 10);
            if (!*visual_list_ptr) {
                *need_redraw = true;
            }
        }

        return true;
    }
    default: {
        if (!*visual_list_ptr) {
            return false;
        }

        int eff_width;
        int d = get_keymap_dir(ch);
        TERM_COLOR a = (*cur_attr_ptr & 0x7f);
        auto c = *cur_char_ptr;

        if (use_bigtile) {
            eff_width = width / 2;
        } else {
            eff_width = width;
        }

        const auto vec = Direction(d).vec();
        if ((a == 0) && (vec.y < 0)) {
            d = 0;
        }
        if ((c == 0) && (vec.x < 0)) {
            d = 0;
        }
        if ((a == 0x7f) && (vec.y > 0)) {
            d = 0;
        }
        if (((byte)c == 0xff) && (vec.x > 0)) {
            d = 0;
        }

        a += (TERM_COLOR)vec.y;
        c += static_cast<char>(vec.x);
        if (c & 0x80) {
            a |= 0x80;
        }

        *cur_attr_ptr = a;
        *cur_char_ptr = c;
        if ((vec.x < 0) && *char_left_ptr > std::max(0, (unsigned char)c - 10)) {
            (*char_left_ptr)--;
        }
        if ((vec.x > 0) && *char_left_ptr + eff_width < std::min(0xff, (unsigned char)c + 10)) {
            (*char_left_ptr)++;
        }
        if ((vec.y < 0) && *attr_top_ptr > std::max(0, (int)(a & 0x7f) - 4)) {
            (*attr_top_ptr)--;
        }
        if ((vec.y > 0) && *attr_top_ptr + height < std::min(0x7f, (a & 0x7f) + 4)) {
            (*attr_top_ptr)++;
        }

        return true;
    }
    }
}

/*!
 * @brief ダンプ用の一時ファイルを開く
 * @param fff 一時ファイルへの参照ポインタ
 * @param file_name ファイル名
 * @return ファイルを開けたらTRUE、開けなかったらFALSE
 * @details
 */
bool open_temporary_file(FILE **fff, char *file_name)
{
    *fff = angband_fopen_temp(file_name, FILE_NAME_SIZE);
    if (*fff != nullptr) {
        return true;
    }

    msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), file_name);
    msg_print(nullptr);
    return false;
}

/*!
 * @brief モンスター情報リスト中のグループを表示する
 * @param wid 表示文字数幅
 * @param per_page リストの表示行
 * @param grp_idx グループのID配列
 * @param group_text グループ名の文字列配列
 * @param grp_cur 現在の選択ID
 * @param grp_top 現在の選択リスト最上部ID
 */
void display_group_list(int wid, int per_page, const std::vector<short> &grp_idx, const std::vector<std::string> &group_text, int grp_cur, int grp_top)
{
    const int size = std::min(grp_idx.size(), group_text.size());
    for (auto i = 0; i < per_page && (i < size); i++) {
        const auto grp = grp_idx[grp_top + i];
        const auto attr = (grp_top + i == grp_cur) ? TERM_L_BLUE : TERM_WHITE;
        term_erase(0, 6 + i, wid);
        c_put_str(attr, group_text[grp], 6 + i, 0);
    }
}

/*!
 * @brief Display visuals.
 */
void display_visual_list(int col, int row, int height, int width, TERM_COLOR attr_top, byte char_left)
{
    for (int i = 0; i < height; i++) {
        term_erase(col, row + i, width);
    }

    if (use_bigtile) {
        width /= 2;
    }

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            TERM_LEN x = col + j;
            TERM_LEN y = row + i;
            if (use_bigtile) {
                x += j;
            }

            int ia = attr_top + i;
            int ic = char_left + j;
            if (ia > 0x7f || ic > 0xff || ic < ' ' ||
                (!use_graphics && ic > 0x7f)) {
                continue;
            }

            TERM_COLOR a = (TERM_COLOR)ia;
            auto c = static_cast<char>(ic);
            if (c & 0x80) {
                a |= 0x80;
            }

            term_queue_bigchar(x, y, { { a, c }, {} });
        }
    }
}

/*!
 * @brief Place the cursor at the collect position for visual mode
 */
void place_visual_list_cursor(TERM_LEN col, TERM_LEN row, TERM_COLOR a, byte c, TERM_COLOR attr_top, byte char_left)
{
    int i = (a & 0x7f) - attr_top;
    int j = c - char_left;

    TERM_LEN x = col + j;
    TERM_LEN y = row + i;
    if (use_bigtile) {
        x += j;
    }

    term_gotoxy(x, y);
}

/*!
 * @brief Move the cursor in a browser window
 */
void browser_cursor(char ch, int *column, IDX *grp_cur, int grp_cnt, IDX *list_cur, int list_cnt)
{
    int d;
    int col = *column;
    IDX grp = *grp_cur;
    IDX list = *list_cur;
    if (ch == ' ') {
        d = 3;
    } else if (ch == '-') {
        d = 9;
    } else {
        d = get_keymap_dir(ch);
    }

    if (!d) {
        return;
    }

    const auto vec = Direction(d).vec();
    if ((vec.x > 0) && vec.y) {
        int browser_rows;
        const auto &[wid, hgt] = term_get_size();
        browser_rows = hgt - 8;
        if (!col) {
            int old_grp = grp;
            grp += vec.y * (browser_rows - 1);
            if (grp >= grp_cnt) {
                grp = grp_cnt - 1;
            }
            if (grp < 0) {
                grp = 0;
            }
            if (grp != old_grp) {
                list = 0;
            }
        } else {
            list += vec.y * browser_rows;
            if (list >= list_cnt) {
                list = list_cnt - 1;
            }
            if (list < 0) {
                list = 0;
            }
        }

        (*grp_cur) = grp;
        (*list_cur) = list;
        return;
    }

    if (vec.x) {
        col += vec.x;
        if (col < 0) {
            col = 0;
        }
        if (col > 1) {
            col = 1;
        }

        (*column) = col;
        return;
    }

    if (!col) {
        int old_grp = grp;
        grp += (IDX)vec.y;
        if (grp >= grp_cnt) {
            grp = grp_cnt - 1;
        }
        if (grp < 0) {
            grp = 0;
        }
        if (grp != old_grp) {
            list = 0;
        }
    } else {
        list += (IDX)vec.y;
        if (list >= list_cnt) {
            list = list_cnt - 1;
        }
        if (list < 0) {
            list = 0;
        }
    }

    (*grp_cur) = grp;
    (*list_cur) = list;
}
