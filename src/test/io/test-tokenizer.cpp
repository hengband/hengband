#include "io/tokenizer.h"
#include "system/h-basic.h"

#include <doctest/doctest.h>

#include <string>
#include <vector>

TEST_CASE("tokenize does not treat multibyte trail bytes as separators")
{
#ifdef JP
#ifdef _WIN32
    const std::string input{ static_cast<char>(0x83), static_cast<char>(0x5c), ':', 't', 'a', 'i', 'l' };
    const std::string expected{ static_cast<char>(0x83), static_cast<char>(0x5c) };
    const std::string single_byte_input{ static_cast<char>(0xb1), ':', 't', 'a', 'i', 'l' };
#else
    const std::string input{ static_cast<char>(0x8f), static_cast<char>(0xa1), static_cast<char>(0xa1), ':', 't', 'a', 'i', 'l' };
    const std::string expected{ static_cast<char>(0x8f), static_cast<char>(0xa1), static_cast<char>(0xa1) };
#endif
#else
    const std::string input{ static_cast<char>(0xe3), static_cast<char>(0x81), static_cast<char>(0x82), ':', 't', 'a', 'i', 'l' };
    const std::string expected{ static_cast<char>(0xe3), static_cast<char>(0x81), static_cast<char>(0x82) };
#endif
    const auto tokens = tokenize(input, 2);

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0] == expected);
    CHECK(tokens[1] == "tail");
#if defined(JP) && defined(_WIN32)
    const auto single_byte_tokens = tokenize(single_byte_input, 2);
    REQUIRE(single_byte_tokens.size() == 2);
    CHECK(single_byte_tokens[0] == std::string(1, static_cast<char>(0xb1)));
    CHECK(single_byte_tokens[1] == "tail");
#endif
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
#if defined(JP) && defined(_WIN32)
    const std::vector<std::string> prefixes{ "\x83" };
#elif defined(JP)
    const std::vector<std::string> prefixes{ "\x8f", "\x8f\xa1", "\xa1", "\x8e" };
#else
    const std::vector<std::string> prefixes{ "\xe3", "\xe3\x81" };
#endif
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
