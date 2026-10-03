#include "info-reader/message-reader.h"
#include "info-reader/parse-error-types.h"
#include "test/info-reader/scoped-reader-state.h"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

TEST_CASE("MessageReader requires arrays for localized message lists")
{
    const test::ScopedReaderState reader_state;

    nlohmann::json data = {
        { "name", "DEFAULT" },
        { "message", nlohmann::json::array({ { { "action", "SPEAK_ALL" }, { "chance", 1 }, { "message", { { "ja", nlohmann::json::array() }, { "en", nlohmann::json::array() } } } } }) },
    };
    CHECK(MessageReader(data).read() == PARSE_ERROR_NONE);

    // JSON objects are iterable and their string values used to be accepted as messages.
    data["message"][0]["message"]["ja"] = { { "first", "hello" } };
    data["message"][0]["message"]["en"] = { { "first", "hello" } };
    CHECK(MessageReader(data).read() == PARSE_ERROR_INVALID_TYPE);

    data["message"][0]["message"]["ja"] = nullptr;
    data["message"][0]["message"]["en"] = nullptr;
    CHECK(MessageReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
}
