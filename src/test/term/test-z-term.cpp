/*!
 * @brief 端末のテスト
 *
 * キーのキューと、画面のマスに置く全角文字の扱いを検証する。
 * 2バイト文字のテストデータは16進エスケープで書く (src/test/README.md を参照)。
 */

#include "term/z-term.h"

#include "game-option/special-options.h"
#include "term/term-color-types.h"
#include "test/scoped-input-term.h"
#include "test/scoped-restore.h"
#include "view/display-symbol.h"

#include <doctest/doctest.h>

#include <string>
#include <string_view>
#include <vector>

namespace {
term_type make_queue(uint16_t size, uint16_t head, uint16_t tail)
{
    term_type term{};
    term.key_size = size;
    term.key_head = head;
    term.key_tail = tail;
    return term;
}
}

TEST_CASE("term_key_queue_pending counts keys in the circular key queue")
{
    SUBCASE("the queue is empty")
    {
        CHECK(term_key_queue_pending(make_queue(64, 0, 0)) == 0);
        CHECK(term_key_queue_pending(make_queue(64, 10, 10)) == 0);
    }

    SUBCASE("the head is after the tail")
    {
        CHECK(term_key_queue_pending(make_queue(64, 5, 2)) == 3);
    }

    SUBCASE("the queue wraps around (keys pushed back onto an empty queue)")
    {
        CHECK(term_key_queue_pending(make_queue(64, 0, 63)) == 1);
        CHECK(term_key_queue_pending(make_queue(64, 3, 60)) == 7);
    }

    SUBCASE("the term has no key queue")
    {
        CHECK(term_key_queue_pending(make_queue(0, 0, 0)) == 0);
    }
}

#ifdef JP
namespace {
#ifdef SJIS
constexpr std::string_view KANJI_A = "\x88\x9f"; //!< 亜
#else
constexpr std::string_view KANJI_A = "\xb0\xa1"; //!< 亜
#endif

/*!
 * @brief 端末の行の先頭から指定したマス数のマスを返す
 */
std::vector<TermChar> read_cells(int y, int n)
{
    std::vector<TermChar> cells;
    for (auto x = 0; x < n; x++) {
        cells.push_back(term_what_cell(x, y)->character);
    }

    return cells;
}
}

TEST_CASE("term_putstr puts a full-width character into two cells")
{
    test::ScopedInputTerm term;
    term_clear();
    term_putstr(0, 0, -1, TERM_WHITE, std::string("a").append(KANJI_A).append("b"));
    const auto cells = read_cells(0, 4);
    CHECK(cells[0] == TermChar::from_byte('a'));
    CHECK(cells[1] == TermChar::wide(KANJI_A));
    CHECK(cells[2] == TermChar::wide_trail());
    CHECK(cells[3] == TermChar::from_byte('b'));
    CHECK(term_locate().first == 4);
}

TEST_CASE("term_putstr does not put a full-width character truncated at the end of the string")
{
    test::ScopedInputTerm term;
    term_clear();
    term_putstr(0, 0, -1, TERM_WHITE, std::string("a").append(KANJI_A.substr(0, 1)));
    const auto cells = read_cells(0, 2);
    CHECK(cells[0] == TermChar::from_byte('a'));
    CHECK(cells[1] == TermChar::from_byte(' '));
}

TEST_CASE("overwriting half of a full-width character blanks the other half")
{
    test::ScopedInputTerm term;
    term_clear();

    SUBCASE("the right half is overwritten with a string")
    {
        term_putstr(0, 0, -1, TERM_WHITE, KANJI_A);
        term_putstr(1, 0, -1, TERM_WHITE, "x");
        CHECK(read_cells(0, 2) == std::vector{ TermChar::from_byte(' '), TermChar::from_byte('x') });
    }

    SUBCASE("the left half is overwritten with a string")
    {
        term_putstr(0, 0, -1, TERM_WHITE, KANJI_A);
        term_putstr(0, 0, -1, TERM_WHITE, "x");
        CHECK(read_cells(0, 2) == std::vector{ TermChar::from_byte('x'), TermChar::from_byte(' ') });
    }

    SUBCASE("the right half is overwritten with a character")
    {
        term_putstr(0, 0, -1, TERM_WHITE, KANJI_A);
        term_putch(1, 0, { TERM_WHITE, 'x' });
        CHECK(read_cells(0, 2) == std::vector{ TermChar::from_byte(' '), TermChar::from_byte('x') });
    }

    SUBCASE("the left half is overwritten with a character")
    {
        term_putstr(0, 0, -1, TERM_WHITE, KANJI_A);
        term_putch(0, 0, { TERM_WHITE, 'x' });
        CHECK(read_cells(0, 2) == std::vector{ TermChar::from_byte('x'), TermChar::from_byte(' ') });
    }

    SUBCASE("the right half of a full-width character is overwritten with another full-width character")
    {
        term_putstr(0, 0, -1, TERM_WHITE, KANJI_A);
        term_putstr(1, 0, -1, TERM_WHITE, KANJI_A);
        CHECK(read_cells(0, 3) == std::vector{ TermChar::from_byte(' '), TermChar::wide(KANJI_A), TermChar::wide_trail() });
    }

    SUBCASE("a full-width character that does not fit is written from the right half")
    {
        term_putstr(0, 0, -1, TERM_WHITE, KANJI_A);
        term_putstr(1, 0, 1, TERM_WHITE, KANJI_A);
        CHECK(read_cells(0, 3) == std::vector{ TermChar::from_byte(' '), TermChar::from_byte(' '), TermChar::from_byte(' ') });
    }
}

TEST_CASE("term_queue_bigchar puts a full-width character only when both cells are on the screen")
{
    test::ScopedInputTerm term;
    const auto restore = test::scoped_restore(use_bigtile);
    use_bigtile = true;
    term_clear();
    const auto &[wid, hgt] = term_get_size();
    term_queue_bigchar(wid - 3, 0, { { TERM_WHITE, 'A' }, {} });
    term_queue_bigchar(wid - 1, 0, { { TERM_WHITE, 'A' }, {} });
    CHECK(term_what_cell(wid - 3, 0)->character.is_wide_lead());
    CHECK(term_what_cell(wid - 2, 0)->character.is_wide_trail());
    CHECK(term_what_cell(wid - 1, 0)->character == TermChar::from_byte('A'));
}

TEST_CASE("term_win::append_text replaces a full-width character split by the range with a space")
{
    test::ScopedInputTerm term;
    term_clear();
    term_putstr(0, 0, -1, TERM_WHITE, std::string("a").append(KANJI_A).append(KANJI_A));
    const auto append_text = [](int x, int n) {
        std::string text;
        game_term->scr->append_text(x, 0, n, text);
        return text;
    };

    CHECK(append_text(0, 5) == std::string("a").append(KANJI_A).append(KANJI_A));
    CHECK(append_text(2, 3) == std::string(" ").append(KANJI_A));
    CHECK(append_text(0, 4) == std::string("a").append(KANJI_A).append(" "));
    CHECK(append_text(2, 2) == "  ");
}

TEST_CASE("term_erase erases the whole full-width character on its edge")
{
    test::ScopedInputTerm term;
    term_clear();
    term_putstr(0, 0, -1, TERM_WHITE, std::string(KANJI_A).append(KANJI_A));

    SUBCASE("erasing from the right half")
    {
        term_erase(1, 0, 1);
        CHECK(read_cells(0, 4) == std::vector{ TermChar::from_byte(' '), TermChar::from_byte(' '), TermChar::wide(KANJI_A), TermChar::wide_trail() });
    }

    SUBCASE("erasing up to the left half")
    {
        term_erase(0, 0, 3);
        CHECK(read_cells(0, 4) == std::vector(4, TermChar::from_byte(' ')));
    }
}
#endif
