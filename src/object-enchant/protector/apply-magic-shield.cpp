/*
 * @brief 盾に耐性等の追加効果を付与する処理
 * @date 2021/08/01
 * @author Hourier
 * @details ドラゴンシールドは必ず付与する. それ以外は確率的に付与する.
 */

#include "object-enchant/protector/apply-magic-shield.h"
#include "artifact/random-art-generator.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-boost.h"
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
ShieldEnchanter::ShieldEnchanter(PlayerType *player_ptr, ItemEntity *o_ptr, DEPTH level, int power)
    : AbstractProtectorEnchanter{ o_ptr, level, power }
    , player_ptr(player_ptr)
{
}

/*
 * @details power > 2はデバッグ専用.
 */
void ShieldEnchanter::apply_magic()
{
    if (this->o_ptr->sval == SV_DRAGON_SHIELD) {
        dragon_resist(this->o_ptr);
        if (!one_in_(3)) {
            return;
        }
    }

    if (this->power <= 1) {
        return;
    }

    if ((this->power > 2) || one_in_(20)) {
        become_random_artifact(this->player_ptr, this->o_ptr, false);
        return;
    }

    this->give_ego_index();
}

/*
 * @details 金属製の盾は魔法を受け付けない.
 * @todo ミラー・シールドはエゴの付与確率が低い. この仕様で良いか要確認.
 */
void ShieldEnchanter::give_ego_index()
{
    while (true) {
        this->o_ptr->ego_idx = get_random_ego(INVEN_SUB_HAND, true);
        auto is_metal = this->o_ptr->sval == SV_SMALL_METAL_SHIELD;
        is_metal |= this->o_ptr->sval == SV_LARGE_METAL_SHIELD;
        if (!is_metal && (this->o_ptr->ego_idx == EgoType::S_DWARVEN)) {
            continue;
        }

        break;
    }

    switch (this->o_ptr->ego_idx) {
    case EgoType::REFLECTION:
        if (this->o_ptr->sval == SV_MIRROR_SHIELD) {
            this->o_ptr->ego_idx = EgoType::NONE;
        }

        return;
    default:
        return;
    }
}
