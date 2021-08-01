/*
 * @brief クロークに耐性等の追加効果を付与する処理
 * @date 2021/08/01
 * @author Hourier
 */

#include "object-enchant/apply-magic-helm.h"
#include "artifact/random-art-generator.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-ego.h"
#include "sv-definition/sv-protector-types.h"
#include "system/object-type-definition.h"

HelmEnchanter::HelmEnchanter(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power)
    : ArmorEnchanterBase{ o_ptr, level, power }
    , owner_ptr(owner_ptr)
{
}

void HelmEnchanter::apply_magic()
{
    if (this->o_ptr->sval == SV_DRAGON_HELM) {
        dragon_resist(this->o_ptr);
        if (!one_in_(3))
            return;
    }

    if (this->power > 1) {
        /* power > 2 is debug only */
        if (one_in_(20) || (this->power > 2)) {
            become_random_artifact(this->owner_ptr, this->o_ptr, false);
            return;
        }

        while (true) {
            bool ok_flag = true;
            this->o_ptr->name2 = get_random_ego(INVEN_HEAD, true);
            switch (this->o_ptr->name2) {
            case EGO_BRILLIANCE:
            case EGO_DARK:
            case EGO_INFRAVISION:
            case EGO_H_PROTECTION:
            case EGO_LITE:
                break;
            case EGO_SEEING:
                if (one_in_(7))
                    add_low_telepathy(this->o_ptr);
                break;
            default:
                /* not existing helm (Magi, Might, etc...)*/
                ok_flag = false;
            }

            if (ok_flag)
                break;
        }

        return;
    } else if (this->power < -1) {
        while (true) {
            bool ok_flag = true;
            this->o_ptr->name2 = get_random_ego(INVEN_HEAD, false);

            switch (this->o_ptr->name2) {
            case EGO_ANCIENT_CURSE:
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
