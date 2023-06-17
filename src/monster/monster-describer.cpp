#include "monster/monster-describer.h"
#include "io/files-util.h"
#include "locale/english.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

// @todo 性別をEnumFlags に切り替えたら引数の型も変えること.
static int get_monster_pronoun_kind(const MonsterRaceInfo &monrace, const bool pron)
{
    if (!pron) {
        return 0x00;
    }

    if (any_bits(monrace.flags1, RF1_FEMALE)) {
        return 0x20;
    }

    if (any_bits(monrace.flags1, RF1_MALE)) {
        return 0x10;
    }

    return 0x00;
}

static std::string get_monster_personal_pronoun(const int kind, const BIT_FLAGS mode)
{
    switch (kind + (mode & (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE))) {
    case 0x00:
        return _("何か", "it");
    case 0x00 + (MD_OBJECTIVE):
        return _("何か", "it");
    case 0x00 + (MD_POSSESSIVE):
        return _("何かの", "its");
    case 0x00 + (MD_POSSESSIVE | MD_OBJECTIVE):
        return _("何か自身", "itself");
    case 0x00 + (MD_INDEF_HIDDEN):
        return _("何か", "something");
    case 0x00 + (MD_INDEF_HIDDEN | MD_OBJECTIVE):
        return _("何か", "something");
    case 0x00 + (MD_INDEF_HIDDEN | MD_POSSESSIVE):
        return _("何かの", "something's");
    case 0x00 + (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE):
        return _("それ自身", "itself");
    case 0x10:
        return _("彼", "he");
    case 0x10 + (MD_OBJECTIVE):
        return _("彼", "him");
    case 0x10 + (MD_POSSESSIVE):
        return _("彼の", "his");
    case 0x10 + (MD_POSSESSIVE | MD_OBJECTIVE):
        return _("彼自身", "himself");
    case 0x10 + (MD_INDEF_HIDDEN):
        return _("誰か", "someone");
    case 0x10 + (MD_INDEF_HIDDEN | MD_OBJECTIVE):
        return _("誰か", "someone");
    case 0x10 + (MD_INDEF_HIDDEN | MD_POSSESSIVE):
        return _("誰かの", "someone's");
    case 0x10 + (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE):
        return _("彼自身", "himself");
    case 0x20:
        return _("彼女", "she");
    case 0x20 + (MD_OBJECTIVE):
        return _("彼女", "her");
    case 0x20 + (MD_POSSESSIVE):
        return _("彼女の", "her");
    case 0x20 + (MD_POSSESSIVE | MD_OBJECTIVE):
        return _("彼女自身", "herself");
    case 0x20 + (MD_INDEF_HIDDEN):
        return _("誰か", "someone");
    case 0x20 + (MD_INDEF_HIDDEN | MD_OBJECTIVE):
        return _("誰か", "someone");
    case 0x20 + (MD_INDEF_HIDDEN | MD_POSSESSIVE):
        return _("誰かの", "someone's");
    case 0x20 + (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE):
        return _("彼女自身", "herself");
    default:
        return _("何か", "it");
    }
}

static std::optional<std::string> decide_monster_personal_pronoun(const MonsterEntity &monster, const BIT_FLAGS mode)
{
    const auto seen = any_bits(mode, MD_ASSUME_VISIBLE) || (none_bits(mode, MD_ASSUME_HIDDEN) && monster.ml);
    const auto pron = (seen && any_bits(mode, MD_PRON_VISIBLE)) || (!seen && any_bits(mode, MD_PRON_HIDDEN));
    if (seen && !pron) {
        return std::nullopt;
    }

    const auto &monrace = monraces_info[monster.ap_r_idx];
    const auto kind = get_monster_pronoun_kind(monrace, pron);
    return get_monster_personal_pronoun(kind, mode);
}

static std::optional<std::string> get_monster_self_pronoun(const MonsterEntity &monster, const BIT_FLAGS mode)
{
    const auto &monrace = monraces_info[monster.ap_r_idx];
    constexpr BIT_FLAGS self = MD_POSSESSIVE | MD_OBJECTIVE;
    if (!match_bits(mode, self, self)) {
        return std::nullopt;
    }

    if (any_bits(monrace.flags1, RF1_FEMALE)) {
        return _("彼女自身", "herself");
    }

    if (any_bits(monrace.flags1, RF1_MALE)) {
        return _("彼自身", "himself");
    }

    return _("それ自身", "itself");
}

static std::string get_describing_monster_name(const MonsterEntity &monster, const bool is_hallucinated, const BIT_FLAGS mode)
{
    const auto &monrace = monraces_info[monster.ap_r_idx];
    if (!is_hallucinated || any_bits(mode, MD_IGNORE_HALLU)) {
        return any_bits(mode, MD_TRUE_NAME) ? monster.get_real_r_ref().name : monrace.name;
    }

    if (one_in_(2)) {
        constexpr auto filename = _("silly_j.txt", "silly.txt");
        const auto silly_name = get_random_line(filename, enum2i(monster.r_idx));
        if (silly_name.has_value()) {
            return silly_name.value();
        }
    }

    MonsterRaceInfo *hallu_race;
    do {
        auto r_idx = MonsterRace::pick_one_at_random();
        hallu_race = &monraces_info[r_idx];
    } while (hallu_race->kind_flags.has(MonsterKindType::UNIQUE));
    return hallu_race->name;
}

#ifdef JP
/*!
 * @brief モンスターの名前末尾に「？」を付ける
 * @param name モンスターの名前
 * @return ユニークの時は「『ユニーク？』」、非ユニークの時は「非ユニーク？」
 * @details 幻覚時のペット、カメレオンが該当する
 */
static std::string replace_monster_name_undefined(std::string_view name)
{
    if (name.starts_with("』")) {
        constexpr auto ja_char_length = 2;
        const auto name_without_brackets = name.substr(0, name.length() - ja_char_length);
        return format("%s？』", name_without_brackets.data());
    }

    return format("%s？", name.data());
}
#endif

static std::optional<std::string> get_fake_monster_name(const PlayerType &player, const MonsterEntity &monster, const std::string &name, const BIT_FLAGS mode)
{
    const auto &monrace = monraces_info[monster.ap_r_idx];
    const auto is_hallucinated = player.effects()->hallucination()->is_hallucinated();
    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE) || (is_hallucinated && none_bits(mode, MD_IGNORE_HALLU))) {
        return std::nullopt;
    }

    if (monster.mflag2.has(MonsterConstantFlagType::CHAMELEON) && none_bits(mode, MD_TRUE_NAME)) {
        return _(replace_monster_name_undefined(name), format("%s?", name.data()));
    }

    if (player.phase_out && !(player.riding && (&player.current_floor_ptr->m_list[player.riding] == &monster))) {
        return format(_("%sもどき", "fake %s"), name.data());
    }

    return name;
}

static std::string describe_non_pet(const PlayerType &player, const MonsterEntity &monster, const std::string &name, const BIT_FLAGS mode)
{
    const auto fake_name = get_fake_monster_name(player, monster, name, mode);
    if (fake_name.has_value()) {
        return fake_name.value();
    }

    if (any_bits(mode, MD_INDEF_VISIBLE)) {
#ifdef JP
        return name;
#else
        std::stringstream ss;
        const auto first_char = name[0];
        const auto article = is_a_vowel(first_char) ? "an " : "a ";
        ss << article;
        ss << name;
        return ss.str();
#endif
    }

    std::stringstream ss;
    if (monster.is_pet() && none_bits(mode, MD_NO_OWNER)) {
        ss << _("あなたの", "your ");
    } else {
        ss << _("", "the ");
    }

    ss << name;
    return ss.str();
}

static std::string add_cameleon_name(const MonsterEntity &monster, const BIT_FLAGS mode)
{
    if (none_bits(mode, MD_IGNORE_HALLU) || monster.mflag2.has_not(MonsterConstantFlagType::CHAMELEON)) {
        return "";
    }

    const auto &monrace = monraces_info[monster.ap_r_idx];
    if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
        return _("(カメレオンの王)", "(Chameleon Lord)");
    }

    return _("(カメレオン)", "(Chameleon)");
}

/*!
 * @brief モンスターの呼称を作成する / Build a string describing a monster in some way.
 * @param m_ptr モンスターの参照ポインタ
 * @param mode 呼称オプション
 * @return std::string 要求されたモンスターの説明を含む文字列
 */
std::string monster_desc(PlayerType *player_ptr, const MonsterEntity *m_ptr, BIT_FLAGS mode)
{
    const auto pronoun = decide_monster_personal_pronoun(*m_ptr, mode);
    if (pronoun.has_value()) {
        return pronoun.value();
    }

    const auto pronoun_self = get_monster_self_pronoun(*m_ptr, mode);
    if (pronoun_self.has_value()) {
        return pronoun_self.value();
    }

    const auto is_hallucinated = player_ptr->effects()->hallucination()->is_hallucinated();
    const auto name = get_describing_monster_name(*m_ptr, is_hallucinated, mode);
    std::stringstream ss;
    if (m_ptr->is_pet() && !m_ptr->is_original_ap()) {
        ss << _(replace_monster_name_undefined(name), format("%s?", name.data()));
    } else {
        ss << describe_non_pet(*player_ptr, *m_ptr, name, mode);
    }

    if (m_ptr->is_named()) {
        ss << _("「", " called ") << m_ptr->nickname << _("」", "");
    }

    if (player_ptr->riding && (&player_ptr->current_floor_ptr->m_list[player_ptr->riding] == m_ptr)) {
        ss << _("(乗馬中)", "(riding)");
    }

    ss << add_cameleon_name(*m_ptr, mode);
    if (any_bits(mode, MD_IGNORE_HALLU) && !m_ptr->is_original_ap()) {
        ss << "(" << monraces_info[m_ptr->r_idx].name << ")";
    }

    if (any_bits(mode, MD_POSSESSIVE)) {
        ss << _("の", "'s");
    }

    return ss.str();
}
