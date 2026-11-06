/*!
 * @brief 端末のキーのキューのテスト
 */

#include "term/z-term.h"

#include <doctest/doctest.h>

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
