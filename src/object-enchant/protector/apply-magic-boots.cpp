/*
 * @brief 靴に耐性等の追加効果を付与する処理
 * @date 2021/08/01
 * @author Hourier
 * @details ドラゴンブーツは必ず付与する. それ以外は確率的に付与する.
 */

#include "object-enchant/protector/apply-magic-boots.h"
#include "artifact/random-art-generator.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-ego.h"
#include "sv-definition/sv-protector-types.h"
#include "system/object-type-definition.h"

/*
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
BootsEnchanter::BootsEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power)
    : AbstractProtectorEnchanter{ o_ptr, level, power }
    , player_ptr(player_ptr)
{
}

/*
 * power > 2はデバッグ専用.
 */
void BootsEnchanter::apply_magic()
{
    if (this->o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE) {
        dragon_resist(this->o_ptr);
        if (!one_in_(3)) {
            return;
        }
    }

    if (this->power > 1) {
        if ((this->power > 2) || one_in_(20)) {
            become_random_artifact(this->player_ptr, this->o_ptr, false);
            return;
        }

        this->o_ptr->ego_idx = get_random_ego(INVEN_FEET, true);
        return;
    }

    if (this->power < -1) {
        this->o_ptr->ego_idx = get_random_ego(INVEN_FEET, false);
    }
}
