#include "lore/lore-util.h"
#include "game-option/birth-options.h"
#include "locale/language-switcher.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-race/race-sex.h"
#include "system/monster-race-info.h"
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

ColoredString::ColoredString(const std::string &text, TERM_COLOR color)
    : text(text)
    , color(color)
{
}

lore_type::lore_type(MonsterRaceId r_idx, monster_lore_mode mode)
    : r_idx(r_idx)
    , mode(mode)
    , msex(MonsterSex::NONE)
    , method(RaceBlowMethodType::NONE)
{
    this->nightmare = ironman_nightmare && (mode != MONSTER_LORE_DEBUG);
    this->r_ptr = &monraces_info[r_idx];
    this->speed = this->nightmare ? this->r_ptr->speed + 5 : this->r_ptr->speed;
    this->drop_gold = this->r_ptr->r_drop_gold;
    this->drop_item = this->r_ptr->r_drop_item;
    this->ability_flags = (this->r_ptr->ability_flags & this->r_ptr->r_ability_flags);
    this->aura_flags = (this->r_ptr->aura_flags & this->r_ptr->r_aura_flags);
    this->behavior_flags = (this->r_ptr->behavior_flags & this->r_ptr->r_behavior_flags);
    this->drop_flags = (this->r_ptr->drop_flags & this->r_ptr->r_drop_flags);
    this->resistance_flags = (this->r_ptr->resistance_flags & this->r_ptr->r_resistance_flags);
    this->feature_flags = (this->r_ptr->feature_flags & this->r_ptr->r_feature_flags);
    this->brightness_flags = this->r_ptr->brightness_flags;
    this->special_flags = (this->r_ptr->special_flags & this->r_ptr->r_special_flags);
    this->misc_flags = (this->r_ptr->misc_flags & this->r_ptr->r_misc_flags);
}

bool lore_type::has_reinforce() const
{
    return this->r_ptr->has_reinforce();
}

std::vector<ColoredString> lore_type::build_speed() const
{
    std::vector<ColoredString> texts;
#ifdef JP
#else
    texts.emplace_back(ColoredString{ "moves" });
#endif
    if (this->speed > STANDARD_SPEED) {
        if (this->speed > 149) {
            texts.emplace_back(ColoredString{ _("信じ難いほど", " incredibly"), TERM_VIOLET });
        } else if (this->speed > 144) {
            texts.emplace_back(ColoredString{ _("音速よりも", " inexplicably"), TERM_RED });
        } else if (this->speed > 139) {
            texts.emplace_back(ColoredString{ _("悪夢的に", " nightmarely"), TERM_L_RED });
        } else if (this->speed > 134) {
            texts.emplace_back(ColoredString{ _("猛烈に", " extremely"), TERM_ORANGE });
        } else if (this->speed > 129) {
            texts.emplace_back(ColoredString{ _("非常に", " very"), TERM_UMBER });
        } else if (this->speed > 124) {
            texts.emplace_back(ColoredString{ _("かなり", " fairly"), TERM_L_UMBER });
        } else if (this->speed < 115) {
            texts.emplace_back(ColoredString{ _("僅かに", " slightly"), TERM_L_GREEN });
        } else if (this->speed < 120) {
            texts.emplace_back(ColoredString{ _("やや", " somewhat") });
        }

        texts.emplace_back(ColoredString{ _("素早く", " quickly"), TERM_L_RED });
    } else if (this->speed < STANDARD_SPEED) {
        if (this->speed < 85) {
            texts.emplace_back(ColoredString{ _("信じ難いほど", " incredibly"), TERM_L_GREEN });
        } else if (this->speed < 90) {
            texts.emplace_back(ColoredString{ _("非常に", " very"), TERM_GREEN });
        } else if (this->speed < 95) {
            texts.emplace_back(ColoredString{ _("かなり", " fairly"), TERM_L_BLUE });
        } else if (this->speed < 100) {
            texts.emplace_back(ColoredString{ _("やや", " somewhat"), TERM_BLUE });
        } else if (this->speed > 104) {
            texts.emplace_back(ColoredString{ _("僅かに", " slightly") });
        }

        texts.emplace_back(ColoredString{ _("ゆっくりと", " slowly"), TERM_L_BLUE });
    } else {
        texts.emplace_back(ColoredString{ _("普通の速さで", " at normal speed") });
    }

#ifdef JP
    texts.emplace_back(ColoredString{ "動いている" });
#endif
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
