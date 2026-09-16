#include "game-option/runtime-arguments.h"
#include "util/finalizer.h"
#include <doctest/doctest.h>

TEST_CASE("Bot JSON timing is an explicit independent switch")
{
    const auto restore = util::make_finalizer([timing = arg_bot_json_timing, output = arg_bot_json_output, path = arg_bot_json_output_path] {
        arg_bot_json_timing = timing;
        arg_bot_json_output = output;
        arg_bot_json_output_path = path;
    });
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
