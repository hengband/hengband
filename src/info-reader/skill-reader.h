#pragma once

#include <nlohmann/json_fwd.hpp>

struct skill_table;

class SkillReader {
public:
    explicit SkillReader(const nlohmann::json &class_data);
    SkillReader(nlohmann::json &&) = delete;

    int read() const;

private:
    int read_weapons(skill_table &skills) const;
    int read_skills(skill_table &skills) const;

    const nlohmann::json &class_data;
};
