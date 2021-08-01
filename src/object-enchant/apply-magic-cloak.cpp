/*
 * @brief クロークに耐性等の追加効果を付与する処理
 * @date 2021/08/01
 * @author Hourier
 */

#include "object-enchant/apply-magic-cloak.h"
#include "artifact/random-art-generator.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-ego.h"
#include "system/object-type-definition.h"

CloakEnchanter::CloakEnchanter(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power)
    : ArmorEnchanterBase{ o_ptr, level, power }
    , owner_ptr(owner_ptr)
{
}

/*
 * @details power > 2はデバッグ専用.
 */
void CloakEnchanter::apply_magic()
{
    if (this->power > 1) {
        if (one_in_(20) || (this->power > 2)) {
            become_random_artifact(this->owner_ptr, this->o_ptr, false);
            return;
        }

        this->o_ptr->name2 = get_random_ego(INVEN_OUTER, true);
        return;
    }

    if (this->power < -1) {
        this->o_ptr->name2 = get_random_ego(INVEN_OUTER, false);
    }
}
