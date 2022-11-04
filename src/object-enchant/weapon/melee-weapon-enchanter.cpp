/*!
 * @brief 剣・鈍器・長柄/斧武器に耐性等の追加効果を付与する処理
 * @date 2022/03/22
 * @author Hourier
 */

#include "object-enchant/weapon/melee-weapon-enchanter.h"
#include "artifact/random-art-generator.h"
#include "system/object-type-definition.h"

MeleeWeaponEnchanter::MeleeWeaponEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power)
    : AbstractWeaponEnchanter(o_ptr, level, power)
    , player_ptr(player_ptr)
{
}

/*!
 * @brief 打撃系オブジェクトに生成ランクごとの強化を与えるサブルーチン
 * @details power > 2はデバッグ専用.
 */
void MeleeWeaponEnchanter::apply_magic()
{
    const auto enchant_flags = this->enchant_flags();
    if (this->power == 0 || enchant_flags.has(MeleeWeaponEnchantType::ONLY_MUNDANITY)) {
        return;
    }

    if (enchant_flags.has(MeleeWeaponEnchantType::MOD_SLAYING_BONUS)) {
        this->give_killing_bonus();
    }

    if (this->power > 1) {
        this->strengthen();
        return;
    }

    if (enchant_flags.has(MeleeWeaponEnchantType::ENABLE_BECOME_EGO) && (this->power < -1)) {
        this->give_cursed();
    }
}

/*!
 * @brief エゴもしくはランダムアーティファクト生成・ダイス強化処理
 * @details power > 2はデバッグ専用.
 */
void MeleeWeaponEnchanter::strengthen()
{
    const auto enchant_flags = this->enchant_flags();

    if (enchant_flags.has(MeleeWeaponEnchantType::ENABLE_BECOME_RANDOM_ARTIFACT) && ((this->power > 2) || one_in_(40))) {
        become_random_artifact(this->player_ptr, this->o_ptr, false);
        return;
    }

    if (enchant_flags.has(MeleeWeaponEnchantType::ENABLE_BECOME_EGO)) {
        this->give_ego_index();
    }

    if (this->o_ptr->art_name > 0) {
        return;
    }

    while (one_in_(10 * this->o_ptr->dd * this->o_ptr->ds)) {
        this->o_ptr->dd++;
    }

    if (this->o_ptr->dd > 9) {
        this->o_ptr->dd = 9;
    }
}
