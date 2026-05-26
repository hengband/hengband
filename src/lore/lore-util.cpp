#include "lore/lore-util.h"
#include "game-option/birth-options.h"
#include "locale/language-switcher.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-race/race-sex.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-records.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"

const who_word_definition Who::words = {
    { WHO_WORD_TYPE::WHO,
        {
            { false, { { MonsterSex::NONE, _("それ", "it") }, { MonsterSex::MALE, _("彼", "he") }, { MonsterSex::FEMALE, _("彼女", "she") } } },
            { true, { { MonsterSex::NONE, _("それら", "they") }, { MonsterSex::MALE, _("彼ら", "they") }, { MonsterSex::FEMALE, _("彼女ら", "they") } } },
        } },
    { WHO_WORD_TYPE::WHOSE,
        {
            { false, { { MonsterSex::NONE, _("それの", "its") }, { MonsterSex::MALE, _("彼の", "his") }, { MonsterSex::FEMALE, _("彼女の", "her") } } },
            { true, { { MonsterSex::NONE, _("それらの", "their") }, { MonsterSex::MALE, _("彼らの", "their") }, { MonsterSex::FEMALE, _("彼女らの", "their") } } },
        } },
    { WHO_WORD_TYPE::WHOM,
        {
            { false, { { MonsterSex::NONE, _("それ", "it") }, { MonsterSex::MALE, _("彼", "him") }, { MonsterSex::FEMALE, _("彼女", "her") } } },
            { true, { { MonsterSex::NONE, _("それら", "them") }, { MonsterSex::MALE, _("彼ら", "them") }, { MonsterSex::FEMALE, _("彼女ら", "them") } } },
        } },
};

/*
 * Prepare hook for c_roff(). It will be changed for spoiler generation in wizard1.c.
 */
hook_c_roff_pf hook_c_roff = c_roff;

lore_msg::lore_msg(std::string_view msg, byte color)
    : msg(msg)
    , color(color)
{
}

lore_type::lore_type(MonraceId monrace_id, monster_lore_mode mode)
    : monrace_id(monrace_id)
    , mode(mode)
    , msex(MonsterSex::NONE)
    , method(RaceBlowMethodType::NONE)
{
    this->nightmare = ironman_nightmare && (mode != MONSTER_LORE_DEBUG);
    this->monrace = MonraceList::get_instance().get_monrace_shared(monrace_id);
    this->record = MonraceRecords::get_instance().get_record(monrace_id);
    this->speed = this->nightmare ? this->monrace->speed + 5 : this->monrace->speed;
    this->drop_gold = this->monrace->r_drop_gold;
    this->drop_item = this->monrace->r_drop_item;
    this->ability_flags = (this->monrace->ability_flags & this->monrace->r_ability_flags);
    this->aura_flags = (this->monrace->aura_flags & this->monrace->r_aura_flags);
    this->behavior_flags = (this->monrace->behavior_flags & this->monrace->r_behavior_flags);
    this->drop_flags = (this->monrace->drop_flags & this->monrace->r_drop_flags);
    this->resistance_flags = (this->monrace->resistance_flags & this->monrace->r_resistance_flags);
    this->feature_flags = (this->monrace->feature_flags & this->monrace->r_feature_flags);
    this->brightness_flags = this->monrace->brightness_flags;
    this->special_flags = (this->monrace->special_flags & this->monrace->r_special_flags);
    this->misc_flags = (this->monrace->misc_flags & this->monrace->r_misc_flags);
}

bool lore_type::has_reinforce() const
{
    return this->monrace->has_reinforce();
}

bool lore_type::is_details_known() const
{
    return MonraceList::get_instance().get_monrace(this->monrace_id).is_details_known();
}

bool lore_type::is_blow_damage_known(int num_blow) const
{
    return MonraceList::get_instance().get_monrace(this->monrace_id).is_blow_damage_known(num_blow);
}

term_color_type lore_type::get_speed_color() const
{
    if (this->speed > 199) {
        return TERM_YELLOW;
    } else if (this->speed > 189) {
        return TERM_YELLOW;
    } else if (this->speed > STANDARD_SPEED) {
        if (this->speed > 149) {
            return TERM_VIOLET;
        } else if (this->speed > 144) {
            return TERM_RED;
        } else if (this->speed > 139) {
            return TERM_L_RED;
        } else if (this->speed > 134) {
            return TERM_ORANGE;
        } else if (this->speed > 129) {
            return TERM_UMBER;
        } else if (this->speed > 124) {
            return TERM_L_UMBER;
        } else if (this->speed < 115) {
            return TERM_L_GREEN;
        } else if (this->speed < 120) {
            return TERM_YELLOW;
        } else {
            return TERM_L_RED;
        }
    } else if (this->speed < STANDARD_SPEED) {
        if (this->speed < 86) {
            return TERM_L_GREEN;
        } else if (this->speed < 91) {
            return TERM_GREEN;
        } else if (this->speed < 96) {
            return TERM_L_BLUE;
        } else if (this->speed > 105) {
            return TERM_WHITE;
        } else if (this->speed > 100) {
            return TERM_BLUE;
        } else {
            return TERM_L_BLUE;
        }
    } else {
        return TERM_WHITE;
    }
}

std::vector<lore_msg> lore_type::build_speed_description() const
{
    std::vector<lore_msg> texts;
    const auto speed_color = this->get_speed_color();
#ifdef JP
#else
    texts.emplace_back("moves");
#endif
    const auto random_movement_description = this->build_random_movement_description();
    texts.insert(texts.end(), random_movement_description.begin(), random_movement_description.end());
    if (this->speed > 199) {
        texts.emplace_back(_("光速で", " at light speed"), speed_color);
    } else if (this->speed > 189) {
        texts.emplace_back(_("亜光速で", " at sub-light speed"), speed_color);
    } else if (this->speed > STANDARD_SPEED) {
        if (this->speed > 149) {
            texts.emplace_back(_("信じ難いほど", " incredibly"), speed_color);
        } else if (this->speed > 144) {
            texts.emplace_back(_("音速よりも", " inexplicably"), speed_color);
        } else if (this->speed > 139) {
            texts.emplace_back(_("悪夢的に", " nightmarely"), speed_color);
        } else if (this->speed > 134) {
            texts.emplace_back(_("猛烈に", " extremely"), speed_color);
        } else if (this->speed > 129) {
            texts.emplace_back(_("非常に", " very"), speed_color);
        } else if (this->speed > 124) {
            texts.emplace_back(_("かなり", " fairly"), speed_color);
        } else if (this->speed < 115) {
            texts.emplace_back(_("僅かに", " slightly"), speed_color);
        } else if (this->speed < 120) {
            texts.emplace_back(_("やや", " somewhat"), speed_color);
        }
        texts.emplace_back(_("素早く", " quickly"), speed_color);
    } else if (this->speed < STANDARD_SPEED) {
        if (this->speed < 86) {
            texts.emplace_back(_("信じ難いほど", " incredibly"), speed_color);
        } else if (this->speed < 91) {
            texts.emplace_back(_("非常に", " very"), speed_color);
        } else if (this->speed < 96) {
            texts.emplace_back(_("かなり", " fairly"), speed_color);
        } else if (this->speed > 105) {
            texts.emplace_back(_("僅かに", " slightly"), speed_color);
        } else if (this->speed > 100) {
            texts.emplace_back(_("やや", " somewhat"), speed_color);
        }
        texts.emplace_back(_("ゆっくりと", " slowly"), speed_color);
    } else {
        texts.emplace_back(_("普通の速さで", " at normal speed"), speed_color);
    }

#ifdef JP
    texts.emplace_back("動いている");
#endif
    return texts;
}

tl::optional<std::vector<lore_msg>> lore_type::build_kill_unique_description() const
{
    if (this->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return tl::nullopt;
    }

    std::vector<lore_msg> texts;
    const auto is_dead = this->monrace->is_dead_unique();
    if (this->monrace->r_deaths > 0) {
        constexpr auto fmt = _("%s^はあなたの先祖を %d 人葬っている", "%s^ has slain %d of your ancestors");
        texts.emplace_back(format(fmt, Who::who(this->msex).data(), this->monrace->r_deaths));
        texts.emplace_back(this->build_revenge_description(is_dead));
        texts.emplace_back("\n");
    } else {
#ifdef JP
        const auto mes = is_dead ? "あなたはこの仇敵をすでに葬り去っている。" : "この仇敵はまだ生きている！";
#else
        const auto mes = is_dead ? "You have slain this foe.  " : "This foe is still alive!  ";
#endif
        texts.emplace_back(mes);
        texts.emplace_back("\n");
    }

    return texts;
}

std::string lore_type::build_revenge_description(bool has_defeated) const
{
#ifdef JP
    return has_defeated ? "が、すでに仇討ちは果たしている！" : "のに、まだ仇討ちを果たしていない。";
#else
    if (has_defeated) {
        return format(", but you have avenged %s!  ", Who::whom(this->msex, this->monrace->r_deaths == 1).data());
    }

    std::stringstream ss;
    ss << ", who remain" << (this->monrace->r_deaths != 1 ? "" : "s") << " unavenged.  ";
    return ss.str();
#endif
}

std::vector<lore_msg> lore_type::build_random_movement_description() const
{
    if (this->behavior_flags.has_none_of({ MonsterBehaviorType::RAND_MOVE_50, MonsterBehaviorType::RAND_MOVE_25 })) {
        return {};
    }

    std::vector<lore_msg> texts;
    if (this->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_50) && this->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_25)) {
        texts.emplace_back(_("かなり", " extremely"));
    } else if (this->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_50)) {
        texts.emplace_back(_("幾分", " somewhat"));
    } else if (this->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_25)) {
        texts.emplace_back(_("少々", " a bit"));
    }

    texts.emplace_back(_("不規則に", " erratically"));
    if (this->speed != STANDARD_SPEED) {
        texts.emplace_back(_("、かつ", ", and"));
    }

    return texts;
}

/*!
 * @brief モンスターの思い出メッセージをあらかじめ指定された関数ポインタに基づき出力する
 * @param str 出力文字列
 */
void hooked_roff(std::string_view str)
{
    hook_c_roff(TERM_WHITE, str);
}
