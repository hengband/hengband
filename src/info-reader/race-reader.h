#pragma once

#include "info-reader/parse-error-types.h"
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <tl/expected.hpp>

class MonraceDefinition;
class MonraceMessage;

class RaceReader {
public:
    explicit RaceReader(const nlohmann::json &monrace_data);
    RaceReader(nlohmann::json &&) = delete;
    RaceReader(const RaceReader &) = delete;
    RaceReader(RaceReader &&) = delete;
    RaceReader &operator=(const RaceReader &) = delete;
    RaceReader &operator=(RaceReader &&) = delete;

    tl::expected<void, ParseErrorType> read() const;
    static std::optional<std::string> validate_references();

private:
    bool grab_one_basic_flag(MonraceDefinition &monrace, std::string_view what) const;
    bool grab_one_spell_flag(MonraceDefinition &monrace, std::string_view what) const;
    tl::expected<void, ParseErrorType> set_mon_name(MonraceDefinition &monrace) const;
    tl::expected<void, ParseErrorType> set_mon_symbol(MonraceDefinition &monrace) const;
    tl::expected<void, ParseErrorType> set_mon_speed(MonraceDefinition &monrace) const;
    tl::expected<void, ParseErrorType> set_mon_evolve(MonraceDefinition &monrace) const;
    tl::expected<void, ParseErrorType> set_mon_sex(MonraceDefinition &monrace) const;
    tl::expected<void, ParseErrorType> set_mon_artifacts(MonraceDefinition &monrace) const;
    tl::expected<void, ParseErrorType> set_mon_escorts(MonraceDefinition &monrace) const;
    tl::expected<void, ParseErrorType> set_mon_blows(MonraceDefinition &monrace) const;
    tl::expected<void, ParseErrorType> set_mon_flags(MonraceDefinition &monrace) const;
    tl::expected<void, ParseErrorType> set_mon_skills(MonraceDefinition &monrace) const;
    tl::expected<void, ParseErrorType> set_mon_final_summons(MonraceDefinition &monrace) const;
    tl::expected<void, ParseErrorType> set_mon_message(MonraceMessage &messages) const;

    const nlohmann::json &monrace_data;
};
