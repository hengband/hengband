#include "artifact/random-art-effects.h"
#include "info-reader/artifact-reader.h"
#include "info-reader/baseitem-reader.h"
#include "info-reader/ego-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "util/enum-converter.h"
#include "world/world.h"
#include <doctest/doctest.h>
#include <limits>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

namespace {
nlohmann::json make_item()
{
    return {
        { "id", 1 },
        { "name", { { "ja", "Test" }, { "en", "Test" } } },
        { "symbol", { { "character", "!" }, { "color", "White" } } },
        { "itemkind", { { "type_value", 1 }, { "subtype_value", 1 } } },
        { "base_item", { { "type_value", 1 }, { "subtype_value", 1 } } },
        { "level", 1 },
        { "rarity", 1 },
        { "weight", 1 },
        { "cost", 1 },
        { "slot", 1 },
        { "rating", 1 },
    };
}

class ActivationStateGuard {
public:
    ActivationStateGuard()
        : saved_index(error_idx)
        , saved_timewalk(AngbandWorld::get_instance().timewalk_m_idx)
    {
        auto &baseitems = BaseitemList::get_instance();
        for (auto &baseitem : baseitems) {
            this->saved_baseitems.push_back(std::move(baseitem));
        }
        baseitems.resize(0);
        this->saved_egos.swap(egos_info);
        AngbandWorld::get_instance().timewalk_m_idx = 1; // Suppress terminal messages.
        error_idx = -1;
    }

    ~ActivationStateGuard()
    {
        auto &baseitems = BaseitemList::get_instance();
        baseitems.resize(this->saved_baseitems.size());
        auto target = baseitems.begin();
        for (auto &baseitem : this->saved_baseitems) {
            std::destroy_at(std::addressof(*target));
            std::construct_at(std::addressof(*target), std::move(baseitem));
            ++target;
        }
        egos_info.swap(this->saved_egos);
        error_idx = this->saved_index;
        AngbandWorld::get_instance().timewalk_m_idx = this->saved_timewalk;
    }

private:
    std::vector<BaseitemDefinition> saved_baseitems;
    std::map<EgoType, EgoItemDefinition> saved_egos;
    int saved_index;
    short saved_timewalk;
};
}

TEST_CASE("Activation readers reject unknown and out-of-range strings as invalid flags")
{
    for (const auto *token : { "UNKNOWN_ACTIVATION", "", "999999999999999999999", "-999999999999999999999", "0", "-1" }) {
        CAPTURE(token);
        ActivationStateGuard guard;
        auto data = make_item();
        data["activate"] = token;
        data["activation"] = token;
        CHECK(BaseitemReader(data).read() == PARSE_ERROR_INVALID_FLAG);
        error_idx = -1;
        CHECK(ArtifactReader(data).read() == PARSE_ERROR_INVALID_FLAG);
        error_idx = -1;
        CHECK(EgoReader(data).read() == PARSE_ERROR_INVALID_FLAG);
        CHECK(egos_info.empty());
    }
}

TEST_CASE("Activation readers reject numeric IDs before narrowing")
{
    const int max = enum2i(RandomArtActType::MAX);
    for (const auto id : { max, max + 1, 9999, 32767, 32768, 65536, 65536 + enum2i(RandomArtActType::BA_FIRE_4), 70000, std::numeric_limits<int>::max() }) {
        CAPTURE(id);
        ActivationStateGuard guard;
        const auto token = std::to_string(id);
        CHECK(grab_one_activation_flag(token) == RandomArtActType::NONE);
        auto data = make_item();
        data["activate"] = token;
        data["activation"] = token;
        CHECK(BaseitemReader(data).read() == PARSE_ERROR_INVALID_FLAG);
        error_idx = -1;
        CHECK(ArtifactReader(data).read() == PARSE_ERROR_INVALID_FLAG);
        error_idx = -1;
        CHECK(EgoReader(data).read() == PARSE_ERROR_INVALID_FLAG);
        CHECK(egos_info.empty());
    }
}

TEST_CASE("Activation readers accept numeric ID boundaries")
{
    for (const auto expected : { RandomArtActType::SUNLIGHT, RandomArtActType::CRIMSON }) {
        CAPTURE(expected);
        ActivationStateGuard guard;
        const auto token = std::to_string(enum2i(expected));
        CHECK(grab_one_activation_flag(token) == expected);
        auto data = make_item();
        data["activate"] = token;
        data["activation"] = token;
        REQUIRE(BaseitemReader(data).read() == PARSE_ERROR_NONE);
        const auto &baseitem = BaseitemList::get_instance().get_baseitem(1);
        CHECK(baseitem.act_idx == expected);
        CHECK(baseitem.flags.has(TR_ACTIVATE));
        REQUIRE(EgoReader(data).read() == PARSE_ERROR_NONE);
        CHECK(egos_info.at(i2enum<EgoType>(1)).act_idx == expected);
    }
}

TEST_CASE("Activation readers preserve named and legacy numeric string conversion")
{
    const auto numeric = std::to_string(enum2i(RandomArtActType::BA_FIRE_4));
    for (const auto &token : { std::string("BA_FIRE_4"), numeric, " +" + numeric, numeric + "suffix" }) {
        CAPTURE(token);
        ActivationStateGuard guard;
        auto data = make_item();
        data["activate"] = token;
        data["activation"] = token;
        CHECK(grab_one_activation_flag(token) == RandomArtActType::BA_FIRE_4);
        REQUIRE(BaseitemReader(data).read() == PARSE_ERROR_NONE);
        const auto &baseitem = BaseitemList::get_instance().get_baseitem(1);
        CHECK(baseitem.act_idx == RandomArtActType::BA_FIRE_4);
        CHECK(baseitem.flags.has(TR_ACTIVATE));
        REQUIRE(EgoReader(data).read() == PARSE_ERROR_NONE);
        CHECK(egos_info.at(i2enum<EgoType>(1)).act_idx == RandomArtActType::BA_FIRE_4);
    }
}

TEST_CASE("Activation readers preserve missing activation and non-string errors")
{
    for (const auto &value : { nlohmann::json(), nlohmann::json(1), nlohmann::json(true) }) {
        CAPTURE(value);
        ActivationStateGuard guard;
        auto data = make_item();
        data["activate"] = value;
        data["activation"] = value;
        const auto expected = value.is_null() ? PARSE_ERROR_NONE : PARSE_ERROR_INVALID_TYPE;
        CHECK(BaseitemReader(data).read() == expected);
        // Artifact success inserts into its singleton; only exercise rejection here.
        if (!value.is_null()) {
            error_idx = -1;
            CHECK(ArtifactReader(data).read() == expected);
        }
        CHECK(EgoReader(data).read() == expected);
    }
}
