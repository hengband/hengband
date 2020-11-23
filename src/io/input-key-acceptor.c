#include "io/input-key-acceptor.h"
#include "cmd-io/macro-util.h"
#include "game-option/map-screen-options.h"
#include "game-option/input-options.h"
#include "io/signal-handlers.h"
#include "term/gameterm.h"
#include "util/string-processor.h"
#include "world/world.h"

bool inkey_base; /* See the "inkey()" function */
bool inkey_xtra; /* See the "inkey()" function */
bool inkey_scan; /* See the "inkey()" function */
bool inkey_flag; /* See the "inkey()" function */

int num_more = 0;

/*
 * This special pointer allows a sequence of keys to be "inserted" into
 * the stream of keys returned by "inkey()".  This key sequence will not
 * trigger any macros, and cannot be bypassed by the Borg.  It is used
 * in Angband to handle "keymaps".
 */
concptr inkey_next = NULL;

/* Save macro trigger string for use in inkey_special() */
static char inkey_macro_trigger_string[1024];

/*
 * Local variable -- we are inside a "macro action"
 *
 * Do not match any macros until "ascii 30" is found.
 */
static bool parse_macro = FALSE;

/*
 * Local variable -- we are inside a "macro trigger"
 *
 * Strip all keypresses until a low ascii value is found.
 */
static bool parse_under = FALSE;

/*
 * Cancel macro action on the queue
 */
static void forget_macro_action(void)
{
    if (!parse_macro)
        return;

    while (TRUE) {
        char ch;
        if (term_inkey(&ch, FALSE, TRUE))
            break;
        if (ch == 0)
            break;
        if (ch == 30)
            break;
    }

    parse_macro = FALSE;
}

/*
 * Helper function called only from "inkey()"
 *
 * This function does almost all of the "macro" processing.
 *
 * We use the "term_key_push()" function to handle "failed" macros, as well
 * as "extra" keys read in while choosing the proper macro, and also to hold
 * the action for the macro, plus a special "ascii 30" character indicating
 * that any macro action in progress is complete.  Embedded macros are thus
 * illegal, unless a macro action includes an explicit "ascii 30" character,
 * which would probably be a massive hack, and might break things.
 *
 * Only 500 (0+1+2+...+29+30) milliseconds may elapse between each key in
 * the macro trigger sequence.  If a key sequence forms the "prefix" of a
 * macro trigger, 500 milliseconds must pass before the key sequence is
 * known not to be that macro trigger.
 */
static char inkey_aux(void)
{
    int k = 0, n, p = 0, w = 0;
    char ch;
    char *buf = inkey_macro_trigger_string;

    num_more = 0;

    if (parse_macro) {
        if (term_inkey(&ch, FALSE, TRUE)) {
            parse_macro = FALSE;
        }
    } else {
        (void)(term_inkey(&ch, TRUE, TRUE));
    }

    if (ch == 30)
        parse_macro = FALSE;

    if (ch == 30)
        return (ch);
    if (parse_macro)
        return (ch);
    if (parse_under)
        return (ch);

    buf[p++] = ch;
    buf[p] = '\0';
    k = macro_find_check(buf);
    if (k < 0)
        return (ch);

    while (TRUE) {
        k = macro_find_maybe(buf);

        if (k < 0)
            break;

        if (0 == term_inkey(&ch, FALSE, TRUE)) {
            buf[p++] = ch;
            buf[p] = '\0';
            w = 0;
        } else {
            w += 1;
            if (w >= 10)
                break;

            term_xtra(TERM_XTRA_DELAY, w);
        }
    }

    k = macro_find_ready(buf);
    if (k < 0) {
        while (p > 0) {
            if (term_key_push(buf[--p]))
                return 0;
        }

        (void)term_inkey(&ch, TRUE, TRUE);
        return (ch);
    }

    concptr pat = macro__pat[k];
    n = strlen(pat);
    while (p > n) {
        if (term_key_push(buf[--p]))
            return 0;
    }

    parse_macro = TRUE;
    if (term_key_push(30))
        return 0;

    concptr act = macro__act[k];

    n = strlen(act);
    while (n > 0) {
        if (term_key_push(act[--n]))
            return 0;
    }

    return 0;
}

/*
 * @brief キー入力を受け付けるメインルーチン / Get a keypress from the user.
 * @param なし
 * return キーを表すコード
 */
char inkey(void)
{
    char ch = 0;
    bool done = FALSE;
    term_type *old = Term;

    if (inkey_next && *inkey_next && !inkey_xtra) {
        ch = *inkey_next++;
        inkey_base = inkey_xtra = inkey_flag = inkey_scan = FALSE;
        return (ch);
    }

    inkey_next = NULL;
    if (inkey_xtra) {
        parse_macro = FALSE;
        parse_under = FALSE;
        term_flush();
    }

    int v;
    (void)term_get_cursor(&v);

    /* Show the cursor if waiting, except sometimes in "command" mode */
    if (!inkey_scan && (!inkey_flag || hilite_player || current_world_ptr->character_icky)) {
        (void)term_set_cursor(1);
    }

    term_activate(angband_term[0]);
    char kk;
    while (!ch) {
        if (!inkey_base && inkey_scan && (0 != term_inkey(&kk, FALSE, FALSE))) {
            break;
        }

        if (!done && (0 != term_inkey(&kk, FALSE, FALSE))) {
            term_activate(old);
            term_fresh();
            term_activate(angband_term[0]);
            current_world_ptr->character_saved = FALSE;

            signal_count = 0;
            done = TRUE;
        }

        if (inkey_base) {
            int w = 0;
            if (!inkey_scan) {
                if (0 == term_inkey(&ch, TRUE, TRUE)) {
                    break;
                }

                break;
            }

            while (TRUE) {
                if (0 == term_inkey(&ch, FALSE, TRUE)) {
                    break;
                } else {
                    w += 10;
                    if (w >= 100)
                        break;

                    term_xtra(TERM_XTRA_DELAY, w);
                }
            }

            break;
        }

        ch = inkey_aux();
        if (ch == 29) {
            ch = 0;
            continue;
        }

        if (parse_under && (ch <= 32)) {
            ch = 0;
            parse_under = FALSE;
        }

        if (ch == 30) {
            ch = 0;
        } else if (ch == 31) {
            ch = 0;
            parse_under = TRUE;
        } else if (parse_under) {
            ch = 0;
        }
    }

    term_activate(old);
    term_set_cursor(v);
    inkey_base = inkey_xtra = inkey_flag = inkey_scan = FALSE;
    return (ch);
}

/*
 * Get a keypress from the user.
 * And interpret special keys as internal code.
 *
 * This function is a Mega-Hack and depend on pref-xxx.prf's.
 * Currently works on Linux(UNIX), Windows, and Macintosh only.
 */
int inkey_special(bool numpad_cursor)
{
    static const struct {
        concptr keyname;
        int keyflag;
    } modifier_key_list[] = {
        { "shift-", SKEY_MOD_SHIFT },
        { "control-", SKEY_MOD_CONTROL },
        { NULL, 0 },
    };

    static const struct {
        bool numpad;
        concptr keyname;
        int keycode;
    } special_key_list[] = {
        { FALSE, "Down]", SKEY_DOWN },
        { FALSE, "Left]", SKEY_LEFT },
        { FALSE, "Right]", SKEY_RIGHT },
        { FALSE, "Up]", SKEY_UP },
        { FALSE, "Page_Up]", SKEY_PGUP },
        { FALSE, "Page_Down]", SKEY_PGDOWN },
        { FALSE, "Home]", SKEY_TOP },
        { FALSE, "End]", SKEY_BOTTOM },
        { TRUE, "KP_Down]", SKEY_DOWN },
        { TRUE, "KP_Left]", SKEY_LEFT },
        { TRUE, "KP_Right]", SKEY_RIGHT },
        { TRUE, "KP_Up]", SKEY_UP },
        { TRUE, "KP_Page_Up]", SKEY_PGUP },
        { TRUE, "KP_Page_Down]", SKEY_PGDOWN },
        { TRUE, "KP_Home]", SKEY_TOP },
        { TRUE, "KP_End]", SKEY_BOTTOM },
        { TRUE, "KP_2]", SKEY_DOWN },
        { TRUE, "KP_4]", SKEY_LEFT },
        { TRUE, "KP_6]", SKEY_RIGHT },
        { TRUE, "KP_8]", SKEY_UP },
        { TRUE, "KP_9]", SKEY_PGUP },
        { TRUE, "KP_3]", SKEY_PGDOWN },
        { TRUE, "KP_7]", SKEY_TOP },
        { TRUE, "KP_1]", SKEY_BOTTOM },
        { FALSE, NULL, 0 },
    };

    static const struct {
        concptr keyname;
        int keycode;
    } gcu_special_key_list[] = {
        { "A", SKEY_UP },
        { "B", SKEY_DOWN },
        { "C", SKEY_RIGHT },
        { "D", SKEY_LEFT },
        { "1~", SKEY_TOP },
        { "4~", SKEY_BOTTOM },
        { "5~", SKEY_PGUP },
        { "6~", SKEY_PGDOWN },
        { NULL, 0 },
    };

    char buf[1024];
    concptr str = buf;
    char key;
    int skey = 0;
    int modifier = 0;
    int i;
    size_t trig_len;

    /*
     * Forget macro trigger ----
     * It's important if we are already expanding macro action
     */
    inkey_macro_trigger_string[0] = '\0';

    key = inkey();
    trig_len = strlen(inkey_macro_trigger_string);
    if (!trig_len)
        return (int)((unsigned char)key);
    if (trig_len == 1 && parse_macro) {
        char c = inkey_macro_trigger_string[0];
        forget_macro_action();
        return (int)((unsigned char)c);
    }

    ascii_to_text(buf, inkey_macro_trigger_string);
    if (prefix(str, "\\[")) {
        str += 2;
        while (TRUE) {
            for (i = 0; modifier_key_list[i].keyname; i++) {
                if (prefix(str, modifier_key_list[i].keyname)) {
                    str += strlen(modifier_key_list[i].keyname);
                    modifier |= modifier_key_list[i].keyflag;
                }
            }

            if (!modifier_key_list[i].keyname)
                break;
        }

        if (!numpad_as_cursorkey)
            numpad_cursor = FALSE;

        for (i = 0; special_key_list[i].keyname; i++) {
            if ((!special_key_list[i].numpad || numpad_cursor) && streq(str, special_key_list[i].keyname)) {
                skey = special_key_list[i].keycode;
                break;
            }
        }

        if (skey) {
            forget_macro_action();
            return (skey | modifier);
        }
    }

    if (prefix(str, "\\e[")) {
        str += 3;

        for (i = 0; gcu_special_key_list[i].keyname; i++) {
            if (streq(str, gcu_special_key_list[i].keyname)) {
                return gcu_special_key_list[i].keycode;
            }
        }
    }

    inkey_macro_trigger_string[0] = '\0';
    return (int)((unsigned char)key);
}
