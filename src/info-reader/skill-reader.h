#pragma once

#include "info-reader/definition-id-checker.h"
#include "info-reader/record-diagnostic.h"
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <tl/expected.hpp>

struct skill_table;

class SkillReader : private RecordDiagnostic {
public:
    using RecordDiagnostic::error;
    using Session = DefinitionIdChecker;
    static Session create_session(tl::optional<int> last_id = tl::nullopt);

    SkillReader(const nlohmann::json &class_data, Session &session);
    SkillReader(nlohmann::json &&, Session &) = delete;
    SkillReader(const SkillReader &) = delete;
    SkillReader(SkillReader &&) = delete;
    SkillReader &operator=(const SkillReader &) = delete;
    SkillReader &operator=(SkillReader &&) = delete;

    tl::expected<void, ParseErrorType> read();

private:
    tl::expected<void, ParseErrorType> read_weapons(skill_table &skills);
    tl::expected<void, ParseErrorType> read_skills(skill_table &skills);

    const nlohmann::json &class_data;
};
