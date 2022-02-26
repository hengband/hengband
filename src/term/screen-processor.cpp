#include "term/screen-processor.h"
#include "io/input-key-acceptor.h"
#include "term/term-color-types.h"
#include "view/display-messages.h"
#include "world/world.h"

/*
 * Hack -- prevent "accidents" in "screen_save()" or "screen_load()"
 */
static int screen_depth = 0;

/*
 * Move the cursor
 */
void move_cursor(int row, int col)
{
    term_gotoxy(col, row);
}

/*
 * Flush all input chars.  Actually, remember the flush,
 * and do a "special flush" before the next "inkey()".
 *
 * This is not only more efficient, but also necessary to make sure
 * that various "inkey()" codes are not "lost" along the way.
 */
void flush(void)
{
    inkey_xtra = true;
}

/*
 * Save the screen, and increase the "icky" depth.
 *
 * This function must match exactly one call to "screen_load()".
 */
void screen_save()
{
    msg_print(nullptr);

    term_save();

    w_ptr->character_icky_depth++;
    screen_depth++;
}

/*
 * Load the screen, and decrease the "icky" depth.
 *
 * This function must match exactly one call to "screen_save()".
 */
void screen_load(ScreenLoadOptType opt)
{
    msg_print(nullptr);

    switch (opt) {
    case ScreenLoadOptType::ONE:
        term_load(false);
        w_ptr->character_icky_depth--;
        screen_depth--;
        break;

    case ScreenLoadOptType::ALL:
        term_load(true);
        w_ptr->character_icky_depth -= static_cast<byte>(screen_depth);
        screen_depth = 0;
        break;

    default:
        break;
    }
}

/*
 * Display a string on the screen using an attribute.
 *
 * At the given location, using the given attribute, if allowed,
 * add the given string.  Do not clear the line.
 */
void c_put_str(TERM_COLOR attr, concptr str, TERM_LEN row, TERM_LEN col)
{
    term_putstr(col, row, -1, attr, str);
}

/*
 * As above, but in "white"
 */
void put_str(concptr str, TERM_LEN row, TERM_LEN col)
{
    term_putstr(col, row, -1, TERM_WHITE, str);
}

/*
 * Display a string on the screen using an attribute, and clear
 * to the end of the line.
 */
void c_prt(TERM_COLOR attr, concptr str, TERM_LEN row, TERM_LEN col)
{
    term_erase(col, row, 255);
    term_addstr(-1, attr, str);
}

/*
 * As above, but in "white"
 */
void prt(concptr str, TERM_LEN row, TERM_LEN col)
{
    /* Spawn */
    c_prt(TERM_WHITE, str, row, col);
}

/*
 * Print some (colored) text to the screen at the current cursor position,
 * automatically "wrapping" existing text (at spaces) when necessary to
 * avoid placing any text into the last column, and clearing every line
 * before placing any text in that line.  Also, allow "newline" to force
 * a "wrap" to the next line.  Advance the cursor as needed so sequential
 * calls to this function will work correctly.
 *
 * Once this function has been called, the cursor should not be moved
 * until all the related "c_roff()" calls to the window are complete.
 *
 * This function will correctly handle any width up to the maximum legal
 * value of 256, though it works best for a standard 80 character width.
 */
void c_roff(TERM_COLOR a, concptr str)
{
    int w, h;
    (void)term_get_size(&w, &h);

    int x, y;
    (void)term_locate(&x, &y);

    if (y == h - 1 && x > w - 3) {
        return;
    }

    for (concptr s = str; *s; s++) {
        char ch;
#ifdef JP
        int k_flag = iskanji(*s);
#endif
        if (*s == '\n') {
            x = 0;
            y++;
            if (y == h) {
                break;
            }

            term_erase(x, y, 255);
            break;
        }

#ifdef JP
        ch = ((k_flag || isprint(*s)) ? *s : ' ');
#else
        ch = (isprint(*s) ? *s : ' ');
#endif

#ifdef JP
        if ((x >= ((k_flag) ? w - 2 : w - 1)) && (ch != ' '))
#else
        if ((x >= w - 1) && (ch != ' '))
#endif
        {
            int i, n = 0;
            const int end_col = x - 1;

            TERM_COLOR av[256];
            char cv[256];
            if (x < w) {
#ifdef JP
                /* 現在が半角文字の場合 */
                if (!k_flag)
#endif
                {
                    for (i = 0; i <= end_col; i++) {
                        term_what(i, y, &av[i], &cv[i]);

                        if (cv[i] == ' ') {
                            n = i + 1;
                        }
#ifdef JP
                        if (iskanji(cv[i])) {
                            n = i + 2;
                            i++;
                            term_what(i, y, &av[i], &cv[i]);
                        }
#endif
                    }
                }
#ifdef JP
                else {
                    /* 現在が全角文字のとき */
                    /* 文頭が「。」「、」等になるときは、その１つ前の語で改行 */
                    if (strncmp(s, "。", 2) == 0 || strncmp(s, "、", 2) == 0) {
                        term_what(x - 1, y, &av[x - 1], &cv[x - 1]);
                        term_what(x - 2, y, &av[x - 2], &cv[x - 2]);
                        n = x - 2;
                    }
                }
#endif
            }
            if (n == 0) {
                n = w;
            }

            term_erase(n, y, 255);
            x = 0;
            y++;
            if (y == h) {
                break;
            }

            term_erase(x, y, 255);
            for (i = n; i <= end_col; i++) {
#ifdef JP
                if (cv[i] == '\0') {
                    break;
                }
#endif
                term_addch(av[i], cv[i]);
                if (++x > w) {
                    x = w;
                }
            }
        }

#ifdef JP
        term_addch((byte)(a | 0x10), ch);
#else
        term_addch(a, ch);
#endif

#ifdef JP
        if (k_flag) {
            s++;
            x++;
            ch = *s;
            term_addch((byte)(a | 0x20), ch);
        }
#endif

        if (++x > w) {
            x = w;
        }
    }
}

/*
 * As above, but in "white"
 */
void roff(concptr str)
{
    /* Spawn */
    c_roff(TERM_WHITE, str);
}

/*
 * Clear part of the screen
 */
void clear_from(int row)
{
    for (int y = row; y < game_term->hgt; y++) {
        term_erase(0, y, 255);
    }
}
