#include "object-enchant/item-feeling.h"
#include <doctest/doctest.h>
#include <initializer_list>

TEST_CASE("is_cursed_feeling accepts the feelings that may mean a curse but not the broken one")
{
    CHECK(is_cursed_feeling(FEEL_TERRIBLE));
    CHECK(is_cursed_feeling(FEEL_WORTHLESS));
    CHECK(is_cursed_feeling(FEEL_CURSED));
    // 壊れているの感触は、付けた時点で呪われていない品にしか付かない
    CHECK_FALSE(is_cursed_feeling(FEEL_BROKEN));
    CHECK_FALSE(is_cursed_feeling(FEEL_NONE));
    CHECK_FALSE(is_cursed_feeling(FEEL_UNCURSED));
    CHECK_FALSE(is_cursed_feeling(FEEL_AVERAGE));
    CHECK_FALSE(is_cursed_feeling(FEEL_SPECIAL));
}

TEST_CASE("to_cursed_feeling keeps what the feeling told about artifacts and egos")
{
    CHECK(to_cursed_feeling(FEEL_SPECIAL) == FEEL_TERRIBLE);
    CHECK(to_cursed_feeling(FEEL_TERRIBLE) == FEEL_TERRIBLE);
    CHECK(to_cursed_feeling(FEEL_EXCELLENT) == FEEL_WORTHLESS);
    CHECK(to_cursed_feeling(FEEL_WORTHLESS) == FEEL_WORTHLESS);
    for (const auto feel : { FEEL_NONE, FEEL_BROKEN, FEEL_CURSED, FEEL_UNCURSED, FEEL_AVERAGE, FEEL_GOOD }) {
        CAPTURE(static_cast<int>(feel));
        CHECK(to_cursed_feeling(feel) == FEEL_CURSED);
    }
}
