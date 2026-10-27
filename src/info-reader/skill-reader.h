#pragma once

#include "info-reader/definition-id-checker.h"
#include "info-reader/parse-error-types.h"
#include <initializer_list>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <tl/expected.hpp>

struct skill_table;

struct SkillReadError {
    std::string class_id;
    std::string path;
    std::string reason;
};

class SkillReader {
public:
    using Session = DefinitionIdChecker;
    static Session create_session(tl::optional<int> last_id = tl::nullopt);

    SkillReader(const nlohmann::json &class_data, Session &session);
    SkillReader(nlohmann::json &&, Session &) = delete;
    SkillReader(const SkillReader &) = delete;
    SkillReader(SkillReader &&) = delete;
    SkillReader &operator=(const SkillReader &) = delete;
    SkillReader &operator=(SkillReader &&) = delete;

    tl::expected<void, ParseErrorType> read();
    const std::optional<SkillReadError> &error() const;

private:
    tl::expected<void, ParseErrorType> read_weapons(skill_table &skills);
    tl::expected<void, ParseErrorType> read_skills(skill_table &skills);
    tl::expected<void, ParseErrorType> check_keys(const nlohmann::json &data, std::string_view path, std::initializer_list<std::string_view> keys);
    tl::expected<void, ParseErrorType> read_integer(const nlohmann::json &data, int &value, int maximum, std::string_view path);
    tl::expected<void, ParseErrorType> fail(ParseErrorType code, std::string_view path, std::string_view reason);

    const nlohmann::json &class_data;
    Session &session;
    std::string class_id;
    std::optional<SkillReadError> diagnostic;
};
