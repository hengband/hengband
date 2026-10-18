#include "io/input-context.h"
#include <doctest/doctest.h>
#include <stdexcept>
#include <variant>

TEST_CASE("Input context is restored after a nested input finishes")
{
    CHECK_FALSE(get_input_context());

    {
        const InputContextScope outer(InputKind::CONFIRM, "Continue? [y/n]");
        REQUIRE(get_input_context());
        CHECK(get_input_context()->kind == InputKind::CONFIRM);
        CHECK(get_input_context()->prompt == "Continue? [y/n]");

        {
            const InputContextScope inner(InputKind::COMMAND_CHAR, "Command: ");
            REQUIRE(get_input_context());
            CHECK(get_input_context()->kind == InputKind::COMMAND_CHAR);
            CHECK(get_input_context()->prompt == "Command: ");
        }

        REQUIRE(get_input_context());
        CHECK(get_input_context()->kind == InputKind::CONFIRM);
        CHECK(get_input_context()->prompt == "Continue? [y/n]");
    }

    CHECK_FALSE(get_input_context());
}

TEST_CASE("Input context is restored when an input scope unwinds")
{
    CHECK_FALSE(get_input_context());
    {
        const InputContextScope outer(InputKind::CONFIRM, "Continue? [y/n]");
        CHECK_THROWS_AS([] {
            const InputContextScope inner(InputKind::COMMAND_CHAR, "Command: ");
            throw std::runtime_error("cancel nested operation");
        }(),
            std::runtime_error);
        REQUIRE(get_input_context());
        CHECK(get_input_context()->kind == InputKind::CONFIRM);
        CHECK(get_input_context()->prompt == "Continue? [y/n]");
    }
    CHECK_FALSE(get_input_context());
}

TEST_CASE("Input context details are restored after a nested quantity scope")
{
    CHECK_FALSE(get_input_context());
    {
        const InputContextScope outer({ InputKind::STRING, "Text: ", StringInputDetails{ 12, "outer seed" } });
        {
            const InputContextScope inner({ InputKind::QUANTITY, "Quantity: ", QuantityInputDetails{ 5, 1 } });
            REQUIRE(get_input_context());
            CHECK(get_input_context()->kind == InputKind::QUANTITY);
            const auto *details = std::get_if<QuantityInputDetails>(&get_input_context()->details);
            REQUIRE(details != nullptr);
            CHECK(details->max == 5);
            CHECK(details->initial == 1);
        }

        REQUIRE(get_input_context());
        CHECK(get_input_context()->kind == InputKind::STRING);
        CHECK(get_input_context()->prompt == "Text: ");
        const auto *details = std::get_if<StringInputDetails>(&get_input_context()->details);
        REQUIRE(details != nullptr);
        CHECK(details->max_length == 12);
        CHECK(details->initial == "outer seed");
    }
    CHECK_FALSE(get_input_context());
}

TEST_CASE("Command input details are restored after a nested confirm scope")
{
    CHECK_FALSE(get_input_context());
    {
        const InputContextScope outer({ InputKind::COMMAND, "", CommandInputDetails{ true } });
        {
            const InputContextScope inner(InputKind::CONFIRM, "Continue? [y/n]");
            REQUIRE(get_input_context());
            CHECK(get_input_context()->kind == InputKind::CONFIRM);
            CHECK(std::holds_alternative<std::monostate>(get_input_context()->details));
        }

        REQUIRE(get_input_context());
        CHECK(get_input_context()->kind == InputKind::COMMAND);
        const auto *details = std::get_if<CommandInputDetails>(&get_input_context()->details);
        REQUIRE(details != nullptr);
        CHECK(details->shopping);
    }
    CHECK_FALSE(get_input_context());
}
