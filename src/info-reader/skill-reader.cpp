#include "info-reader/skill-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "locale/language-switcher.h"
#include "object/tval-types.h"
#include "player/player-skill.h"
#include "util/enum-converter.h"
#include <array>
#include <fmt/format.h>
#include <utility>

namespace {
constexpr auto weapon_names = std::to_array<std::string_view>({ "BOW", "DIGGING", "HAFTED", "POLEARM", "SWORD" });
constexpr auto skill_names = std::to_array<std::string_view>({ "MARTIAL_ARTS", "TWO_WEAPON", "RIDING", "SHIELD" });
}

SkillReader::Session SkillReader::create_session(tl::optional<int> last_id)
{
    return Session(DefinitionIdOrder::SEQUENTIAL, { 0, static_cast<int>(class_skills_info.size()) - 1 }, _("職業技能", "class skill"), last_id);
}

SkillReader::SkillReader(const nlohmann::json &class_data, Session &session)
    : RecordDiagnostic(session, IntegerDiagnostic::DISTINGUISH_TYPE)
    , class_data(class_data)
{
}

/*!
 * @brief 職業技能定義(JSON Object)のパース関数
 * @return 成功時は空の値、失敗時は解析エラーコード
 * @details 失敗時の診断は error() で取得できる。
 */
tl::expected<void, ParseErrorType> SkillReader::read()
{
    this->begin_read(this->class_data);
    if (const auto result = this->check_keys(this->class_data, "$", std::to_array<std::string_view>({ "id", "weapons", "skills" })); !result) {
        return result;
    }

    const auto id_result = this->session.read(get_json_value(this->class_data, "id"));
    if (!id_result) {
        return this->fail(id_result.error().code, "$.id", id_result.error().reason);
    }
    const auto id = *id_result;

    skill_table skills{};
    if (const auto result = this->read_weapons(skills); !result) {
        return result;
    }
    if (const auto result = this->read_skills(skills); !result) {
        return result;
    }
    class_skills_info[id] = std::move(skills);
    return {};
}

tl::expected<void, ParseErrorType> SkillReader::read_weapons(skill_table &skills)
{
    const auto &weapons = this->class_data["weapons"];
    if (const auto result = this->check_keys(weapons, "$.weapons", weapon_names); !result) {
        return result;
    }
    for (size_t i = 0; i < weapon_names.size(); ++i) {
        const auto &weapon = weapons[weapon_names[i]];
        const auto path = fmt::format("$.weapons.{}", weapon_names[i]);
        if (const auto result = this->check_keys(weapon, path, std::to_array<std::string_view>({ "start_ranks", "max_ranks" })); !result) {
            return result;
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
                return this->fail(ParseErrorType::INVALID_TYPE, field_path, _("配列が必要です", "expected an array"));
            }
            if (ranks.size() != starts.size()) {
                return this->fail(ParseErrorType::INVALID_VALUE, field_path, fmt::format(_("要素数は{}個が必要ですが、{}個あります", "expected {} entries, got {}"), starts.size(), ranks.size()));
            }
        }
        for (size_t sval = 0; sval < starts.size(); ++sval) {
            int start, maximum;
            const auto start_path = fmt::format("{}.start_ranks[{}]", path, sval);
            if (const auto result = this->read_integer(start_ranks[sval], start, 0, 4, start_path); !result) {
                return result;
            }
            if (const auto result = this->read_integer(max_ranks[sval], maximum, 0, 4, fmt::format("{}.max_ranks[{}]", path, sval)); !result) {
                return result;
            }
            if (start > maximum) {
                return this->fail(ParseErrorType::INVALID_VALUE, start_path, fmt::format(_("初期ランク{}が上限{}を超えています", "start rank {} exceeds maximum {}"), start, maximum));
            }
            starts[sval] = PlayerSkill::weapon_exp_at(i2enum<PlayerSkillRank>(start));
            maxima[sval] = PlayerSkill::weapon_exp_at(i2enum<PlayerSkillRank>(maximum));
        }
    }
    return {};
}

tl::expected<void, ParseErrorType> SkillReader::read_skills(skill_table &skills)
{
    const auto &data = this->class_data["skills"];
    if (const auto result = this->check_keys(data, "$.skills", skill_names); !result) {
        return result;
    }
    for (size_t i = 0; i < skill_names.size(); ++i) {
        const auto &skill = data[skill_names[i]];
        const auto path = fmt::format("$.skills.{}", skill_names[i]);
        if (const auto result = this->check_keys(skill, path, std::to_array<std::string_view>({ "start_exp", "max_exp" })); !result) {
            return result;
        }
        int start, maximum;
        const auto max_exp = PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER);
        if (const auto result = this->read_integer(skill["start_exp"], start, 0, max_exp, fmt::format("{}.start_exp", path)); !result) {
            return result;
        }
        if (const auto result = this->read_integer(skill["max_exp"], maximum, 0, max_exp, fmt::format("{}.max_exp", path)); !result) {
            return result;
        }
        if (start > maximum) {
            return this->fail(ParseErrorType::INVALID_VALUE, fmt::format("{}.start_exp", path), fmt::format(_("初期経験値{}が上限{}を超えています", "start experience {} exceeds maximum {}"), start, maximum));
        }
        const auto kind = i2enum<PlayerSkillKindType>(i);
        skills.s_start[kind] = static_cast<SUB_EXP>(start);
        skills.s_max[kind] = static_cast<SUB_EXP>(maximum);
    }
    return {};
}
