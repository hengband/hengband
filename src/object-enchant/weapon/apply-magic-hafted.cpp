/*!
 * @brief 鈍器に耐性等の追加効果を付与する処理
 * @date 2022/03/22
 * @author Hourier
 */

#include "object-enchant/weapon/apply-magic-hafted.h"
#include "floor/floor-base-definitions.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-boost.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/object-type-definition.h"

/*!
 * @brief 鈍器強化クラスのコンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
HaftedEnchanter::HaftedEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power)
    : MeleeWeaponEnchanter(player_ptr, o_ptr, level, power)
{
}

void HaftedEnchanter::apply_magic()
{
    this->decide_skip();
    MeleeWeaponEnchanter::apply_magic();
}

void HaftedEnchanter::give_ego_index()
{
    while (true) {
        this->o_ptr->ego_idx = get_random_ego(INVEN_MAIN_HAND, true);
        if (this->o_ptr->ego_idx == EgoType::SHARPNESS) {
            continue;
        }

        break;
    }

    switch (this->o_ptr->ego_idx) {
    case EgoType::EARTHQUAKES:
        if (one_in_(3) && (this->level > 60)) {
            this->o_ptr->art_flags.set(TR_BLOWS);
        } else {
            this->o_ptr->pval = static_cast<short>(m_bonus(3, this->level));
        }

        break;
    default:
        break;
    }
}

void HaftedEnchanter::give_cursed()
{
    if (randint0(MAX_DEPTH) >= this->level) {
        return;
    }

    while (true) {
        this->o_ptr->ego_idx = get_random_ego(INVEN_MAIN_HAND, false);
        if (this->o_ptr->ego_idx == EgoType::WEIRD) {
            continue;
        }

        return;
    }
}
