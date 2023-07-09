#include "io/input-key-acceptor.h"
#include "cmd-io/macro-util.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/input-options.h"
#include "game-option/map-screen-options.h"
#include "io/signal-handlers.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
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
concptr inkey_next = nullptr;

/* Save macro trigger string for use in inkey_special() */
static char inkey_macro_trigger_string[1024];

/*
 * Local variable -- we are inside a "macro action"
 *
 * Do not match any macros until "ascii 30" is found.
 */
static bool parse_macro = false;

/*
 * Local variable -- we are inside a "macro trigger"
 *
 * Strip all keypresses until a low ascii value is found.
 */
static bool parse_under = false;

/*!
 * @brief 全てのウィンドウの再描画を行う
 * @details
 * カーソル位置がずれるので戻す。
 */
static void all_term_fresh()
{
    TERM_LEN x, y;
    term_activate(angband_terms[0]);
    term_locate(&x, &y);

    RedrawingFlagsUpdater::get_instance().fill_up_sub_flags();
    handle_stuff(p_ptr);

    term_activate(angband_terms[0]);
    term_gotoxy(x, y);
    term_fresh();
}

/*
 * Cancel macro action on the queue
 */
static void forget_macro_action(void)
{
    if (!parse_macro) {
        return;
    }

    while (true) {
        char ch;
        if (term_inkey(&ch, false, true)) {
            break;
        }
        if (ch == 0) {
            break;
        }
        if (ch == 30) {
            break;
        }
    }

    parse_macro = false;
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
        if (term_inkey(&ch, false, true)) {
            parse_macro = false;
        }
    } else {
        (void)(term_inkey(&ch, true, true));
    }

    if (ch == 30) {
        parse_macro = false;
    }

    if (ch == 30) {
        return ch;
    }
    if (parse_macro) {
        return ch;
    }
    if (parse_under) {
        return ch;
    }

    buf[p++] = ch;
    buf[p] = '\0';
    k = macro_find_check(buf);
    if (k < 0) {
        return ch;
    }

    while (true) {
        k = macro_find_maybe(buf);

        if (k < 0) {
            break;
        }

        if (0 == term_inkey(&ch, false, true)) {
            buf[p++] = ch;
            buf[p] = '\0';
            w = 0;
        } else {
            w += 1;
            if (w >= 10) {
                break;
            }

            term_xtra(TERM_XTRA_DELAY, w);
        }
    }

    k = macro_find_ready(buf);
    if (k < 0) {
        while (p > 0) {
            if (term_key_push(buf[--p])) {
                return 0;
            }
        }

        (void)term_inkey(&ch, true, true);
        return ch;
    }

    concptr pat = macro_patterns[k].data();
    n = strlen(pat);
    while (p > n) {
        if (term_key_push(buf[--p])) {
            return 0;
        }
    }

    parse_macro = true;
    if (term_key_push(30)) {
        return 0;
    }

    concptr act = macro_actions[k].data();

    n = strlen(act);
    while (n > 0) {
        if (term_key_push(act[--n])) {
            return 0;
        }
    }

    return 0;
}

/*
 * @brief キー入力を受け付けるメインルーチン / Get a keypress from the user.
 * @param do_all_term_refresh trueであれば強制的にhandle_stuffと再描画を行う。デフォルト false
 * return キーを表すコード
 */
char inkey(bool do_all_term_refresh)
{
    char ch = 0;
    bool done = false;
    term_type *old = game_term;

    if (inkey_next && *inkey_next && !inkey_xtra) {
        ch = *inkey_next++;
        inkey_base = inkey_xtra = inkey_flag = inkey_scan = false;
        return ch;
    }

    inkey_next = nullptr;
    if (inkey_xtra) {
        parse_macro = false;
        parse_under = false;
        term_flush();
    }

    int v;
    (void)term_get_cursor(&v);

    /* Show the cursor if waiting, except sometimes in "command" mode */
    if (!inkey_scan && (!inkey_flag || hilite_player || w_ptr->character_icky_depth > 0)) {
        (void)term_set_cursor(1);
    }

    term_activate(angband_terms[0]);
    char kk;
    while (!ch) {
        if (!inkey_base && inkey_scan && (0 != term_inkey(&kk, false, false))) {
            break;
        }

        if (!done && (0 != term_inkey(&kk, false, false))) {
            start_term_fresh();
            if (do_all_term_refresh) {
                all_term_fresh();
            } else {
                term_fresh();
            }
            w_ptr->character_saved = false;

            signal_count = 0;
            done = true;
        }

        if (inkey_base) {
            int w = 0;
            if (!inkey_scan) {
                if (0 == term_inkey(&ch, true, true)) {
                    break;
                }

                break;
            }

            while (true) {
                if (0 == term_inkey(&ch, false, true)) {
                    break;
                } else {
                    w += 10;
                    if (w >= 100) {
                        break;
                    }

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
            parse_under = false;
        }

        if (ch == 30) {
            ch = 0;
        } else if (ch == 31) {
            ch = 0;
            parse_under = true;
        } else if (parse_under) {
            ch = 0;
        }
    }

    term_activate(old);
    term_set_cursor(v);
    inkey_base = inkey_xtra = inkey_flag = inkey_scan = false;
    return ch;
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
        { nullptr, 0 },
    };

    static const struct {
        bool numpad;
        concptr keyname;
        int keycode;
    } special_key_list[] = {
        { false, "Down]", SKEY_DOWN },
        { false, "Left]", SKEY_LEFT },
        { false, "Right]", SKEY_RIGHT },
        { false, "Up]", SKEY_UP },
        { false, "Page_Up]", SKEY_PGUP },
        { false, "Page_Down]", SKEY_PGDOWN },
        { false, "Home]", SKEY_TOP },
        { false, "End]", SKEY_BOTTOM },
        { true, "KP_Down]", SKEY_DOWN },
        { true, "KP_Left]", SKEY_LEFT },
        { true, "KP_Right]", SKEY_RIGHT },
        { true, "KP_Up]", SKEY_UP },
        { true, "KP_Page_Up]", SKEY_PGUP },
        { true, "KP_Page_Down]", SKEY_PGDOWN },
        { true, "KP_Home]", SKEY_TOP },
        { true, "KP_End]", SKEY_BOTTOM },
        { true, "KP_2]", SKEY_DOWN },
        { true, "KP_4]", SKEY_LEFT },
        { true, "KP_6]", SKEY_RIGHT },
        { true, "KP_8]", SKEY_UP },
        { true, "KP_9]", SKEY_PGUP },
        { true, "KP_3]", SKEY_PGDOWN },
        { true, "KP_7]", SKEY_TOP },
        { true, "KP_1]", SKEY_BOTTOM },
        { false, nullptr, 0 },
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
        { nullptr, 0 },
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
    if (!trig_len) {
        return (int)((unsigned char)key);
    }
    if (trig_len == 1 && parse_macro) {
        char c = inkey_macro_trigger_string[0];
        forget_macro_action();
        return (int)((unsigned char)c);
    }

    ascii_to_text(buf, inkey_macro_trigger_string, sizeof(buf));
    if (prefix(str, "\\[")) {
        str += 2;
        while (true) {
            for (i = 0; modifier_key_list[i].keyname; i++) {
                if (prefix(str, modifier_key_list[i].keyname)) {
                    str += strlen(modifier_key_list[i].keyname);
                    modifier |= modifier_key_list[i].keyflag;
                }
            }

            if (!modifier_key_list[i].keyname) {
                break;
            }
        }

        if (!numpad_as_cursorkey) {
            numpad_cursor = false;
        }

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

/*!
 * @brief 全てのウィンドウの描画を止める
 */
void stop_term_fresh(void)
{
    for (auto &angband_term : angband_terms) {
        if (angband_term != nullptr) {
            angband_term->never_fresh = true;
        }
    }
}

/*!
 * @brief 全てのウィンドウの描画を再開する
 */
void start_term_fresh(void)
{
    for (auto &angband_term : angband_terms) {
        if (angband_term != nullptr) {
            angband_term->never_fresh = false;
        }
    }
}

/*!
 * @brief マクロ実行中かの判定関数
 * @return 実行中であればtrue
 */
bool macro_running(void)
{
    /* マクロ展開中のみ詳細に判定する */
    if (parse_macro) {
        int diff = angband_terms[0]->key_head - angband_terms[0]->key_tail;

        /* 最終入力を展開した直後はdiff==1となる */
        if (diff != 1) {
            return true;
        }

        /* 最終入力の処理中はまだtrueを返す */
        if (inkey_next && *inkey_next && !inkey_xtra) {
            return true;
        }

        return false;
    }

    return false;
}
