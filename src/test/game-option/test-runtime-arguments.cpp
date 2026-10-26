#include "game-option/runtime-arguments.h"
#include "test/scoped-restore.h"
#include <doctest/doctest.h>

TEST_CASE("Bot JSON timing is an explicit independent switch")
{
    const auto restore = test::scoped_restore(arg_bot_json_timing, arg_bot_json_output, arg_bot_json_output_path);
    arg_bot_json_timing = false;
    arg_bot_json_output = false;

    SUBCASE("JSON output does not enable timing")
    {
        const auto result = parse_runtime_argument("bot-json-output=test.jsonl");
        CHECK(result == RuntimeArgumentResult::HANDLED);
        CHECK(arg_bot_json_output);
        CHECK_FALSE(arg_bot_json_timing);
        CHECK(arg_bot_json_output_path == "test.jsonl");
    }
    SUBCASE("Timing does not enable JSON output")
    {
        const auto result = parse_runtime_argument("bot-json-timing");
        CHECK(result == RuntimeArgumentResult::HANDLED);
        CHECK(arg_bot_json_timing);
        CHECK_FALSE(arg_bot_json_output);
        const auto repeated = parse_runtime_argument("bot-json-timing");
        CHECK(repeated == RuntimeArgumentResult::HANDLED);
    }
    SUBCASE("Unrecognized suffixes do not enable timing")
    {
        const auto with_value = parse_runtime_argument("bot-json-timing=false");
        const auto with_suffix = parse_runtime_argument("bot-json-timing-extra");
        CHECK(with_value != RuntimeArgumentResult::HANDLED);
        CHECK(with_suffix != RuntimeArgumentResult::HANDLED);
        CHECK_FALSE(arg_bot_json_timing);
    }
}

TEST_CASE("Headless terminal size is parsed for each terminal")
{
    const auto restore = test::scoped_restore(arg_headless_term_sizes);
    arg_headless_term_sizes = {};

    SUBCASE("Valid sizes are stored for the specified terminals")
    {
        CHECK(parse_runtime_argument("headless-term-size=0:120x40") == RuntimeArgumentResult::HANDLED);
        CHECK(parse_runtime_argument("headless-term-size=7:1x1") == RuntimeArgumentResult::HANDLED);
        REQUIRE(arg_headless_term_sizes[0].has_value());
        CHECK(arg_headless_term_sizes[0]->cols == 120);
        CHECK(arg_headless_term_sizes[0]->rows == 40);
        REQUIRE(arg_headless_term_sizes[7].has_value());
        CHECK(arg_headless_term_sizes[7]->cols == 1);
        CHECK(arg_headless_term_sizes[7]->rows == 1);
        CHECK_FALSE(arg_headless_term_sizes[1].has_value());
    }
    SUBCASE("The last specification wins")
    {
        CHECK(parse_runtime_argument("headless-term-size=1:60x20") == RuntimeArgumentResult::HANDLED);
        CHECK(parse_runtime_argument("headless-term-size=1:40x10") == RuntimeArgumentResult::HANDLED);
        REQUIRE(arg_headless_term_sizes[1].has_value());
        CHECK(arg_headless_term_sizes[1]->cols == 40);
        CHECK(arg_headless_term_sizes[1]->rows == 10);
    }
    SUBCASE("Sizes on the boundaries are accepted")
    {
        CHECK(parse_runtime_argument("headless-term-size=0:80x24") == RuntimeArgumentResult::HANDLED);
        CHECK(parse_runtime_argument("headless-term-size=0:255x255") == RuntimeArgumentResult::HANDLED);
        CHECK(parse_runtime_argument("headless-term-size=1:255x255") == RuntimeArgumentResult::HANDLED);
    }
    SUBCASE("Sizes out of range are rejected")
    {
        CHECK(parse_runtime_argument("headless-term-size=0:79x24") == RuntimeArgumentResult::INVALID);
        CHECK(parse_runtime_argument("headless-term-size=0:80x23") == RuntimeArgumentResult::INVALID);
        CHECK(parse_runtime_argument("headless-term-size=1:0x1") == RuntimeArgumentResult::INVALID);
        CHECK(parse_runtime_argument("headless-term-size=1:1x0") == RuntimeArgumentResult::INVALID);
        CHECK(parse_runtime_argument("headless-term-size=1:256x20") == RuntimeArgumentResult::INVALID);
        CHECK(parse_runtime_argument("headless-term-size=1:60x256") == RuntimeArgumentResult::INVALID);
        CHECK(parse_runtime_argument("headless-term-size=-1:80x24") == RuntimeArgumentResult::INVALID);
        CHECK(parse_runtime_argument("headless-term-size=8:80x24") == RuntimeArgumentResult::INVALID);
        CHECK(arg_headless_term_sizes == decltype(arg_headless_term_sizes){});
    }
    SUBCASE("Malformed values are rejected")
    {
        CHECK(parse_runtime_argument("headless-term-size=0:80") == RuntimeArgumentResult::INVALID);
        CHECK(parse_runtime_argument("headless-term-size=0:80x") == RuntimeArgumentResult::INVALID);
        CHECK(parse_runtime_argument("headless-term-size=0:x24") == RuntimeArgumentResult::INVALID);
        CHECK(parse_runtime_argument("headless-term-size=x:80x24") == RuntimeArgumentResult::INVALID);
        CHECK(parse_runtime_argument("headless-term-size=:80x24") == RuntimeArgumentResult::INVALID);
        CHECK(parse_runtime_argument("headless-term-size=80x24") == RuntimeArgumentResult::INVALID);
        CHECK(parse_runtime_argument("headless-term-size=0:80x24x1") == RuntimeArgumentResult::INVALID);
        CHECK(parse_runtime_argument("headless-term-size=0:80X24") == RuntimeArgumentResult::INVALID);
        CHECK(parse_runtime_argument("headless-term-size=") == RuntimeArgumentResult::INVALID);
        CHECK(arg_headless_term_sizes == decltype(arg_headless_term_sizes){});
    }
    SUBCASE("The option without a value is not handled")
    {
        CHECK(parse_runtime_argument("headless-term-size") == RuntimeArgumentResult::NOT_HANDLED);
    }
}
