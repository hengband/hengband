#include "monster/monster-describer.h"
#include "io/files-util.h"
#include "locale/english.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "system/angband-system.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <fmt/format.h>
#include <sstream>
#include <string>
#include <string_view>
#include <tl/optional.hpp>

// @todo 性別をEnumFlags に切り替えたら引数の型も変えること.
static int get_monster_pronoun_kind(const MonraceDefinition &monrace, const bool pron)
{
    if (!pron) {
        return 0x00;
    }
    if (monrace.is_female()) {
        return 0x20;
    }
    if (monrace.is_male()) {
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

static tl::optional<std::string> decide_monster_personal_pronoun(const MonsterEntity &monster, const BIT_FLAGS mode)
{
    const auto seen = any_bits(mode, MD_ASSUME_VISIBLE) || (none_bits(mode, MD_ASSUME_HIDDEN) && monster.ml);
    const auto pron = (seen && any_bits(mode, MD_PRON_VISIBLE)) || (!seen && any_bits(mode, MD_PRON_HIDDEN));
    if (seen && !pron) {
        return tl::nullopt;
    }

    const auto &monrace = monster.get_appearance_monrace();
    const auto kind = get_monster_pronoun_kind(monrace, pron);
    return get_monster_personal_pronoun(kind, mode);
}

static tl::optional<std::string> get_monster_self_pronoun(const MonsterEntity &monster, const BIT_FLAGS mode)
{
    const auto &monrace = monster.get_appearance_monrace();
    constexpr BIT_FLAGS self = MD_POSSESSIVE | MD_OBJECTIVE;
    if (!match_bits(mode, self, self)) {
        return tl::nullopt;
    }

    if (monrace.is_female()) {
        return _("彼女自身", "herself");
    }

    if (monrace.is_male()) {
        return _("彼自身", "himself");
    }

    return _("それ自身", "itself");
}

static std::string get_describing_monster_name(const MonsterEntity &monster, const bool is_hallucinated, const BIT_FLAGS mode)
{
    const auto &monrace = monster.get_appearance_monrace();
    if (!is_hallucinated || any_bits(mode, MD_IGNORE_HALLU)) {
        return any_bits(mode, MD_TRUE_NAME) ? monster.get_real_monrace().name.string() : monrace.name.string();
    }

    if (one_in_(2)) {
        constexpr auto filename = _("silly_j.txt", "silly.txt");
        const auto silly_name = get_random_line(filename, enum2i(monster.r_idx));
        if (silly_name) {
            return *silly_name;
        }
    }

    const auto &monraces = MonraceList::get_instance();
    const auto ids = monraces.search([](const auto &monrace) { return monrace.kind_flags.has_not(MonsterKindType::UNIQUE); });
    return monraces.get_monrace(rand_choice(ids)).name.string();
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
    constexpr std::string_view closing_quotation = "』";
    if (name.ends_with(closing_quotation)) {
        name.remove_suffix(closing_quotation.length());
        return fmt::format("{}？{}", name, closing_quotation);
    }

    return fmt::format("{}？", name);
}
#endif

static tl::optional<std::string> get_fake_monster_name(const PlayerType &player, const MonsterEntity &monster, const std::string &name, const BIT_FLAGS mode)
{
    const auto &monrace = monster.get_appearance_monrace();
    const auto is_hallucinated = player.effects()->hallucination().is_hallucinated();
    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE) || (is_hallucinated && none_bits(mode, MD_IGNORE_HALLU))) {
        return tl::nullopt;
    }

    if (monster.mflag2.has(MonsterConstantFlagType::CHAMELEON) && none_bits(mode, MD_TRUE_NAME)) {
        return _(replace_monster_name_undefined(name), format("%s?", name.data()));
    }

    if (AngbandSystem::get_instance().is_phase_out() && !monster.is_riding()) {
        return format(_("%sもどき", "fake %s"), name.data());
    }

    return name;
}

static std::string describe_non_pet(const PlayerType &player, const MonsterEntity &monster, const std::string &name, const BIT_FLAGS mode)
{
    const auto fake_name = get_fake_monster_name(player, monster, name, mode);
    if (fake_name) {
        return *fake_name;
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

    const auto &monrace = monster.get_appearance_monrace();
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
std::string monster_desc(PlayerType *player_ptr, const MonsterEntity &monster, BIT_FLAGS mode)
{
    const auto pronoun = decide_monster_personal_pronoun(monster, mode);
    if (pronoun) {
        return *pronoun;
    }

    const auto pronoun_self = get_monster_self_pronoun(monster, mode);
    if (pronoun_self) {
        return *pronoun_self;
    }

    const auto is_hallucinated = player_ptr->effects()->hallucination().is_hallucinated();
    const auto name = get_describing_monster_name(monster, is_hallucinated, mode);
    std::stringstream ss;
    if (monster.is_pet() && !monster.is_original_ap()) {
        ss << _(replace_monster_name_undefined(name), format("%s?", name.data()));
    } else {
        ss << describe_non_pet(*player_ptr, monster, name, mode);
    }

    if (monster.is_named()) {
        ss << _("「", " called ") << monster.nickname << _("」", "");
    }

    if (monster.is_riding()) {
        ss << _("(乗馬中)", "(riding)");
    }

    ss << add_cameleon_name(monster, mode);
    if (any_bits(mode, MD_IGNORE_HALLU) && !monster.is_original_ap()) {
        ss << "(" << monster.get_monrace().name << ")";
    }

    if (any_bits(mode, MD_POSSESSIVE)) {
        ss << _("の", "'s");
    }

    return ss.str();
}
