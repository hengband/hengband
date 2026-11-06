#include "util/int-char-converter.h"
#include <doctest/doctest.h>
#include <initializer_list>

TEST_CASE("I2AD continues the labels with digits after z")
{
    CHECK(I2AD(0) == 'a');
    CHECK(I2AD(25) == 'z');
    CHECK(I2AD(26) == '0');
    CHECK(I2AD(31) == '5');
}

TEST_CASE("AD2I turns the labels of I2AD back into the positions")
{
    for (auto i = 0; i < AD_LABEL_COUNT; i++) {
        CHECK(AD2I(I2AD(i)) == i);
    }

    CHECK(AD2I(I2AD(AD_LABEL_COUNT)) == -1);
}

TEST_CASE("AD2I returns -1 for characters that are not labels")
{
    // '0' より前・'9' と 'a' の間・'z' より後の記号、大文字、制御文字、0x80 以上のバイトは、どれもラベルではない
    for (const auto ch : { '%', '!', '/', ':', '@', '[', '`', '{', '~', 'A', 'Z', ' ', '\r', '\x80', '\xff' }) {
        CAPTURE(static_cast<int>(static_cast<unsigned char>(ch)));
        CHECK(AD2I(ch) == -1);
    }
}
