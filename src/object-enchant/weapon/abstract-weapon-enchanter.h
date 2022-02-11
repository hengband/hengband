#pragma once

#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

class ObjectType;
class AbstractWeaponEnchanter : EnchanterBase {
protected:
    AbstractWeaponEnchanter(ObjectType *o_ptr, DEPTH level, int power);
    ObjectType *o_ptr;
    DEPTH level;
    int power;
};
