#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string_view>

class MonraceDefinition;

class RaceReader {
public:
    explicit RaceReader(const nlohmann::json &monrace_data);
    RaceReader(nlohmann::json &&) = delete;
    RaceReader(const RaceReader &) = delete;
    RaceReader(RaceReader &&) = delete;
    RaceReader &operator=(const RaceReader &) = delete;
    RaceReader &operator=(RaceReader &&) = delete;

    int read() const;

private:
    bool grab_one_basic_flag(MonraceDefinition &monrace, std::string_view what) const;
    bool grab_one_spell_flag(MonraceDefinition &monrace, std::string_view what) const;
    int set_mon_name(MonraceDefinition &monrace) const;
    int set_mon_symbol(MonraceDefinition &monrace) const;
    int set_mon_speed(MonraceDefinition &monrace) const;
    int set_mon_evolve(MonraceDefinition &monrace) const;
    int set_mon_sex(MonraceDefinition &monrace) const;
    int set_mon_artifacts(MonraceDefinition &monrace) const;
    int set_mon_escorts(MonraceDefinition &monrace) const;
    int set_mon_blows(MonraceDefinition &monrace) const;
    int set_mon_flags(MonraceDefinition &monrace) const;
    int set_mon_skills(MonraceDefinition &monrace) const;
    int set_mon_final_summons(MonraceDefinition &monrace) const;
    int set_mon_message(MonraceDefinition &monrace) const;

    const nlohmann::json &monrace_data;
};
