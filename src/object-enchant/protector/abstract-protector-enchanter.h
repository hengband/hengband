#pragma once

#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

class ObjectType;
class AbstractProtectorEnchanter : EnchanterBase {
protected:
    AbstractProtectorEnchanter(ObjectType *o_ptr, DEPTH level, int power);
    virtual ~AbstractProtectorEnchanter() = default;
    ObjectType *o_ptr;
    int power;
};
