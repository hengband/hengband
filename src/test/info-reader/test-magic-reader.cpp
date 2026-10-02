#include "info-reader/info-reader-util.h"
#include "info-reader/magic-reader.h"
#include "info-reader/parse-error-types.h"
#include "player-info/class-info.h"
#include "util/finalizer.h"
#include "world/world.h"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
#include <utility>

TEST_CASE("MagicReader preserves an existing class on a later parse error")
{
    auto &world = AngbandWorld::get_instance();
    const auto restore = util::make_finalizer([saved = class_magics_info, index = error_idx, timewalk = world.timewalk_m_idx, &world] {
        class_magics_info = saved;
        error_idx = index;
        world.timewalk_m_idx = timewalk;
    });
    class_magics_info.assign(1, {});
    error_idx = -1;
    world.timewalk_m_idx = 1; // Suppress error messages without an initialized terminal.

    nlohmann::json data = {
        { "name", "WARRIOR" },
        { "spell_type", "NONE" },
        { "magic_status", "STR" },
        { "has_glove_mp_penalty", false },
        { "has_magic_fail_rate_cap", false },
        { "is_spell_trainable", false },
        { "first_spell_level", 99 },
        { "armour_weight_limit", 0 },
        { "realms", nlohmann::json::array() },
    };
    REQUIRE(MagicReader(data).read() == PARSE_ERROR_NONE);
    CHECK(error_idx == 0);
    CHECK(class_magics_info[0].spell_book == ItemKindType::NONE);
    CHECK(class_magics_info[0].spell_first == 99);

    data["spell_type"] = "LIFE";
    data["magic_status"] = "WIS";
    data["has_glove_mp_penalty"] = true;
    data["first_spell_level"] = 1;
    data["realms"] = nlohmann::json::object();
    CHECK(MagicReader(data).read() == PARSE_ERROR_INVALID_TYPE);
    CHECK(class_magics_info[0].spell_book == ItemKindType::NONE);
    CHECK(class_magics_info[0].has_glove_mp_penalty == false);
    CHECK(class_magics_info[0].spell_first == 99);
    CHECK(error_idx == 0);
}
