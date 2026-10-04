#include "artifact/fixed-art-types.h"
#include "artifact/random-art-effects.h"
#include "info-reader/artifact-reader.h"
#include "info-reader/baseitem-reader.h"
#include "info-reader/ego-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "object-enchant/activation-info-table.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/system/artifact-list-test-access.h"
#include "util/enum-converter.h"
#include <array>
#include <doctest/doctest.h>
#include <limits>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr auto unused_activation_ranges = std::array{
    std::pair{ 43, 48 },
    std::pair{ 61, 64 },
    std::pair{ 78, 79 },
    std::pair{ 90, 90 },
    std::pair{ 146, 243 },
};

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

/*!
 * @brief 発動を指定したテスト用のデータを作る
 * @param activation ベースアイテム・アーティファクト (activate) とエゴ (activation) の発動の指定
 */
nlohmann::json make_item_with_activation(const nlohmann::json &activation)
{
    auto data = make_item();
    data["activate"] = activation;
    data["activation"] = activation;
    return data;
}

class ActivationStateGuard {
public:
    ActivationStateGuard()
    {
        auto &baseitems = BaseitemList::get_instance();
        for (auto &baseitem : baseitems) {
            this->saved_baseitems.push_back(std::move(baseitem));
        }
        baseitems.resize(0);
        this->saved_egos.swap(egos_info);
    }

    ActivationStateGuard(const ActivationStateGuard &) = delete;
    ActivationStateGuard &operator=(const ActivationStateGuard &) = delete;

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
    }

private:
    std::vector<BaseitemDefinition> saved_baseitems;
    std::map<EgoType, EgoItemDefinition> saved_egos;
    test::ScopedReaderState reader_state;
    test::ArtifactListTestAccess artifacts;
};

/*!
 * @brief ベースアイテム・アーティファクト・エゴの読み込み処理が、いずれも発動の指定を不正なフラグとして拒否することを確かめる
 * @param token 発動の指定
 * @param also_check_grab grab_one_activation_flag() でも発動なしになることを確かめるか
 * (数値でない指定は std::stoi() が例外を投げるため、読み込み処理を通してだけ確かめる)
 */
void check_rejected_by_all_readers(const std::string &token, bool also_check_grab)
{
    ActivationStateGuard guard;
    if (also_check_grab) {
        CHECK(grab_one_activation_flag(token) == RandomArtActType::NONE);
    }

    const auto data = make_item_with_activation(token);
    CHECK(BaseitemReader(data).read() == PARSE_ERROR_INVALID_FLAG);
    error_idx = -1;
    CHECK(ArtifactReader(data).read() == PARSE_ERROR_INVALID_FLAG);
    error_idx = -1;
    CHECK(EgoReader(data).read() == PARSE_ERROR_INVALID_FLAG);
    CHECK(egos_info.empty());
}

/*!
 * @brief ベースアイテム・アーティファクト・エゴの読み込み処理が、いずれも発動の指定を受け入れ、発動を設定することを確かめる
 * @param token 発動の指定
 * @param expected 設定されるべき発動
 */
void check_accepted_by_all_readers(const std::string &token, RandomArtActType expected)
{
    ActivationStateGuard guard;
    CHECK(grab_one_activation_flag(token) == expected);
    const auto data = make_item_with_activation(token);
    REQUIRE(BaseitemReader(data).read() == PARSE_ERROR_NONE);
    const auto &baseitem = BaseitemList::get_instance().get_baseitem(1);
    CHECK(baseitem.act_idx == expected);
    CHECK(baseitem.flags.has(TR_ACTIVATE));
    error_idx = -1;
    REQUIRE(ArtifactReader(data).read() == PARSE_ERROR_NONE);
    const auto &artifact = ArtifactList::get_instance().get_artifact(i2enum<FixedArtifactId>(1));
    CHECK(artifact.act_idx == expected);
    CHECK(artifact.flags.has(TR_ACTIVATE));
    error_idx = -1;
    REQUIRE(EgoReader(data).read() == PARSE_ERROR_NONE);
    CHECK(egos_info.at(i2enum<EgoType>(1)).act_idx == expected);
}
}

TEST_CASE("Activation readers reject unknown and out-of-range strings as invalid flags")
{
    for (const auto *token : { "UNKNOWN_ACTIVATION", "", "999999999999999999999", "-999999999999999999999", "0", "-1" }) {
        CAPTURE(token);
        check_rejected_by_all_readers(token, false);
    }
}

TEST_CASE("Activation readers reject numeric IDs before narrowing")
{
    const int max = enum2i(RandomArtActType::MAX);
    for (const auto id : { max, max + 1, 9999, 32767, 32768, 65536, 65536 + enum2i(RandomArtActType::BA_FIRE_4), 70000, std::numeric_limits<int>::max() }) {
        CAPTURE(id);
        check_rejected_by_all_readers(std::to_string(id), true);
    }
}

TEST_CASE("Activation readers reject unused numeric IDs")
{
    for (const auto &[first, last] : unused_activation_ranges) {
        for (auto id = first; id <= last; ++id) {
            const auto numeric = std::to_string(id);
            for (const auto &token : { numeric, " +" + numeric, numeric + "suffix" }) {
                CAPTURE(token);
                check_rejected_by_all_readers(token, true);
            }
        }
    }
}

TEST_CASE("Activation readers accept every registered activation")
{
    for (const auto &activation : activation_info) {
        const auto numeric = std::to_string(enum2i(activation.index));
        for (const auto &token : { activation.flag, numeric, " +" + numeric, numeric + "suffix" }) {
            CAPTURE(token);
            check_accepted_by_all_readers(token, activation.index);
        }
    }
}

TEST_CASE("Activation registry covers all non-reserved IDs with unique names")
{
    REQUIRE_FALSE(activation_info.empty());
    std::set<int> ids;
    std::set<std::string> names;
    for (const auto &activation : activation_info) {
        CAPTURE(activation.flag);
        CAPTURE(activation.index);
        CHECK_FALSE(activation.flag.empty());
        CHECK(activation.index > RandomArtActType::NONE);
        CHECK(activation.index < RandomArtActType::MAX);
        CHECK(ids.insert(enum2i(activation.index)).second);
        CHECK(names.insert(activation.flag).second);
    }

    for (auto id = 1; id < enum2i(RandomArtActType::MAX); ++id) {
        CAPTURE(id);
        auto is_unused = false;
        for (const auto &[first, last] : unused_activation_ranges) {
            is_unused |= (id >= first) && (id <= last);
        }
        CHECK(ids.contains(id) == !is_unused);
    }
}

TEST_CASE("Activation readers accept numeric ID boundaries")
{
    for (const auto expected : { RandomArtActType::SUNLIGHT, RandomArtActType::CRIMSON }) {
        CAPTURE(expected);
        check_accepted_by_all_readers(std::to_string(enum2i(expected)), expected);
    }
}

TEST_CASE("Activation readers preserve named and legacy numeric string conversion")
{
    const auto numeric = std::to_string(enum2i(RandomArtActType::BA_FIRE_4));
    for (const auto &token : { std::string("BA_FIRE_4"), numeric, " +" + numeric, numeric + "suffix" }) {
        CAPTURE(token);
        check_accepted_by_all_readers(token, RandomArtActType::BA_FIRE_4);
    }
}

TEST_CASE("Activation readers preserve missing activation and non-string errors")
{
    for (const auto &value : { nlohmann::json(), nlohmann::json(1), nlohmann::json(true) }) {
        CAPTURE(value);
        ActivationStateGuard guard;
        const auto data = make_item_with_activation(value);
        const auto expected = value.is_null() ? PARSE_ERROR_NONE : PARSE_ERROR_INVALID_TYPE;
        CHECK(BaseitemReader(data).read() == expected);
        error_idx = -1;
        CHECK(ArtifactReader(data).read() == expected);
        CHECK(EgoReader(data).read() == expected);
    }
}

TEST_CASE("Activation reader guard restores existing artifact definitions")
{
    test::ArtifactListTestAccess outer_guard;
    auto &artifacts = ArtifactList::get_instance();
    const auto id = i2enum<FixedArtifactId>(42);
    ArtifactDefinition original;
    original.name = "Existing";
    original.act_idx = RandomArtActType::CRIMSON;
    original.flags.set(TR_ACTIVATE);
    artifacts.emplace(id, std::move(original));
    const auto *previous = std::addressof(artifacts.get_artifact(id));
    {
        ActivationStateGuard guard;
        CHECK(artifacts.empty());
        auto data = make_item();
        data["activate"] = "BA_FIRE_4";
        REQUIRE(ArtifactReader(data).read() == PARSE_ERROR_NONE);
        CHECK(artifacts.size() == 1);
    }
    REQUIRE(artifacts.size() == 1);
    const auto &restored = artifacts.get_artifact(id);
    CHECK(std::addressof(restored) == previous);
    CHECK(restored.name == "Existing");
    CHECK(restored.act_idx == RandomArtActType::CRIMSON);
    CHECK(restored.flags.has(TR_ACTIVATE));
}
