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
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

#include <climits>
#include <algorithm>

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
bool askfor_aux(char *buf, int len, bool numpad_cursor)
{
    /*
     * Text color
     * TERM_YELLOW : Overwrite mode
     * TERM_WHITE : Insert mode
     */
    byte color = TERM_YELLOW;

    int y, x;
    term_locate(&x, &y);
    if (len < 1)
        len = 1;
    if ((x < 0) || (x >= 80))
        x = 0;
    if (x + len > 80)
        len = 80 - x;

    buf[len] = '\0';

    int pos = 0;
    while (true) {
        term_erase(x, y, len);
        term_putstr(x, y, -1, color, buf);

        term_gotoxy(x + pos, y);
        int skey = inkey_special(numpad_cursor);

        switch (skey) {
        case SKEY_LEFT:
        case KTRL('b'): {
            int i = 0;
            color = TERM_WHITE;

            if (0 == pos)
                break;
            while (true) {
                int next_pos = i + 1;
#ifdef JP
                if (iskanji(buf[i]))
                    next_pos++;
#endif
                if (next_pos >= pos)
                    break;

                i = next_pos;
            }

            pos = i;
            break;
        }

        case SKEY_RIGHT:
        case KTRL('f'):
            color = TERM_WHITE;
            if ('\0' == buf[pos])
                break;

#ifdef JP
            if (iskanji(buf[pos]))
                pos += 2;
            else
                pos++;
#else
            pos++;
#endif
            break;

        case ESCAPE:
            buf[0] = '\0';
            return false;

        case '\n':
        case '\r':
            return true;

        case '\010': {
            int i = 0;
            color = TERM_WHITE;
            if (0 == pos)
                break;
            while (true) {
                int next_pos = i + 1;
#ifdef JP
                if (iskanji(buf[i]))
                    next_pos++;
#endif
                if (next_pos >= pos)
                    break;

                i = next_pos;
            }

            pos = i;
        }
            /* Fall through */

        case 0x7F:
        case KTRL('d'): {
            color = TERM_WHITE;
            if ('\0' == buf[pos])
                break;
            int src = pos + 1;
#ifdef JP
            if (iskanji(buf[pos]))
                src++;
#endif

            int dst = pos;
            while ('\0' != (buf[dst++] = buf[src++]))
                ;
            break;
        }

        default: {
            char tmp[100];
            if (skey & SKEY_MASK)
                break;
            char c = (char)skey;

            if (color == TERM_YELLOW) {
                buf[0] = '\0';
                color = TERM_WHITE;
            }

            strcpy(tmp, buf + pos);
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
#ifdef JP
                if (pos < len && (isprint(c) || iskana(c)))
#else
                if (pos < len && isprint(c))
#endif
                {
                    buf[pos++] = c;
                } else {
                    bell();
                }
            }

            buf[pos] = '\0';
            angband_strcat(buf, tmp, len + 1);

            break;
        }
        }
    }
}

/*
 * Get some string input at the cursor location.
 *
 * Allow to use numpad keys as cursor keys.
 */
bool askfor(char *buf, int len) { return askfor_aux(buf, len, true); }

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
bool get_string(concptr prompt, char *buf, int len)
{
    bool res;
    msg_print(nullptr);
    prt(prompt, 0, 0);
    res = askfor(buf, len);
    prt("", 0, 0);
    return (res);
}

/*
 * Verify something with the user
 *
 * The "prompt" should take the form "Query? "
 *
 * Note that "[y/n]" is appended to the prompt.
 */
bool get_check(concptr prompt) { return get_check_strict(p_ptr, prompt, 0); }

/*
 * Verify something with the user strictly
 *
 * mode & CHECK_OKAY_CANCEL : force user to answer 'O'kay or 'C'ancel
 * mode & CHECK_NO_ESCAPE   : don't allow ESCAPE key
 * mode & CHECK_NO_HISTORY  : no message_add
 * mode & CHECK_DEFAULT_Y   : accept any key as y, except n and Esc.
 */
bool get_check_strict(player_type *player_ptr, concptr prompt, BIT_FLAGS mode)
{
    char buf[80];
    if (!rogue_like_commands)
        mode &= ~CHECK_OKAY_CANCEL;

    if (mode & CHECK_OKAY_CANCEL) {
        angband_strcpy(buf, prompt, sizeof(buf) - 15);
        strcat(buf, "[(O)k/(C)ancel]");
    } else if (mode & CHECK_DEFAULT_Y) {
        angband_strcpy(buf, prompt, sizeof(buf) - 5);
        strcat(buf, "[Y/n]");
    } else {
        angband_strcpy(buf, prompt, sizeof(buf) - 5);
        strcat(buf, "[y/n]");
    }

    if (auto_more) {
        player_ptr->window_flags |= PW_MESSAGE;
        handle_stuff(player_ptr);
        num_more = 0;
    }

    msg_print(nullptr);

    prt(buf, 0, 0);
    if (!(mode & CHECK_NO_HISTORY) && player_ptr->playing) {
        message_add(buf);
        player_ptr->window_flags |= (PW_MESSAGE);
        handle_stuff(player_ptr);
    }

    bool flag = false;
    while (true) {
        int i = inkey();

        if (!(mode & CHECK_NO_ESCAPE)) {
            if (i == ESCAPE) {
                flag = false;
                break;
            }
        }

        if (mode & CHECK_OKAY_CANCEL) {
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

        if (mode & CHECK_DEFAULT_Y) {
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
bool get_com(concptr prompt, char *command, bool z_escape)
{
    msg_print(nullptr);
    prt(prompt, 0, 0);
    if (get_com_no_macros)
        *command = (char)inkey_special(false);
    else
        *command = inkey();

    prt("", 0, 0);
    if (*command == ESCAPE)
        return false;
    if (z_escape && ((*command == 'z') || (*command == 'Z')))
        return false;

    return true;
}

/*
 * Request a "quantity" from the user
 *
 * Hack -- allow "command_arg" to specify a quantity
 */
QUANTITY get_quantity(concptr prompt, QUANTITY max)
{
    // FIXME : QUANTITY、COMMAND_CODE、その他の型サイズがまちまちな変数とのやり取りが多数ある。この処理での数の入力を0からSHRT_MAXに制限することで不整合の発生を回避する。
    max = std::clamp<QUANTITY>(max, 0, SHRT_MAX);

    bool res;
    char tmp[80];
    char buf[80];

    QUANTITY amt;
    if (command_arg) {
        amt = command_arg;
        command_arg = 0;
        if (amt > max)
            amt = max;

        return (amt);
    }

    COMMAND_CODE code;
    bool result = repeat_pull(&code);
    amt = (QUANTITY)code;
    if ((max != 1) && result) {
        if (amt > max)
            amt = max;
        if (amt < 0)
            amt = 0;

        return (amt);
    }

    if (!prompt) {
        sprintf(tmp, _("いくつですか (1-%d): ", "Quantity (1-%d): "), max);
        prompt = tmp;
    }

    msg_print(nullptr);
    prt(prompt, 0, 0);
    amt = 1;
    sprintf(buf, "%d", amt);

    /*
     * Ask for a quantity
     * Don't allow to use numpad as cursor key.
     */
    res = askfor_aux(buf, 6, false);

    prt("", 0, 0);
    if (!res)
        return 0;

   if (isalpha(buf[0]))
        amt = max;
    else
        amt = std::clamp<int>(atoi(buf), 0, max);

   if (amt)
        repeat_push((COMMAND_CODE)amt);

    return (amt);
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
