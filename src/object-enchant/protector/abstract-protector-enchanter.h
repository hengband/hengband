#pragma once

#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

class ObjectType;
class AbstractProtectorEnchanter : public EnchanterBase {
public:
    virtual ~AbstractProtectorEnchanter() = default;

protected:
    AbstractProtectorEnchanter(ObjectType *o_ptr, DEPTH level, int power);
    ObjectType *o_ptr;
    int power;
};
