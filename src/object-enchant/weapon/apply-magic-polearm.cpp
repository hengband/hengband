﻿/*!
 * @brief 長柄/斧に耐性等の追加効果を付与する処理
 * @date 2022/03/22
 * @author Hourier
 */

#include "object-enchant/weapon/apply-magic-polearm.h"
#include "floor/floor-base-definitions.h"
#include "inventory/inventory-slot-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/object-type-definition.h"

/*!
 * @brief 長柄/斧強化クラスのコンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
PolearmEnchanter::PolearmEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power)
    : MeleeWeaponEnchanter(player_ptr, o_ptr, level, power)
{
}

void PolearmEnchanter::give_ego_index()
{
    while (true) {
        this->o_ptr->ego_idx = get_random_ego(INVEN_MAIN_HAND, true);
        if ((this->o_ptr->ego_idx == EgoType::SHARPNESS) || (this->o_ptr->ego_idx == EgoType::EARTHQUAKES)) {
            continue;
        }

        break;
    }
}

void PolearmEnchanter::give_cursed()
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

MeleeWeaponEnchantFlags PolearmEnchanter::enchant_flags() const
{
    switch (this->o_ptr->sval) {
    case SV_DEATH_SCYTHE:
        return { MeleeWeaponEnchantType::ONLY_MUNDANITY };

    default:
        return MELEE_WEAPON_NORMAL_ENCHANT_FLAGS;
    }
}
