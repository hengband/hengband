#include "view/display-messages.h"
#include "core/output-updater.h"
#include "core/window-redrawer.h"
#include "game-option/cheat-options.h"
#include "game-option/input-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/option-flags.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "term/gameterm.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include "world/world.h"

/* The next "free" index to use */
u32b message__next;

/* The index of the oldest message (none yet) */
u32b message__last;

/* The next "free" offset */
u32b message__head;

/* The offset to the oldest used char (none yet) */
u32b message__tail;

/* The array of offsets, by index [MESSAGE_MAX] */
u32b *message__ptr;

/* The array of chars, by offset [MESSAGE_BUF] */
char *message__buf;

/* Used in msg_print() for "buffering" */
bool msg_flag;

COMMAND_CODE now_message;

/*!
 * @brief 保存中の過去ゲームメッセージの数を返す。 / How many messages are "available"?
 * @return 残っているメッセージの数
 */
s32b message_num(void)
{
    int n;
    int last = message__last;
    int next = message__next;

    if (next < last)
        next += MESSAGE_MAX;

    n = (next - last);
    return (n);
}

/*!
 * @brief 過去のゲームメッセージを返す。 / Recall the "text" of a saved message
 * @params age メッセージの世代
 * @return メッセージの文字列ポインタ
 */
concptr message_str(int age)
{
    if ((age < 0) || (age >= message_num()))
        return ("");

    s32b x = (message__next + MESSAGE_MAX - (age + 1)) % MESSAGE_MAX;
    s32b o = message__ptr[x];
    concptr s = &message__buf[o];
    return (s);
}

/*!
 * @brief ゲームメッセージをログに追加する。 / Add a new message, with great efficiency
 * @params str 保存したいメッセージ
 * @return なし
 */
void message_add(concptr str)
{
    u32b i;
    int x, m;
    char u[4096];
    char splitted1[81];
    concptr splitted2;

    if (!str)
        return;

    u32b n = strlen(str);
    if (n >= MESSAGE_BUF / 4)
        return;

    if (n > 80) {
#ifdef JP
        concptr t = str;
        for (n = 0; n < 80; n++, t++) {
            if (iskanji(*t)) {
                t++;
                n++;
            }
        }

        /* 最後の文字が漢字半分 */
        if (n == 81)
            n = 79;
#else
        for (n = 80; n > 60; n--)
            if (str[n] == ' ')
                break;
        if (n == 60)
            n = 80;
#endif
        splitted2 = str + n;
        strncpy(splitted1, str, n);
        splitted1[n] = '\0';
        str = splitted1;
    } else {
        splitted2 = NULL;
    }

    m = message_num();
    int k = m / 4;
    if (k > MESSAGE_MAX / 32)
        k = MESSAGE_MAX / 32;
    for (i = message__next; m; m--) {
        int j = 1;
        char buf[1024];
        char *t;
        concptr old;
        if (i-- == 0)
            i = MESSAGE_MAX - 1;

        old = &message__buf[message__ptr[i]];
        if (!old)
            continue;

        strcpy(buf, old);
#ifdef JP
        for (t = buf; *t && (*t != '<' || (*(t + 1) != 'x')); t++)
            if (iskanji(*t))
                t++;
#else
        for (t = buf; *t && (*t != '<'); t++)
            ;
#endif
        if (*t) {
            if (strlen(buf) < A_MAX)
                break;

            *(t - 1) = '\0';
            j = atoi(t + 2);
        }

        if (streq(buf, str) && (j < 1000)) {
            j++;
            message__next = i;
            str = u;
            sprintf(u, "%s <x%d>", buf, j);
            n = strlen(str);
            if (!now_message)
                now_message++;
        } else {
            /*流れた行の数を数えておく */
            num_more++;
            now_message++;
        }

        break;
    }

    for (i = message__next; k; k--) {
        int q;
        concptr old;

        if (i-- == 0)
            i = MESSAGE_MAX - 1;

        if (i == message__last)
            break;

        q = (message__head + MESSAGE_BUF - message__ptr[i]) % MESSAGE_BUF;

        if (q > MESSAGE_BUF / 2)
            continue;

        old = &message__buf[message__ptr[i]];
        if (!streq(old, str))
            continue;

        x = message__next++;
        if (message__next == MESSAGE_MAX)
            message__next = 0;
        if (message__next == message__last)
            message__last++;
        if (message__last == MESSAGE_MAX)
            message__last = 0;

        message__ptr[x] = message__ptr[i];
        if (splitted2 != NULL) {
            message_add(splitted2);
        }

        return;
    }

    if (message__head + n + 1 >= MESSAGE_BUF) {
        for (i = message__last; TRUE; i++) {
            if (i == MESSAGE_MAX)
                i = 0;
            if (i == message__next)
                break;
            if (message__ptr[i] >= message__head) {
                message__last = i + 1;
            }
        }

        if (message__tail >= message__head)
            message__tail = 0;

        message__head = 0;
    }

    if (message__head + n + 1 > message__tail) {
        message__tail = message__head + n + 1;
        while (message__buf[message__tail - 1])
            message__tail++;

        for (i = message__last; TRUE; i++) {
            if (i == MESSAGE_MAX)
                i = 0;
            if (i == message__next)
                break;
            if ((message__ptr[i] >= message__head) && (message__ptr[i] < message__tail)) {
                message__last = i + 1;
            }
        }
    }

    x = message__next++;
    if (message__next == MESSAGE_MAX)
        message__next = 0;
    if (message__next == message__last)
        message__last++;
    if (message__last == MESSAGE_MAX)
        message__last = 0;

    message__ptr[x] = message__head;
    for (i = 0; i < n; i++) {
        message__buf[message__head + i] = str[i];
    }

    message__buf[message__head + i] = '\0';
    message__head += n + 1;

    if (splitted2 != NULL) {
        message_add(splitted2);
    }
}

/*
 * Hack -- flush
 */
static void msg_flush(player_type *player_ptr, int x)
{
    byte a = TERM_L_BLUE;
    bool nagasu = FALSE;
    if ((auto_more && !player_ptr->now_damaged) || num_more < 0) {
        int i;
        for (i = 0; i < 8; i++) {
            if (angband_term[i] && (window_flag[i] & PW_MESSAGE))
                break;
        }
        if (i < 8) {
            if (num_more < angband_term[i]->hgt)
                nagasu = TRUE;
        } else {
            nagasu = TRUE;
        }
    }

    player_ptr->now_damaged = FALSE;
    if (!player_ptr->playing || !nagasu) {
        term_putstr(x, 0, -1, a, _("-続く-", "-more-"));
        while (TRUE) {
            int cmd = inkey();
            if (cmd == ESCAPE) {
                /* auto_moreのとき、全て流す */
                num_more = -9999;
                break;
            } else if (cmd == ' ') {
                /* 1画面だけ流す */
                num_more = 0;
                break;
            } else if ((cmd == '\n') || (cmd == '\r')) {
                /* 1行だけ流す */
                num_more--;
                break;
            }

            if (quick_messages)
                break;
            bell();
        }
    }

    term_erase(0, 0, 255);
}

void msg_erase(void) { msg_print(NULL); }

/*
 * todo ここのp_ptrを削除するのは破滅的に作業が増えるので保留
 * Output a message to the top line of the screen.
 *
 * Break long messages into multiple pieces (40-72 chars).
 *
 * Allow multiple short messages to "share" the top line.
 *
 * Prompt the user to make sure he has a chance to read them.
 *
 * These messages are memorized for later reference (see above).
 *
 * We could do "term_fresh()" to provide "flicker" if needed.
 *
 * The global "msg_flag" variable can be cleared to tell us to
 * "erase" any "pending" messages still on the screen.
 *
 * Note that we must be very careful about using the
 * "msg_print()" functions without explicitly calling the special
 * "msg_print(NULL)" function, since this may result in the loss
 * of information if the screen is cleared, or if anything is
 * displayed on the top line.
 *
 * Note that "msg_print(NULL)" will clear the top line
 * even if no messages are pending.  This is probably a hack.
 */
void msg_print(concptr msg)
{
    static int p = 0;
    char *t;
    char buf[1024];

    if (current_world_ptr->timewalk_m_idx)
        return;

    if (!msg_flag) {
        term_erase(0, 0, 255);
        p = 0;
    }

    int n = (msg ? strlen(msg) : 0);
    if (p && (!msg || ((p + n) > 72))) {
        msg_flush(p_ptr, p);
        msg_flag = FALSE;
        p = 0;
    }

    if (!msg)
        return;
    if (n > 1000)
        return;

    if (!cheat_turn) {
        strcpy(buf, msg);
    } else {
        sprintf(buf, ("T:%d - %s"), (int)current_world_ptr->game_turn, msg);
    }

    n = strlen(buf);
    if (current_world_ptr->character_generated)
        message_add(buf);

    t = buf;
    while (n > 72) {
        int check, split = 72;
#ifdef JP
        bool k_flag = FALSE;
        int wordlen = 0;
        for (check = 0; check < 72; check++) {
            if (k_flag) {
                k_flag = FALSE;
                continue;
            }

            if (iskanji(t[check])) {
                k_flag = TRUE;
                split = check;
            } else if (t[check] == ' ') {
                split = check;
                wordlen = 0;
            } else {
                wordlen++;
                if (wordlen > 20)
                    split = check;
            }
        }

#else
        for (check = 40; check < 72; check++) {
            if (t[check] == ' ')
                split = check;
        }
#endif

        char oops = t[split];
        t[split] = '\0';
        term_putstr(0, 0, split, TERM_WHITE, t);
        msg_flush(p_ptr, split + 1);
        t[split] = oops;
        t[--split] = ' ';
        t += split;
        n -= split;
    }

    term_putstr(p, 0, n, TERM_WHITE, t);
    p_ptr->window |= (PW_MESSAGE);
    update_output(p_ptr);

    msg_flag = TRUE;
#ifdef JP
    p += n;
#else
    p += n + 1;
#endif

    if (fresh_message)
        term_fresh();
}

/*
 * Display a formatted message, using "vstrnfmt()" and "msg_print()".
 */
void msg_format(concptr fmt, ...)
{
    va_list vp;
    char buf[1024];
    va_start(vp, fmt);
    (void)vstrnfmt(buf, 1024, fmt, vp);
    va_end(vp);
    msg_print(buf);
}
