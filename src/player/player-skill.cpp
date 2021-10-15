#include "player/player-skill.h"
#include "core/player-update-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/* Proficiency of weapons and misc. skills (except riding) */
constexpr SUB_EXP WEAPON_EXP_UNSKILLED = 0;
constexpr SUB_EXP WEAPON_EXP_BEGINNER = 4000;
constexpr SUB_EXP WEAPON_EXP_SKILLED = 6000;
constexpr SUB_EXP WEAPON_EXP_EXPERT = 7000;
constexpr SUB_EXP WEAPON_EXP_MASTER = 8000;

/*
 * The skill table
 */
std::vector<skill_table> s_info;

/*!
 * @brief 技能値到達表記テーブル
 */
const concptr exp_level_str[5] =
#ifdef JP
    { "[初心者]", "[入門者]", "[熟練者]", "[エキスパート]", "[達人]" };
#else
    { "[Unskilled]", "[Beginner]", "[Skilled]", "[Expert]", "[Master]" };
#endif

PlayerSkill::PlayerSkill(player_type *player_ptr)
    : player_ptr(player_ptr)
{
}

SUB_EXP PlayerSkill::weapon_exp_at(int level)
{
    switch (level) {
    case EXP_LEVEL_UNSKILLED:
        return WEAPON_EXP_UNSKILLED;
    case EXP_LEVEL_BEGINNER:
        return WEAPON_EXP_BEGINNER;
    case EXP_LEVEL_SKILLED:
        return WEAPON_EXP_SKILLED;
    case EXP_LEVEL_EXPERT:
        return WEAPON_EXP_EXPERT;
    case EXP_LEVEL_MASTER:
        return WEAPON_EXP_MASTER;
    }

    return WEAPON_EXP_UNSKILLED;
}

/*!
 * @brief 武器や各種スキル（騎乗以外）の抽象的表現ランクを返す。 /  Return proficiency level of weapons and misc. skills (except riding)
 * @param weapon_exp 経験値
 * @return ランク値
 */
int PlayerSkill::weapon_exp_level(int weapon_exp)
{
    if (weapon_exp < WEAPON_EXP_BEGINNER)
        return EXP_LEVEL_UNSKILLED;
    else if (weapon_exp < WEAPON_EXP_SKILLED)
        return EXP_LEVEL_BEGINNER;
    else if (weapon_exp < WEAPON_EXP_EXPERT)
        return EXP_LEVEL_SKILLED;
    else if (weapon_exp < WEAPON_EXP_MASTER)
        return EXP_LEVEL_EXPERT;
    else
        return EXP_LEVEL_MASTER;
}

bool PlayerSkill::valid_weapon_exp(int weapon_exp)
{
    return (WEAPON_EXP_UNSKILLED <= weapon_exp) && (weapon_exp <= WEAPON_EXP_MASTER);
}

void PlayerSkill::gain_melee_weapon_exp(const object_type *o_ptr)
{
    auto now_exp = this->player_ptr->weapon_exp[o_ptr->tval][o_ptr->sval];
    if (now_exp >= s_info[enum2i(this->player_ptr->pclass)].w_max[o_ptr->tval][o_ptr->sval])
        return;

    SUB_EXP amount = 0;
    if (now_exp < WEAPON_EXP_BEGINNER)
        amount = 80;
    else if (now_exp < WEAPON_EXP_SKILLED)
        amount = 10;
    else if ((now_exp < WEAPON_EXP_EXPERT) && (this->player_ptr->lev > 19))
        amount = 1;
    else if ((player_ptr->lev > 34) && one_in_(2))
        amount = 1;

    this->player_ptr->weapon_exp[o_ptr->tval][o_ptr->sval] += amount;
    set_bits(this->player_ptr->update, PU_BONUS);
}

void PlayerSkill::gain_range_weapon_exp(const object_type *o_ptr)
{
    auto now_exp = this->player_ptr->weapon_exp[o_ptr->tval][o_ptr->sval];
    if (now_exp >= s_info[enum2i(this->player_ptr->pclass)].w_max[o_ptr->tval][o_ptr->sval])
        return;

    SUB_EXP amount = 0;
    if (now_exp < WEAPON_EXP_BEGINNER)
        amount = 80;
    else if (now_exp < WEAPON_EXP_SKILLED)
        amount = 25;
    else if ((now_exp < WEAPON_EXP_EXPERT) && (this->player_ptr->lev > 19))
        amount = 10;
    else if (this->player_ptr->lev > 34)
        amount = 2;

    this->player_ptr->weapon_exp[o_ptr->tval][o_ptr->sval] += amount;
    set_bits(this->player_ptr->update, PU_BONUS);
}

void PlayerSkill::gain_martial_arts_skill_exp()
{
    auto now_exp = this->player_ptr->skill_exp[SKILL_MARTIAL_ARTS];
    if (now_exp >= s_info[enum2i(this->player_ptr->pclass)].s_max[SKILL_MARTIAL_ARTS])
        return;

    SUB_EXP amount = 0;
    if (now_exp < WEAPON_EXP_BEGINNER)
        amount = 40;
    else if (now_exp < WEAPON_EXP_SKILLED)
        amount = 5;
    else if ((now_exp < WEAPON_EXP_EXPERT) && (this->player_ptr->lev > 19))
        amount = 1;
    else if ((this->player_ptr->lev > 34) && one_in_(3))
        amount = 1;

    this->player_ptr->skill_exp[SKILL_MARTIAL_ARTS] += amount;
    set_bits(this->player_ptr->update, PU_BONUS);
}

void PlayerSkill::gain_two_weapon_skill_exp()
{
    auto now_exp = this->player_ptr->skill_exp[SKILL_TWO_WEAPON];
    if (now_exp >= s_info[enum2i(this->player_ptr->pclass)].s_max[SKILL_TWO_WEAPON])
        return;

    SUB_EXP amount = 0;
    if (now_exp < WEAPON_EXP_BEGINNER)
        amount = 80;
    else if (now_exp < WEAPON_EXP_SKILLED)
        amount = 4;
    else if ((now_exp < WEAPON_EXP_EXPERT) && (this->player_ptr->lev > 19))
        amount = 1;
    else if ((this->player_ptr->lev > 34) && one_in_(3))
        amount = 1;

    this->player_ptr->skill_exp[SKILL_TWO_WEAPON] += amount;
    set_bits(this->player_ptr->update, PU_BONUS);
}
