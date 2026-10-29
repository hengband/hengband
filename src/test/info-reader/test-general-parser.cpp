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

TEST_CASE("validate_fixed_map_row preserves caller limits and symbol rules")
{
    FixedMapRowLimits limits{ 2, 3 };
    CHECK(validate_fixed_map_row("abc", 1, limits).has_value());
    CHECK(validate_fixed_map_row("", 0, limits).has_value());
    CHECK(validate_fixed_map_row("abcd", 0, limits).error() == ParseErrorType::OUT_OF_BOUNDS);
    CHECK(validate_fixed_map_row("abc", 2, limits).error() == ParseErrorType::OUT_OF_BOUNDS);
    limits.overflow_error = ParseErrorType::INVALID_VALUE;
    CHECK(validate_fixed_map_row("abcd", 0, limits).error() == ParseErrorType::INVALID_VALUE);
    limits.allow_empty = false;
    CHECK(validate_fixed_map_row("", 0, limits).error() == ParseErrorType::INVALID_VALUE);
    limits.expected_width = 2;
    CHECK(validate_fixed_map_row("abc", 0, limits).error() == ParseErrorType::INVALID_VALUE);
    CHECK(validate_fixed_map_row("ab", 0, limits).has_value());
    CHECK(validate_fixed_map_row("a\t", 0, limits).error() == ParseErrorType::INVALID_VALUE);
    CHECK(validate_fixed_map_row("ab", 0, limits, [](unsigned char c) { return c == 'a'; }).error() == ParseErrorType::INVALID_VALUE);
    CHECK(validate_fixed_map_row("aa", 0, limits, [](unsigned char c) { return c == 'a'; }).has_value());
}
