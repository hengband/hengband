#include "util/int-char-converter.h"
#include <doctest/doctest.h>

TEST_CASE("I2AD continues the labels with digits after z")
{
    CHECK(I2AD(0) == 'a');
    CHECK(I2AD(25) == 'z');
    CHECK(I2AD(26) == '0');
    CHECK(I2AD(31) == '5');
}
