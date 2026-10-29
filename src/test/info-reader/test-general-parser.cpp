#include "info-reader/general-parser.h"
#include <doctest/doctest.h>
#include <string>

TEST_CASE("parse_fixed_map_symbol accepts exactly one printable ASCII byte")
{
    CHECK_FALSE(parse_fixed_map_symbol(""));
    CHECK_FALSE(parse_fixed_map_symbol("AB"));
    for (int value = 0; value < 256; ++value) {
        const std::string input(1, static_cast<char>(value));
        const auto parsed = parse_fixed_map_symbol(input);
        CHECK(parsed.has_value() == (value >= 32 && value <= 126));
        if (parsed) {
            CHECK(*parsed == input.front());
        }
    }
}
