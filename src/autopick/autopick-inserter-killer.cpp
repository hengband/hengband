#include "autopick/autopick-inserter-killer.h"
#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-editor-util.h"
#include "autopick/autopick-util.h"
#include "cmd-io/macro-util.h"
#include "game-option/input-options.h"
#include "game-option/keymap-directory-getter.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "io/macro-configurations-store.h"
#include "main/sound-of-music.h"
#include "term/screen-processor.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include <algorithm>
#include <fmt/format.h>

/*!
 * @brief Check if this line is expression or not. And update it if it is.
 */
void check_expression_line(AutopickTextBody *tb, int y)
{
    const auto &s = tb->lines[y].line;
    if ((s[0] == '?' && s[1] == ':') || (tb->lines[y].state.has(AutopickEntryStatus::BYPASS))) {
        tb->dirty_flags |= DIRTY_EXPRESSION;
    }
}

/*!
 * @brief Insert return code and split the line
 */
void insert_return_code(AutopickTextBody *tb)
{
    // 2バイト文字の途中では分けず、その文字の後ろで分ける
    auto &line = tb->lines[tb->cy].line;
    auto split_pos = std::min(tb->cx, static_cast<int>(line.length()));
    if (is_second_byte_of_kanji(line, split_pos)) {
        split_pos++;
    }

    // 挿入すると line の参照が無効になるので、先に分ける
    auto rest = line.substr(split_pos);
    line.erase(split_pos);
    tb->lines.emplace(tb->lines.begin() + tb->cy + 1)->line = std::move(rest);
    tb->dirty_flags |= DIRTY_EXPRESSION;
    tb->changed = true;
}

/*!
 * @brief Get a trigger key and insert ASCII string for the trigger
 */
bool insert_macro_line(AutopickTextBody *tb)
{
    const auto trigger = inkey_macro_trigger();
    char tmp[1024];
    ascii_to_text(tmp, trigger, sizeof(tmp));
    if (!tmp[0]) {
        return false;
    }

    tb->cx = 0;
    insert_return_code(tb);
    tb->lines[tb->cy].line = fmt::format("P:{}", tmp);

    const auto i = macro_find_exact(trigger.data());
    if (i == -1) {
        tmp[0] = '\0';
    } else {
        ascii_to_text(tmp, macro_actions[i], sizeof(tmp));
    }

    insert_return_code(tb);
    tb->lines[tb->cy].line = fmt::format("A:{}", tmp);
    return true;
}

/*!
 * @brief Get a command key and insert ASCII string for the key
 */
bool insert_keymap_line(AutopickTextBody *tb)
{
    const auto mode = rogue_like_commands ? KeymapMode::ROGUE : KeymapMode::ORIGINAL;
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
    tb->lines[tb->cy].line = fmt::format("C:{}:{}", enum2i(mode), tmp);

    const auto &act = keymap_actions_map.at(mode).at(static_cast<uint8_t>(buf[0]));
    if (act) {
        ascii_to_text(tmp, *act, sizeof(tmp));
    }

    insert_return_code(tb);
    tb->lines[tb->cy].line = fmt::format("A:{}", tmp);
    return true;
}

/*!
 * @brief Insert single letter at cursor position.
 */
void insert_single_letter(AutopickTextBody *tb, int key)
{
    const auto &line = tb->lines[tb->cy].line;
    const auto head_len = std::min<size_t>(tb->cx, line.length());
    auto buf = line.substr(0, head_len);

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

    // 行の上限を超える分は、2バイト文字を分断しないように切り詰める
    buf.append(line, head_len);
    tb->lines[tb->cy].line = str_substr(std::move(buf), 0, MAX_LINELEN - 1);
    const int len = tb->lines[tb->cy].line.length();
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
void kill_line_segment(AutopickTextBody *tb, int y, int x0, int x1, bool whole)
{
    auto &s = tb->lines[y].line;
    if (whole && (x0 == 0) && (s[x1] == '\0') && !tb->is_last_line(y)) {
        tb->lines.erase(tb->lines.begin() + y);
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

    tb->lines[y].line = std::move(buf);
    check_expression_line(tb, y);
    tb->changed = true;
}
