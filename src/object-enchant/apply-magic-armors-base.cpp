#include "object-enchant/apply-magic-armors-base.h"
#include "object-enchant/object-boost.h"
#include "system/object-type-definition.h"

/*
 * @details パワー0の時は何もしない
 */
ArmorEnchanterBase::ArmorEnchanterBase(object_type *o_ptr, DEPTH level, int power)
    : o_ptr(o_ptr)
    , power(power)
{
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
            this->o_ptr->curse_flags.set(TRC::CURSED);
        }
    }
}
