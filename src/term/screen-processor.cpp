#include "term/screen-processor.h"
#include "io/input-key-acceptor.h"
#include "locale/japanese.h"
#include "term/term-color-types.h"
#include "view/display-messages.h"
#include "view/display-symbol.h"
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

    AngbandWorld::get_instance().character_icky_depth++;
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
    auto &world = AngbandWorld::get_instance();
    switch (opt) {
    case ScreenLoadOptType::ONE:
        term_load(false);
        world.character_icky_depth--;
        screen_depth--;
        break;
    case ScreenLoadOptType::ALL:
        term_load(true);
        world.character_icky_depth -= static_cast<byte>(screen_depth);
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
void c_put_str(TERM_COLOR attr, std::string_view sv, TERM_LEN row, TERM_LEN col)
{
    term_putstr(col, row, -1, attr, sv);
}

/*
 * As above, but in "white"
 */
void put_str(std::string_view sv, TERM_LEN row, TERM_LEN col)
{
    term_putstr(col, row, -1, TERM_WHITE, sv);
}

/*
 * Display a string on the screen using an attribute, and clear
 * to the end of the line.
 */
void c_prt(TERM_COLOR attr, std::string_view sv, TERM_LEN row, TERM_LEN col)
{
    term_erase(col, row);
    term_addstr(-1, attr, sv);
}

/*
 * As above, but in "white"
 */
void prt(std::string_view sv, TERM_LEN row, TERM_LEN col)
{
    /* Spawn */
    c_prt(TERM_WHITE, sv, row, col);
}

static std::vector<DisplaySymbol> c_roff_wrap(int x, int y, int w, const char *s)
{
    if (x >= w) {
        return {};
    }

    std::vector<DisplaySymbol> wrap_chars;
    auto wrap_col = w;

    if (_(iskanji(*s), false)) {
        /* 現在が全角文字の場合 */
        /* 行頭が行頭禁則文字になるときは、その１つ前の語で改行 */
        if (is_kinsoku({ s, 2 })) {
            DisplaySymbol ds;
            ds = term_what(x - 2, y, ds);
            wrap_chars.push_back(ds);
            ds = term_what(x - 1, y, ds);
            wrap_chars.push_back(ds);
            wrap_col = x - 2;
        }
    } else {
        /* 現在が半角文字の場合 */
        for (auto i = 0; i < x; i++) {
            DisplaySymbol ds;
            ds = term_what(i, y, ds);

            if (ds.character == ' ') {
                wrap_col = i + 1;
                wrap_chars.clear();
            } else if (_(iskanji(ds.character), false)) {
                wrap_col = i + 2;
                i++;
                wrap_chars.clear();
            } else {
                wrap_chars.push_back(ds);
            }
        }
    }

    term_erase(wrap_col, y);
    return wrap_chars;
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
void c_roff(TERM_COLOR a, std::string_view str)
{
    const auto &[wid, hgt] = term_get_size();
    int x, y;
    (void)term_locate(&x, &y);

    if (y == hgt - 1 && x > wid - 3) {
        return;
    }

    for (auto s = str.begin(); s != str.end(); ++s) {
        const auto is_kanji = _(iskanji(*s), false);

        if (*s == '\n') {
            if (y + 1 < hgt) {
                term_erase(0, y + 1);
            }

            return;
        }

        const auto ch = (is_kanji || isprint(*s)) ? *s : ' ';

        if ((x >= ((is_kanji) ? wid - 2 : wid - 1)) && (ch != ' ')) {
            const auto wrap_chars = c_roff_wrap(x, y, wid, &*s);

            y++;
            if (y == hgt) {
                return;
            }

            term_erase(0, y);
            for (const auto &symbol : wrap_chars) {
                term_addch(symbol);
            }
            x = wrap_chars.size();
        }

        term_addch({ static_cast<uint8_t>(_(a | 0x10, a)), ch });
        if (is_kanji) {
            s++;
            x++;
            term_addch({ static_cast<uint8_t>(a | 0x20), *s });
        }

        if (++x > wid) {
            x = wid;
        }
    }
}

/*
 * As above, but in "white"
 */
void roff(std::string_view str)
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
        TermOffsetSetter tos(0, std::nullopt);
        term_erase(0, y);
    }
}
