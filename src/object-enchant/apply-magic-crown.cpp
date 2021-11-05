/*
 * @brief 冠に耐性等の追加効果を付与する処理
 * @date 2021/08/01
 * @author Hourier
 */

#include "object-enchant/apply-magic-crown.h"
#include "artifact/random-art-generator.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-ego.h"
#include "system/object-type-definition.h"

/*
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
CrownEnchanter::CrownEnchanter(PlayerType *player_ptr, object_type *o_ptr, DEPTH level, int power)
    : AbstractProtectorEnchanter{ o_ptr, level, power }
    , player_ptr(player_ptr)
{
}

void CrownEnchanter::apply_magic()
{
    if (this->power > 1) {
        this->give_ego_index();
        return;
    }

    if (this->power < -1) {
        this->give_cursed();
    }
}

/*
 * @details power > 2はデバッグ専用.
 */
void CrownEnchanter::give_ego_index()
{
    if (one_in_(20) || (this->power > 2)) {
        become_random_artifact(this->player_ptr, this->o_ptr, false);
        return;
    }

    while (true) {
        this->o_ptr->name2 = get_random_ego(INVEN_HEAD, true);
        switch (this->o_ptr->name2) {
        case EGO_TELEPATHY:
        case EGO_MAGI:
        case EGO_MIGHT:
        case EGO_REGENERATION:
        case EGO_LORDLINESS:
        case EGO_BASILISK:
            return;
        case EGO_SEEING:
            if (one_in_(3)) {
                add_low_telepathy(this->o_ptr);
            }

            return;
        default:
            continue;
        }
    }
}

void CrownEnchanter::give_cursed()
{
    while (true) {
        this->o_ptr->name2 = get_random_ego(INVEN_HEAD, false);
        switch (this->o_ptr->name2) {
        case EGO_H_DEMON:
            return;
        default:
            continue;
        }
    }
}
