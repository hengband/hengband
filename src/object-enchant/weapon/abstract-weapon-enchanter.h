#pragma once

#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

class ItemEntity;
class AbstractWeaponEnchanter : public EnchanterBase {
public:
    virtual ~AbstractWeaponEnchanter() = default;

protected:
    AbstractWeaponEnchanter(ItemEntity *o_ptr, DEPTH level, int power);
    ItemEntity *o_ptr;
    DEPTH level;
    int power;
    bool should_skip = false;

    void give_killing_bonus();
    virtual void decide_skip();
};
