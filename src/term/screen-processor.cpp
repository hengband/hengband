#include "term/screen-processor.h"
#include "io/input-key-acceptor.h"
#include "term/term-color-types.h"
#include "util/buffer-shaper.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "view/display-symbol.h"
#include "world/world.h"
#include <string>

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
    msg_erase();

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
    msg_erase();
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

/*!
 * @brief c_roff() が書いた行から、指定した桁で終わる文字を取り出す
 * @param end_col 文字の次の桁
 * @param y 行
 * @return 文字 (全角文字なら2バイト。行の先頭なら空)
 * @details 全角文字の後半のマスには、c_roff() が属性に 0x20 の印を付けている。
 */
static std::string get_char_before(int end_col, int y)
{
    if (end_col <= 0) {
        return {};
    }

    const auto last = term_what(end_col - 1, y, {});
    if (((last.color & 0x20) == 0) || (end_col < 2)) {
        return std::string(1, last.character);
    }

    return { term_what(end_col - 2, y, {}).character, last.character };
}

/*!
 * @brief c_roff() で右端に達したときに折り返す桁を決める
 * @param x 次の文字を置く桁
 * @param y 行
 * @param w 端末の幅
 * @param ch 次に置く文字 (全角文字なら2バイト)
 * @return 折り返す桁。この桁から x の手前までのマスを次の行へ送る (x なら何も送らない)
 * @details 文字の境目のうち、classify_line_break() で語の切れ目になるもっとも右のものを選ぶ。
 * 前が空白だけの桁は、空白だけの行ができるので選ばない。
 * x の前で分けられず、needs_line_break_backtrack() でさかのぼってよいとき (行頭禁則文字の前か行末禁則文字の後ろ) は、
 * 語の切れ目が無いか行の前半にしか無ければ、語の途中で折り返す。どちらも無ければ x で折り返す。
 * 送ったマスと ch で次の行の最終桁に達する桁は選ばない (最終桁には書かない)。
 * 語の切れ目が行の前半か後半かは、shape_buffer() と同じく、語の切れ目の前の空白を除いた桁で決める。
 * 全角文字の後半のマスには、c_roff() が属性に 0x20 の印を付けている。
 */
static int find_wrap_column(int x, int y, int w, std::string_view ch)
{
    const auto is_second_half = [y](int col) { return (term_what(col, y, {}).color & 0x20) != 0; };
    const auto needs_backtrack = needs_line_break_backtrack(get_char_before(x, y), ch);
    // 前が空白だけの桁では、空白だけの行ができるので折り返さない
    auto first_text_col = 0;
    while ((first_text_col < x) && (term_what(first_text_col, y, {}).character == ' ')) {
        first_text_col++;
    }

    auto mid_word_col = 0;
    for (auto col = x; col > first_text_col; col--) {
        if (x - col + std::ssize(ch) > w - 1) {
            break;
        }

        if ((col < x) && is_second_half(col)) {
            continue;
        }

        const auto right_width = ((col + 1 < x) && is_second_half(col + 1)) ? 2 : 1;
        const auto right = (col < x) ? get_char_before(col + right_width, y) : std::string(ch);

        const auto left = get_char_before(col, y);
        const auto kind = classify_line_break(left, right);
        if (kind == LineBreakKind::WORD_BREAK) {
            // 禁則でさかのぼるとき (行頭禁則文字の前か行末禁則文字の後ろ) は、語の切れ目が行の前半にしか無ければ語の途中で折り返す
            const auto line_width = (left == " ") ? col - 1 : col;
            const auto is_far = needs_backtrack && (line_width * 2 <= x) && (mid_word_col > 0);
            return is_far ? mid_word_col : col;
        }

        if ((kind == LineBreakKind::MID_WORD) && (mid_word_col == 0)) {
            mid_word_col = col;
        }
    }

    return (needs_backtrack && (mid_word_col > 0)) ? mid_word_col : x;
}

static std::vector<DisplaySymbol> c_roff_wrap(int x, int y, int w, std::string_view ch)
{
    // 最終桁に空白を書いた後 (x が幅に達している) も、禁則で分けられなければ (行頭禁則文字の前か行末禁則文字の後ろ) 前の語の切れ目までさかのぼる
    const auto wrap_col = find_wrap_column(x, y, w, ch);
    std::vector<DisplaySymbol> wrap_chars;
    for (auto col = wrap_col; col < x; col++) {
        wrap_chars.push_back(term_what(col, y, {}));
    }

    term_erase(wrap_col, y);
    return wrap_chars;
}

/*
 * Print some (colored) text to the screen at the current cursor position,
 * automatically "wrapping" existing text (see find_wrap_column()) when necessary to
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
    auto [x, y] = term_locate();
    if (y == hgt - 1 && x > wid - 3) {
        return;
    }

    for (auto s = str.begin(); s != str.end(); ++s) {
        const auto is_kanji = is_multibyte_char_at(std::string_view(s, str.end()));

        if (*s == '\n') {
            if (y + 1 < hgt) {
                term_erase(0, y + 1);
            }

            return;
        }

        const auto ch = (is_kanji || isprint(*s)) ? *s : ' ';

        if ((x >= ((is_kanji) ? wid - 2 : wid - 1)) && (ch != ' ')) {
            const auto wrap_chars = c_roff_wrap(x, y, wid, std::string_view(s, std::next(s, is_kanji ? 2 : 1)));

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
        TermOffsetSetter tos(0, tl::nullopt);
        term_erase(0, y);
    }
}
