#include "core/asking-player.h"
#include "cmd-io/macro-util.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/input-options.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h" //!< @todo 相互依存している、後で何とかする.
#include "main/sound-of-music.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <algorithm>
#include <charconv>
#include <climits>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
std::vector<char> clear_buffer(int len, std::string_view initial_value)
{
    std::vector<char> buf(len + 1, '\0');
    initial_value.copy(buf.data(), len);
    return buf;
}
}

/*
 * Get some string input at the cursor location.
 * Assume the buffer is initialized to a default string.
 *
 * The default buffer is in Overwrite mode and displayed in yellow at
 * first.  Normal chars clear the yellow text and append the char in
 * white text.
 *
 * LEFT (^B) and RIGHT (^F) movement keys move the cursor position.
 * If the text is still displayed in yellow (Overwite mode), it will
 * turns into white (Insert mode) when cursor moves.
 *
 * DELETE (^D) deletes a char at the cursor position.
 * BACKSPACE (^H) deletes a char at the left of cursor position.
 * ESCAPE clears the buffer and the window and returns FALSE.
 * RETURN accepts the current buffer contents and returns TRUE.
 */
std::optional<std::string> askfor(int len, std::string_view initial_value, bool numpad_cursor)
{
    /*
     * Text color
     * TERM_YELLOW : Overwrite mode
     * TERM_WHITE : Insert mode
     */
    auto color = TERM_YELLOW;

    int y, x;
    term_locate(&x, &y);
    if (len < 1) {
        len = 1;
    }

    if ((x < 0) || (x >= MAIN_TERM_MIN_COLS)) {
        x = 0;
    }

    if (x + len > MAIN_TERM_MIN_COLS) {
        len = MAIN_TERM_MIN_COLS - x;
    }

    auto buf = clear_buffer(len, initial_value);
    auto pos = 0;
    while (true) {
        term_erase(x, y, len);
        term_putstr(x, y, -1, color, buf.data());
        term_gotoxy(x + pos, y);
        const auto skey = inkey_special(numpad_cursor);
        switch (skey) {
        case SKEY_LEFT:
        case KTRL('b'): {
            auto i = 0;
            color = TERM_WHITE;
            if (0 == pos) {
                break;
            }

            while (true) {
                auto next_pos = i + 1;
#ifdef JP
                if (iskanji(buf[i])) {
                    next_pos++;
                }
#endif
                if (next_pos >= pos) {
                    break;
                }

                i = next_pos;
            }

            pos = i;
            break;
        }
        case SKEY_RIGHT:
        case KTRL('f'):
            color = TERM_WHITE;
            if ('\0' == buf[pos]) {
                break;
            }

#ifdef JP
            if (iskanji(buf[pos])) {
                pos += 2;
            } else {
                pos++;
            }
#else
            pos++;
#endif
            break;
        case SKEY_TOP:
        case KTRL('a'):
            color = TERM_WHITE;
            pos = 0;
            break;
        case SKEY_BOTTOM:
        case KTRL('e'):
            color = TERM_WHITE;
            pos = std::string_view(buf.data()).length();
            break;
        case ESCAPE:
            return std::nullopt;
        case '\n':
        case '\r':
            return buf.data();
        case '\010': {
            auto i = 0;
            color = TERM_WHITE;
            if (pos == 0) {
                break;
            }

            while (true) {
                auto next_pos = i + 1;
#ifdef JP
                if (iskanji(buf[i])) {
                    next_pos++;
                }
#endif
                if (next_pos >= pos) {
                    break;
                }

                i = next_pos;
            }

            pos = i;
        }
            [[fallthrough]];
        case 0x7F:
        case KTRL('d'): {
            color = TERM_WHITE;
            if (buf[pos] == '\0') {
                break;
            }

            auto src = pos + 1;
#ifdef JP
            if (iskanji(buf[pos])) {
                src++;
            }
#endif
            auto dst = pos;
            while ('\0' != (buf[dst++] = buf[src++])) {
                ;
            }

            break;
        }
        default: {
            if (skey & SKEY_MASK) {
                break;
            }

            const auto c = static_cast<char>(skey);
            if (color == TERM_YELLOW) {
                buf = clear_buffer(len, "");
                color = TERM_WHITE;
            }

            const auto str_right_cursor = std::string(buf.data()).substr(pos);
#ifdef JP
            if (iskanji(c)) {
                inkey_base = true;
                char next = inkey();
                if (pos + 1 < len) {
                    buf[pos++] = c;
                    buf[pos++] = next;
                } else {
                    bell();
                }
            } else
#endif
            {
                const auto is_print = _(isprint(c) || iskana(c), isprint(c));
                if (pos < len && is_print) {
                    buf[pos++] = c;
                } else {
                    bell();
                }
            }

            buf[pos] = '\0';
            angband_strcat(buf.data(), str_right_cursor, buf.size());
            break;
        }
        }
    }
}

/*
 * Get a string from the user
 *
 * The "prompt" should take the form "Prompt: "
 *
 * Note that the initial contents of the string is used as
 * the default response, so be sure to "clear" it if needed.
 *
 * We clear the input, and return FALSE, on "ESCAPE".
 */
std::optional<std::string> input_string(std::string_view prompt, int len, std::string_view initial_value, bool numpad_cursor)
{
    msg_print(nullptr);
    prt(prompt, 0, 0);
    const auto ask_result = askfor(len, initial_value, numpad_cursor);
    prt("", 0, 0);
    return ask_result;
}

/*
 * Verify something with the user
 *
 * The "prompt" should take the form "Query? "
 *
 * Note that "[y/n]" is appended to the prompt.
 */
bool input_check(std::string_view prompt)
{
    return input_check_strict(p_ptr, prompt, UserCheck::NONE);
}

/*!
 * @details initializer_list を使うと再帰呼び出し扱いになるので一旦FlagGroup で受ける
 */
bool input_check_strict(PlayerType *player_ptr, std::string_view prompt, UserCheck one_mode)
{
    EnumClassFlagGroup<UserCheck> mode = { one_mode };
    return input_check_strict(player_ptr, prompt, mode);
}

/*
 * Verify something with the user strictly
 *
 * OKAY_CANCEL : force user to answer 'O'kay or 'C'ancel
 * NO_ESCAPE   : don't allow ESCAPE key
 * NO_HISTORY  : no message_add
 * DEFAULT_Y   : accept any key as y, except n and Esc.
 */
bool input_check_strict(PlayerType *player_ptr, std::string_view prompt, EnumClassFlagGroup<UserCheck> mode)
{
    if (!rogue_like_commands) {
        mode.reset(UserCheck::OKAY_CANCEL);
    }

    std::stringstream ss;
    ss << prompt;
    if (mode.has(UserCheck::OKAY_CANCEL)) {
        ss << "[(O)k/(C)ancel]";
    } else if (mode.has(UserCheck::DEFAULT_Y)) {
        ss << "[Y/n]";
    } else {
        ss << "[y/n]";
    }

    const auto buf = ss.str();
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (auto_more) {
        rfu.set_flag(SubWindowRedrawingFlag::MESSAGE);
        handle_stuff(player_ptr);
        num_more = 0;
    }

    msg_print(nullptr);

    prt(buf, 0, 0);
    if (mode.has_not(UserCheck::NO_HISTORY) && player_ptr->playing) {
        message_add(buf);
        rfu.set_flag(SubWindowRedrawingFlag::MESSAGE);
        handle_stuff(player_ptr);
    }

    bool flag = false;
    while (true) {
        int i = inkey();

        if (mode.has_not(UserCheck::NO_ESCAPE)) {
            if (i == ESCAPE) {
                flag = false;
                break;
            }
        }

        if (mode.has(UserCheck::OKAY_CANCEL)) {
            if (i == 'o' || i == 'O') {
                flag = true;
                break;
            } else if (i == 'c' || i == 'C') {
                flag = false;
                break;
            }
        } else {
            if (i == 'y' || i == 'Y') {
                flag = true;
                break;
            } else if (i == 'n' || i == 'N') {
                flag = false;
                break;
            }
        }

        if (mode.has(UserCheck::DEFAULT_Y)) {
            flag = true;
            break;
        }

        bell();
    }

    prt("", 0, 0);
    return flag;
}

/*
 * Prompts for a keypress
 *
 * The "prompt" should take the form "Command: "
 *
 * Returns TRUE unless the character is "Escape"
 */
std::optional<char> input_command(std::string_view prompt, bool z_escape)
{
    msg_print(nullptr);
    prt(prompt, 0, 0);
    char command;
    if (get_com_no_macros) {
        command = static_cast<char>(inkey_special(false));
    } else {
        command = inkey();
    }

    prt("", 0, 0);
    const auto is_z = (command == 'z') || (command == 'Z');
    if ((command == ESCAPE) || (z_escape && is_z)) {
        return std::nullopt;
    }

    return command;
}

/*
 * @brief 数量をユーザ入力する
 * @param max 最大値 (売出し商品の個数等)
 * @param initial_prompt 初期値
 * @details 数値でない値 ('a'等)を入力したら最大数を選択したとみなす.
 */
int input_quantity(int max, std::string_view initial_prompt)
{
    int amt;
    if (command_arg) {
        amt = command_arg;
        command_arg = 0;
        if (amt > max) {
            amt = max;
        }

        return amt;
    }

    short code;
    auto result = repeat_pull(&code);
    amt = code;
    if ((max != 1) && result) {
        if (amt > max) {
            return max;
        }

        if (amt < 0) {
            return 0;
        }

        return amt;
    }

    std::string prompt;
    if (!initial_prompt.empty()) {
        prompt = initial_prompt;
    } else {
        prompt = format(_("いくつですか (1-%d): ", "Quantity (1-%d): "), max);
    }

    msg_print(nullptr);
    const auto input_amount = input_string(prompt, 6, "1", false);
    if (!input_amount) {
        return 0;
    }

    if (isalpha((*input_amount)[0])) {
        amt = max;
    } else {
        try {
            amt = std::clamp<int>(std::stoi(*input_amount), 0, max);
        } catch (const std::exception &) {
            amt = 0;
        }
    }

    if (amt > 0) {
        repeat_push(static_cast<short>(amt));
    }

    return amt;
}

/*
 * Pause for user response
 */
void pause_line(int row)
{
    prt("", row, 0);
    put_str(_("[ 何かキーを押して下さい ]", "[Press any key to continue]"), row, _(26, 23));

    (void)inkey();
    prt("", row, 0);
}

std::optional<int> input_integer(std::string_view prompt, int min, int max, int initial_value)
{
    std::stringstream ss;
    ss << prompt << "(" << min << "-" << max << "): ";
    auto digit = std::max(std::to_string(min).length(), std::to_string(max).length());
    while (true) {
        const auto input_str = input_string(ss.str(), digit, std::to_string(initial_value), false);
        if (!input_str) {
            return std::nullopt;
        }

        try {
            auto val = std::stoi(input_str.value());
            if ((val < min) || (val > max)) {
                msg_format(_("%dから%dの間で指定して下さい。", "It must be between %d to %d."), min, max);
                continue;
            }

            return val;
        } catch (std::invalid_argument const &) {
            msg_print(_("数値を入力して下さい。", "Please input numeric value."));
            continue;
        } catch (std::out_of_range const &) {
            msg_print(_("入力可能な数値の範囲を超えています。", "Input value overflows the maximum number."));
            continue;
        }
    }
}
