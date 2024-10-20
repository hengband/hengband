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

std::vector<lore_msg> lore_type::build_speed_description() const
{
    std::vector<lore_msg> texts;
#ifdef JP
#else
    texts.emplace_back("moves");
#endif
    const auto random_movement_description = this->build_random_movement_description();
    texts.insert(texts.end(), random_movement_description.begin(), random_movement_description.end());
    if (this->speed > STANDARD_SPEED) {
        if (this->speed > 139) {
            texts.emplace_back(_("信じ難いほど", " incredibly"), TERM_RED);
        } else if (this->speed > 134) {
            texts.emplace_back(_("猛烈に", " extremely"), TERM_ORANGE);
        } else if (this->speed > 129) {
            texts.emplace_back(_("非常に", " very"), TERM_ORANGE);
        } else if (this->speed > 124) {
            texts.emplace_back(_("かなり", " fairly"), TERM_UMBER);
        } else if (this->speed < 120) {
            texts.emplace_back(_("やや", " somewhat"), TERM_L_UMBER);
        }

        texts.emplace_back(_("素早く", " quickly"), TERM_L_RED);
    } else if (this->speed < STANDARD_SPEED) {
        if (this->speed < 90) {
            texts.emplace_back(_("信じ難いほど", " incredibly"), TERM_L_GREEN);
        } else if (this->speed < 95) {
            texts.emplace_back(_("非常に", " very"), TERM_BLUE);
        } else if (this->speed < 100) {
            texts.emplace_back(_("かなり", " fairly"), TERM_BLUE);
        } else if (this->speed > 104) {
            texts.emplace_back(_("やや", " somewhat"), TERM_GREEN);
        }

        texts.emplace_back(_("ゆっくりと", " slowly"), TERM_L_BLUE);
    } else {
        texts.emplace_back(_("普通の速さで", " at normal speed"));
    }

#ifdef JP
    texts.emplace_back("動いている");
#endif
    return texts;
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
