#pragma once

#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

struct object_type;
class ArmorEnchanterBase : EnchanterBase {
protected:
    ArmorEnchanterBase(object_type *o_ptr, DEPTH level, int power);
    ArmorEnchanterBase() = delete;
    virtual ~ArmorEnchanterBase() = default;
    object_type *o_ptr;
    int power;
};
