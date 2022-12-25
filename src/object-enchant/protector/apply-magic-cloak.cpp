/*
 * @brief クロークに耐性等の追加効果を付与する処理
 * @date 2021/08/01
 * @author Hourier
 */

#include "object-enchant/protector/apply-magic-cloak.h"
#include "artifact/random-art-generator.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-ego.h"
#include "sv-definition/sv-protector-types.h"
#include "system/item-entity.h"

/*
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
CloakEnchanter::CloakEnchanter(PlayerType *player_ptr, ItemEntity *o_ptr, DEPTH level, int power)
    : AbstractProtectorEnchanter{ o_ptr, level, power }
    , player_ptr(player_ptr)
{
}

/*
 * @details power > 2はデバッグ専用.
 */
void CloakEnchanter::apply_magic()
{
    if (this->o_ptr->bi_key.sval() == SV_ELVEN_CLOAK) {
        this->o_ptr->pval = randint1(4);
    }

    if (this->power > 1) {
        if ((this->power > 2) || one_in_(20)) {
            become_random_artifact(this->player_ptr, this->o_ptr, false);
            return;
        }

        this->o_ptr->ego_idx = get_random_ego(INVEN_OUTER, true);
        return;
    }

    if (this->power < -1) {
        this->o_ptr->ego_idx = get_random_ego(INVEN_OUTER, false);
    }
}
