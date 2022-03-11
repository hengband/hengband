/*!
 * @brief 武器系のアイテムを強化する処理
 * @date 2022/01/30
 * @author Hourier
 */

#include "object-enchant/weapon/apply-magic-weapon.h"
#include "artifact/random-art-generator.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object/tval-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 武器強化クラスのコンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
WeaponEnchanter::WeaponEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power)
    : AbstractWeaponEnchanter(o_ptr, level, power)
    , player_ptr(player_ptr)
{
}

/*!
 * @brief 武器系オブジェクトに生成ランクごとの強化を与えるサブルーチン
 * Apply magic to an item known to be a "weapon"
 * @details power > 2はデバッグ専用.
 */
void WeaponEnchanter::apply_magic()
{
    if (this->should_skip) {
        return;
    }

    switch (this->o_ptr->tval) {
    case ItemKindType::BOW: {
        if (this->power > 1) {
            if ((this->power > 2) || one_in_(20)) {
                become_random_artifact(this->player_ptr, this->o_ptr, false);
                break;
            }

            this->o_ptr->ego_idx = get_random_ego(INVEN_BOW, true);
        }

        break;
    }
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
    case ItemKindType::SHOT: {
        if (this->power > 1) {
            if (this->power > 2) {
                become_random_artifact(this->player_ptr, this->o_ptr, false);
                break;
            }

            this->o_ptr->ego_idx = get_random_ego(INVEN_AMMO, true);
            while (one_in_(10L * this->o_ptr->dd * this->o_ptr->ds)) {
                this->o_ptr->dd++;
            }

            if (this->o_ptr->dd > 9) {
                this->o_ptr->dd = 9;
            }

            break;
        }

        if (this->power < -1) {
            if (randint0(MAX_DEPTH) < this->level) {
                this->o_ptr->ego_idx = get_random_ego(INVEN_AMMO, false);
            }
        }

        break;
    }
    default:
        break;
    }
}
