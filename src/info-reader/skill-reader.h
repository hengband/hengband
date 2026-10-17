#pragma once

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
    explicit SkillReader(const nlohmann::json &class_data);
    SkillReader(nlohmann::json &&) = delete;
    SkillReader(const SkillReader &) = delete;
    SkillReader(SkillReader &&) = delete;
    SkillReader &operator=(const SkillReader &) = delete;
    SkillReader &operator=(SkillReader &&) = delete;

    int read();
    const std::optional<SkillReadError> &error() const;

private:
    tl::expected<void, parse_error_type> read_weapons(skill_table &skills);
    tl::expected<void, parse_error_type> read_skills(skill_table &skills);
    tl::expected<void, parse_error_type> check_keys(const nlohmann::json &data, std::string_view path, std::initializer_list<std::string_view> keys);
    tl::expected<void, parse_error_type> read_integer(const nlohmann::json &data, int &value, int maximum, std::string_view path);
    tl::expected<void, parse_error_type> fail(parse_error_type code, std::string_view path, std::string_view reason);

    const nlohmann::json &class_data;
    std::string class_id;
    std::optional<SkillReadError> diagnostic;
};
