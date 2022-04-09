#include "object-enchant/weapon/abstract-weapon-enchanter.h"
#include "object-enchant/object-boost.h"
#include "object/tval-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/object-type-definition.h"

AbstractWeaponEnchanter::AbstractWeaponEnchanter(ObjectType *o_ptr, DEPTH level, int power)
    : o_ptr(o_ptr)
    , level(level)
    , power(power)
{
}

void AbstractWeaponEnchanter::decide_skip()
{
    if (this->power == 0) {
        this->should_skip = true;
    }
}

/*!
 * @brief 武器に殺戮修正を付与する
 */
void AbstractWeaponEnchanter::give_killing_bonus()
{
    if (this->should_skip) {
        return;
    }

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
