/*
 * @brief 防具類に耐性等を付与する処理の共通部分
 * @date 2021/08/01
 * @author Hourier
 */

#include "object-enchant/protector/abstract-protector-enchanter.h"
#include "object-enchant/object-boost.h"
#include "system/object-type-definition.h"

/*
 * @brief コンストラクタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
AbstractProtectorEnchanter::AbstractProtectorEnchanter(object_type *o_ptr, DEPTH level, int power)
    : o_ptr(o_ptr)
    , power(power)
{
    if (this->power == 0) {
        return;
    }

    auto toac1 = (ARMOUR_CLASS)(randint1(5) + m_bonus(5, level));
    auto toac2 = (ARMOUR_CLASS)m_bonus(10, level);
    if (this->power > 0) {
        this->o_ptr->to_a += toac1;
        if (this->power > 1) {
            this->o_ptr->to_a += toac2;
        }

        return;
    }
    
    if (this->power < 0) {
        this->o_ptr->to_a -= toac1;
        if (this->power < -1) {
            this->o_ptr->to_a -= toac2;
        }

        if (this->o_ptr->to_a < 0) {
            this->o_ptr->curse_flags.set(CurseTraitType::CURSED);
        }
    }
}
