#pragma once

#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

class ItemEntity;
class AbstractProtectorEnchanter : public EnchanterBase {
public:
    virtual ~AbstractProtectorEnchanter() = default;

protected:
    AbstractProtectorEnchanter(ItemEntity *o_ptr, DEPTH level, int power);
    ItemEntity *o_ptr;
    int power;
};
