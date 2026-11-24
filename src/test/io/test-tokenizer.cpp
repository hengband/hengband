#include "io/tokenizer.h"
#include "system/h-basic.h"

#include <doctest/doctest.h>

#include <string>
#include <vector>

TEST_CASE("tokenize does not treat multibyte trail bytes as separators")
{
    const std::string input{ static_cast<char>(0xe3), static_cast<char>(0x81), static_cast<char>(0x82), ':', 't', 'a', 'i', 'l' };
    const std::string expected{ static_cast<char>(0xe3), static_cast<char>(0x81), static_cast<char>(0x82) };
    const auto tokens = tokenize(input, 2);

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0] == expected);
    CHECK(tokens[1] == "tail");
}

TEST_CASE("tokenize continues to split legacy field delimiters")
{
    const auto tokens = tokenize("one/two:three", 3);

    REQUIRE(tokens.size() == 3);
    CHECK(tokens[0] == "one");
    CHECK(tokens[1] == "two");
    CHECK(tokens[2] == "three");
}

TEST_CASE("tokenize preserves delimiters after incomplete multibyte characters")
{
    const std::vector<std::string> prefixes{ "\xe3", "\xe3\x81" };
    for (const auto &prefix : prefixes) {
        for (const auto delimiter : { ':', '/' }) {
            const auto tokens = tokenize(prefix + delimiter + "tail", 2);
            REQUIRE(tokens.size() == 2);
            CHECK(tokens[0] == prefix);
            CHECK(tokens[1] == "tail");
        }

        const auto tokens = tokenize(prefix, 2);
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0] == prefix);
    }
}
