#include "object-enchant/item-feeling.h"
#include <doctest/doctest.h>

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
