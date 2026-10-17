#include "bot/bot-control-server.h"
#include <doctest/doctest.h>
#include <stdexcept>

TEST_CASE("Input context is restored after a nested input finishes")
{
    CHECK_FALSE(get_bot_input_context());

    {
        const BotInputContextScope outer("confirm", "Continue? [y/n]");
        REQUIRE(get_bot_input_context());
        CHECK(get_bot_input_context()->kind == "confirm");
        CHECK(get_bot_input_context()->prompt == "Continue? [y/n]");

        {
            const BotInputContextScope inner("command_char", "Command: ");
            REQUIRE(get_bot_input_context());
            CHECK(get_bot_input_context()->kind == "command_char");
            CHECK(get_bot_input_context()->prompt == "Command: ");
        }

        REQUIRE(get_bot_input_context());
        CHECK(get_bot_input_context()->kind == "confirm");
        CHECK(get_bot_input_context()->prompt == "Continue? [y/n]");
    }

    CHECK_FALSE(get_bot_input_context());
}

TEST_CASE("Input context is restored when an input scope unwinds")
{
    CHECK_FALSE(get_bot_input_context());
    {
        const BotInputContextScope outer("confirm", "Continue? [y/n]");
        CHECK_THROWS_AS([] {
            const BotInputContextScope inner("command_char", "Command: ");
            throw std::runtime_error("cancel nested operation");
        }(),
            std::runtime_error);
        REQUIRE(get_bot_input_context());
        CHECK(get_bot_input_context()->kind == "confirm");
        CHECK(get_bot_input_context()->prompt == "Continue? [y/n]");
    }
    CHECK_FALSE(get_bot_input_context());
}
