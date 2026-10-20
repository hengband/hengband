#include "core/asking-player.h"
#include "cmd-io/macro-util.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/input-options.h"
#include "io/command-repeater.h"
#include "io/input-context.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h" //!< @todo 相互依存している、後で何とかする.
#include "main/sound-of-music.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <algorithm>
#include <charconv>
#include <climits>
#include <fmt/format.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

/*!
 * @brief 入力欄の開始位置と画面幅に合わせて文字列のバイト上限を求める
 */
static int get_input_length(int len, int x)
{
    const auto start = ((x < 0) || (x >= MAIN_TERM_MIN_COLS)) ? 0 : x;
    return std::clamp(len, 1, MAIN_TERM_MIN_COLS - start);
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
tl::optional<std::string> askfor(int len, std::string_view initial_value, bool numpad_cursor)
{
    /*
     * Text color
     * TERM_YELLOW : Overwrite mode
     * TERM_WHITE : Insert mode
     */
    auto color = TERM_YELLOW;

    auto [x, y] = term_locate();
    if ((x < 0) || (x >= MAIN_TERM_MIN_COLS)) {
        x = 0;
    }

    len = get_input_length(len, x);

    std::string buf(initial_value);
    auto pos = 0;
    while (true) {
        term_erase(x, y);
        term_putstr(x, y, -1, color, buf);
        term_gotoxy(x + pos, y);
        const auto skey = inkey_special(numpad_cursor);
        switch (skey) {
        case SKEY_LEFT:
        case KTRL('b'): {
            color = TERM_WHITE;
            if (0 == pos) {
                break;
            }

            const auto mb_chars = str_find_all_multibyte_chars(buf);
            pos = mb_chars.contains(pos - 2) ? pos - 2 : pos - 1;
            break;
        }
        case SKEY_RIGHT:
        case KTRL('f'): {
            color = TERM_WHITE;
            if (std::cmp_equal(pos, buf.length())) {
                break;
            }

            const auto mb_chars = str_find_all_multibyte_chars(buf);
            pos = mb_chars.contains(pos) ? pos + 2 : pos + 1;
            break;
        }
        case SKEY_HOME:
        case KTRL('a'):
            color = TERM_WHITE;
            pos = 0;
            break;
        case SKEY_END:
        case KTRL('e'):
            color = TERM_WHITE;
            pos = buf.length();
            break;
        case ESCAPE:
            return tl::nullopt;
        case '\n':
        case '\r':
            return buf;
        case '\010': { // Backspace
            color = TERM_WHITE;
            if (pos == 0) {
                break;
            }

            const auto mb_chars = str_find_all_multibyte_chars(buf);
            const auto delete_bytes = mb_chars.contains(pos - 2) ? 2 : 1;
            pos -= delete_bytes;
            buf.erase(pos, delete_bytes);
            break;
        }
        case 0x7F: // Delete
        case KTRL('d'): {
            color = TERM_WHITE;
            if (std::cmp_equal(pos, buf.length())) {
                break;
            }

            const auto mb_chars = str_find_all_multibyte_chars(buf);
            const auto delete_bytes = mb_chars.contains(pos) ? 2 : 1;
            buf.erase(pos, delete_bytes);
            break;
        }
        default: {
            if (skey & SKEY_MASK) {
                break;
            }

            const auto c = static_cast<char>(skey);
            if (color == TERM_YELLOW) {
                buf = "";
                color = TERM_WHITE;
            }

#ifdef JP
            if (iskanji(c)) {
                inkey_base = true;
                const auto next = inkey();
                if (std::cmp_less(buf.length(), len - 1)) {
                    buf.insert(pos++, 1, c);
                    buf.insert(pos++, 1, next);
                    break;
                }
            } else if (isprint(c) || iskana(c)) {
#else
            if (isprint(c)) {
#endif
                if (std::cmp_less(buf.length(), len)) {
                    buf.insert(pos++, 1, c);
                    break;
                }
            }

            bell();
            break;
        }
        }
    }
}

/*!
 * @brief 指定した文脈で文字列の入力を待つ
 * @details 数量入力も同じ編集処理を利用するが、公開する文脈は数量のまま保つ。
 */
static tl::optional<std::string> input_string_with_context(InputContext input_context, int len, std::string_view initial_value, bool numpad_cursor)
{
    msg_erase();
    prt(input_context.prompt, 0, 0);
    if (auto *details = std::get_if<StringInputDetails>(&input_context.details)) {
        details->max_length = get_input_length(len, term_locate().first);
    }

    const InputContextScope context_scope(std::move(input_context));
    const auto ask_result = askfor(len, initial_value, numpad_cursor);
    prt("", 0, 0);
    return ask_result;
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
tl::optional<std::string> input_string(std::string_view prompt, int len, std::string_view initial_value, bool numpad_cursor)
{
    InputContext input_context{ InputKind::STRING, std::string(prompt), StringInputDetails{ len, std::string(initial_value) } };
    return input_string_with_context(std::move(input_context), len, initial_value, numpad_cursor);
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
        window_stuff(player_ptr);
        num_more = 0;
    }

    msg_erase();

    prt(buf, 0, 0);
    if (mode.has_not(UserCheck::NO_HISTORY) && player_ptr->playing) {
        message_add(buf);
        rfu.set_flag(SubWindowRedrawingFlag::MESSAGE);
        window_stuff(player_ptr);
    }

    bool flag = false;
    const InputContextScope input_context(InputKind::CONFIRM, buf);
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

/*!
 * @brief 1文字の入力を受け付ける
 * @param prompt プロンプト ("Command: " などの形)
 * @return 入力した文字。ESCで中断したらnullopt
 */
tl::optional<char> input_command(std::string_view prompt)
{
    return input_command_with_context({ InputKind::COMMAND_CHAR, std::string(prompt), {} });
}

/*!
 * @brief 入力待ちの文脈を指定して、1文字の入力を受け付ける
 * @param input_context 入力待ちの間に公開する文脈。プロンプトは文脈のものを表示する
 * @return 入力した文字。ESCで中断したらnullopt
 * @details 1文字の入力のうち、方向の入力のように種類と詳細を公開したいものに使う。
 */
tl::optional<char> input_command_with_context(InputContext input_context)
{
    msg_erase();
    prt(input_context.prompt, 0, 0);
    char command;
    {
        const InputContextScope context_scope(std::move(input_context));
        if (get_com_no_macros) {
            command = static_cast<char>(inkey_special(false));
        } else {
            command = inkey();
        }
    }

    prt("", 0, 0);
    if (command == ESCAPE) {
        return tl::nullopt;
    }

    return command;
}

/*!
 * @brief 入力待ちの文脈を公開して、キーを1つ待つ
 * @param kind 入力の種類
 * @param prompt 公開するプロンプト (表示は呼び出し元が行う)
 * @return 入力したキー
 * @details 独自に表示した選択肢や確認に答えるキーを、1つずつ読む処理に使う。
 */
char input_key_with_context(InputKind kind, std::string_view prompt)
{
    const InputContextScope input_context(kind, prompt);
    return inkey();
}

/*
 * @brief 数量をユーザ入力する
 * @param max 最大値 (売出し商品の個数等)
 * @param initial_prompt 初期値
 * @details 数値でない値 ('a'等)を入力したら最大数を選択したとみなす.
 */
int input_quantity(int max, std::string_view initial_prompt)
{
    if (command_arg) {
        int amt = command_arg;
        command_arg = 0;
        if (amt > max) {
            amt = max;
        }

        return amt;
    }

    const auto code = repeat_pull();
    if ((max != 1) && code) {
        return std::clamp<int>(*code, 0, max);
    }

    std::string prompt;
    if (!initial_prompt.empty()) {
        prompt = initial_prompt;
    } else {
        prompt = fmt::format(_("いくつですか (1-{}): ", "Quantity (1-{}): "), max);
    }

    msg_erase();
    InputContext input_context{ InputKind::QUANTITY, prompt, QuantityInputDetails{ max, 1 } };
    const auto input_amount = input_string_with_context(std::move(input_context), 6, "1", false);
    if (!input_amount) {
        return 0;
    }

    int amt;
    if (isalpha((*input_amount)[0])) {
        amt = max;
    } else {
        try {
            amt = std::clamp(std::stoi(*input_amount), 0, max);
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

tl::optional<int> input_integer(std::string_view prompt, int min, int max, int initial_value)
{
    std::stringstream ss;
    ss << prompt << "(" << min << "-" << max << "): ";
    auto digit = std::max(std::to_string(min).length(), std::to_string(max).length());
    while (true) {
        const auto input_str = input_string(ss.str(), digit, std::to_string(initial_value), false);
        if (!input_str) {
            return tl::nullopt;
        }

        try {
            auto val = std::stoi(*input_str);
            if ((val < min) || (val > max)) {
                msg_print(_("{}から{}の間で指定して下さい。", "It must be between {} to {}."), min, max);
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
