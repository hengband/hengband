/*!
 * @brief 弓系のアイテムを強化する処理
 * @date 2022/03/11
 * @author Hourier
 */

#include "object-enchant/weapon/apply-magic-bow.h"
#include "artifact/random-art-generator.h"
#include "inventory/inventory-slot-types.h"
#include "system/item-entity.h"

/*!
 * @brief 弓強化クラスのコンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
BowEnchanter::BowEnchanter(PlayerType *player_ptr, ItemEntity *o_ptr, DEPTH level, int power)
    : AbstractWeaponEnchanter(o_ptr, level, power)
    , player_ptr(player_ptr)
{
}

/*!
 * @brief 弓系オブジェクトに生成ランクごとの強化を与えるサブルーチン
 * Apply magic to an item known to be a "weapon"
 * @details power > 2はデバッグ専用.
 */
void BowEnchanter::apply_magic()
{
    this->decide_skip();
    if (this->should_skip) {
        return;
    }

    this->give_killing_bonus();
    if (this->power > 1) {
        if ((this->power > 2) || one_in_(20)) {
            become_random_artifact(this->player_ptr, this->o_ptr, false);
            return;
        }

        this->o_ptr->ego_idx = get_random_ego(INVEN_BOW, true);
    }
}
