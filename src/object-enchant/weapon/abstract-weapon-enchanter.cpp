#include "object-enchant/weapon/abstract-weapon-enchanter.h"
#include "object-enchant/object-boost.h"
#include "system/object-type-definition.h"

AbstractWeaponEnchanter::AbstractWeaponEnchanter(object_type* o_ptr, DEPTH level, int power)
    : o_ptr(o_ptr)
    , level(level)
    , power(power)
{
    auto tohit1 = static_cast<short>(randint1(5) + m_bonus(5, this->level));
    auto todam1 = static_cast<short>(randint1(5) + m_bonus(5, this->level));
    auto tohit2 = static_cast<short>(m_bonus(10, this->level));
    auto todam2 = static_cast<short>(m_bonus(10, this->level));

    if ((this->o_ptr->tval == ItemKindType::BOLT) || (this->o_ptr->tval == ItemKindType::ARROW) || (this->o_ptr->tval == ItemKindType::SHOT)) {
        tohit2 = (tohit2 + 1) / 2;
        todam2 = (todam2 + 1) / 2;
    }

    if (this->power > 0) {
        this->o_ptr->to_h += tohit1;
        this->o_ptr->to_d += todam1;
        if (this->power > 1) {
            this->o_ptr->to_h += tohit2;
            this->o_ptr->to_d += todam2;
        }

        return;
    }
    
    if (this->power < 0) {
        this->o_ptr->to_h -= tohit1;
        this->o_ptr->to_d -= todam1;
        if (this->power < -1) {
            this->o_ptr->to_h -= tohit2;
            this->o_ptr->to_d -= todam2;
        }

        if (this->o_ptr->to_h + this->o_ptr->to_d < 0) {
            this->o_ptr->curse_flags.set(CurseTraitType::CURSED);
        }
    }
}
