#include "autopick/autopick-inserter-killer.h"
#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-util.h"
#include "cmd-io/macro-util.h"
#include "game-option/input-options.h"
#include "game-option/keymap-directory-getter.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "term/screen-processor.h"
#include "util/string-processor.h"

/*!
 * @brief Check if this line is expression or not. And update it if it is.
 */
void check_expression_line(text_body_type *tb, int y)
{
    const auto &s = *tb->lines_list[y];
    if ((s[0] == '?' && s[1] == ':') || (tb->states[y] & LSTAT_BYPASS)) {
        tb->dirty_flags |= DIRTY_EXPRESSION;
    }
}

/*!
 * @brief 行を追加可能かチェックする
 * @param tb text_body_type
 * @param add_num 追加する行数
 * @retval true 追加可能
 * @retval false 最大行数を超えるため追加不可
 */
bool can_insert_line(text_body_type *tb, int add_num)
{
    const int count = count_line(tb);
    return !is_greater_autopick_max_line(count + add_num);
}

/*!
 * @brief Insert return code and split the line
 */
bool insert_return_code(text_body_type *tb)
{
    auto num_lines = count_line(tb);
    if (is_greater_autopick_max_line(num_lines)) {
        return false;
    }

    num_lines--;

    for (; tb->cy < num_lines; num_lines--) {
        std::swap(tb->lines_list[num_lines + 1], tb->lines_list[num_lines]);
        std::swap(tb->states[num_lines + 1], tb->states[num_lines]);
    }

    std::string buf;
    int i;
    for (i = 0; ((*tb->lines_list[tb->cy])[i] != '\0') && (i < tb->cx); i++) {
#ifdef JP
        if (iskanji((*tb->lines_list[tb->cy])[i])) {
            buf.push_back((*tb->lines_list[tb->cy])[i++]);
        }
#endif
        buf.push_back((*tb->lines_list[tb->cy])[i]);
    }

    tb->lines_list[tb->cy + 1] = std::make_unique<std::string>(tb->lines_list[tb->cy]->substr(i));
    tb->lines_list[tb->cy] = std::make_unique<std::string>(std::move(buf));
    tb->dirty_flags |= DIRTY_EXPRESSION;
    tb->changed = true;
    return true;
}

/*!
 * @brief Get a trigger key and insert ASCII string for the trigger
 */
bool insert_macro_line(text_body_type *tb)
{
    if (!can_insert_line(tb, 2)) {
        return false;
    }
    int i, n = 0;
    flush();
    inkey_base = true;
    i = inkey();
    char buf[1024]{};
    while (i) {
        buf[n++] = (char)i;
        inkey_base = true;
        inkey_scan = true;
        i = inkey();
    }

    buf[n] = '\0';
    flush();

    char tmp[1024];
    ascii_to_text(tmp, buf, sizeof(tmp));
    if (!tmp[0]) {
        return false;
    }

    tb->cx = 0;
    insert_return_code(tb);
    tb->lines_list[tb->cy] = std::make_unique<std::string>(format("P:%s", tmp));

    i = macro_find_exact(buf);
    if (i == -1) {
        tmp[0] = '\0';
    } else {
        ascii_to_text(tmp, macro_actions[i], sizeof(tmp));
    }

    insert_return_code(tb);
    tb->lines_list[tb->cy] = std::make_unique<std::string>(format("A:%s", tmp));

    return true;
}

/*!
 * @brief Get a command key and insert ASCII string for the key
 */
bool insert_keymap_line(text_body_type *tb)
{
    if (!can_insert_line(tb, 2)) {
        return false;
    }
    BIT_FLAGS mode;
    if (rogue_like_commands) {
        mode = KEYMAP_MODE_ROGUE;
    } else {
        mode = KEYMAP_MODE_ORIG;
    }

    flush();
    char buf[2]{};
    buf[0] = inkey();
    buf[1] = '\0';

    flush();
    char tmp[1024];
    ascii_to_text(tmp, buf, sizeof(tmp));
    if (!tmp[0]) {
        return false;
    }

    tb->cx = 0;
    insert_return_code(tb);
    tb->lines_list[tb->cy] = std::make_unique<std::string>(format("C:%d:%s", mode, tmp));

    concptr act = keymap_act[mode][(byte)(buf[0])];
    if (act) {
        ascii_to_text(tmp, act, sizeof(tmp));
    }

    insert_return_code(tb);
    tb->lines_list[tb->cy] = std::make_unique<std::string>(format("A:%s", tmp));

    return true;
}

/*!
 * @brief Insert single letter at cursor position.
 */
void insert_single_letter(text_body_type *tb, int key)
{
    int i;
    std::string buf;
    for (i = 0; ((*tb->lines_list[tb->cy])[i] != '\0') && (i < tb->cx); i++) {
        buf.push_back((*tb->lines_list[tb->cy])[i]);
    }

#ifdef JP
    if (iskanji(key)) {
        inkey_base = true;
        const auto next = inkey();
        if (buf.size() + 2 < MAX_LINELEN) {
            buf.push_back(static_cast<char>(key));
            buf.push_back(next);
            tb->cx += 2;
        } else {
            bell();
        }
    } else
#endif
    {
        if (buf.size() + 1 < MAX_LINELEN) {
            buf.push_back(static_cast<char>(key));
        }
        tb->cx++;
    }

    for (; ((*tb->lines_list[tb->cy])[i] != '\0') && (buf.size() + 1 < MAX_LINELEN); i++) {
        buf.push_back((*tb->lines_list[tb->cy])[i]);
    }

    tb->lines_list[tb->cy] = std::make_unique<std::string>(std::move(buf));
    const int len = tb->lines_list[tb->cy]->length();
    if (len < tb->cx) {
        tb->cx = len;
    }

    tb->dirty_line = tb->cy;
    check_expression_line(tb, tb->cy);
    tb->changed = true;
}

/*!
 * @brief Kill segment of a line
 */
void kill_line_segment(text_body_type *tb, int y, int x0, int x1, bool whole)
{
    auto &s = *tb->lines_list[y];
    if (whole && (x0 == 0) && (s[x1] == '\0') && tb->lines_list[y + 1]) {
        int i;
        for (i = y; tb->lines_list[i + 1]; i++) {
            std::swap(tb->lines_list[i], tb->lines_list[i + 1]);
        }

        tb->lines_list[i].reset();
        tb->dirty_flags |= DIRTY_EXPRESSION;
        return;
    }

    if (x0 == x1) {
        return;
    }

    std::string buf;
    for (auto x = 0; x < x0; x++) {
        buf.push_back(s[x]);
    }

    for (auto x = x1; s[x]; x++) {
        buf.push_back(s[x]);
    }

    tb->lines_list[y] = std::make_unique<std::string>(std::move(buf));
    check_expression_line(tb, y);
    tb->changed = true;
}
