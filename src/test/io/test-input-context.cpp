#include "io/input-context.h"
#include <doctest/doctest.h>
#include <stdexcept>
#include <variant>

TEST_CASE("Input context is restored after a nested input finishes")
{
    CHECK_FALSE(get_input_context());

    {
        const InputContextScope outer("confirm", "Continue? [y/n]");
        REQUIRE(get_input_context());
        CHECK(get_input_context()->kind == "confirm");
        CHECK(get_input_context()->prompt == "Continue? [y/n]");

        {
            const InputContextScope inner("command_char", "Command: ");
            REQUIRE(get_input_context());
            CHECK(get_input_context()->kind == "command_char");
            CHECK(get_input_context()->prompt == "Command: ");
        }

        REQUIRE(get_input_context());
        CHECK(get_input_context()->kind == "confirm");
        CHECK(get_input_context()->prompt == "Continue? [y/n]");
    }

    CHECK_FALSE(get_input_context());
}

TEST_CASE("Input context is restored when an input scope unwinds")
{
    CHECK_FALSE(get_input_context());
    {
        const InputContextScope outer("confirm", "Continue? [y/n]");
        CHECK_THROWS_AS([] {
            const InputContextScope inner("command_char", "Command: ");
            throw std::runtime_error("cancel nested operation");
        }(),
            std::runtime_error);
        REQUIRE(get_input_context());
        CHECK(get_input_context()->kind == "confirm");
        CHECK(get_input_context()->prompt == "Continue? [y/n]");
    }
    CHECK_FALSE(get_input_context());
}

TEST_CASE("Input context details are restored after a nested quantity scope")
{
    CHECK_FALSE(get_input_context());
    {
        const InputContextScope outer({ "string", "Text: ", StringInputDetails{ 12, "outer seed" } });
        {
            const InputContextScope inner({ "quantity", "Quantity: ", QuantityInputDetails{ 5, 1 } });
            REQUIRE(get_input_context());
            CHECK(get_input_context()->kind == "quantity");
            const auto *details = std::get_if<QuantityInputDetails>(&get_input_context()->details);
            REQUIRE(details != nullptr);
            CHECK(details->max == 5);
            CHECK(details->initial == 1);
        }

        REQUIRE(get_input_context());
        CHECK(get_input_context()->kind == "string");
        CHECK(get_input_context()->prompt == "Text: ");
        const auto *details = std::get_if<StringInputDetails>(&get_input_context()->details);
        REQUIRE(details != nullptr);
        CHECK(details->max_length == 12);
        CHECK(details->initial == "outer seed");
    }
    CHECK_FALSE(get_input_context());
}
