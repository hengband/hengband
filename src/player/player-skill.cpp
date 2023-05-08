#include "player/player-skill.h"
#include "core/player-update-types.h"
#include "monster-race/monster-race.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player/player-realm.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/* Proficiency of weapons and misc. skills (except riding) */
constexpr SUB_EXP WEAPON_EXP_UNSKILLED = 0;
constexpr SUB_EXP WEAPON_EXP_BEGINNER = 4000;
constexpr SUB_EXP WEAPON_EXP_SKILLED = 6000;
constexpr SUB_EXP WEAPON_EXP_EXPERT = 7000;
constexpr SUB_EXP WEAPON_EXP_MASTER = 8000;

/* Proficiency of riding */
constexpr SUB_EXP RIDING_EXP_UNSKILLED = 0;
constexpr SUB_EXP RIDING_EXP_BEGINNER = 500;
constexpr SUB_EXP RIDING_EXP_SKILLED = 2000;
constexpr SUB_EXP RIDING_EXP_EXPERT = 5000;
constexpr SUB_EXP RIDING_EXP_MASTER = 8000;

/* Proficiency of spells */
constexpr SUB_EXP SPELL_EXP_UNSKILLED = 0;
constexpr SUB_EXP SPELL_EXP_BEGINNER = 900;
constexpr SUB_EXP SPELL_EXP_SKILLED = 1200;
constexpr SUB_EXP SPELL_EXP_EXPERT = 1400;
constexpr SUB_EXP SPELL_EXP_MASTER = 1600;

/*
 * The skill table
 */
std::vector<skill_table> class_skills_info;

namespace {

using GainAmountList = std::array<int, enum2i(PlayerSkillRank::MASTER)>;

void gain_attack_skill_exp(PlayerType *player_ptr, short &exp, const GainAmountList &gain_amount_list)
{
    auto gain_amount = 0;
    auto calc_gain_amount = [&gain_amount_list, exp](PlayerSkillRank rank, int next_rank_exp) {
        return std::min(gain_amount_list[enum2i(rank)], next_rank_exp - exp);
    };

    if (exp < WEAPON_EXP_BEGINNER) {
        gain_amount = calc_gain_amount(PlayerSkillRank::UNSKILLED, WEAPON_EXP_BEGINNER);
    } else if (exp < WEAPON_EXP_SKILLED) {
        gain_amount = calc_gain_amount(PlayerSkillRank::BEGINNER, WEAPON_EXP_SKILLED);
    } else if ((exp < WEAPON_EXP_EXPERT) && (player_ptr->lev > 19)) {
        gain_amount = calc_gain_amount(PlayerSkillRank::SKILLED, WEAPON_EXP_EXPERT);
    } else if ((exp < WEAPON_EXP_MASTER) && (player_ptr->lev > 34)) {
        gain_amount = calc_gain_amount(PlayerSkillRank::EXPERT, WEAPON_EXP_MASTER);
    }

    exp += static_cast<short>(gain_amount);
    set_bits(player_ptr->update, PU_BONUS);
}

void gain_spell_skill_exp_aux(PlayerType *player_ptr, short &exp, const GainAmountList &gain_amount_list, int spell_level)
{
    const auto dlev = player_ptr->current_floor_ptr->dun_level;
    const auto plev = player_ptr->lev;

    auto gain_amount = 0;
    auto calc_gain_amount = [&gain_amount_list, exp](PlayerSkillRank rank, int next_rank_exp) {
        return std::min(gain_amount_list[enum2i(rank)], next_rank_exp - exp);
    };

    if (exp < SPELL_EXP_BEGINNER) {
        gain_amount = calc_gain_amount(PlayerSkillRank::UNSKILLED, SPELL_EXP_BEGINNER);
    } else if (exp < SPELL_EXP_SKILLED) {
        if ((dlev > 4) && ((dlev + 10) > plev)) {
            gain_amount = calc_gain_amount(PlayerSkillRank::BEGINNER, SPELL_EXP_SKILLED);
        }
    } else if (exp < SPELL_EXP_EXPERT) {
        if (((dlev + 5) > plev) && ((dlev + 5) > spell_level)) {
            gain_amount = calc_gain_amount(PlayerSkillRank::SKILLED, SPELL_EXP_EXPERT);
        }
    } else if (exp < SPELL_EXP_MASTER) {
        if (((dlev + 5) > plev) && (dlev > spell_level)) {
            gain_amount = calc_gain_amount(PlayerSkillRank::EXPERT, SPELL_EXP_MASTER);
        }
    }

    exp += static_cast<short>(gain_amount);
    set_bits(player_ptr->update, PU_BONUS);
}

}

PlayerSkill::PlayerSkill(PlayerType *player_ptr)
    : player_ptr(player_ptr)
{
}

SUB_EXP PlayerSkill::weapon_exp_at(PlayerSkillRank rank)
{
    switch (rank) {
    case PlayerSkillRank::UNSKILLED:
        return WEAPON_EXP_UNSKILLED;
    case PlayerSkillRank::BEGINNER:
        return WEAPON_EXP_BEGINNER;
    case PlayerSkillRank::SKILLED:
        return WEAPON_EXP_SKILLED;
    case PlayerSkillRank::EXPERT:
        return WEAPON_EXP_EXPERT;
    case PlayerSkillRank::MASTER:
        return WEAPON_EXP_MASTER;
    }

    return WEAPON_EXP_UNSKILLED;
}

SUB_EXP PlayerSkill::spell_exp_at(PlayerSkillRank rank)
{
    switch (rank) {
    case PlayerSkillRank::UNSKILLED:
        return SPELL_EXP_UNSKILLED;
    case PlayerSkillRank::BEGINNER:
        return SPELL_EXP_BEGINNER;
    case PlayerSkillRank::SKILLED:
        return SPELL_EXP_SKILLED;
    case PlayerSkillRank::EXPERT:
        return SPELL_EXP_EXPERT;
    case PlayerSkillRank::MASTER:
        return SPELL_EXP_MASTER;
    }

    return SPELL_EXP_UNSKILLED;
}
/*!
 * @brief 武器や各種スキル（騎乗以外）の抽象的表現ランクを返す。 /  Return proficiency level of weapons and misc. skills (except riding)
 * @param weapon_exp 経験値
 * @return ランク値
 */
PlayerSkillRank PlayerSkill::weapon_skill_rank(int weapon_exp)
{
    if (weapon_exp < WEAPON_EXP_BEGINNER) {
        return PlayerSkillRank::UNSKILLED;
    } else if (weapon_exp < WEAPON_EXP_SKILLED) {
        return PlayerSkillRank::BEGINNER;
    } else if (weapon_exp < WEAPON_EXP_EXPERT) {
        return PlayerSkillRank::SKILLED;
    } else if (weapon_exp < WEAPON_EXP_MASTER) {
        return PlayerSkillRank::EXPERT;
    } else {
        return PlayerSkillRank::MASTER;
    }
}

bool PlayerSkill::valid_weapon_exp(int weapon_exp)
{
    return (WEAPON_EXP_UNSKILLED <= weapon_exp) && (weapon_exp <= WEAPON_EXP_MASTER);
}

/*!
 * @brief 騎乗スキルの抽象的ランクを返す。 / Return proficiency level of riding
 * @param riding_exp 経験値
 * @return ランク値
 */
PlayerSkillRank PlayerSkill::riding_skill_rank(int riding_exp)
{
    if (riding_exp < RIDING_EXP_BEGINNER) {
        return PlayerSkillRank::UNSKILLED;
    } else if (riding_exp < RIDING_EXP_SKILLED) {
        return PlayerSkillRank::BEGINNER;
    } else if (riding_exp < RIDING_EXP_EXPERT) {
        return PlayerSkillRank::SKILLED;
    } else if (riding_exp < RIDING_EXP_MASTER) {
        return PlayerSkillRank::EXPERT;
    } else {
        return PlayerSkillRank::MASTER;
    }
}

/*!
 * @brief プレイヤーの呪文レベルの抽象的ランクを返す。 / Return proficiency level of spells
 * @param spell_exp 経験値
 * @return ランク値
 */
PlayerSkillRank PlayerSkill::spell_skill_rank(int spell_exp)
{
    if (spell_exp < SPELL_EXP_BEGINNER) {
        return PlayerSkillRank::UNSKILLED;
    } else if (spell_exp < SPELL_EXP_SKILLED) {
        return PlayerSkillRank::BEGINNER;
    } else if (spell_exp < SPELL_EXP_EXPERT) {
        return PlayerSkillRank::SKILLED;
    } else if (spell_exp < SPELL_EXP_MASTER) {
        return PlayerSkillRank::EXPERT;
    } else {
        return PlayerSkillRank::MASTER;
    }
}

concptr PlayerSkill::skill_name(PlayerSkillKindType skill)
{
    switch (skill) {
    case PlayerSkillKindType::MARTIAL_ARTS:
        return _("マーシャルアーツ", "Martial Arts");
    case PlayerSkillKindType::TWO_WEAPON:
        return _("二刀流", "Dual Wielding");
    case PlayerSkillKindType::RIDING:
        return _("乗馬", "Riding");
    case PlayerSkillKindType::SHIELD:
        return _("盾", "Shield");
    case PlayerSkillKindType::MAX:
        break;
    }

    return _("不明", "Unknown");
}

concptr PlayerSkill::skill_rank_str(PlayerSkillRank rank)
{
    switch (rank) {
    case PlayerSkillRank::UNSKILLED:
        return _("[初心者]", "[Unskilled]");
    case PlayerSkillRank::BEGINNER:
        return _("[入門者]", "[Beginner]");
    case PlayerSkillRank::SKILLED:
        return _("[熟練者]", "[Skilled]");
    case PlayerSkillRank::EXPERT:
        return _("[エキスパート]", "[Expert]");
    case PlayerSkillRank::MASTER:
        return _("[達人]", "[Master]");
    }

    return _("[不明]", "[Unknown]");
}

void PlayerSkill::gain_melee_weapon_exp(const ItemEntity *o_ptr)
{
    const GainAmountList gain_amount_list{ { 80, 10, 1, (one_in_(2) ? 1 : 0) } };
    constexpr GainAmountList others_gain_amount_list{ { 8, 1, 0, 0 } };
    const auto tval = o_ptr->bi_key.tval();
    for (auto sval = 0U; sval < this->player_ptr->weapon_exp[tval].size(); ++sval) {
        auto &now_exp = this->player_ptr->weapon_exp[tval][sval];
        if (now_exp < this->player_ptr->weapon_exp_max[tval][sval]) {
            gain_attack_skill_exp(this->player_ptr, now_exp,
                (static_cast<int>(sval) == o_ptr->bi_key.sval()) ? gain_amount_list : others_gain_amount_list);
        }
    }
}

void PlayerSkill::gain_range_weapon_exp(const ItemEntity *o_ptr)
{
    constexpr GainAmountList gain_amount_list{ { 80, 25, 10, 2 } };
    constexpr GainAmountList others_gain_amount_list{ { 8, 2, 0, 0 } };
    const auto tval = o_ptr->bi_key.tval();
    for (auto sval = 0U; sval < this->player_ptr->weapon_exp[tval].size(); ++sval) {
        auto &now_exp = this->player_ptr->weapon_exp[tval][sval];
        if (now_exp < this->player_ptr->weapon_exp_max[tval][sval]) {
            gain_attack_skill_exp(this->player_ptr, now_exp,
                (static_cast<int>(sval) == o_ptr->bi_key.sval()) ? gain_amount_list : others_gain_amount_list);
        }
    }
}

void PlayerSkill::gain_martial_arts_skill_exp()
{
    if (this->player_ptr->skill_exp[PlayerSkillKindType::MARTIAL_ARTS] < class_skills_info[enum2i(this->player_ptr->pclass)].s_max[PlayerSkillKindType::MARTIAL_ARTS]) {
        const GainAmountList gain_amount_list{ 40, 5, 1, (one_in_(3) ? 1 : 0) };
        gain_attack_skill_exp(this->player_ptr, this->player_ptr->skill_exp[PlayerSkillKindType::MARTIAL_ARTS], gain_amount_list);
    }
}

void PlayerSkill::gain_two_weapon_skill_exp()
{
    if (this->player_ptr->skill_exp[PlayerSkillKindType::TWO_WEAPON] < class_skills_info[enum2i(this->player_ptr->pclass)].s_max[PlayerSkillKindType::TWO_WEAPON]) {
        const GainAmountList gain_amount_list{ 80, 4, 1, (one_in_(3) ? 1 : 0) };
        gain_attack_skill_exp(this->player_ptr, this->player_ptr->skill_exp[PlayerSkillKindType::TWO_WEAPON], gain_amount_list);
    }
}

void PlayerSkill::gain_riding_skill_exp_on_melee_attack(const MonsterRaceInfo *r_ptr)
{
    auto now_exp = this->player_ptr->skill_exp[PlayerSkillKindType::RIDING];
    auto max_exp = class_skills_info[enum2i(this->player_ptr->pclass)].s_max[PlayerSkillKindType::RIDING];
    if (now_exp >= max_exp) {
        return;
    }

    auto riding_level = monraces_info[this->player_ptr->current_floor_ptr->m_list[this->player_ptr->riding].r_idx].level;
    int inc = 0;

    if ((now_exp / 200 - 5) < r_ptr->level) {
        inc += 1;
    }

    if ((now_exp / 100) < riding_level) {
        if ((now_exp / 100 + 15) < riding_level) {
            inc += 1 + (riding_level - (now_exp / 100 + 15));
        } else {
            inc += 1;
        }
    }

    this->player_ptr->skill_exp[PlayerSkillKindType::RIDING] = std::min<SUB_EXP>(max_exp, now_exp + inc);
    set_bits(this->player_ptr->update, PU_BONUS);
}

void PlayerSkill::gain_riding_skill_exp_on_range_attack()
{
    auto now_exp = this->player_ptr->skill_exp[PlayerSkillKindType::RIDING];
    auto max_exp = class_skills_info[enum2i(this->player_ptr->pclass)].s_max[PlayerSkillKindType::RIDING];
    if (now_exp >= max_exp) {
        return;
    }

    const auto *floor_ptr = this->player_ptr->current_floor_ptr;
    const auto &monster = floor_ptr->m_list[this->player_ptr->riding];
    const auto &monrace = monraces_info[monster.r_idx];
    if (((this->player_ptr->skill_exp[PlayerSkillKindType::RIDING] - (RIDING_EXP_BEGINNER * 2)) / 200 < monrace.level) && one_in_(2)) {
        this->player_ptr->skill_exp[PlayerSkillKindType::RIDING] += 1;
        set_bits(this->player_ptr->update, PU_BONUS);
    }
}

void PlayerSkill::gain_riding_skill_exp_on_fall_off_check(int dam)
{
    auto now_exp = this->player_ptr->skill_exp[PlayerSkillKindType::RIDING];
    auto max_exp = class_skills_info[enum2i(this->player_ptr->pclass)].s_max[PlayerSkillKindType::RIDING];
    if (now_exp >= max_exp || max_exp <= 1000) {
        return;
    }

    auto riding_level = monraces_info[this->player_ptr->current_floor_ptr->m_list[this->player_ptr->riding].r_idx].level;

    if ((dam / 2 + riding_level) <= (now_exp / 30 + 10)) {
        return;
    }

    int inc = 0;
    if ((now_exp / 100 + 15) < riding_level) {
        inc += 1 + (riding_level - (now_exp / 100 + 15));
    } else {
        inc += 1;
    }

    this->player_ptr->skill_exp[PlayerSkillKindType::RIDING] = std::min<SUB_EXP>(max_exp, now_exp + inc);
    set_bits(this->player_ptr->update, PU_BONUS);
}

void PlayerSkill::gain_spell_skill_exp(int realm, int spell_idx)
{
    if ((realm < 1) || ((static_cast<int>(std::size(mp_ptr->info)) < realm) && (realm != REALM_MUSIC) && (realm != REALM_HEX))) {
        return;
    }

    if (((spell_idx < 0) || (32 <= spell_idx)) ||
        ((realm != this->player_ptr->realm1) && (realm != this->player_ptr->realm2))) {
        return;
    }

    constexpr GainAmountList gain_amount_list_first{ { 60, 8, 2, 1 } };
    constexpr GainAmountList gain_amount_list_second{ { 60, 8, 2, 0 } };

    const auto is_first_realm = (realm == this->player_ptr->realm1);
    const auto *s_ptr = &mp_ptr->info[realm - 1][spell_idx];

    gain_spell_skill_exp_aux(this->player_ptr, this->player_ptr->spell_exp[spell_idx + (is_first_realm ? 0 : 32)],
        (is_first_realm ? gain_amount_list_first : gain_amount_list_second), s_ptr->slevel);
}

void PlayerSkill::gain_continuous_spell_skill_exp(int realm, int spell_idx)
{
    if (((spell_idx < 0) || (32 <= spell_idx)) ||
        ((realm != REALM_MUSIC) && (realm != REALM_HEX))) {
        return;
    }

    const auto *s_ptr = &technic_info[realm - MIN_TECHNIC][spell_idx];

    const GainAmountList gain_amount_list{ 5, (one_in_(2) ? 1 : 0), (one_in_(5) ? 1 : 0), (one_in_(5) ? 1 : 0) };

    gain_spell_skill_exp_aux(this->player_ptr, this->player_ptr->spell_exp[spell_idx], gain_amount_list, s_ptr->slevel);
}

PlayerSkillRank PlayerSkill::gain_spell_skill_exp_over_learning(int spell_idx)
{
    if ((spell_idx < 0) || (static_cast<int>(std::size(this->player_ptr->spell_exp)) <= spell_idx)) {
        return PlayerSkillRank::UNSKILLED;
    }

    auto &exp = this->player_ptr->spell_exp[spell_idx];

    if (exp >= SPELL_EXP_EXPERT) {
        exp = SPELL_EXP_MASTER;
    } else if (exp >= SPELL_EXP_SKILLED) {
        if (spell_idx >= 32) {
            exp = SPELL_EXP_EXPERT;
        } else {
            exp += SPELL_EXP_EXPERT - SPELL_EXP_SKILLED;
        }
    } else if (exp >= SPELL_EXP_BEGINNER) {
        exp = SPELL_EXP_SKILLED + (exp - SPELL_EXP_BEGINNER) * 2 / 3;
    } else {
        exp = SPELL_EXP_BEGINNER + exp / 3;
    }

    set_bits(this->player_ptr->update, PU_BONUS);

    return PlayerSkill::spell_skill_rank(exp);
}

/*!
 * @brief 呪文の経験値を返す /
 * Returns experience of a spell
 * @param use_realm 魔法領域
 * @param spell_idx 呪文ID
 * @return 経験値
 */
EXP PlayerSkill::exp_of_spell(int realm, int spell_idx) const
{
    PlayerClass pc(this->player_ptr);
    if (pc.equals(PlayerClassType::SORCERER)) {
        return SPELL_EXP_MASTER;
    } else if (pc.equals(PlayerClassType::RED_MAGE)) {
        return SPELL_EXP_SKILLED;
    } else if (realm == this->player_ptr->realm1) {
        return this->player_ptr->spell_exp[spell_idx];
    } else if (realm == this->player_ptr->realm2) {
        return this->player_ptr->spell_exp[spell_idx + 32];
    } else {
        return 0;
    }
}

/*!
 * @brief 特別な武器スキル最大値の適用を行う
 * 性格セクシーギャルの場合ムチスキルの最大値が達人になる
 * 種族マーフォークの場合三叉槍とトライデントのスキルが達人になる
 * （但し、いずれも職業がスペルマスターではない場合に限る）
 */
void PlayerSkill::apply_special_weapon_skill_max_values()
{
    this->player_ptr->weapon_exp_max = class_skills_info[enum2i(this->player_ptr->pclass)].w_max;
    if (PlayerClass(this->player_ptr).equals(PlayerClassType::SORCERER)) {
        return;
    }

    auto &w_exp_max = this->player_ptr->weapon_exp_max;
    if (this->player_ptr->ppersonality == PERSONALITY_SEXY) {
        w_exp_max[ItemKindType::HAFTED][SV_WHIP] = PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER);
    }

    if (PlayerRace(player_ptr).equals(PlayerRaceType::MERFOLK)) {
        w_exp_max[ItemKindType::POLEARM][SV_TRIDENT] = PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER);
        w_exp_max[ItemKindType::POLEARM][SV_TRIFURCATE_SPEAR] = PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER);
    }
}

/*!
 * @brief 武器スキル経験値を最大値で制限する
 */
void PlayerSkill::limit_weapon_skills_by_max_value()
{
    for (auto tval : TV_WEAPON_RANGE) {
        auto &exp_table = this->player_ptr->weapon_exp[tval];
        const auto &max_exp_table = this->player_ptr->weapon_exp_max[tval];
        for (auto i = 0U; i < exp_table.size(); ++i) {
            exp_table[i] = std::min(exp_table[i], max_exp_table[i]);
        }
    }
}
