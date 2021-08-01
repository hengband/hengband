#include "object-enchant/apply-magic-shield.h"
#include "artifact/random-art-generator.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-ego.h"
#include "sv-definition/sv-protector-types.h"
#include "system/object-type-definition.h"

ShieldEnchanter::ShieldEnchanter(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power)
    : ArmorEnchanterBase{ o_ptr, level, power }
    , owner_ptr(owner_ptr)
{
}

/*
 * @details 金属製の盾は魔法を受け付けない. power > 2はデバッグ専用.
 */
void ShieldEnchanter::apply_magic()
{
    this->enchant();
    if (this->stop_continuous_application) {
        return;
    }

    if (this->power <= 1) {
        return;
    }

    if (one_in_(20) || (this->power > 2)) {
        become_random_artifact(this->owner_ptr, this->o_ptr, false);
        return;
    }

    this->give_ego_index();
}

void ShieldEnchanter::enchant()
{
    switch (this->o_ptr->sval) {
    case SV_DRAGON_SHIELD:
        dragon_resist(this->o_ptr);
        if (!one_in_(3)) {
            this->stop_continuous_application = true;
        }

        return;
    default:
        return;
    }
}

void ShieldEnchanter::give_ego_index()
{
    while (true) {
        this->o_ptr->name2 = get_random_ego(INVEN_SUB_HAND, true);
        auto is_metal = this->o_ptr->sval == SV_SMALL_METAL_SHIELD;
        is_metal |= this->o_ptr->sval == SV_LARGE_METAL_SHIELD;
        if (!is_metal && this->o_ptr->name2 == EGO_S_DWARVEN) {
            continue;
        }

        break;
    }

    switch (this->o_ptr->name2) {
    case EGO_REFLECTION:
        if (this->o_ptr->sval == SV_MIRROR_SHIELD) {
            this->o_ptr->name2 = 0;
        }

        return;
    default:
        return;
    }
}
