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

CrownEnchanter::CrownEnchanter(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power)
    : ArmorEnchanterBase{ o_ptr, level, power }
    , owner_ptr(owner_ptr)
{
}

/*
 * @details power > 2はデバッグ専用.
 */
void CrownEnchanter::apply_magic()
{
    if (this->power > 1) {
        if (one_in_(20) || (this->power > 2)) {
            become_random_artifact(this->owner_ptr, this->o_ptr, false);
            return;
        }

        while (true) {
            bool ok_flag = true;
            this->o_ptr->name2 = get_random_ego(INVEN_HEAD, true);

            switch (this->o_ptr->name2) {
            case EGO_TELEPATHY:
            case EGO_MAGI:
            case EGO_MIGHT:
            case EGO_REGENERATION:
            case EGO_LORDLINESS:
            case EGO_BASILISK:
                break;
            case EGO_SEEING:
                if (one_in_(3))
                    add_low_telepathy(this->o_ptr);
                break;
            default:
                /* not existing crown (wisdom,lite, etc...) */
                ok_flag = false;
            }

            if (ok_flag)
                break;
        }

        return;
    }

    if (this->power < -1) {
        while (true) {
            bool ok_flag = true;
            this->o_ptr->name2 = get_random_ego(INVEN_HEAD, false);

            switch (this->o_ptr->name2) {
            case EGO_H_DEMON:
                ok_flag = false;
                break;
            default:
                break;
            }

            if (ok_flag)
                break;
        }
    }
}
