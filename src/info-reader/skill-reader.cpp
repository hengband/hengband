#include "info-reader/skill-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "object/tval-types.h"
#include "player/player-skill.h"
#include "util/enum-converter.h"
#include <array>
#include <string_view>
#include <utility>

namespace {
constexpr auto weapon_names = std::to_array({ "BOW", "DIGGING", "HAFTED", "POLEARM", "SWORD" });
constexpr auto skill_names = std::to_array({ "MARTIAL_ARTS", "TWO_WEAPON", "RIDING", "SHIELD" });

bool has_keys(const nlohmann::json &data, std::initializer_list<std::string_view> keys)
{
    if (!data.is_object() || data.size() != keys.size()) {
        return false;
    }
    for (const auto key : keys) {
        if (!data.contains(key)) {
            return false;
        }
    }
    return true;
}
}

SkillReader::SkillReader(const nlohmann::json &class_data)
    : class_data(class_data)
{
}

int SkillReader::read() const
{
    if (!has_keys(this->class_data, { "id", "weapons", "skills" })) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    int id;
    if (const auto err = info_set_integer(this->class_data["id"], id, true, Range(0, static_cast<int>(class_skills_info.size()) - 1))) {
        return err;
    }
    // 職業IDは0から連続。重複と途中の欠落を拒否する。
    if (id != error_idx + 1) {
        return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
    }

    skill_table skills{};
    if (const auto err = this->read_weapons(skills)) {
        return err;
    }
    if (const auto err = this->read_skills(skills)) {
        return err;
    }
    class_skills_info[id] = std::move(skills);
    error_idx = id;
    return PARSE_ERROR_NONE;
}

int SkillReader::read_weapons(skill_table &skills) const
{
    const auto &weapons = this->class_data["weapons"];
    if (!has_keys(weapons, { "BOW", "DIGGING", "HAFTED", "POLEARM", "SWORD" })) {
        return PARSE_ERROR_INVALID_TYPE;
    }
    for (size_t i = 0; i < weapon_names.size(); ++i) {
        const auto &weapon = weapons[weapon_names[i]];
        if (!has_keys(weapon, { "start_ranks", "max_ranks" })) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        const auto tval = ItemKindType::BOW + static_cast<int>(i);
        auto &starts = skills.w_start[tval];
        auto &maxima = skills.w_max[tval];
        const auto &start_ranks = weapon["start_ranks"];
        const auto &max_ranks = weapon["max_ranks"];
        if (!start_ranks.is_array() || !max_ranks.is_array()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        if (start_ranks.size() != starts.size() || max_ranks.size() != maxima.size()) {
            return PARSE_ERROR_OUT_OF_BOUNDS;
        }
        for (size_t sval = 0; sval < starts.size(); ++sval) {
            int start, maximum;
            if (const auto err = info_set_integer(start_ranks[sval], start, true, Range(0, 4))) {
                return err;
            }
            if (const auto err = info_set_integer(max_ranks[sval], maximum, true, Range(0, 4))) {
                return err;
            }
            if (start > maximum) {
                return PARSE_ERROR_INVALID_VALUE;
            }
            starts[sval] = PlayerSkill::weapon_exp_at(i2enum<PlayerSkillRank>(start));
            maxima[sval] = PlayerSkill::weapon_exp_at(i2enum<PlayerSkillRank>(maximum));
        }
    }
    return PARSE_ERROR_NONE;
}

int SkillReader::read_skills(skill_table &skills) const
{
    const auto &data = this->class_data["skills"];
    if (!has_keys(data, { "MARTIAL_ARTS", "TWO_WEAPON", "RIDING", "SHIELD" })) {
        return PARSE_ERROR_INVALID_TYPE;
    }
    for (size_t i = 0; i < skill_names.size(); ++i) {
        const auto &skill = data[skill_names[i]];
        if (!has_keys(skill, { "start_exp", "max_exp" })) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        int start, maximum;
        const auto max_exp = PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER);
        if (const auto err = info_set_integer(skill["start_exp"], start, true, Range(0, max_exp))) {
            return err;
        }
        if (const auto err = info_set_integer(skill["max_exp"], maximum, true, Range(0, max_exp))) {
            return err;
        }
        if (start > maximum) {
            return PARSE_ERROR_INVALID_VALUE;
        }
        const auto kind = i2enum<PlayerSkillKindType>(i);
        skills.s_start[kind] = static_cast<SUB_EXP>(start);
        skills.s_max[kind] = static_cast<SUB_EXP>(maximum);
    }
    return PARSE_ERROR_NONE;
}
