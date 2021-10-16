#pragma once

#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

struct object_type;
class AbstractProtectorEnchanter : EnchanterBase {
protected:
    AbstractProtectorEnchanter(object_type *o_ptr, DEPTH level, int power);
    virtual ~AbstractProtectorEnchanter() = default;
    object_type *o_ptr;
    int power;
};
