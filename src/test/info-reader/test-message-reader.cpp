#include "info-reader/info-reader-util.h"
#include "info-reader/message-reader.h"
#include "info-reader/parse-error-types.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-message.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/scoped-rng.h"
#include "test/system/monrace-list-test-access.h"
#include "test/system/monrace-message-list-test-access.h"
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <string_view>
#include <utility>
#include <vector>

namespace {
nlohmann::json make_message(std::string_view action, const nlohmann::json &texts)
{
    return { { "action", action }, { "chance", 1 }, { "use_name", false }, { "message", { { "ja", texts }, { "en", texts } } } };
}

/*!
 * @brief id_list から参照する種族ID 4 と 5 だけを定義した状態にする
 */
struct DefinedMonracesGuard {
    // Monraceのfixtureはerror_idxも変更するため、他のreaderの状態より先に置く。
    test::MonraceListTestAccess monraces;

    DefinedMonracesGuard()
    {
        MonraceList::get_instance().emplace(static_cast<MonraceId>(4));
        MonraceList::get_instance().emplace(static_cast<MonraceId>(5));
    }

    DefinedMonracesGuard(const DefinedMonracesGuard &) = delete;
    DefinedMonracesGuard(DefinedMonracesGuard &&) = delete;
    DefinedMonracesGuard &operator=(const DefinedMonracesGuard &) = delete;
    DefinedMonracesGuard &operator=(DefinedMonracesGuard &&) = delete;
};
}

TEST_CASE("MessageReader preserves existing messages after a late parse error")
{
    const DefinedMonracesGuard monraces;
    test::MonraceMessageListTestAccess state;
    const auto rng = test::scoped_rng();
    const test::ScopedReaderState reader_state;
    auto &messages = MonraceMessageList::get_instance();
    messages.emplace(4, MonsterMessageType::SPEAK_ALL, 1, false, "original");
    messages.emplace_default(MonsterMessageType::SPEAK_ALL, 1, false, "default");
    const auto valid = make_message("SPEAK_ALL", nlohmann::json::array({ "new" }));
    auto invalid = make_message("SPEAK_BATTLE", nlohmann::json::array({ "staged", "valid" }));
    int expected = PARSE_ERROR_INVALID_FLAG;
    SUBCASE("invalid second localized string")
    {
        invalid["message"]["ja"][1] = 7;
        invalid["message"]["en"][1] = 7;
    }
    SUBCASE("unknown later action")
    {
        invalid["action"] = "UNKNOWN";
    }
    SUBCASE("invalid later chance")
    {
        invalid["chance"] = 101;
    }
    SUBCASE("null later string")
    {
        invalid = make_message("SPEAK_BATTLE", nlohmann::json::array({ "staged", nullptr }));
        expected = PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    nlohmann::json data = { { "id_list", { 4, 5 } }, { "message", { valid, invalid } } };
    CHECK(MessageReader(data).read() == expected);
    CHECK(messages.get_message(4, "", MonsterMessageType::SPEAK_ALL) == "original");
    CHECK(state.message_count(4, MonsterMessageType::SPEAK_ALL) == 1);
    CHECK(state.message_count(5, MonsterMessageType::SPEAK_ALL) == 0);
    CHECK(state.message_count(4, MonsterMessageType::SPEAK_BATTLE) == 0);
    data.erase("id_list");
    data["name"] = "DEFAULT";
    CHECK(MessageReader(data).read() == expected);
    CHECK(messages.get_message(6, "", MonsterMessageType::SPEAK_ALL) == "default");
    CHECK(state.default_message_count(MonsterMessageType::SPEAK_ALL) == 1);
    CHECK(state.default_message_count(MonsterMessageType::SPEAK_BATTLE) == 0);
}

TEST_CASE("MessageReader publishes successful groups and preserves missing-language early success")
{
    const DefinedMonracesGuard monraces;
    test::MonraceMessageListTestAccess state;
    const auto rng = test::scoped_rng();
    const test::ScopedReaderState reader_state;
    auto valid = make_message("SPEAK_ALL", nlohmann::json::array({ "accepted" }));
    auto skipped = make_message("SPEAK_BATTLE", nlohmann::json::array({ "other language" }));
#ifdef JP
    skipped["message"].erase("ja");
#else
    skipped["message"].erase("en");
#endif
    const auto later = make_message("UNKNOWN", nlohmann::json::array({ "unreachable" }));
    nlohmann::json data = { { "id_list", { 4, 5 } }, { "message", { valid } } };
    SUBCASE("normal success") {}
    SUBCASE("early success")
    {
        data["message"].push_back(skipped);
        data["message"].push_back(later);
    }
    SUBCASE("default success")
    {
        data.erase("id_list");
        data["name"] = "DEFAULT";
    }
    SUBCASE("default early success")
    {
        data.erase("id_list");
        data["name"] = "DEFAULT";
        data["message"].push_back(skipped);
        data["message"].push_back(later);
    }
    REQUIRE(MessageReader(data).read() == PARSE_ERROR_NONE);
    CHECK(error_idx == -1);
    auto &messages = MonraceMessageList::get_instance();
    CHECK(messages.get_message(4, "", MonsterMessageType::SPEAK_ALL) == "accepted");
    CHECK(messages.get_message(5, "", MonsterMessageType::SPEAK_ALL) == "accepted");
    if (data.contains("id_list")) {
        CHECK(state.message_count(4, MonsterMessageType::SPEAK_ALL) == 1);
        CHECK(state.message_count(5, MonsterMessageType::SPEAK_ALL) == 1);
    } else {
        CHECK(state.default_message_count(MonsterMessageType::SPEAK_ALL) == 1);
    }
    CHECK(state.message_count(4, MonsterMessageType::SPEAK_BATTLE) == 0);
    CHECK(state.default_message_count(MonsterMessageType::SPEAK_BATTLE) == 0);
}

TEST_CASE("MessageReader rejects monster IDs without monster definitions")
{
    const DefinedMonracesGuard monraces;
    test::MonraceMessageListTestAccess state;
    const test::ScopedReaderState reader_state;
    const auto message = make_message("SPEAK_ALL", nlohmann::json::array({ "unused" }));
    // 範囲内でも定義の無い種族ID。既知のIDの後ろに置いても、どのIDにも登録しない。
    for (const auto undefined_id : { 6, 9999 }) {
        CAPTURE(undefined_id);
        const nlohmann::json data = { { "id_list", { 4, undefined_id } }, { "message", { message } } };
        CHECK(MessageReader(data).read() == PARSE_ERROR_INVALID_VALUE);
        CHECK(state.message_count(4, MonsterMessageType::SPEAK_ALL) == 0);
        CHECK(state.message_count(undefined_id, MonsterMessageType::SPEAK_ALL) == 0);
    }

    const nlohmann::json defined = { { "id_list", { 4, 5 } }, { "message", { message } } };
    REQUIRE(MessageReader(defined).read() == PARSE_ERROR_NONE);
    CHECK(state.message_count(4, MonsterMessageType::SPEAK_ALL) == 1);
    CHECK(state.message_count(5, MonsterMessageType::SPEAK_ALL) == 1);
}

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

/*!
 * @brief ID解決の各エラーを伝播し、先行IDを含むグループ全体を公開しない
 */
TEST_CASE("MessageReader propagates ID helper errors without publishing messages")
{
    const DefinedMonracesGuard monraces;
    test::MonraceMessageListTestAccess state;
    const auto rng = test::scoped_rng();
    const test::ScopedReaderState reader_state(73);
    auto &messages = MonraceMessageList::get_instance();
    messages.emplace(4, MonsterMessageType::SPEAK_ALL, 1, false, "original");
    messages.emplace_default(MonsterMessageType::SPEAK_ALL, 1, false, "default");
    struct InvalidIds {
        nlohmann::json value;
        parse_error_type error;
    };
    const std::vector<InvalidIds> invalid_ids = {
        { nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { 1, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { nlohmann::json::object(), PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { nlohmann::json::array({ nullptr }), PARSE_ERROR_INVALID_FLAG },
        { nlohmann::json::array({ true }), PARSE_ERROR_INVALID_FLAG },
        { nlohmann::json::array({ 4.0 }), PARSE_ERROR_INVALID_TYPE },
        { nlohmann::json::array({ 0 }), PARSE_ERROR_INVALID_FLAG },
        { nlohmann::json::array({ 10000 }), PARSE_ERROR_INVALID_FLAG },
        { nlohmann::json::array({ std::numeric_limits<uint64_t>::max() }), PARSE_ERROR_INVALID_FLAG },
        { nlohmann::json::array({ 4, 6 }), PARSE_ERROR_INVALID_VALUE },
        { nlohmann::json::array({ 4, 5, 10000 }), PARSE_ERROR_INVALID_FLAG },
    };
    for (const auto &invalid : invalid_ids) {
        CAPTURE(invalid.value.dump());
        const nlohmann::json data = { { "id_list", invalid.value }, { "message", { make_message("SPEAK_ALL", nlohmann::json::array({ "new" })) } } };
        CHECK(MessageReader(data).read() == invalid.error);
        CHECK(error_idx == 73);
        CHECK(state.message_count(4, MonsterMessageType::SPEAK_ALL) == 1);
        CHECK(state.message_count(5, MonsterMessageType::SPEAK_ALL) == 0);
        CHECK(state.default_message_count(MonsterMessageType::SPEAK_ALL) == 1);
        CHECK(messages.get_message(4, "", MonsterMessageType::SPEAK_ALL) == "original");
        CHECK(messages.get_message(6, "", MonsterMessageType::SPEAK_ALL) == "default");
    }
}

/*!
 * @brief メッセージ解析の各エラーと優先順位を、個別グループと既定グループで維持する
 */
TEST_CASE("MessageReader propagates message helper errors and keeps validation order")
{
    const DefinedMonracesGuard monraces;
    test::MonraceMessageListTestAccess state;
    const auto rng = test::scoped_rng();
    const test::ScopedReaderState reader_state(73);
    auto &messages = MonraceMessageList::get_instance();
    messages.emplace(4, MonsterMessageType::SPEAK_ALL, 1, false, "original");
    messages.emplace_default(MonsterMessageType::SPEAK_ALL, 1, false, "default");
    const auto valid = make_message("SPEAK_ALL", nlohmann::json::array({ "new" }));
    const auto later = make_message("SPEAK_BATTLE", nlohmann::json::array({ "staged" }));
    const nlohmann::json original = { { "id_list", { 4, 5 } }, { "message", { valid, later } } };
    struct InvalidField {
        const char *path;
        nlohmann::json value;
        parse_error_type error;
    };
    const std::vector<InvalidField> invalid_fields = {
        { "/message", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/message", nlohmann::json::object(), PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/message/1", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/message/1/action", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/message/1/action", 1, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/message/1/action", "UNKNOWN", PARSE_ERROR_INVALID_FLAG },
        { "/message/1/chance", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/message/1/chance", true, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/message/1/chance", 0.5, PARSE_ERROR_INVALID_TYPE },
        { "/message/1/chance", 0, PARSE_ERROR_INVALID_FLAG },
        { "/message/1/chance", 101, PARSE_ERROR_INVALID_FLAG },
        { "/message/1/message", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/message/1/message", nlohmann::json::object(), PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/message/1/message", { { "ja", "text" }, { "en", "text" } }, PARSE_ERROR_INVALID_TYPE },
        { "/message/1/message", { { "ja", nlohmann::json::array({ nullptr }) }, { "en", nlohmann::json::array({ nullptr }) } }, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/message/1/message", { { "ja", nlohmann::json::array({ 7 }) }, { "en", nlohmann::json::array({ 7 }) } }, PARSE_ERROR_INVALID_FLAG },
    };
    const auto check_unchanged = [&](const nlohmann::json &data, parse_error_type expected) {
        CHECK(MessageReader(data).read() == expected);
        CHECK(error_idx == 73);
        CHECK(state.message_count(4, MonsterMessageType::SPEAK_ALL) == 1);
        CHECK(state.message_count(5, MonsterMessageType::SPEAK_ALL) == 0);
        CHECK(state.message_count(4, MonsterMessageType::SPEAK_BATTLE) == 0);
        CHECK(state.default_message_count(MonsterMessageType::SPEAK_ALL) == 1);
        CHECK(state.default_message_count(MonsterMessageType::SPEAK_BATTLE) == 0);
        CHECK(messages.get_message(4, "", MonsterMessageType::SPEAK_ALL) == "original");
        CHECK(messages.get_message(6, "", MonsterMessageType::SPEAK_ALL) == "default");
    };
    for (const auto use_default : { false, true }) {
        CAPTURE(use_default);
        for (const auto &invalid : invalid_fields) {
            CAPTURE(invalid.path);
            auto data = original;
            if (use_default) {
                data.erase("id_list");
                data["name"] = "DEFAULT";
            }
            data[nlohmann::json::json_pointer(invalid.path)] = invalid.value;
            check_unchanged(data, invalid.error);
        }
    }
    auto data = original;
    data["message"][0]["action"] = "UNKNOWN";
    data["id_list"] = { 6 };
    check_unchanged(data, PARSE_ERROR_INVALID_VALUE);
    data["message"] = nullptr;
    check_unchanged(data, PARSE_ERROR_TOO_FEW_ARGUMENTS);
    data = original;
    data.erase("id_list");
    data["name"] = "UNKNOWN";
    data["message"][0]["action"] = "UNKNOWN";
    check_unchanged(data, PARSE_ERROR_TOO_FEW_ARGUMENTS);

    data = original;
    data["id_list"] = nlohmann::json::array();
    CHECK(MessageReader(data).read() == PARSE_ERROR_NONE);
    CHECK(error_idx == 73);
    CHECK(state.message_count(4, MonsterMessageType::SPEAK_ALL) == 1);
    CHECK(state.message_count(5, MonsterMessageType::SPEAK_ALL) == 0);
    CHECK(state.default_message_count(MonsterMessageType::SPEAK_ALL) == 1);
    data["id_list"] = { 4, 5 };
    data["message"] = nlohmann::json::array();
    CHECK(MessageReader(data).read() == PARSE_ERROR_NONE);
    CHECK(error_idx == 73);
    CHECK(state.message_count(4, MonsterMessageType::SPEAK_ALL) == 1);
    CHECK(state.message_count(5, MonsterMessageType::SPEAK_ALL) == 0);
}
