/*!
 * @brief 剣に耐性等の追加効果を付与する処理
 * @date 2022/03/22
 * @author Hourier
 */

#include "object-enchant/weapon/apply-magic-sword.h"
#include "floor/floor-base-definitions.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-boost.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/object-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 剣強化クラスのコンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
SwordEnchanter::SwordEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power)
    : MeleeWeaponEnchanter(player_ptr, o_ptr, level, power)
{
}

void SwordEnchanter::decide_skip()
{
    AbstractWeaponEnchanter::decide_skip();
    this->should_skip |= this->o_ptr->sval == SV_POISON_NEEDLE;
}

void SwordEnchanter::apply_magic()
{
    this->decide_skip();
    if (this->should_skip) {
        return;
    }

    this->give_killing_bonus();
    if (this->o_ptr->sval == SV_DIAMOND_EDGE) {
        return;
    }

    MeleeWeaponEnchanter::apply_magic();
}

void SwordEnchanter::give_ego_index()
{
    while (true) {
        this->o_ptr->ego_idx = get_random_ego(INVEN_MAIN_HAND, true);
        if (this->o_ptr->ego_idx == EgoType::EARTHQUAKES) {
            continue;
        }

        break;
    }

    switch (this->o_ptr->ego_idx) {
    case EgoType::SHARPNESS:
        this->o_ptr->pval = static_cast<short>(m_bonus(5, this->level) + 1);
        break;
    default:
        break;
    }
}

void SwordEnchanter::give_cursed()
{
    if (randint0(MAX_DEPTH) >= this->level) {
        return;
    }

    auto n = 0;
    while (true) {
        this->o_ptr->ego_idx = get_random_ego(INVEN_MAIN_HAND, false);
        const auto *e_ptr = &egos_info[this->o_ptr->ego_idx];
        if ((this->o_ptr->sval != SV_HAYABUSA) || (e_ptr->max_pval >= 0)) {
            return;
        }

        if (++n > 1000) {
            msg_print(_("エラー:隼の剣に割り当てるエゴ無し", "Error: Cannot find for Hayabusa."));
            return;
        }

        continue;
    }
}
