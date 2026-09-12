#include "info-reader/skill-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "object/tval-types.h"
#include "player/player-skill.h"
#include "util/enum-converter.h"
#include <algorithm>
#include <array>
#include <fmt/format.h>
#include <utility>

namespace {
constexpr auto weapon_names = std::to_array({ "BOW", "DIGGING", "HAFTED", "POLEARM", "SWORD" });
constexpr auto skill_names = std::to_array({ "MARTIAL_ARTS", "TWO_WEAPON", "RIDING", "SHIELD" });
}

SkillReader::SkillReader(const nlohmann::json &class_data)
    : class_data(class_data)
{
}

const std::optional<SkillReadError> &SkillReader::error() const
{
    return this->diagnostic;
}

int SkillReader::fail(int code, std::string_view path, std::string_view reason)
{
    this->diagnostic = SkillReadError{ this->class_id, std::string(path), std::string(reason) };
    return code;
}

int SkillReader::check_keys(const nlohmann::json &data, std::string_view path, std::initializer_list<std::string_view> keys)
{
    if (!data.is_object()) {
        return this->fail(PARSE_ERROR_INVALID_TYPE, path, "expected an object");
    }
    for (const auto key : keys) {
        if (!data.contains(key)) {
            return this->fail(PARSE_ERROR_TOO_FEW_ARGUMENTS, fmt::format("{}.{}", path, key), "missing required field");
        }
    }
    for (const auto &key : data.items()) {
        if (std::find(keys.begin(), keys.end(), key.key()) == keys.end()) {
            return this->fail(PARSE_ERROR_UNDEFINED_DIRECTIVE, fmt::format("{}.{}", path, key.key()), "unknown field");
        }
    }
    return PARSE_ERROR_NONE;
}

int SkillReader::read_integer(const nlohmann::json &data, int &value, int maximum, std::string_view path)
{
    if (const auto err = info_set_integer(data, value, true, Range(0, maximum))) {
        const auto reason = data.is_number_integer() ? fmt::format("expected an integer in [0, {}]", maximum) : "expected an integer";
        return this->fail(err, path, reason);
    }
    return PARSE_ERROR_NONE;
}

int SkillReader::read()
{
    this->diagnostic.reset();
    this->class_id = this->class_data.is_object() && this->class_data.contains("id") ? this->class_data["id"].dump() : "<unknown>";
    if (const auto err = this->check_keys(this->class_data, "$", { "id", "weapons", "skills" })) {
        return err;
    }

    int id;
    if (const auto err = this->read_integer(this->class_data["id"], id, static_cast<int>(class_skills_info.size()) - 1, "$.id")) {
        return err;
    }
    // 職業IDは0から連続。重複と途中の欠落を拒否する。
    if (id != error_idx + 1) {
        return this->fail(PARSE_ERROR_NON_SEQUENTIAL_RECORDS, "$.id", fmt::format("expected class id {} (duplicate or missing class)", error_idx + 1));
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

int SkillReader::read_weapons(skill_table &skills)
{
    const auto &weapons = this->class_data["weapons"];
    if (const auto err = this->check_keys(weapons, "$.weapons", { "BOW", "DIGGING", "HAFTED", "POLEARM", "SWORD" })) {
        return err;
    }
    for (size_t i = 0; i < weapon_names.size(); ++i) {
        const auto &weapon = weapons[weapon_names[i]];
        const auto path = fmt::format("$.weapons.{}", weapon_names[i]);
        if (const auto err = this->check_keys(weapon, path, { "start_ranks", "max_ranks" })) {
            return err;
        }
        const auto tval = ItemKindType::BOW + static_cast<int>(i);
        auto &starts = skills.w_start[tval];
        auto &maxima = skills.w_max[tval];
        const auto &start_ranks = weapon["start_ranks"];
        const auto &max_ranks = weapon["max_ranks"];
        for (const auto *field : { "start_ranks", "max_ranks" }) {
            const auto &ranks = weapon[field];
            const auto field_path = fmt::format("{}.{}", path, field);
            if (!ranks.is_array()) {
                return this->fail(PARSE_ERROR_INVALID_TYPE, field_path, "expected an array");
            }
            if (ranks.size() != starts.size()) {
                return this->fail(PARSE_ERROR_OUT_OF_BOUNDS, field_path, fmt::format("expected {} entries, got {}", starts.size(), ranks.size()));
            }
        }
        for (size_t sval = 0; sval < starts.size(); ++sval) {
            int start, maximum;
            const auto start_path = fmt::format("{}.start_ranks[{}]", path, sval);
            if (const auto err = this->read_integer(start_ranks[sval], start, 4, start_path)) {
                return err;
            }
            if (const auto err = this->read_integer(max_ranks[sval], maximum, 4, fmt::format("{}.max_ranks[{}]", path, sval))) {
                return err;
            }
            if (start > maximum) {
                return this->fail(PARSE_ERROR_INVALID_VALUE, start_path, fmt::format("start rank {} exceeds maximum {}", start, maximum));
            }
            starts[sval] = PlayerSkill::weapon_exp_at(i2enum<PlayerSkillRank>(start));
            maxima[sval] = PlayerSkill::weapon_exp_at(i2enum<PlayerSkillRank>(maximum));
        }
    }
    return PARSE_ERROR_NONE;
}

int SkillReader::read_skills(skill_table &skills)
{
    const auto &data = this->class_data["skills"];
    if (const auto err = this->check_keys(data, "$.skills", { "MARTIAL_ARTS", "TWO_WEAPON", "RIDING", "SHIELD" })) {
        return err;
    }
    for (size_t i = 0; i < skill_names.size(); ++i) {
        const auto &skill = data[skill_names[i]];
        const auto path = fmt::format("$.skills.{}", skill_names[i]);
        if (const auto err = this->check_keys(skill, path, { "start_exp", "max_exp" })) {
            return err;
        }
        int start, maximum;
        const auto max_exp = PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER);
        if (const auto err = this->read_integer(skill["start_exp"], start, max_exp, fmt::format("{}.start_exp", path))) {
            return err;
        }
        if (const auto err = this->read_integer(skill["max_exp"], maximum, max_exp, fmt::format("{}.max_exp", path))) {
            return err;
        }
        if (start > maximum) {
            return this->fail(PARSE_ERROR_INVALID_VALUE, fmt::format("{}.start_exp", path), fmt::format("start experience {} exceeds maximum {}", start, maximum));
        }
        const auto kind = i2enum<PlayerSkillKindType>(i);
        skills.s_start[kind] = static_cast<SUB_EXP>(start);
        skills.s_max[kind] = static_cast<SUB_EXP>(maximum);
    }
    return PARSE_ERROR_NONE;
}
