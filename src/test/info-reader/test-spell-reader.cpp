#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/spell-reader.h"
#include "system/spell-info-list.h"
#include "util/finalizer.h"
#include "world/world.h"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

TEST_CASE("SpellReader requires arrays for books and spells")
{
    auto &world = AngbandWorld::get_instance();
    const auto restore = util::make_finalizer([index = error_idx, timewalk = world.timewalk_m_idx, &world] {
        error_idx = index;
        world.timewalk_m_idx = timewalk;
    });
    auto &spells = SpellInfoList::get_instance();
    nlohmann::json data = { { "name", "LIFE" }, { "books", nlohmann::json::array() } };

    error_idx = -1;
    world.timewalk_m_idx = 1; // Suppress error messages without an initialized terminal.
    CHECK(SpellReader(data, spells).read() == PARSE_ERROR_NONE);

    data["books"] = nlohmann::json::array({ { { "spells", nlohmann::json::array() } } });
    CHECK(SpellReader(data, spells).read() == PARSE_ERROR_NONE);

    // A JSON object is iterable too, so these used to be accepted as record arrays.
    data["books"] = { { "first", { { "spells", nlohmann::json::array() } } } };
    CHECK(SpellReader(data, spells).read() == PARSE_ERROR_INVALID_TYPE);

    data["books"] = nlohmann::json::array({ { { "spells", nlohmann::json::object() } } });
    CHECK(SpellReader(data, spells).read() == PARSE_ERROR_INVALID_TYPE);
}
