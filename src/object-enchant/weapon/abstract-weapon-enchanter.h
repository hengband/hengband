#pragma once

#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

struct object_type;
class AbstractWeaponEnchanter : EnchanterBase {
protected:
    AbstractWeaponEnchanter(object_type *o_ptr, DEPTH level, int power);
    object_type *o_ptr;
    DEPTH level;
    int power;
};
