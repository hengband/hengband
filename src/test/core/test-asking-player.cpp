#include "core/asking-player.h"
#include "io/command-repeater.h"
#include "io/input-context.h"
#include "io/input-key-requester.h"
#include "test/scoped-input-term.h"
#include <doctest/doctest.h>
#include <string>

TEST_CASE("input_string publishes the string context and accepts the initial text")
{
    test::ScopedInputTerm term;
    term.expect({ InputKind::STRING, "Text:", StringInputDetails{ 20, "seed" } }, "\r");
    CHECK(input_string("Text: ", 20, "seed", false) == "seed");
}

TEST_CASE("input_string publishes the byte limit that fits the input field")
{
    test::ScopedInputTerm term;
    const std::string long_prompt(75, 'P');
    term.expect({ InputKind::STRING, long_prompt, StringInputDetails{ 5, "" } }, "abcdefgh\r");
    CHECK(input_string(long_prompt, 20, "", false) == "abcde");
}

TEST_CASE("input_string clears the context when cancelled")
{
    test::ScopedInputTerm term;
    term.expect({ InputKind::STRING, "Cancel:", StringInputDetails{ 20, "seed" } }, "\033");
    CHECK_FALSE(input_string("Cancel: ", 20, "seed", false));
}

TEST_CASE("input_command publishes the command character context")
{
    test::ScopedInputTerm term;

    SUBCASE("accept a key")
    {
        term.expect({ InputKind::COMMAND_CHAR, "Command:", {} }, "x");
        CHECK(input_command("Command: ") == 'x');
    }

    SUBCASE("cancel")
    {
        term.expect({ InputKind::COMMAND_CHAR, "Command:", {} }, "\033");
        CHECK_FALSE(input_command("Command: "));
    }
}

TEST_CASE("input_key_with_context publishes the given context while waiting for a key")
{
    test::ScopedInputTerm term;
    term.expect({ InputKind::CONFIRM, "Really? [y/n/Auto]", {} }, "A");
    CHECK(input_key_with_context(InputKind::CONFIRM, "Really? [y/n/Auto]") == 'A');
}

TEST_CASE("input_key_with_context publishes the any-key context also when all terms are refreshed")
{
    test::ScopedInputTerm term;
    term.expect({ InputKind::ANY_KEY, "Hit any key.", {} }, " ");
    CHECK(input_key_with_context(InputKind::ANY_KEY, "Hit any key.", true) == ' ');
}

TEST_CASE("pause_line publishes the any-key context")
{
#ifdef JP
#ifdef SJIS
    constexpr auto prompt = "[ \x89\xbd\x82\xa9\x83\x4c\x81\x5b\x82\xf0\x89\x9f\x82\xb5\x82\xc4\x89\xba\x82\xb3\x82\xa2 ]"; // [ 何かキーを押して下さい ]
#else
    constexpr auto prompt = "[ \xb2\xbf\xa4\xab\xa5\xad\xa1\xbc\xa4\xf2\xb2\xa1\xa4\xb7\xa4\xc6\xb2\xbc\xa4\xb5\xa4\xa4 ]"; // [ 何かキーを押して下さい ]
#endif
#else
    constexpr auto prompt = "[Press any key to continue]";
#endif
    test::ScopedInputTerm term;
    term.expect({ InputKind::ANY_KEY, prompt, {} }, "x");
    pause_line(23);
}

TEST_CASE("input_quantity publishes the quantity context")
{
    test::ScopedInputTerm term;

    SUBCASE("accept a number")
    {
        term.expect({ InputKind::QUANTITY, "Quantity:", QuantityInputDetails{ 8, 1 } }, "3\r");
        CHECK(input_quantity(8, "Quantity: ") == 3);
    }

    SUBCASE("cancel")
    {
        term.expect({ InputKind::QUANTITY, "Quantity:", QuantityInputDetails{ 8, 1 } }, "\033");
        CHECK(input_quantity(8, "Quantity: ") == 0);
    }

    SUBCASE("an alphabetic key selects the maximum")
    {
        term.expect({ InputKind::QUANTITY, "Quantity:", QuantityInputDetails{ 8, 1 } }, "a\r");
        CHECK(input_quantity(8, "Quantity: ") == 8);
    }
}

TEST_CASE("input_quantity reuses the recorded quantity without waiting when the command is repeated")
{
    // 数量 3 を入力した後に、n でコマンドを繰り返す。ScopedInputTerm がコマンドとして '.' を記録しており、
    // 入力した数量はその後ろに記録される
    test::ScopedInputTerm term;
    term.expect({ InputKind::QUANTITY, "Quantity:", QuantityInputDetails{ 8, 1 } }, "3\r");
    REQUIRE(input_quantity(8, "Quantity: ") == 3);
    command_cmd = 'n';
    repeat_check();
    REQUIRE(command_cmd == '.');

    SUBCASE("the recorded quantity is used")
    {
        CHECK(input_quantity(5, "Quantity: ") == 3);
    }

    SUBCASE("the recorded quantity is clamped to the maximum")
    {
        CHECK(input_quantity(2, "Quantity: ") == 2);
    }
}

TEST_CASE("input_quantity does not wait and keeps the outer context when the command has an argument")
{
    test::ScopedInputTerm term;
    {
        const InputContextScope outer(InputKind::CONFIRM, "Outer");
        command_arg = 9;
        CHECK(input_quantity(4, "Quantity: ") == 4);
        CHECK(command_arg == 0);
        REQUIRE(get_input_context());
        CHECK(get_input_context()->kind == InputKind::CONFIRM);
    }

    CHECK_FALSE(get_input_context());
}

TEST_CASE("input_quantity restores the outer string context after a nested input")
{
    test::ScopedInputTerm term;
    {
        const InputContextScope outer({ InputKind::STRING, "Outer: ", StringInputDetails{ 12, "outer seed" } });
        term.expect({ InputKind::QUANTITY, "Nested:", QuantityInputDetails{ 5, 1 } }, "2\r");
        CHECK(input_quantity(5, "Nested: ") == 2);

        const auto &restored = get_input_context();
        REQUIRE(restored);
        CHECK(restored->kind == InputKind::STRING);
        CHECK(restored->prompt == "Outer:");
        CHECK(restored->details == InputDetails{ StringInputDetails{ 12, "outer seed" } });
    }

    CHECK_FALSE(get_input_context());
}
